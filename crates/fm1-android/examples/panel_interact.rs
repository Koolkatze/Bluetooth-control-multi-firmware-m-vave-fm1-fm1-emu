//! Regression: fast Android panel taps must survive JNI/worker batching.
//! Emulates the real FM-1 firmware + inputs through the same Rust worker used
//! by AAudio. Capture framebuffer snapshots after PLAY / SEQ / HOME / PLAY.
//! `cargo run --release -p fm1-android --example panel_interact -- urban1.fwsc OUTDIR`
use fm1_android::{run_worker, Cmd, Machine, Shared};
use std::{fs, path::PathBuf, sync::{mpsc, atomic::Ordering}, thread, time::Duration};

fn drive(shared: &Shared, ms: u32) {
    for _ in 0..ms / 3 {
        for _ in 0..144 { shared.ring.pop(); }
        thread::sleep(Duration::from_millis(3));
        assert!(!shared.halted.load(Ordering::Relaxed), "FM-1 emulator CPU halted");
    }
}
fn snapshot(shared: &Shared) -> Vec<u16> {
    shared.lcd.lock().expect("LCD mutex").clone()
}
fn changed(a: &[u16], b: &[u16]) -> usize {
    a.iter().zip(b).filter(|(x,y)|x!=y).count()
}
fn save_ppm(path: &std::path::Path, px: &[u16]) {
    let mut data = format!("P6\n240 240\n255\n").into_bytes();
    for &v in px {
        // Framebuffer stores 16-bit RGB565 in device display byte order.
        let v = v.rotate_left(8);
        data.push((((v>>11)&31)*255/31) as u8);
        data.push((((v>>5)&63)*255/63) as u8);
        data.push(((v&31)*255/31) as u8);
    }
    fs::write(path, data).expect("write screenshot");
}
fn tap(tx: &mpsc::Sender<Cmd>, id: u8) {
    // Deliberately send both edges BEFORE the worker has any time to
    // emulate scanner ticks: this reproduced the Android 'unresponsive' bug.
    tx.send(Cmd::Key(id,true)).unwrap();
    tx.send(Cmd::Key(id,false)).unwrap();
}
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let fw = std::env::args().nth(1).ok_or("usage: panel_interact FIRMWARE.fwsc OUTDIR")?;
    let dir = PathBuf::from(std::env::args().nth(2).unwrap_or_else(|| "build/urban-panel".into()));
    fs::create_dir_all(&dir)?;
    let machine = Machine::boot(&fs::read(fw)?,60)?;
    let shared = Shared::new();
    let worker_shared = shared.clone();
    let (tx,rx) = mpsc::channel::<Cmd>();
    let worker = thread::spawn(move|| run_worker(machine,worker_shared,rx,48000,None));

    drive(&shared,1200);
    let stopped = snapshot(&shared);
    save_ppm(&dir.join("01-stopped.ppm"), &stopped);

    tap(&tx,12); // PLAY: FM-1 physical matrix ID 12
    drive(&shared,1400);
    let playing = snapshot(&shared);
    let diff_play = changed(&stopped,&playing);
    save_ppm(&dir.join("02-playing.ppm"),&playing);
    println!("PLAY screen changed {diff_play} pixels");
    assert!(diff_play>30,"PLAY tap was lost: firmware still displays STOPPED");

    tap(&tx,11); // SEQ (the Tracks page)
    drive(&shared,500);
    let tracks=snapshot(&shared);
    let diff_tracks=changed(&playing,&tracks);
    save_ppm(&dir.join("03-tracks.ppm"),&tracks);
    println!("SEQ screen changed {diff_tracks} pixels");
    assert!(diff_tracks>50,"SEQ tap was lost: firmware did not open TRACKS");

    tap(&tx,8); // HOME
    drive(&shared,450);
    let song=snapshot(&shared);
    let diff_home=changed(&tracks,&song);
    save_ppm(&dir.join("04-song.ppm"),&song);
    println!("HOME screen changed {diff_home} pixels");
    assert!(diff_home>50,"HOME tap was lost: firmware did not return to SONG");

    tap(&tx,12);
    drive(&shared,400);
    let stopped_again=snapshot(&shared);
    let diff_stop=changed(&song,&stopped_again);
    save_ppm(&dir.join("05-stopped.ppm"),&stopped_again);
    println!("STOP screen changed {diff_stop} pixels");
    assert!(diff_stop>30,"second PLAY tap was lost");

    shared.quit.store(true, Ordering::Release);
    let _ = tx.send(Cmd::Stop);
    worker.join().expect("worker panic");
    println!("URBAN-1 native Android-equivalent panel interaction test passed");
    Ok(())
}
