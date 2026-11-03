/* SPDX-License-Identifier: MPL-2.0
 * Blackhole Remote: MIDI controller.
 * Original 240 x 240 pixel-art interface for the physical M-VAVE FM-1.
 * Included into CTL-1 app/ui.c after its panel/model definitions.
 * All coordinates and partial canvas blits stay within the square LCD.
 *
 * No graphics are streamed from the phone. A 64-colour RGB565 bitmap
 * adapted from the square reference image is drawn natively at 240x144,
 * plus a live 64x44 image preview, without overfilling the 240x240 LCD.
 * The playable keyboard always follows plat_keys() -> ui.keys, and
 * physical note LEDs follow the same state in bh_remote_leds().
 */

#define BH_BG       RGB(7, 7, 20)
#define BH_VOID     RGB(0, 0, 5)
#define BH_CARD     RGB(19, 16, 40)
#define BH_BORDER   RGB(62, 43, 101)
#define BH_MUTED    RGB(123, 111, 163)
#define BH_WHITE    RGB(245, 238, 255)
#define BH_PURPLE   RGB(176, 84, 250)
#define BH_VIOLET   RGB(100, 49, 195)
#define BH_MAGENTA  RGB(236, 79, 206)
#define BH_ORANGE   RGB(255, 159, 77)
#define BH_CYAN     RGB(42, 231, 245)
#define BH_GREEN    RGB(98, 241, 152)
#define BH_KEYW     RGB(220, 221, 253)
#define BH_KEYB     RGB(18, 15, 41)

static uint32_t bh_top_signature = 0xffffffffu;
static uint32_t bh_key_signature = 0xffffffffu;

/* 37,376 indexed colour samples generated off-line by
 * tools/generate_blackhole_bitmap.py from the approved square artwork.
 * The indexed data are ROM constants; painting into gfx.c's existing
 * 240x144 canvas does NOT allocate a second framebuffer on the FM-1. */
#include "blackhole_bitmap.h"

static uint32_t bh_boot_splash_until;

static void bh_remote_splash(void)
{
    uint32_t i;
    /* 240x144 real raster pixels: no more 4x4 procedural tiles. */
    cv_begin(240, 144, BH_BG);
    for (i = 0; i < 240u * 144u; ++i)
        cv_px[i] = bh_art_palette[bh_art_splash[i]];
    cv_blit(0, 0);

    /* Native 96-pixel bottom band completes the exact 240x240 square.
     * Draw useful brand information, never invented percentages or
     * fictitious BLE/USB link states during the boot screen. */
    cv_begin(240, 96, BH_BG);
    cv_rect(0, 0, 240, 2, BH_BORDER);
    cv_rect(8, 10, 224, 72, BH_CARD);
    cv_rect(10, 12, 220, 68, BH_BG);
    cv_text((240 - text_w(&FONT_B, "FM-1 / MIDI")) / 2, 19,
            &FONT_B, "FM-1 / MIDI", BH_WHITE);
    cv_text((240 - text_w(&FONT_XS, "USB + BLE CONTROLLER")) / 2, 43,
            &FONT_XS, "USB + BLE CONTROLLER", BH_CYAN);
    cv_text((240 - text_w(&FONT_XS, "INITIALIZING...")) / 2, 61,
            &FONT_XS, "INITIALIZING...", BH_PURPLE);
    cv_rect(20, 88, 200, 3, BH_BORDER);
    cv_rect(20, 88, 120, 3, BH_CYAN);
    cv_blit(0, 144);
}

/* A real 64x44 cropped image of the accretion disk, on the live HUD.
 * It uses the SAME RGB565 palette as the detailed splash image. */
static void bh_draw_live_hole(int32_t left, int32_t top)
{
    uint32_t y, x;
    for (y = 0; y < 44u; ++y) {
        for (x = 0; x < 64u; ++x) {
            int32_t dx = left + (int32_t)x, dy = top + (int32_t)y;
            if ((uint32_t)dx < 240u && (uint32_t)dy < 144u)
                cv_px[(uint32_t)dy * 240u + (uint32_t)dx] =
                    bh_art_palette[bh_art_widget[y * 64u + x]];
        }
    }
}

/* Header + persistent USB/BLE/battery HUD: 240 x 144. Percent is an
 * estimate from the FM-1 battery ADC, not a calibrated fuel gauge. */
static void bh_remote_header(void)
{
    int pct = plat_battery_percent();
    int usb = plat_usb_midi_connected();
    int ble = FM1B_BLE && plat_ble_connected();
    int ble_wait = FM1B_BLE && plat_ble_linked();
    uint32_t phase = plat_ms() / 650u & 1u;
    uint32_t sig = ((uint32_t)(pct + 2) & 127u)
                 | ((uint32_t)usb << 8)
                 | ((uint32_t)ble << 9)
                 | ((uint32_t)ble_wait << 10)
                 | ((uint32_t)phase << 11);
    char number[16];
    if (bh_top_signature == sig)
        return;
    bh_top_signature = sig;

    cv_begin(240, 144, BH_BG);
    cv_rect(0, 0, 240, 2, BH_VIOLET);
    cv_text(7, 8, &FONT_B, "BLACKHOLE", BH_WHITE);
    cv_text(134, 8, &FONT_B, "REMOTE", BH_PURPLE);
    cv_text(8, 28, &FONT_XS, "MIDI CONTROLLER", BH_CYAN);
    cv_rect(5, 44, 230, 1, BH_BORDER);

    cv_rect(5, 49, 71, 48, BH_CARD);
    cv_rect(84, 49, 71, 48, BH_CARD);
    cv_rect(163, 49, 72, 48, BH_CARD);
    cv_rect(5, 49, 71, 2, BH_PURPLE);
    cv_rect(84, 49, 71, 2, usb ? BH_CYAN : BH_BORDER);
    cv_rect(163, 49, 72, 2, ble ? BH_CYAN : BH_BORDER);

    cv_text(10, 54, &FONT_XS, "BATT EST.", BH_MUTED);
    if (pct < 0) {
        cv_text(11, 69, &FONT_B, "--%", BH_WHITE);
    } else {
        put_s(put_u(number, (uint32_t)pct), "%");
        cv_text(11, 67, &FONT_B, number, pct <= 15 ? BH_ORANGE : BH_GREEN);
        cv_rect(10, 90, 57, 3, BH_BORDER);
        cv_rect(10, 90, (pct * 57) / 100, 3, BH_GREEN);
    }

    cv_text(89, 54, &FONT_XS, "USB MIDI", BH_MUTED);
    cv_text(89, 70, &FONT_B, usb ? "LINK" : "OFF",
            usb ? BH_GREEN : BH_MUTED);
    cv_rect(148, 77, 4, 4, usb ? BH_CYAN : BH_BORDER);

    cv_text(168, 54, &FONT_XS, "BLUETOOTH", BH_MUTED);
    cv_text(168, 70, &FONT_B,
            !FM1B_BLE ? "N/A" : ble ? "LINK" : ble_wait ? "WAIT" : "OFF",
            ble ? BH_GREEN : ble_wait ? BH_ORANGE : BH_MUTED);
    cv_rect(226, 77, 4, 4, ble ? BH_CYAN : BH_BORDER);

    /* Small live black-hole glyph between the two activity labels. */
    bh_draw_live_hole(88, 100);
    cv_text(9, 118, &FONT_XS, "PHYSICAL", BH_MUTED);
    cv_text(9, 131, &FONT_XS, "CONTROL", BH_CYAN);
    cv_text(176, 118, &FONT_XS, "27 KEYS", BH_MUTED);
    cv_text(176, 131, &FONT_XS, "ACTIVE", phase ? BH_CYAN : BH_PURPLE);
    cv_blit(0, 0);
}

/* Direct, continuously updated rendering from the EXACT matrix read by
 * wb_remote_input: 16 white + 11 black keys = all 27 FM-1 keys. */
static void bh_remote_keyboard(void)
{
    int32_t i, x, k;
    uint32_t held = ui.keys & ((1u << NKEYS) - 1u);
    if (bh_key_signature == held)
        return;
    bh_key_signature = held;

    cv_begin(240, 96, BH_BG);
    cv_rect(0, 0, 240, 2, BH_PURPLE);
    cv_text(7, 5, &FONT_B, "LIVE KEYBOARD", BH_WHITE);
    cv_text(185, 8, &FONT_XS, "CH 16", BH_CYAN);
    cv_rect(4, 23, 232, 54, BH_BORDER);
    for (i = 0; i < 16; i++) {
        x = 6 + i * 14;
        k = WHITE_K[i];
        cv_rect(x, 25, 13, 49, (held >> k & 1u) ? BH_CYAN : BH_KEYW);
        cv_rect(x, 72, 13, 2, (held >> k & 1u) ? BH_GREEN : BH_BORDER);
    }
    for (i = 0; i < 15; i++) {
        k = WHITE_K[i] + 1;
        if (WHITE_K[i + 1] != k + 1)
            continue;
        x = 6 + i * 14 + 9;
        cv_rect(x, 25, 8, 31, (held >> k & 1u) ? BH_MAGENTA : BH_KEYB);
        if (held >> k & 1u)
            cv_rect(x, 53, 8, 3, BH_WHITE);
    }
    cv_rect(4, 81, 232, 1, BH_BORDER);
    cv_text(7, 85, &FONT_XS, "27-KEY PHYSICAL MIDI  /  REMOTE V2", BH_MUTED);
    cv_blit(0, 144);
}

/* Physical key LEDs follow actual switches exactly, not a sequencer,
 * mock note animation, or page-specific CTL-1 keyboard behaviour. */
static void bh_remote_leds(void)
{
    uint32_t keys = ui.keys & ((1u << NKEYS) - 1u);
    uint32_t buttons = ui.btn & ((1u << NB) - 1u);
    uint32_t idle_buttons = (((1u << NB) - 1u) & ~buttons) & ~(1u << B_PLAY);
    if (buttons & (1u << B_PLAY))
        buttons |= LED_PLAY_GREEN;
    plat_lights(buttons, keys, idle_buttons, 0u, 1);
}

static void bh_remote_frame(void)
{
    bh_remote_header();
    plat_yield();
    bh_remote_keyboard();
    bh_remote_leds();
}
