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
    public static native void midi(byte[] bytes);
    public static native void control(int kind, int key, int value);
    public static native byte[] lcd();
    /** Bit 0 ready; bit 1 firmware audio started; bit 2 CPU halted;
        upper bits = cumulative callback starvation samples. */
    public static native long stats();
}
