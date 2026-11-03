#!/usr/bin/env python3
"""Blackhole Remote: MIDI controller — physical M-VAVE FM-1 firmware.


Usage:
    python3 tools/patch_ctl1_workbench.py /path/to/fm1-ctl

This edits only the LOCAL clone's app/ui.c after strict source checks.
It does NOT compile, flash, update or connect to the FM-1 hardware.
Keep a separate clone of upstream CTL-1 before running this script.

Upstream: https://github.com/charlesvestal/fm1-ctl
Licence of upstream sources remains MPL-2.0.
"""
from __future__ import annotations

import argparse
from pathlib import Path

TAG = "/* FM1-WORKBENCH-REMOTE-V1 */"


def apply(text: str) -> str:
    if TAG in text:
        raise ValueError("CTL-1 source already has the Workbench Remote extension")

    def swap(old: str, new: str) -> None:
        nonlocal text
        count = text.count(old)
        if count != 1:
            raise ValueError(
                f"CTL-1 source version incompatible with a safe patch: "
                f"expected one anchor, got {count}: {old[:110]!r}"
            )
        text = text.replace(old, new, 1)

    anchor = (
        "static void play(uint32_t st, uint32_t d1, uint32_t d2) "
        "{ send(pkt3(st | proj.chan, d1, d2), TO_USB | TO_BLE); }"
    )
    helpers = r"""
/* FM1-WORKBENCH-REMOTE-V1 */
/*
 * Dedicated USB/BLE MIDI channel 16. Every hardware button and encoder is
 * identified by physical position, regardless of the local CTL-1 page.
 * Musical MIDI channels 1..15 are not hijacked.
 *
 * Remote-only operation is intentional. Without it, CTL-1 also executes
 * page/transport/keyboard actions and sends duplicate MIDI while the same
 * switch is supposed to operate the virtual firmware in Workbench.
 */
#define WB_REMOTE_ONLY 1

static void wb_remote_packet(uint32_t st, uint32_t d1, uint32_t d2)
{
    send(pkt3(st, d1, d2), TO_USB | TO_BLE | TO_NOSYNTH);
}
static void wb_remote_note(int key, int on, int vel)
{
    if (key >= 0 && key < NKEYS)
        wb_remote_packet(on ? 0x9fu : 0x8fu, (uint32_t)(key + 48),
                         on ? (uint32_t)vel : 0u);
}
static void wb_remote_button(int button, int on)
{
    /* plat.h: FX..REC IDs 0..11, OCT-/OCT+ IDs 12/13.
     * FM-1 emulator: OCT-/OCT+ 0/1, FX..REC 2..13. */
    if (button >= 0 && button < NB)
        wb_remote_packet(0xbfu, (uint32_t)(80 + ((button + 2) % NB)),
                         on ? 127u : 0u);
}
static void wb_remote_encoder(int role, int32_t ticks)
{
    /* fm1-base role order: SELECT, ALGORITHM, PRESETS, KNOB1..4.
     * MIDI CC order: SELECT, PRESETS, ALGORITHM, KNOB1..4. */
    static const uint8_t cc_by_role[NE] = {94, 96, 95, 97, 98, 99, 100};
    if (role < 0 || role >= NE)
        return;
    /* Losslessly split large encoder turns into MIDI relative messages.
     * 1..6 = positive; 127..122 = negative. */
    while (ticks) {
        int32_t steps = ticks > 0 ? (ticks > 6 ? 6 : ticks) :
                           (ticks < -6 ? -6 : ticks);
        wb_remote_packet(0xbfu, cc_by_role[role],
                         (uint32_t)(steps > 0 ? steps : 128 + steps));
        ticks -= steps;
    }
}
static void wb_remote_input(void)
{
    static uint32_t held_keys = 0u, held_buttons = 0u;
    static int initialized = 0;
    static int32_t master_smooth = 0;
    static int master_previous = -1;
    static uint32_t next_hello_ms = 0u;
    uint32_t keys = plat_keys(), buttons = plat_buttons();
    uint32_t changed_keys = keys ^ held_keys;
    uint32_t changed_buttons = buttons ^ held_buttons;
    uint32_t now = plat_ms();
    int i, value, raw, port;
    uint32_t midi_packet;
    int32_t ticks;

    if (!initialized) {
        initialized = 1;
        master_smooth = (int32_t)plat_master();
    }

    /* A periodic handshake makes Workbench AUTO detect this firmware after
     * USB/BLE connects (even when it connects after firmware boot). */
    if ((int32_t)(now - next_hello_ms) >= 0) {
        wb_remote_packet(0xbfu, 102u, 42u);
        /* MIDI handshake only. Do not cover the keyboard/status with a
         * splash-style message on every hello or Bluetooth reconnect. */
        next_hello_ms = now + 1500u;
        /* Resend held key/button STATE after reconnection. The receiver
         * suppresses duplicate state changes; no retriggered note-ons. */
        for (i = 0; i < NKEYS; ++i)
            if (keys & (1u << i))
                wb_remote_note(i, 1, (int)proj.vel);
        for (i = 0; i < NB; ++i)
            if (buttons & (1u << i))
                wb_remote_button(i, 1);
    }

    held_keys = keys;
    held_buttons = buttons;
    /* Keep physical key/button LEDs and idle timer responsive, but do NOT
     * interpret the keys as local CTL-1 pages or transport actions. */
    ui.keys = keys;
    ui.btn = buttons;
    if (changed_keys || changed_buttons)
        ui.act_t = now;
    for (i = 0; i < NKEYS; ++i)
        if (changed_keys & (1u << i))
            wb_remote_note(i, (keys & (1u << i)) != 0, (int)proj.vel);
    for (i = 0; i < NB; ++i)
        if (changed_buttons & (1u << i))
            wb_remote_button(i, (buttons & (1u << i)) != 0);

    /* Read EVERY encoder, even SELECT / ALGORITHM and all four knobs.
     * Emulated firmware decides whether a turn changes a page or value. */
    for (i = 0; i < NE; ++i) {
        ticks = plat_enc(i);
        if (ticks)
            wb_remote_encoder(i, ticks);
    }
    /* Keep RX queues drained without MIDI-thru loops back into Android.
     * Workbench Remote v1 is a physical-controller OUTPUT protocol. */
    while (plat_midi_in(&midi_packet, &port)) {
        (void)midi_packet;
        (void)port;
    }
    raw = (int)plat_master();
    master_smooth += ((int32_t)raw - master_smooth) / 4;
    value = (int)(master_smooth * 127 / 4000);
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    if (value != master_previous) {
        master_previous = value;
        wb_remote_packet(0xbfu, 101u, (uint32_t)value);
    }
}
"""
    swap(anchor, anchor + "\n" + helpers)

    # This is a DEDICATED controller mode. The original CTL-1 input loop
    # generates its own Start/Stop, sustain, CC, program changes and notes.
    # Running that loop simultaneously would double-trigger PLAY/REC/etc.
    # Wrap the original in #else for easy reversion in the source clone.
    swap(
        "static void input(void)\n{",
        "static void input(void)\n{\n"
        "#if WB_REMOTE_ONLY\n"
        "    wb_remote_input();\n"
        "#else"
    )
    swap(
        "}\n\n/* ------------------------------------------------------------- screen --- */",
        "#endif\n}\n\n/* ------------------------------------------------------------- screen --- */"
    )
    return text


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ctl1_dir", type=Path, help="Local clone of charlesvestal/fm1-ctl")
    parser.add_argument("--dry-run", action="store_true", help="Check applicability without editing")
    args = parser.parse_args()
    file = args.ctl1_dir.expanduser().resolve() / "app" / "ui.c"
    if not file.is_file():
        parser.error(f"Missing upstream CTL-1 source {file}")
    original = file.read_text(encoding="utf-8")
    modified = apply(original)  # fully validate in memory BEFORE any write
    if args.dry_run:
        print("OK: CTL-1 source accepts Workbench Remote v1 patch; no files changed")
        return
    backup = file.with_suffix(".workbench-backup.orig")
    if backup.exists():
        parser.error(f"Backup already exists; refusing to overwrite: {backup}")
    backup.write_text(original, encoding="utf-8")
    try:
        file.write_text(modified, encoding="utf-8")
    except Exception:
        file.write_text(original, encoding="utf-8")
        raise
    print(f"OK: patched {file}")
    print(f"Original saved as {backup}")
    print("No firmware was flashed. This build is BLACKHOLE REMOTE: MIDI controller.")


if __name__ == "__main__":
    main()
