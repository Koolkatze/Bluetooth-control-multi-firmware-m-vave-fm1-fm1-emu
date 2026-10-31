//! Real native engine smoke and throughput diagnostic.
//! cargo run --release -p fm1-android --example native_bench -- package.fwsc
//!
//! Runs the actual FM-1 firmware engine on the host, with a fake 48kHz AAudio
//! consumer. It does not require Android SDK or speakers.
use fm1_android::{run_worker, Cmd, Machine, Shared};
use std::sync::atomic::Ordering;
use std::sync::mpsc;
use std::time::{Duration, Instant};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let firmware = std::env::args().nth(1).ok_or("usage: native_bench firmware.fwsc")?;
    let bytes = std::fs::read(&firmware)?;
    let machine = Machine::boot(&bytes, 60)?;
    let shared = Shared::new();
    let worker_state = shared.clone();
    let (tx, rx) = mpsc::channel::<Cmd>();
    let worker = std::thread::spawn(move || run_worker(machine, worker_state, rx, 48000, None));
    // Simulate native prefill before the AAudio stream starts.
    let prefill = Instant::now();
    while shared.ring.available() < 512 && prefill.elapsed() < Duration::from_millis(1000) {
        std::thread::sleep(Duration::from_millis(2));
    }
    let start = Instant::now();
    let mut received = 0u64;
    let mut absent = 0u64;
    // Simulate 128-frame PCM bursts. Jitter in the CI sleep timer is expected;
    // report both emulator and PCM rates rather than asserting speed >= 1.0.
    for _ in 0..850 {
        if start.elapsed() > Duration::from_secs(3) { break; }
        for _ in 0..128 {
            if shared.ring.pop().is_some() { received += 1; }
            else { absent += 1; }
        }
        std::thread::sleep(Duration::from_micros(2667));
    }
    let elapsed = start.elapsed();
    let emulated_ms = shared.emulated_ms.load(Ordering::Relaxed);
    let pcm = shared.produced_frames.load(Ordering::Relaxed);
    let available = shared.ring.available();
    shared.quit.store(true, Ordering::Release);
    let _ = tx.send(Cmd::Stop);
    worker.join().map_err(|_| "worker panic")?;
    println!("Native FM-1 bench: {firmware} | {:.2}s wall | {emulated_ms} ms emulated | {pcm} PCM frames | {received} played | {absent} missing | {available} queued | {:.0} PCM/s",
        elapsed.as_secs_f64(), pcm as f64 / (elapsed.as_secs_f64().max(0.001)));
    assert!(emulated_ms > 100, "firmware CPU made no meaningful progress");
    assert!(pcm > 1000, "native audio producer did not feed the ring");
    Ok(())
}
