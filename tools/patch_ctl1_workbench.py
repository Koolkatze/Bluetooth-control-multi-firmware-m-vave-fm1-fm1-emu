#!/usr/bin/env python3
"""Optional CTL-1 source adapter for the M-VAVE FM-1 Workbench Remote v1.

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
 * Dedicated MIDI channel 16: physical switches and encoders retain their
 * electrical identity even if the on-device CTL-1 page changes.
 * Midi messages to USB/BLE, not to the local GM synth.
 * See Workbench docs/midi-mapping.md.
 */
static void wb_remote_packet(uint32_t st, uint32_t d1, uint32_t d2)
{
    send(pkt3(st, d1, d2), TO_USB | TO_BLE | TO_NOSYNTH);
}
static void wb_remote_note(int key, int on, int velocity)
{
    /* key 0..26 is chromatic F3..G5 on the original hardware */
    if (key >= 0 && key < NKEYS)
        wb_remote_packet(on ? 0x9fu : 0x8fu, 48u + (uint32_t)key,
                         on ? (uint32_t)(velocity > 0 ? velocity : 100) : 0u);
}
static void wb_remote_button(int button, int on)
{
    /* plat.h: FX..REC ids 0..11, OCT-/OCT+ ids 12/13.
     * Workbench panel: OCT-/OCT+ ids 0/1, FX..REC ids 2..13.
     */
    if (button >= 0 && button < NB)
        wb_remote_packet(0xbfu, 80u + (uint32_t)((button + 2) % NB),
                         on ? 127u : 0u);
}
static void wb_remote_encoder(int role, int32_t clicks)
{
    /* plat.h order: SELECT, ALGORITHM, PRESETS, KNOB1..4 */
    static const uint8_t remote_cc[NE] = {94, 96, 95, 97, 98, 99, 100};
    int32_t n = clicks > 24 ? 24 : clicks < -24 ? -24 : clicks;
    if (role < 0 || role >= NE) return;
    while (n) {
        int32_t part = n > 0 ? (n > 6 ? 6 : n) : (n < -6 ? -6 : n);
        wb_remote_packet(0xbfu, remote_cc[role],
                         (uint32_t)(part > 0 ? part : 128 + part));
        n -= part;
    }
}
static void wb_remote_master(int32_t raw)
{
    static int previous = -1;
    int v = raw * 127 / 4000;  /* identical to CTL-1 master_pot ADC curve */
    if (v < 0) v = 0;
    if (v > 127) v = 127;
    if (v != previous) {
        previous = v;
        wb_remote_packet(0xbfu, 101u, (uint32_t)v);
    }
}
"""
    swap(anchor, anchor + "\n" + helpers)
    # Physical key events are transmitted as remote keys. To avoid a double
    # note-on, physical key performance remains audible on CTL-1 locally but
    # does not ALSO travel over USB/BLE. Sequencer/arp MIDI stays untouched.
    swap(
        "            play(0x90u, q[i], vel);",
        "            send(pkt3(0x90u | proj.chan, q[i], vel), TO_SYNTH);",
    )
    swap(
        "            play(0x80u, ui.knotes[k][i], 0);",
        "            send(pkt3(0x80u | proj.chan, ui.knotes[k][i], 0), TO_SYNTH);",
    )
    # Emit the physical matrix transition BEFORE UI-dependent handling,
    # including sequencer step edit and touch-sensitive velocity.
    swap(
        "        if (ch >> i & 1u) {\n"
        "            if (ui.view != V_SEQ && touch_live && touch_sensed(i))",
        "        if (ch >> i & 1u) {\n"
        "            wb_remote_note((int)i, (keys >> i & 1u) != 0, (int)proj.vel);\n"
        "            if (ui.view != V_SEQ && touch_live && touch_sensed(i))",
    )
    swap(
        "        if (ch >> i & 1u) {\n"
        "            if (btn >> i & 1u)\n"
        "                button_down((int)i);",
        "        if (ch >> i & 1u) {\n"
        "            wb_remote_button((int)i, (btn >> i & 1u) != 0);\n"
        "            if (btn >> i & 1u)\n"
        "                button_down((int)i);",
    )
    for role in ["EN_SELECT", "EN_ALGO", "EN_PRESET"]:
        swap(
            f"    if ((e = plat_enc({role})) != 0) {{",
            f"    if ((e = plat_enc({role})) != 0) {{\n"
            f"        wb_remote_encoder({role}, e);",
        )
    swap(
        "        if ((e = plat_enc(EN_K1 + (int)i)) != 0) {",
        "        if ((e = plat_enc(EN_K1 + (int)i)) != 0) {\n"
        "            wb_remote_encoder(EN_K1 + (int)i, e);",
    )
    swap(
        "    ui.master_raw += (raw - ui.master_raw) / 4;",
        "    ui.master_raw += (raw - ui.master_raw) / 4;\n"
        "    wb_remote_master(ui.master_raw);",
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
    backup = file.with_suffix(".workbench-backup.c")
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
    print("No firmware was compiled or flashed. See docs/midi-mapping.md before flashing.")


if __name__ == "__main__":
    main()
