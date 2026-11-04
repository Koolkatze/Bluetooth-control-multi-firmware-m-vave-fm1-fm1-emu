/* SPDX-License-Identifier: MPL-2.0
 * Blackhole Remote: MIDI controller, 240x240 square M-VAVE FM-1 LCD.
 *
 * Boot: short pixel-art glitch IN -> original high-resolution black-hole
 * artwork stays on screen indefinitely -> a REAL piano key or panel button
 * press triggers glitch OUT -> live controller.
 *
 * Live: the SAME 240x144 black-hole bitmap fills the background of the
 * top section. Telemetry cards are PREBLENDED at 60% panel / 40% artwork
 * (i.e. 40% TRANSPARENT). The 240x96 physical 27-key piano is fully opaque.
 * Both the on-screen notes and physical LEDs follow plat_keys() / ui.keys.
 * USB/BLE MIDI mapping and transmission are handled independently of UI.
 *
 * Hardware caveat: fm1-base's RGB565 canvas uses BYTE-SWAPPED pixels
 * (see gfx.c: cv_rect(), cv_pset(), swap16()). Any direct indexed bitmap
 * write MUST swap16() the palette entry or colours become green/incorrect.
 * The renderer never allocates a second 240x240 frame buffer.
 */

#define BH_BG       RGB(7, 7, 20)
#define BH_CARD     RGB(12, 10, 28)
#define BH_BORDER   RGB(62, 43, 101)
#define BH_MUTED    RGB(169, 149, 200)
#define BH_WHITE    RGB(245, 238, 255)
#define BH_PURPLE   RGB(176, 84, 250)
#define BH_MAGENTA  RGB(236, 79, 206)
#define BH_ORANGE   RGB(255, 159, 77)
#define BH_CYAN     RGB(42, 231, 245)
#define BH_GREEN    RGB(98, 241, 152)
#define BH_KEYW     RGB(220, 221, 253)
#define BH_KEYB     RGB(18, 15, 41)

#include "blackhole_bitmap.h"

static uint32_t bh_top_signature = 0xffffffffu;
static uint32_t bh_key_signature = 0xffffffffu;

/* State 0: glitch entrance; 1: WAIT indefinitely; 2: glitch exit;
 * 3: live controller. No timeout is allowed to leave the WAIT state. */
enum { BH_GLITCH_IN = 0, BH_WAIT_KEY, BH_GLITCH_OUT, BH_CONTROLLER };
#define BH_GLITCH_IN_MS 360u
#define BH_GLITCH_OUT_MS 360u
#define BH_GLITCH_FRAME_MS 65u
static uint8_t bh_intro_state;
static uint8_t bh_intro_pending;
static uint32_t bh_intro_started_ms;
static uint32_t bh_intro_last_draw_ms;
static uint32_t bh_intro_previous_keys;
static uint32_t bh_intro_previous_buttons;

/* Blend two RGB565 values BEFORE swapping to match the physical LCD.
 * 40% of the image is visible through the HUD card (60% dark panel). */
static uint16_t bh_mix565(uint16_t art, uint16_t panel)
{
    uint32_t r = (((uint32_t)(art >> 11) & 31u) * 2u +
                  ((uint32_t)(panel >> 11) & 31u) * 3u + 2u) / 5u;
    uint32_t g = (((uint32_t)(art >> 5) & 63u) * 2u +
                  ((uint32_t)(panel >> 5) & 63u) * 3u + 2u) / 5u;
    uint32_t b = (((uint32_t)art & 31u) * 2u +
                  ((uint32_t)panel & 31u) * 3u + 2u) / 5u;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

/* Every raw cv_px store is byte-swapped, exactly as gfx.c expects. */
static void bh_background(void)
{
    uint32_t i;
    cv_begin(240, 144, BH_BG);
    for (i = 0; i < 240u * 144u; ++i)
        cv_px[i] = swap16(bh_art_palette[bh_art_splash[i]]);
}

static void bh_glass_rect(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t xx, yy;
    for (yy = y; yy < y + h; ++yy) {
        if (yy < 0 || yy >= 144) continue;
        for (xx = x; xx < x + w; ++xx) {
            uint32_t pos;
            uint16_t art;
            if (xx < 0 || xx >= 240) continue;
            pos = (uint32_t)yy * 240u + (uint32_t)xx;
            art = swap16(cv_px[pos]);
            cv_px[pos] = swap16(bh_mix565(art, BH_CARD));
        }
    }
}

/* Real source art, not procedural four-pixel rectangles. The palette
 * conversion is done once per pixel. The glitch is deliberate horizontal
 * tearing and chromatic scanlines; it lasts only for the transitions. */
static void bh_glitch_image(uint32_t elapsed, int exiting)
{
    uint32_t y, x, frame = elapsed / BH_GLITCH_FRAME_MS;
    uint32_t duration = exiting ? BH_GLITCH_OUT_MS : BH_GLITCH_IN_MS;
    uint32_t amount = exiting ? elapsed * 18u / duration :
                                  (duration - elapsed) * 18u / duration;
    if (amount > 18u) amount = 18u;
    cv_begin(240, 144, BH_BG);
    for (y = 0; y < 144u; ++y) {
        int32_t tear = 0;
        uint32_t h = (y / 5u) * 29u + frame * 67u + 13u;
        if ((h & 7u) < 3u)
            tear = (int32_t)(h % (amount * 2u + 1u)) - (int32_t)amount;
        for (x = 0; x < 240u; ++x) {
            int32_t sx = (int32_t)x + tear;
            uint16_t c = BH_BG;
            if (sx >= 0 && sx < 240)
                c = bh_art_palette[bh_art_splash[y * 240u + (uint32_t)sx]];
            if (amount && (h % 13u == 3u || h % 19u == 7u) &&
                    (x + frame * 23u) % 17u < amount / 3u)
                c = (h & 1u) ? BH_MAGENTA : BH_CYAN;
            /* The final exit frame darkens only the art and immediately
             * hands off to the live screen after the transition finishes. */
            if (exiting && elapsed > (duration * 3u / 4u)) {
                uint32_t q = (duration - elapsed) * 4u * 255u / duration;
                uint32_t red = ((c >> 11) & 31u) * q / 255u;
                uint32_t grn = ((c >> 5) & 63u) * q / 255u;
                uint32_t blu = (c & 31u) * q / 255u;
                c = (uint16_t)((red << 11) | (grn << 5) | blu);
            }
            cv_px[y * 240u + x] = swap16(c);
        }
    }
    if (amount) {
        /* Wide, short tearing bars are clipped to the image area. */
        uint32_t k;
        for (k = 0; k < 3u; ++k) {
            int32_t yy = (int32_t)((frame * 31u + k * 43u) % 139u);
            cv_rect((int32_t)((frame * 17u + k * 53u) % 65u),
                    yy, 90 + (int32_t)((k * 19u) % 67u), 1 + (int32_t)(k & 1u),
                    k & 1u ? BH_MAGENTA : BH_CYAN);
        }
    }
    cv_blit(0, 0);
}

/* Draw the bottom 96 LCD pixels without allocating a full 240x240 FB. */
static void bh_intro_caption(uint32_t elapsed, int glitch)
{
    uint32_t phase = elapsed / BH_GLITCH_FRAME_MS;
    cv_begin(240, 96, BH_BG);
    cv_rect(0, 0, 240, 2, BH_BORDER);
    cv_rect(8, 8, 224, 76, BH_CARD);
    cv_rect(10, 10, 220, 72, BH_BG);
    cv_text((240 - text_w(&FONT_B, "BLACKHOLE REMOTE")) / 2,
            17, &FONT_B, "BLACKHOLE REMOTE", BH_WHITE);
    cv_text((240 - text_w(&FONT_XS, "MIDI CONTROLLER")) / 2,
            39, &FONT_XS, "MIDI CONTROLLER", BH_PURPLE);
    cv_text((240 - text_w(&FONT_XS, "PRESS ANY KEY")) / 2,
            61, &FONT_XS, "PRESS ANY KEY", BH_CYAN);
    cv_rect(18, 88, 204, 2, BH_BORDER);
    if (glitch) {
        cv_rect((int32_t)(phase * 29u % 132u), 24, 53, 2, BH_MAGENTA);
        cv_rect((int32_t)(phase * 43u % 154u), 57, 57, 1, BH_CYAN);
    }
    cv_blit(0, 144);
}

/* App-level splash callback: readable reference, before ui_init(). */
static void bh_remote_splash(void)
{
    bh_background();
    cv_blit(0, 0);
    bh_intro_caption(0u, 0);
}

static void bh_intro_start(void)
{
    bh_intro_state = BH_GLITCH_IN;
    bh_intro_pending = 0u;
    bh_intro_started_ms = plat_ms();
    bh_intro_last_draw_ms = 0xffffffffu;
    bh_intro_previous_keys = plat_keys();
    bh_intro_previous_buttons = plat_buttons();
    bh_top_signature = 0xffffffffu;
    bh_key_signature = 0xffffffffu;
    /* The first visible frame is glitched, not an instant jump to UI. */
}

/* Returns 1 while splash or transition owns the LCD, 0 in controller.
 * Always called AFTER wb_remote_input(), which sends MIDI and updates
 * ui.keys/ui.btn even during the splash; LEDs work in ALL states.
 * Either a new piano-key or a new panel-button press dismisses splash.
 */
static int bh_intro_step(void)
{
    uint32_t now = plat_ms();
    uint32_t keys = ui.keys & ((1u << NKEYS) - 1u);
    uint32_t buttons = ui.btn & ((1u << NB) - 1u);
    uint32_t new_keys = keys & ~bh_intro_previous_keys;
    uint32_t new_buttons = buttons & ~bh_intro_previous_buttons;
    uint32_t elapsed;
    bh_intro_previous_keys = keys;
    bh_intro_previous_buttons = buttons;

    if (bh_intro_state == BH_CONTROLLER)
        return 0;
    if (new_keys || new_buttons)
        bh_intro_pending = 1u;

    if (bh_intro_state == BH_GLITCH_IN) {
        elapsed = now - bh_intro_started_ms;
        if (elapsed >= BH_GLITCH_IN_MS) {
            bh_intro_state = BH_WAIT_KEY;
            bh_remote_splash();
            if (bh_intro_pending) {
                bh_intro_state = BH_GLITCH_OUT;
                bh_intro_started_ms = now;
                bh_intro_last_draw_ms = 0xffffffffu;
            }
            return 1;
        }
        if (bh_intro_last_draw_ms == 0xffffffffu ||
                now - bh_intro_last_draw_ms >= BH_GLITCH_FRAME_MS) {
            bh_intro_last_draw_ms = now;
            bh_glitch_image(elapsed, 0);
            bh_intro_caption(elapsed, 1);
        }
        return 1;
    }
    if (bh_intro_state == BH_WAIT_KEY) {
        if (bh_intro_pending) {
            bh_intro_state = BH_GLITCH_OUT;
            bh_intro_started_ms = now;
            bh_intro_last_draw_ms = 0xffffffffu;
        }
        return 1; /* Remain here indefinitely until a REAL key press. */
    }
    elapsed = now - bh_intro_started_ms;
    if (elapsed >= BH_GLITCH_OUT_MS) {
        bh_intro_state = BH_CONTROLLER;
        bh_top_signature = 0xffffffffu;
        bh_key_signature = 0xffffffffu;
        return 0;
    }
    if (bh_intro_last_draw_ms == 0xffffffffu ||
            now - bh_intro_last_draw_ms >= BH_GLITCH_FRAME_MS) {
        bh_intro_last_draw_ms = now;
        bh_glitch_image(elapsed, 1);
        bh_intro_caption(elapsed, 1);
    }
    return 1;
}

/* The black hole is a LARGE, true 240x144 bitmap BACKGROUND, not a
 * decorative 64x44 widget. No title/logo obscures it during operation. */
static void bh_remote_header(void)
{
    int pct = plat_battery_percent();
    int usb = plat_usb_midi_connected();
    int ble = FM1B_BLE && plat_ble_connected();
    int ble_wait = FM1B_BLE && plat_ble_linked();
    uint32_t sig = ((uint32_t)(pct + 2) & 127u)
                 | ((uint32_t)usb << 8)
                 | ((uint32_t)ble << 9)
                 | ((uint32_t)ble_wait << 10)
                 | ((uint32_t)(pct < 0 && plat_ms() >= 1500u) << 11);
    char number[16];
    if (bh_top_signature == sig)
        return;
    bh_top_signature = sig;
    bh_background();

    /* Every card has exactly 40% image visibility / 60% dark panel.
     * Values and indicator lines remain opaque for legibility. */
    bh_glass_rect(5, 83, 71, 53);
    bh_glass_rect(84, 83, 71, 53);
    bh_glass_rect(163, 83, 72, 53);
    cv_rect(5, 83, 71, 2, BH_PURPLE);
    cv_rect(84, 83, 71, 2, usb ? BH_CYAN : BH_BORDER);
    cv_rect(163, 83, 72, 2, ble ? BH_CYAN : BH_BORDER);

    cv_text(10, 87, &FONT_XS, "BATT EST.", BH_MUTED);
    if (pct < 0) {
        /* Avoid a permanently empty --% panel when the ADC cannot
         * acquire the battery divider. Never invent a battery level. */
        cv_text(11, 104, &FONT_B,
                plat_ms() < 1500u ? "READ.." : "NO ADC",
                plat_ms() < 1500u ? BH_WHITE : BH_ORANGE);
    } else {
        put_s(put_u(number, (uint32_t)pct), "%");
        cv_text(11, 103, &FONT_B, number, pct <= 15 ? BH_ORANGE : BH_GREEN);
        cv_rect(10, 127, 57, 3, BH_BORDER);
        cv_rect(10, 127, (pct * 57) / 100, 3, BH_GREEN);
    }
    cv_text(89, 87, &FONT_XS, "USB MIDI", BH_MUTED);
    cv_text(89, 105, &FONT_B, usb ? "LINK" : "OFF",
            usb ? BH_GREEN : BH_MUTED);
    cv_rect(148, 118, 4, 4, usb ? BH_CYAN : BH_BORDER);

    cv_text(168, 87, &FONT_XS, "BLUETOOTH", BH_MUTED);
    cv_text(168, 105, &FONT_B,
            !FM1B_BLE ? "N/A" : ble ? "LINK" : ble_wait ? "WAIT" : "OFF",
            ble ? BH_GREEN : ble_wait ? BH_ORANGE : BH_MUTED);
    cv_rect(227, 118, 4, 4, ble ? BH_CYAN : BH_BORDER);
    cv_blit(0, 0);
}

/* OPAQUE 240x96 keyboard: the image never bleeds through these pixels.
 * 16 white + 11 black, real physical scan matrix. */
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
    cv_text(7, 85, &FONT_XS, "27-KEY PHYSICAL MIDI / REMOTE", BH_MUTED);
    cv_blit(0, 144);
}

/* Exact LED state from physical switches; no animation or fake notes. */
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
