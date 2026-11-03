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
    swap(anchor, anchor + "\n" + helpers + '\n#include "blackhole_remote_display.inc"\n')

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
    # The remote screen is wholly separate from CTL-1's six pages and
    # redraws on physical key presses / connection / ADC changes.
    swap(
        "void ui_frame(void)\n{\n    input();",
        "void ui_frame(void)\n{\n"
        "#if WB_REMOTE_ONLY\n"
        "    input();\n"
        "    dev_scan();\n"
        "    bh_remote_frame();\n"
        "    ui.frame++;\n"
        "#else\n"
        "    input();"
    )
    swap(
        "    ui.frame++;\n}\n\n/* the Bluetooth page scans",
        "    ui.frame++;\n"
        "#endif\n"
        "}\n\n/* the Bluetooth page scans"
    )
    swap(
        "    ui.touched = -1;\n    ui.master_sent = -1;",
        "    ui.touched = -1;\n"
        "    bh_top_signature = 0xffffffffu;\n"
        "    bh_key_signature = 0xffffffffu;\n"
        "    ui.master_sent = -1;"
    )
    return text



def blackhole_extensions(root: Path) -> dict[Path, str]:
    """Validate all upstream base/CTL-1 revisions before mutating them.

    One Workbench MIDI profile is kept (CC102=42, channel 16), so Android
    remains compatible. The new name, splash, 240x240 screen and calibrated
    percentage estimate live exclusively in the physical firmware build.
    """
    edits: dict[Path, list[tuple[str, str]]] = {
        root / "app" / "app.h": [
            ('#define APP_NAME "CTL-1"', '#define APP_NAME "BLACKHOLE REMOTE"'),
            ('#define APP_TAGLINE "A MIDI controller for the FM-1"',
             '#define APP_TAGLINE "MIDI controller"'),
            ('#define APP_BLE_AT_BOOT (!proj.ble_off)',
             '#define APP_BLE_AT_BOOT 1 /* Blackhole Remote advertises at every boot */'),
        ],
        root / "app" / "fm1.toml": [
            ('name = "CTL"', 'name = "blackhole"'),
            ('usb_manufacturer = "CTL-1"', 'usb_manufacturer = "Blackhole Remote"'),
            ('usb_product = "CTL-1 FM-1"', 'usb_product = "Blackhole MIDI FM-1"'),
            ('tagline = "A MIDI controller for the FM-1"',
             'tagline = "Blackhole Remote: MIDI controller"'),
            ('repo = "https://github.com/charlesvestal/fm1-ctl"',
             'repo = "https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu"'),
        ],
        root / "app" / "app.c": [
            (
                'void app_splash(void)                    /* ui.c\'s paper and ink */\n'
                '{\n'
                '    lcd_fill(0, 0, 240, 240, RGB(251, 248, 241));\n'
                '    cv_begin(240, 64, RGB(251, 248, 241));\n'
                '    {\n'
                '        int32_t w = text_w(&FONT_L, APP_NAME);\n'
                '        cv_text((240 - w) / 2, 0, &FONT_L, APP_NAME, RGB(30, 43, 90));\n'
                '        cv_rect((240 - w) / 2, 37, w, 2, RGB(229, 72, 46));\n'
                '        w = text_w(&FONT_S, APP_TAGLINE);\n'
                '        cv_text((240 - w) / 2, 44, &FONT_S, APP_TAGLINE, RGB(122, 128, 152));\n'
                '    }\n'
                '    cv_blit(0, 92);\n'
                '}',
                'void app_splash(void)\n'
                '{\n'
                '    bh_remote_splash(); /* native 240x240 pixel-art black hole */\n'
                '}',
            ),
        ],
        root / "base" / "include" / "plat.h": [
            (
                'int plat_battery(void);',
                'int plat_battery(void);\n'
                '/* Approximate percentage from battery ADC, not a fuel gauge.\n'
                ' * -1 before the first reading. On USB power, still reads ADC. */\n'
                'int plat_battery_percent(void);\n'
                '/* 1 iff a USB host has configured the device USB MIDI link. */\n'
                'int plat_usb_midi_connected(void);',
            ),
        ],
        root / "base" / "device" / "plat_fm1.c": [
            (
                '/* LEDs: the picture is built off-line',
                '/* Blackhole Remote battery readout. The stock bar thresholds\n'
                ' * are 531 / 561 / 591 ADC counts; the wider 516..615 range\n'
                ' * is a deliberately approximate linear 0..100 scale.\n'
                ' * Do not mistake it for a calibrated coulomb counter. */\n'
                'int plat_battery_percent(void)\n'
                '{\n'
                '    int32_t raw = batt_raw;\n'
                '    if (raw <= 0) return -1;\n'
                '    return (int)clamp((raw - 516) * 100 / 99, 0, 100);\n'
                '}\n'
                'int plat_usb_midi_connected(void)\n'
                '{\n'
                '    return usb.config && !usb.suspended;\n'
                '}\n\n'
                '/* LEDs: the picture is built off-line',
            ),
        ],
        root / "base" / "host" / "plat_host.c": [
            (
                'int plat_battery(void) { return 3; }',
                'int plat_battery(void) { return 3; }\n'
                'int plat_battery_percent(void) { return 100; }\n'
                'int plat_usb_midi_connected(void) { return 0; }',
            ),
        ],
    }
    changes = {}
    for file, replacements in edits.items():
        source = file.read_text(encoding="utf-8")
        for before, after in replacements:
            if source.count(before) != 1:
                raise ValueError(
                    f"Blackhole Remote unsupported upstream in {file}: "
                    f"expected one anchor {before[:90]!r}; found {source.count(before)}"
                )
            source = source.replace(before, after, 1)
        changes[file] = source
    graphics = Path(__file__).with_name("blackhole_remote_display.inc")
    if not graphics.is_file():
        raise ValueError(f"Missing square Blackhole Remote renderer: {graphics}")
    changes[root / "app" / "blackhole_remote_display.inc"] = graphics.read_text(encoding="utf-8")
    if "bh_remote_keyboard" not in changes[root / "app" / "blackhole_remote_display.inc"]:
        raise ValueError("The responsive 27-key renderer was not provided")
    return changes


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ctl1_dir", type=Path, help="Local clone of charlesvestal/fm1-ctl")
    parser.add_argument("--dry-run", action="store_true", help="Check applicability without editing")
    args = parser.parse_args()
    file = args.ctl1_dir.expanduser().resolve() / "app" / "ui.c"
    if not file.is_file():
        parser.error(f"Missing upstream CTL-1 source {file}")
    original = file.read_text(encoding="utf-8")
    modified = apply(original)
    changes = blackhole_extensions(args.ctl1_dir.expanduser().resolve())
    changes[file] = modified
    if args.dry_run:
        print("OK: pinned CTL-1 and fm1-base accept Blackhole Remote 240x240 patch")
        print("OK: battery ADC percentage, USB/BLE link, 27 lit keys, MIDI mapping")
        return
    # Preflight every source and backup before writing any file.
    for source in changes:
        if source.exists() and source.with_suffix(source.suffix + ".blackhole.orig").exists():
            parser.error(f"Backup already exists for {source}")
    for source, updated in changes.items():
        if source.exists():
            source.with_suffix(source.suffix + ".blackhole.orig").write_text(
                source.read_text(encoding="utf-8"), encoding="utf-8")
        source.write_text(updated, encoding="utf-8")
    print("OK: Blackhole Remote MIDI controller source patched:")
    for source in changes:
        print(f"  {source}")
    print("Blackhole display: 240x240 splash + live 27-key responsive keyboard")
    print("MIDI: original Workbench Remote v1 protocol preserved on USB/BLE")
    print("No firmware was flashed; use a safe recovery method before testing.")


if __name__ == "__main__":
    main()
