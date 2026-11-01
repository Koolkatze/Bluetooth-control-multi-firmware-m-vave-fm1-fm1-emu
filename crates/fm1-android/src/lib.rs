//! FM-1 Android native engine entrypoint.
//!
//! Android AAudio's callback ONLY copies lock-free PCM. Rust does firmware,
//! ARM64 native JIT, MIDI, LCD emulation, and sample-rate conversion off-thread.
//! Neither JavaScript nor WebView is in the audio path.
mod engine;

#[cfg(target_os = "android")]
mod android {
    use super::engine::{run_worker, AudioRing, Cmd, Machine, Shared};
    use jni::objects::{JByteArray, JClass, JString};
    use jni::sys::{jbyteArray, jint, jlong, jstring};
    use jni::JNIEnv;
    use std::ffi::c_void;
    use std::path::PathBuf;
    use std::ptr;
    use std::sync::{mpsc, Arc, Mutex, OnceLock};
    use std::sync::atomic::Ordering;
    use std::thread::JoinHandle;

    #[repr(C)]
    struct AAudioStream { _private: [u8; 0] }
    #[repr(C)]
    struct AAudioStreamBuilder { _private: [u8; 0] }
    type ResultCode = i32;
    const AAUDIO_OK: ResultCode = 0;
    const CALLBACK_CONTINUE: i32 = 0;
    const AAUDIO_FORMAT_PCM_FLOAT: i32 = 2;
    const AAUDIO_PERFORMANCE_MODE_LOW_LATENCY: i32 = 12;
    const AAUDIO_SHARING_MODE_SHARED: i32 = 1;

    #[link(name = "aaudio")]
    unsafe extern "C" {
        fn AAudio_createStreamBuilder(builder: *mut *mut AAudioStreamBuilder) -> ResultCode;
        fn AAudioStreamBuilder_delete(builder: *mut AAudioStreamBuilder) -> ResultCode;
        fn AAudioStreamBuilder_setChannelCount(builder: *mut AAudioStreamBuilder, channels: i32);
        fn AAudioStreamBuilder_setFormat(builder: *mut AAudioStreamBuilder, format: i32);
        fn AAudioStreamBuilder_setSharingMode(builder: *mut AAudioStreamBuilder, mode: i32);
        fn AAudioStreamBuilder_setPerformanceMode(builder: *mut AAudioStreamBuilder, mode: i32);
        fn AAudioStreamBuilder_setDataCallback(
            builder: *mut AAudioStreamBuilder,
            callback: Option<unsafe extern "C" fn(*mut AAudioStream, *mut c_void, *mut c_void, i32) -> i32>,
            user: *mut c_void,
        );
        fn AAudioStreamBuilder_openStream(
            builder: *mut AAudioStreamBuilder, output: *mut *mut AAudioStream
        ) -> ResultCode;
        fn AAudioStream_getSampleRate(stream: *mut AAudioStream) -> i32;
        fn AAudioStream_getFramesPerBurst(stream: *mut AAudioStream) -> i32;
        fn AAudioStream_setBufferSizeInFrames(stream: *mut AAudioStream, frames: i32) -> i32;
        fn AAudioStream_requestStart(stream: *mut AAudioStream) -> ResultCode;
        fn AAudioStream_requestStop(stream: *mut AAudioStream) -> ResultCode;
        fn AAudioStream_close(stream: *mut AAudioStream) -> ResultCode;
    }

    unsafe extern "C" fn pcm_callback(
        _stream: *mut AAudioStream, user: *mut c_void,
        audio_data: *mut c_void, frames: i32,
    ) -> i32 {
        if user.is_null() || audio_data.is_null() || frames <= 0 {
            return CALLBACK_CONTINUE;
        }
        let shared = &*(user as *const Shared);
        let pcm = std::slice::from_raw_parts_mut(audio_data as *mut f32, frames as usize * 2);
        // AUDIO THREAD: no allocations, mutexes, logging, or JNI calls.
        // Accumulate starvation and publish ONE atomic update per callback.
        let mut missing = 0u64;
        for pair in pcm.chunks_exact_mut(2) {
            if let Some(sample) = shared.ring.pop() {
                pair[0] = sample[0];
                pair[1] = sample[1];
            } else {
                pair[0] = 0.0;
                pair[1] = 0.0;
                missing += 1;
            }
        }
        if missing != 0 && shared.playing.load(Ordering::Relaxed) {
            shared.ring.underruns.fetch_add(missing, Ordering::Relaxed);
        }
        CALLBACK_CONTINUE
    }

    struct EngineState {
        shared: Arc<Shared>,
        commands: mpsc::Sender<Cmd>,
        worker: Option<JoinHandle<()>>,
        stream: usize, // native pointer, protected by STATE mutex
    }

    impl EngineState {
        fn stop(mut self) {
            self.shared.quit.store(true, Ordering::Release);
            let _ = self.commands.send(Cmd::Stop);
            if self.stream != 0 {
                let ptr = self.stream as *mut AAudioStream;
                unsafe {
                    AAudioStream_requestStop(ptr);
                    // close waits until callbacks have left and no longer access shared
                    AAudioStream_close(ptr);
                }
            }
            if let Some(worker) = self.worker.take() { let _ = worker.join(); }
        }
    }

    static STATE: OnceLock<Mutex<Option<EngineState>>> = OnceLock::new();
    fn state() -> &'static Mutex<Option<EngineState>> {
        STATE.get_or_init(|| Mutex::new(None))
    }

    fn open_audio(shared: &Arc<Shared>) -> Result<(*mut AAudioStream, u32), String> {
        let mut builder = ptr::null_mut();
        let rc = unsafe { AAudio_createStreamBuilder(&mut builder) };
        if rc != AAUDIO_OK || builder.is_null() {
            return Err(format!("AAudio builder: {rc}"));
        }
        unsafe {
            AAudioStreamBuilder_setChannelCount(builder, 2);
            AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
            AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
            AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
            AAudioStreamBuilder_setDataCallback(
                builder, Some(pcm_callback), Arc::as_ptr(shared) as *mut c_void,
            );
        }
        let mut stream = ptr::null_mut();
        let result = unsafe { AAudioStreamBuilder_openStream(builder, &mut stream) };
        unsafe { AAudioStreamBuilder_delete(builder); }
        if result != AAUDIO_OK || stream.is_null() {
            return Err(format!("AAudio no pudo abrir la salida ({result})"));
        }
        let hz = unsafe { AAudioStream_getSampleRate(stream) };
        if hz < 32000 || hz > 192000 {
            unsafe { AAudioStream_close(stream); }
            return Err(format!("Frecuencia de audio no soportada: {hz} Hz"));
        }
        let burst = unsafe { AAudioStream_getFramesPerBurst(stream) };
        if burst > 0 {
            // Two hardware bursts are usually sufficient; avoid the long
            // WebAudio/WebView latency by not requesting an oversized buffer.
            unsafe { AAudioStream_setBufferSizeInFrames(stream, burst * 2); }
        }
        Ok((stream, hz as u32))
    }

    fn respond(env: &mut JNIEnv, value: &str) -> jstring {
        match env.new_string(value) {
            Ok(s) => s.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_boot(
        mut env: JNIEnv, _: JClass, firmware: JByteArray,
        mhz: jint, identity: JString, save_dir: JString,
    ) -> jstring {
        let firmware = match env.convert_byte_array(firmware) {
            Ok(data) => data,
            Err(e) => return respond(&mut env, &format!("ERROR:firmware: {e}")),
        };
        let identity: String = match env.get_string(&identity) {
            Ok(s) => s.into(),
            Err(e) => return respond(&mut env, &format!("ERROR:id: {e}")),
        };
        let save_dir: String = match env.get_string(&save_dir) {
            Ok(s) => s.into(),
            Err(e) => return respond(&mut env, &format!("ERROR:storage: {e}")),
        };
        if identity.is_empty() || identity.len() > 80 ||
            !identity.bytes().all(|b| b.is_ascii_hexdigit()) {
            return respond(&mut env, "ERROR:Identificador de firmware inválido");
        }
        // A single synth per app; stop the previous stream before replacing it.
        let mut active = state().lock().unwrap();
        if let Some(old) = active.take() { old.stop(); }
        let machine = match Machine::boot(&firmware, mhz as u32) {
            Ok(v) => v,
            Err(e) => return respond(&mut env, &format!("ERROR:{e}")),
        };
        let shared = Shared::new();
        let (stream, sample_rate) = match open_audio(&shared) {
            Ok(v) => v,
            Err(e) => return respond(&mut env, &format!("ERROR:{e}")),
        };
        let saved = PathBuf::from(save_dir).join(format!("fm1-native-{identity}.bin"));
        let (tx, rx) = mpsc::channel();
        let worker_shared = shared.clone();
        let worker = std::thread::Builder::new()
            .name("FM1-native-CPU".into())
            .spawn(move || run_worker(machine, worker_shared, rx, sample_rate, Some(saved)));
        let worker = match worker {
            Ok(v) => v,
            Err(e) => {
                unsafe { AAudioStream_close(stream); }
                return respond(&mut env, &format!("ERROR:Motor Rust: {e}"));
            }
        };
        // Pre-fill PCM BEFORE starting the audio callback. Previously AAudio
        // immediately consumed an empty ring during JIT warmup / firmware boot.
        // Wait only a bounded time so a genuinely stalled firmware still
        // reports a meaningful problem instead of freezing the app.
        let prefill_start = std::time::Instant::now();
        while shared.ring.available() < 512 &&
              prefill_start.elapsed() < std::time::Duration::from_millis(1000) &&
              !shared.halted.load(Ordering::Acquire) {
            std::thread::sleep(std::time::Duration::from_millis(2));
        }
        let rc = unsafe { AAudioStream_requestStart(stream) };
        if rc != AAUDIO_OK {
            shared.quit.store(true, Ordering::Release);
            let _ = worker.join();
            unsafe { AAudioStream_close(stream); }
            return respond(&mut env, &format!("ERROR:AAudio start: {rc}"));
        }
        *active = Some(EngineState {
            shared, commands: tx, worker: Some(worker), stream: stream as usize,
        });
        respond(&mut env, &format!("OK:{sample_rate}"))
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_stop(
        _env: JNIEnv, _: JClass,
    ) {
        if let Ok(mut state) = state().lock() {
            if let Some(active) = state.take() { active.stop(); }
        }
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_control(
        _env: JNIEnv, _: JClass, kind: jint, key: jint, value: jint,
    ) {
        let cmd = match kind {
            0 => Cmd::Key(key as u8, value != 0),
            1 => Cmd::Enc(key as u8, value),
            2 => Cmd::Pot(value as u16),
            3 => Cmd::Analog(key as u8, value as u16),
            4 => Cmd::Velocity(value.max(1) as u32),
            _ => return,
        };
        // UI control events may not be dropped: a missed PLAY/SAVE press
        // makes the firmware look frozen. The mutex is NOT used by AAudio's
        // callback or the synth worker; blocking here only races boot/stop.
        if let Ok(state) = state().lock() {
            if let Some(active) = state.as_ref() { let _ = active.commands.send(cmd); }
        }
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_midi(
        mut env: JNIEnv, _: JClass, bytes: JByteArray,
    ) {
        if let Ok(bytes) = env.convert_byte_array(bytes) {
            if bytes.is_empty() || bytes.len() > 3 { return; }
            let mut msg = [0u8; 3];
            msg[..bytes.len()].copy_from_slice(&bytes);
            if let Ok(state) = state().try_lock() {
                if let Some(active) = state.as_ref() {
                    let _ = active.commands.send(Cmd::Midi(msg, bytes.len() as u8));
                }
            }
        }
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_lcd(
        env: JNIEnv, _: JClass,
    ) -> jbyteArray {
        let mut packed = vec![0u8; 240 * 240 * 2];
        if let Ok(state) = state().try_lock() {
            if let Some(active) = state.as_ref() {
                if let Ok(lcd) = active.shared.lcd.try_lock() {
                    for (i, &pixel) in lcd.iter().enumerate() {
                        let pair = pixel.to_le_bytes();
                        packed[2 * i] = pair[0];
                        packed[2 * i + 1] = pair[1];
                    }
                }
            }
        }
        match env.byte_array_from_slice(&packed) {
            Ok(array) => array.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }

    /// Diagnostics that do not touch AAudio callback and are safe to sample
    /// from the WebView every 250ms: firmware time, output frames produced,
    /// ring fill. All values are monotonic except instantaneous ring fill.
    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_diagnostics(
        mut env: JNIEnv, _: JClass,
    ) -> jstring {
        let message = if let Ok(state) = state().try_lock() {
            if let Some(active) = state.as_ref() {
                let s = &active.shared;
                format!("{},{},{}",
                    s.emulated_ms.load(Ordering::Relaxed),
                    s.produced_frames.load(Ordering::Relaxed),
                    s.ring.available())
            } else {
                "0,0,0".to_owned()
            }
        } else {
            "".to_owned()
        };
        respond(&mut env, &message)
    }

    #[no_mangle]
    pub extern "system" fn Java_com_koolkatze_fm1offline_NativeSynth_stats(
        _env: JNIEnv, _: JClass,
    ) -> jlong {
        if let Ok(state) = state().try_lock() {
            if let Some(active) = state.as_ref() {
                let s = &active.shared;
                let underruns = s.ring.underruns.load(Ordering::Relaxed).min(u32::MAX as u64);
                let bits = if s.ready.load(Ordering::Acquire) { 1u64 } else { 0 }
                    | if s.playing.load(Ordering::Acquire) { 2 } else { 0 }
                    | if s.halted.load(Ordering::Acquire) { 4 } else { 0 };
                return ((underruns << 3) | bits) as jlong;
            }
        }
        0
    }
}

pub use engine::{run_worker, AudioRing, Cmd, Machine, Shared};
