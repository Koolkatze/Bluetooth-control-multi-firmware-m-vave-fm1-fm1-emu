package com.koolkatze.fm1offline;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Pure JDK test: no Android SDK, Bluetooth hardware or firmware binary needed. */
public final class MidiPanelMapperTest {
    private static final class Events implements MidiPanelMapper.Sink {
        final List<String> output = new ArrayList<>();
        @Override public void midi(byte[] msg) {
            StringBuilder b = new StringBuilder("M");
            for (byte v : msg) b.append(String.format(":%02X", v & 255));
            output.add(b.toString());
        }
        @Override public void panel(int kind, int id, int value) {
            output.add("P:" + kind + ":" + id + ":" + value);
        }
        void expect(String... expected) {
            if (!output.equals(Arrays.asList(expected)))
                throw new AssertionError("expected " + Arrays.toString(expected) + ", got " + output);
            output.clear();
        }
    }

    public static void main(String[] args) {
        MidiPanelMapper map = new MidiPanelMapper();
        Events e = new Events();

        map.accept(new byte[]{(byte)0x90, 53, 100}, e);
        e.expect("M:90:35:64"); // plain MIDI mode must not hijack the keyboard

        map.setMode(MidiPanelMapper.Mode.CTL1);
        map.accept(new byte[]{(byte)0x90, 53, 100, (byte)0x80, 53, 0}, e);
        e.expect("P:0:14:1", "P:0:14:0");

        map.accept(new byte[]{(byte)0x99, 36, 100}, e);
        e.expect("M:99:24:64"); // sequencer drums remain musical MIDI

        // CTL-1 knob bank A, absolute CC values -> signed encoder steps.
        map.accept(new byte[]{(byte)0xb0, 74, 50}, e);
        e.expect(); // first value establishes the baseline
        map.accept(new byte[]{(byte)0xb0, 74, 53, (byte)0xb0, 71, 75,
                (byte)0xb0, 71, 74}, e);
        e.expect("P:1:2:3", "P:1:3:-1");
        map.accept(new byte[]{(byte)0xb0, 73, 60, (byte)0xb0, 73, 62}, e);
        e.expect("P:1:2:2"); // bank B controls the same emulated KNOB1

        map.accept(new byte[]{(byte)0xb0, 64, 127, (byte)0xb0, 64, 0}, e);
        e.expect("P:0:4:1", "P:0:4:0");
        map.accept(new byte[]{(byte)0xb0, 11, 127}, e);
        e.expect("P:2:0:1023");
        map.accept(new byte[]{(byte)0xc0, 3, (byte)0xc0, 4}, e);
        e.expect("P:1:6:1");
        map.accept(new byte[]{(byte)0xfa, (byte)0xfc}, e);
        e.expect("P:0:12:1", "P:0:12:0", "P:0:12:1", "P:0:12:0");

        // Running status spanning packets, interleaved MIDI realtime.
        map.accept(new byte[]{(byte)0x90, 53}, e);
        e.expect();
        map.accept(new byte[]{100, 54, 100, (byte)0xf8, 53, 0, 54, 0}, e);
        e.expect("P:0:14:1", "P:0:15:1", "M:F8", "P:0:14:0", "P:0:15:0");

        // A disconnected controller must never leave virtual notes pressed.
        map.accept(new byte[]{(byte)0x90, 55, 100}, e);
        e.expect("P:0:16:1");
        map.releaseKeys(e);
        e.expect("P:0:16:0");
        map.releaseKeys(e);
        e.expect();

        map.setMode(MidiPanelMapper.Mode.REMOTE);
        map.accept(new byte[]{(byte)0x9f, 48, 100, (byte)0x8f, 48, 0}, e);
        e.expect("P:0:14:1", "P:0:14:0");
        map.accept(new byte[]{(byte)0xbf, 80, 127, (byte)0xbf, 80, 0,
                (byte)0xbf, 94, 1, (byte)0xbf, 95, 127,
                (byte)0xbf, 100, 6, (byte)0xbf, 101, 127}, e);
        e.expect("P:0:0:1", "P:0:0:0", "P:1:0:1",
                "P:1:6:-1", "P:1:5:6", "P:2:0:1023");
        map.accept(new byte[]{(byte)0x90, 60, 99, (byte)0x80, 60, 0}, e);
        e.expect("M:90:3C:63", "M:80:3C:00"); // ch1 musical performance preserved
        map.accept(new byte[]{(byte)0xf0, 1, 2, 3, (byte)0xf7}, e);
        e.expect();
        System.out.println("MIDI panel mapper: all assertions passed");
    }
}
