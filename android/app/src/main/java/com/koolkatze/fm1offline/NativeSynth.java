package com.koolkatze.fm1offline;

/**
 * Direct Android AAudio synth, NOT the WebView/AudioWorklet.
 *
 * FM-1 CPU emulation and firmware/JIT run in Rust on a dedicated background
 * thread. Android's low-latency AAudio callback only consumes native PCM.
 *
 * If this library is unavailable (unsupported ABI or older debug install),
 * the web emulation engine remains available as a fallback.
 */
public final class NativeSynth {
    public static final boolean AVAILABLE;
    static {
        boolean loaded;
        try {
            System.loadLibrary("fm1_android");
            loaded = true;
        } catch (UnsatisfiedLinkError | SecurityException error) {
            android.util.Log.e("FM1Mobile", "Native AAudio synth unavailable", error);
            loaded = false;
        }
        AVAILABLE = loaded;
    }

    private NativeSynth() {}

    public static native String boot(byte[] fwsc, int mhz, String identity, String saveDirectory);
    public static native void stop();
    /** Reopen only AAudio for a Bluetooth/USB/speaker route change.
     * Firmware CPU, song, MIDI, flash and LCD remain running.
     * Returns OK:<sample-rate> or ERROR:<reason>. Call off UI thread. */
    public static native String recoverAudio();
    /** Bit 0 AAudio error; bit 1 no output stream; higher bits
     * count actual hardware callback invocations for watchdog. */
    public static native long audioHealth();
    public static native void midi(byte[] bytes);
    public static native void control(int kind, int key, int value);
    public static native byte[] lcd();
    /** Bit 0 ready; bit 1 firmware audio started; bit 2 CPU halted;
        upper bits = cumulative callback starvation samples. */
    public static native long stats();
    /** Native firmware ms, PCM frames submitted, and instantaneous ring fill. */
    public static native String diagnostics();
}
