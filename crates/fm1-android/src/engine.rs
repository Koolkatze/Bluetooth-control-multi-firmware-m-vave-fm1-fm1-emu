//! Native FM-1 engine shared by the Android AAudio backend.
//!
//! Unlike the browser AudioWorklet, FM-1 CPU/JIT instructions run on their own
//! worker thread. The AAudio callback only reads a lock-free SPSC buffer and
//! never allocates, compiles, waits for a mutex, or invokes JNI.
//! The firmware still generates 44.1kHz PCM; the worker converts it into the
//! output device's native sample rate using a continuous fractional phase.
use fm1_core::{Bus, Cpu};
use fm1_soc::{machine, Action, Soc};
use std::cell::UnsafeCell;
use std::collections::VecDeque;
use std::sync::atomic::{AtomicBool, AtomicU64, AtomicUsize, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;
use std::time::{Duration, Instant};

const RAM_SIZE: usize = 1 << 20;
const RAM_BASE: u32 = 0x01c0_0000;
const AUDIO_CAP: usize = 8192; // power-of-two, single writer and single reader
// A 512-frame lead (10.7ms @ 48k) was too small for a firmware that creates
// 256/512-sample DMA bursts and occasionally spends milliseconds in native JIT.
// 1024 frames covers ~21ms without adding a huge 100ms+ host buffer.
const AUDIO_LEAD: usize = 1024;
const WORK_CHUNK: usize = 128;  // produce an entire callback quantum per worker turn
const LCD_PIXELS: usize = 240 * 240;

/// Only the audio callback reads, and only the worker writes. Acquire/Release
/// establishes that the PCM floats are visible before the write counter moves.
pub struct AudioRing {
    samples: Box<[UnsafeCell<[f32; 2]>]>,
    read: AtomicUsize,
    write: AtomicUsize,
    pub underruns: AtomicU64,
}
unsafe impl Send for AudioRing {}
unsafe impl Sync for AudioRing {}

impl AudioRing {
    pub fn new() -> Self {
        Self {
            samples: (0..AUDIO_CAP).map(|_| UnsafeCell::new([0.0; 2]))
                .collect::<Vec<_>>().into_boxed_slice(),
            read: AtomicUsize::new(0),
            write: AtomicUsize::new(0),
            underruns: AtomicU64::new(0),
        }
    }
    pub fn available(&self) -> usize {
        self.write.load(Ordering::Acquire) - self.read.load(Ordering::Acquire)
    }
    pub fn push(&self, sample: [f32; 2]) -> bool {
        let w = self.write.load(Ordering::Relaxed);
        let r = self.read.load(Ordering::Acquire);
        if w - r == AUDIO_CAP { return false; }
        unsafe { *self.samples[w & (AUDIO_CAP - 1)].get() = sample; }
        self.write.store(w + 1, Ordering::Release);
        true
    }
    pub fn pop(&self) -> Option<[f32; 2]> {
        let r = self.read.load(Ordering::Relaxed);
        if r == self.write.load(Ordering::Acquire) { return None; }
        let sample = unsafe { *self.samples[r & (AUDIO_CAP - 1)].get() };
        self.read.store(r + 1, Ordering::Release);
        Some(sample)
    }
}
impl Default for AudioRing { fn default() -> Self { Self::new() } }

pub struct Shared {
    pub ring: AudioRing,
    pub lcd: Mutex<Vec<u16>>,
    pub quit: AtomicBool,
    pub ready: AtomicBool,
    pub playing: AtomicBool,
    pub halted: AtomicBool,
    /// Emulated firmware millisecond clock, updated outside AAudio callback.
    pub emulated_ms: AtomicU64,
    /// PCM frames produced and submitted to the ring. Helps diagnose stalls.
    pub produced_frames: AtomicU64,
}
impl Shared {
    pub fn new() -> Arc<Self> {
        Arc::new(Self {
            ring: AudioRing::new(), lcd: Mutex::new(vec![0; LCD_PIXELS]),
            quit: AtomicBool::new(false), ready: AtomicBool::new(false),
            playing: AtomicBool::new(false), halted: AtomicBool::new(false),
            emulated_ms: AtomicU64::new(0), produced_frames: AtomicU64::new(0),
        })
    }
}

pub enum Cmd {
    Midi([u8; 3], u8),
    Key(u8, bool),
    Enc(u8, i32),
    Pot(u16),
    Analog(u8, u16),
    Velocity(u32),
    Stop,
}

pub struct Machine {
    cpu: Cpu,
    cpu1: Option<Cpu>,
    soc: Soc,
    pending_tick: bool,
    halted: bool,
    package_flash: Vec<u8>,
}

impl Machine {
    pub fn boot(pkg: &[u8], mhz: u32) -> Result<Self, String> {
        if pkg.len() < 960 || pkg.len() > 16 * 1024 * 1024 {
            return Err("Tamaño de firmware no válido".into());
        }
        let st = std::panic::catch_unwind(|| fm1_soc::stock_from_package(pkg, None))
            .map_err(|_| "Paquete de firmware dañado".to_owned())??;
        let mut soc = Soc::new(RAM_SIZE, st.xip);
        soc.periph.hz = mhz.clamp(60, 240) as u64 * 1_000_000;
        soc.periph.skip_idle = true;
        let mut cpu = Cpu::new(st.entry);
        cpu.jit_on = true; // native ARM64 JIT if executable memory is permitted
        cpu.user = true;
        let params = RAM_BASE + 0x7fc00;
        soc.write32(params, params + 0x40);
        cpu.regs[0] = fm1_soc::stock_boot(&mut soc, &st.flash, params, st.board);
        let package_flash = soc.flash.mem.clone();
        Ok(Self { cpu, cpu1: None, soc, pending_tick: false, halted: false, package_flash })
    }

    fn run(&mut self, n: u32) {
        let jit = self.cpu.jit_on;
        let Self { cpu, cpu1, soc, pending_tick, .. } = self;
        if machine::run_dual(cpu1, soc, n, RAM_SIZE, jit,
            |soc, k| machine::run_blocks(cpu, soc, k, pending_tick)).is_err() {
            self.halted = true;
        }
    }

    fn command(&mut self, cmd: Cmd) -> bool {
        match cmd {
            Cmd::Midi(bytes, n) => {
                for &b in &bytes[..(n as usize).min(3)] {
                    self.soc.periph.apply(&Action::Midi { b });
                }
            }
            Cmd::Key(id, down) => self.soc.periph.apply(&Action::Key { id, down }),
            Cmd::Enc(enc, clicks) => self.soc.periph.apply(&Action::Enc { enc, clicks }),
            Cmd::Pot(value) => self.soc.periph.apply(&Action::Adc { ch: 4, value: value.min(1023) }),
            Cmd::Analog(idx, value) =>
                self.soc.periph.apply(&Action::Adc { ch: idx, value: value.min(1023) }),
            Cmd::Velocity(us) =>
                self.soc.periph.smk_vel = self.soc.periph.hz * us as u64 / 1_000_000,
            Cmd::Stop => return false,
        }
        true
    }

    /// Persist changed flash sectors without copying the firmware's original
    /// byte stream, so changing firmwares never flashes the physical device.
    fn changed_sectors(&self) -> Vec<(u32, Vec<u8>)> {
        const SECTOR: usize = 4096;
        let old = &self.package_flash;
        let now = &self.soc.flash.mem;
        (0..old.len().min(now.len()) / SECTOR)
            .filter(|i| old[i * SECTOR..(i + 1) * SECTOR] != now[i * SECTOR..(i + 1) * SECTOR])
            .map(|i| ((i * SECTOR) as u32, now[i * SECTOR..(i + 1) * SECTOR].to_vec()))
            .collect()
    }

    pub fn restore(&mut self, sectors: &[(u32, Vec<u8>)]) {
        for (offset, data) in sectors {
            let at = *offset as usize;
            if data.len() != 4096 || at % 4096 != 0 || at + data.len() > self.soc.flash.mem.len() { continue; }
            self.soc.flash.mem[at..at + data.len()].copy_from_slice(data);
            let lo = at.max(fm1_soc::XIP_FLASH_OFFSET);
            let hi = (at + data.len()).min(fm1_soc::XIP_FLASH_OFFSET + self.soc.xip.len());
            if lo < hi {
                self.soc.xip[lo - fm1_soc::XIP_FLASH_OFFSET..hi - fm1_soc::XIP_FLASH_OFFSET]
                    .copy_from_slice(&data[lo - at..hi - at]);
            }
        }
    }
}

pub fn run_worker(mut machine: Machine, shared: Arc<Shared>, rx: mpsc::Receiver<Cmd>,
                  output_rate: u32, save: Option<std::path::PathBuf>) {
    if let Some(path) = &save {
        if let Ok(bytes) = std::fs::read(path) {
            machine.restore(&decode_sectors(&bytes));
        }
    }
    shared.ready.store(true, Ordering::Release);
    let mut src = VecDeque::<[f32; 2]>::with_capacity(2048);
    let mut fraction = 0f64;
    let step = 44100f64 / output_rate as f64;
    let mut last_lcd = Instant::now();
    let mut last_save = Instant::now();
    let mut no_audio_chunks = 0usize;
    let mut last_diagnostics = Instant::now();
    while !shared.quit.load(Ordering::Acquire) && !machine.halted {
        // MIDI and panel commands are checked once per 128 output frames or
        // every native emulation burst, not once for each individual sample.
        while let Ok(command) = rx.try_recv() {
            if !machine.command(command) {
                shared.quit.store(true, Ordering::Release);
                break;
            }
        }
        if shared.quit.load(Ordering::Acquire) { break; }

        if shared.ring.available() >= AUDIO_LEAD {
            // Sleep only when the ring is already above its target fill.
            // Very short sleeps on Android can wake late; this does not need
            // to be exact because the ring is already primed.
            thread::sleep(Duration::from_micros(200));
        } else {
            // The old loop produced only ONE sample before checking clock,
            // channel, queue, atomics, LCD and storage. Batch production
            // makes the CPU/JIT worker materially more efficient at 48 kHz.
            let mut generated = 0usize;
            while generated < WORK_CHUNK && src.len() >= 2 &&
                  shared.ring.available() < AUDIO_LEAD {
                let a = src[0];
                let b = src[1];
                let frac = fraction as f32;
                let output = [
                    a[0] + (b[0] - a[0]) * frac,
                    a[1] + (b[1] - a[1]) * frac,
                ];
                if !shared.ring.push(output) { break; }
                generated += 1;
                fraction += step;
                // Drop consumed source frames; keep the next frame for
                // interpolation across successive bursts and ring callbacks.
                while fraction >= 1.0 {
                    src.pop_front();
                    fraction -= 1.0;
                }
            }
            if generated != 0 {
                shared.produced_frames.fetch_add(generated as u64, Ordering::Relaxed);
            } else if src.len() < 2 {
                // Firmware I2S produces DMA blocks. Run exactly one CPU slice
                // and try to fill many PCM frames on the next iteration.
                machine.run(20_000);
                let samples = std::mem::take(&mut machine.soc.periph.audio);
                if samples.is_empty() {
                    no_audio_chunks += 1;
                    if no_audio_chunks % 128 == 0 { thread::yield_now(); }
                } else {
                    no_audio_chunks = 0;
                    shared.playing.store(true, Ordering::Release);
                    for pair in samples.chunks_exact(2) {
                        src.push_back([pair[0] as f32 / 32768.0,
                                       pair[1] as f32 / 32768.0]);
                    }
                }
            }
        }
        if last_diagnostics.elapsed() >= Duration::from_millis(100) {
            shared.emulated_ms.store(machine.soc.periph.ms(), Ordering::Relaxed);
            last_diagnostics = Instant::now();
        }
        if last_lcd.elapsed() >= Duration::from_millis(40) {
            if let Ok(mut lcd) = shared.lcd.try_lock() {
                lcd.copy_from_slice(&machine.soc.lcd.fb);
            }
            last_lcd = Instant::now();
        }
        if last_save.elapsed() >= Duration::from_secs(3) {
            if let Some(path) = &save {
                let bytes = encode_sectors(&machine.changed_sectors());
                let temp = path.with_extension("tmp");
                if std::fs::write(&temp, bytes).is_ok() {
                    let _ = std::fs::rename(temp, path);
                }
            }
            last_save = Instant::now();
        }
    }
    if let Some(path) = &save {
        let bytes = encode_sectors(&machine.changed_sectors());
        let temp = path.with_extension("tmp");
        if std::fs::write(&temp, bytes).is_ok() { let _ = std::fs::rename(temp, path); }
    }
    shared.halted.store(machine.halted, Ordering::Release);
    shared.ready.store(false, Ordering::Release);
}

/// The native saved flash is confined to application-private storage and uses
/// a bounded sector count. It cannot be mistaken for an updater .fwsc.
fn encode_sectors(sectors: &[(u32, Vec<u8>)]) -> Vec<u8> {
    let mut out = b"FM1N\x01".to_vec();
    out.extend_from_slice(&(sectors.len() as u32).to_le_bytes());
    for (at, data) in sectors {
        if data.len() != 4096 { continue; }
        out.extend_from_slice(&at.to_le_bytes());
        out.extend_from_slice(data);
    }
    out
}
fn decode_sectors(bytes: &[u8]) -> Vec<(u32, Vec<u8>)> {
    if bytes.len() < 9 || &bytes[..5] != b"FM1N\x01" { return vec![]; }
    let count = u32::from_le_bytes(bytes[5..9].try_into().unwrap()) as usize;
    if count > 512 || bytes.len() != 9 + count * 4100 { return vec![]; }
    (0..count).map(|i| {
        let offset = 9 + i * 4100;
        let at = u32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap());
        (at, bytes[offset + 4..offset + 4100].to_vec())
    }).collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn spsc_ring_produces_in_order_and_does_not_overwrite_unread_samples() {
        let r = AudioRing::new();
        assert!(r.pop().is_none());
        for i in 0..AUDIO_CAP {
            assert!(r.push([i as f32, -(i as f32)]));
        }
        assert!(!r.push([1.0, 2.0]));
        for i in 0..AUDIO_CAP {
            assert_eq!(r.pop(), Some([i as f32, -(i as f32)]));
        }
        assert!(r.pop().is_none());
        assert_eq!(r.available(), 0);
    }
    #[test]
    fn ring_spsc_cross_thread_keeps_stereo_order() {
        let ring = Arc::new(AudioRing::new());
        let producer = ring.clone();
        let join = thread::spawn(move || {
            for i in 0..120000 {
                while !producer.push([i as f32, -i as f32]) { thread::yield_now(); }
            }
        });
        for i in 0..120000 {
            let sample = loop {
                if let Some(v) = ring.pop() { break v; }
                thread::yield_now();
            };
            assert_eq!(sample, [i as f32, -i as f32]);
        }
        join.join().unwrap();
    }
    #[test]
    fn audio_lead_is_large_enough_for_firmware_dma_bursts_but_bounded() {
        assert!(AUDIO_LEAD >= 512);
        assert!(AUDIO_LEAD <= 2048, "avoid large instrument latency");
        assert!(AUDIO_LEAD + WORK_CHUNK < AUDIO_CAP);
    }
    #[test]
    fn native_flash_sectors_round_trip_and_reject_malformed_data() {
        let sectors = vec![(4096u32, vec![0x41;4096]), (8192, vec![0xfa;4096])];
        assert_eq!(decode_sectors(&encode_sectors(&sectors)), sectors);
        assert_eq!(decode_sectors(&encode_sectors(&sectors)[..20]), vec![]);
    }
}
