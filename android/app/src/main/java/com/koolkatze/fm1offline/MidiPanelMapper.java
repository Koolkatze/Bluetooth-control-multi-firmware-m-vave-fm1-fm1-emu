package com.koolkatze.fm1offline;

import java.util.Arrays;

/**
 * Translates MIDI emitted by a physical FM-1 into the ORIGINAL FM-1 emulated
 * key matrix / encoder IDs. Independent of Android, firmware type and audio
 * engine. USB MIDI and BLE MIDI must use this same decoder.
 *
 * Standard CTL-1 sends musical notes and Play-page knob CCs, but it does NOT
 * send discrete MIDI messages for every button/encoder. Complete panel
 * mirroring requires a controller firmware implementing the optional
 * Workbench Remote protocol (channel 16) documented in docs/midi-mapping.md.
 */
public final class MidiPanelMapper {
    public enum Mode { MIDI_ONLY, CTL1, REMOTE }

    public interface Sink {
        void midi(byte[] message);
        /** kind 0=key (id 0..40), 1=encoder (hardware id 0..6), 2=MASTER (0..1023). */
        void panel(int kind, int id, int value);
    }

    private Mode mode = Mode.MIDI_ONLY;
    /** True only with the user's AUTO preference; explicit modes are never overridden. */
    private boolean remoteAutoDetect = false;
    // The patched physical controller announces this profile over channel 16.
    private static final int REMOTE_HELLO_CC = 102;
    private static final int REMOTE_HELLO_VALUE = 42;
    private int runningStatus = 0;
    private int status = 0;
    private int needed = 0;
    private int have = 0;
    private final int[] data = new int[2];
    private boolean inSysex = false;
    private final int[] previousCc = new int[128];
    private int previousProgram = -1;
    private final boolean[] pressed = new boolean[41];

    public MidiPanelMapper() { reset(); }

    public synchronized Mode getMode() { return mode; }

    public synchronized void setMode(Mode next) { setMode(next, false); }

    public synchronized void setMode(Mode next, boolean enableAutoRemote) {
        mode = next == null ? Mode.MIDI_ONLY : next;
        remoteAutoDetect = enableAutoRemote;
        reset();
    }

    public synchronized void reset() {
        runningStatus = status = needed = have = 0;
        inSysex = false;
        Arrays.fill(previousCc, -1);
        previousProgram = -1;
        Arrays.fill(pressed, false);
    }

    /** Release the matrix if BLE/USB disconnects with a held key or button. */
    public synchronized void releaseKeys(Sink sink) {
        if (sink == null) return;
        for (int id = 0; id < pressed.length; id++) {
            if (pressed[id]) {
                pressed[id] = false;
                sink.panel(0, id, 0);
            }
        }
    }

    private void key(Sink sink, int id, int value) {
        if (id < 0 || id >= pressed.length) return;
        pressed[id] = value != 0;
        sink.panel(0, id, value);
    }

    /** Handles complete messages, coalesced packets, and USB/BLE running status.
     * Real-time bytes can occur BETWEEN data bytes without breaking a message.
     * No MIDI bytes are echoed back to the physical instrument. */
    public synchronized void accept(byte[] bytes, Sink sink) {
        if (bytes == null || sink == null) return;
        for (byte raw : bytes) {
            final int b = raw & 0xff;
            if (b >= 0xf8) {
                // Clock and transport are *musical MIDI*, not button presses.
                // A START/STOP packet must not toggle PLAY in two places.
                if (mode != Mode.REMOTE || (b != 0xfa && b != 0xfb && b != 0xfc))
                    sink.midi(new byte[]{(byte)b});
                continue;
            }
            if (b == 0xf0) { inSysex = true; status = runningStatus = needed = have = 0; continue; }
            if (inSysex) {
                if (b == 0xf7) inSysex = false;
                continue; // the emulator has no streaming SysEx input here
            }
            if ((b & 0x80) != 0) {
                status = b;
                have = 0;
                if (b < 0xf0) {
                    runningStatus = b;
                    needed = ((b & 0xf0) == 0xc0 || (b & 0xf0) == 0xd0) ? 1 : 2;
                } else {
                    runningStatus = 0;
                    needed = (b == 0xf1 || b == 0xf3) ? 1 : b == 0xf2 ? 2 : 0;
                    if (needed == 0) {
                        sink.midi(new byte[]{(byte)b});
                        status = 0;
                    }
                }
                continue;
            }
            if (status == 0) {
                if (runningStatus == 0) continue;
                status = runningStatus;
                needed = ((status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0) ? 1 : 2;
            }
            if (needed <= 0 || have >= data.length) continue;
            data[have++] = b;
            if (have == needed) {
                byte[] message = needed == 1
                        ? new byte[]{(byte)status, (byte)data[0]}
                        : new byte[]{(byte)status, (byte)data[0], (byte)data[1]};
                route(message, sink);
                have = 0;
                if (status < 0xf0) status = runningStatus;
                else { status = 0; needed = 0; }
            }
        }
    }

    private void route(byte[] message, Sink sink) {
        final int s = message[0] & 255;
        final int kind = s & 0xf0;
        final int channel = (s & 15) + 1;
        final int d1 = message.length > 1 ? message[1] & 127 : 0;
        final int d2 = message.length > 2 ? message[2] & 127 : 0;

        // Fully automatic USB/BLE profile switching, based on an explicit
        // Workbench Remote handshake (not guesses based on incoming CCs).
        if (s == 0xbf && d1 == REMOTE_HELLO_CC && d2 == REMOTE_HELLO_VALUE) {
            if (remoteAutoDetect && mode != Mode.REMOTE) {
                releaseKeys(sink);
                // Do not reset the parser's running status mid-stream.
                Arrays.fill(previousCc, -1);
                previousProgram = -1;
                mode = Mode.REMOTE;
            }
            return; // never deliver the private handshake to emulated firmware
        }

        if (mode == Mode.REMOTE && channel == 16) {
            if ((kind == 0x80 || kind == 0x90) && d1 >= 48 && d1 <= 74) {
                key(sink, 14 + d1 - 48, (kind == 0x90 && d2 > 0) ? 1 : 0);
                return;
            }
            if (kind == 0xb0 && d1 >= 80 && d1 <= 93) {
                key(sink, d1 - 80, d2 >= 64 ? 1 : 0);
                return;
            }
            if (kind == 0xb0 && d1 >= 94 && d1 <= 100) {
                final int clicks = d2 > 64 ? d2 - 128 : d2; // +1..+6; 127..122 = -1..-6
                if (clicks != 0 && Math.abs(clicks) <= 12)
                    sink.panel(1, new int[]{0, 6, 1, 2, 3, 4, 5}[d1 - 94], clicks);
                return;
            }
            if (kind == 0xb0 && d1 == 101) {
                sink.panel(2, 0, (d2 * 1023 + 63) / 127);
                return;
            }
        }

        if (mode == Mode.CTL1) {
            // CTL-1's default octave is F3..G5. With an altered octave, scales,
            // chords or CTL-1's own arp/seq the physical-key index is unknown:
            // notes outside this range continue as ordinary performance MIDI.
            if (channel != 10 && (kind == 0x80 || kind == 0x90)
                    && d1 >= 53 && d1 <= 79) {
                key(sink, 14 + d1 - 53, (kind == 0x90 && d2 > 0) ? 1 : 0);
                return;
            }
            // CTL-1 Play page: A/B/C/D controller banks; defaults only.
            if (kind == 0xb0) {
                final int knob = legacyKnob(d1);
                if (knob >= 0) {
                    int old = previousCc[d1];
                    previousCc[d1] = d2;
                    if (old >= 0) {
                        int delta = d2 - old;
                        if (delta != 0) sink.panel(1, 2 + knob, Math.max(-6, Math.min(6, delta)));
                    }
                    return;
                }
                // CC64 = sustain and CC123 = all notes off, NOT ENV/REC.
                // Their use as virtual page buttons caused destructive and
                // confusing control actions; keep both as musical MIDI.
                if (d1 == 64 || d1 == 123) {
                    sink.midi(message);
                    return;
                }
                if (d1 == 11) {
                    sink.panel(2, 0, (d2 * 1023 + 63) / 127); // optional MASTER CC11
                    return;
                }
            }
            if (kind == 0xc0) {
                if (previousProgram >= 0) {
                    int steps = (d1 - previousProgram + 128) % 128;
                    if (steps > 64) steps -= 128;
                    if (steps != 0) sink.panel(1, 6, Math.max(-12, Math.min(12, steps)));
                }
                previousProgram = d1;
                return;
            }
        }
        sink.midi(message);
    }

    private static int legacyKnob(int cc) {
        switch (cc) {
            case 74: case 73: case 16: case 20: return 0;
            case 71: case 72: case 17: case 21: return 1;
            case 1:  case 75: case 18: case 22: return 2;
            case 7:  case 76: case 19: case 23: return 3;
            default: return -1;
        }
    }
}
