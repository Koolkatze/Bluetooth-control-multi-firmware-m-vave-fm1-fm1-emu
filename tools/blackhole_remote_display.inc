/* SPDX-License-Identifier: MPL-2.0
 * Blackhole Remote: MIDI controller.
 * Original 240 x 240 pixel-art interface for the physical M-VAVE FM-1.
 * Included into CTL-1 app/ui.c after its panel/model definitions.
 * All coordinates and partial canvas blits stay within the square LCD.
 *
 * No graphics are streamed from the phone. The original pixel-art
 * singularity is procedurally drawn on the hardware itself at boot.
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

/* A deliberately low-resolution, 4-pixel-tile accretion disk. Integer
 * arithmetic avoids floating-point work and keeps the source portable
 * to the AC79 device SDK and the host emulator. Tile squares are the
 * pixels — NOT antialiased vector circles. */
static void bh_hole_pixels(int32_t cx, int32_t cy, int32_t radius,
                           int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    int32_t x, y, r2, dx, dy, ring_y;
    uint32_t tile;
    for (y = top; y < bottom; y += 4) {
        for (x = left; x < right; x += 4) {
            uint16_t color = BH_BG;
            dx = x + 2 - cx;
            dy = y + 2 - cy;
            /* Scale elliptical radius so the hole works at splash and
             * live-widget sizes. For r2=100, x is roughly radius. */
            r2 = (dx * dx * 100 + dy * dy * 147) / (radius * radius);
            tile = ((uint32_t)(x * 73 + y * 151) ^ (uint32_t)(x * y * 3)) & 31u;
            if (r2 < 25)
                color = BH_VOID;
            else if (r2 < 35)
                color = BH_MAGENTA;
            else if (r2 < 45)
                color = BH_ORANGE;
            else if (r2 < 62)
                color = tile < 12u ? BH_PURPLE : BH_VIOLET;
            else if (r2 < 93 && tile < 20u)
                color = BH_VIOLET;
            else if (r2 < 165 && tile < 4u)
                color = BH_BORDER;

            /* The luminous, slanted accretion disk crosses behind the
             * dark event horizon, so the centre stays truly black. */
            ring_y = dy + dx / 8;
            if (r2 >= 25 && r2 < 185 && (ring_y < 6 && ring_y > -6)) {
                color = tile < 20u ? BH_ORANGE : BH_MAGENTA;
            }
            if (r2 < 23)
                color = BH_VOID;
            if (color != BH_BG)
                cv_rect(x, y, 4, 4, color);
        }
    }
}

static void bh_stars(int32_t y0, int32_t y1)
{
    int i;
    /* Deterministic sparse stars; no random generator, no assets. */
    for (i = 0; i < 33; i++) {
        int32_t x = (i * 67 + i * i * 11 + 7) % 236;
        int32_t y = y0 + ((i * 43 + i * i * 5) % (y1 - y0));
        if ((x > 49 && x < 194) && (y > 10 && y < 135))
            continue;
        cv_rect(x, y, i % 7 == 0 ? 2 : 1, i % 7 == 0 ? 2 : 1,
                i % 4 == 0 ? BH_CYAN : BH_MUTED);
    }
}

static void bh_remote_splash(void)
{
    /* fm1-base's default canvas holds 240 * 144 pixels. Two bands
     * render exactly 240 * 240 without ever overflowing that buffer. */
    cv_begin(240, 144, BH_BG);
    bh_stars(4, 140);
    bh_hole_pixels(120, 73, 68, 8, 0, 232, 144);
    cv_rect(7, 6, 23, 2, BH_CYAN);
    cv_rect(7, 6, 2, 16, BH_CYAN);
    cv_rect(210, 6, 23, 2, BH_CYAN);
    cv_rect(231, 6, 2, 16, BH_CYAN);
    cv_rect(7, 135, 24, 2, BH_PURPLE);
    cv_rect(208, 135, 25, 2, BH_PURPLE);
    cv_blit(0, 0);

    cv_begin(240, 96, BH_BG);
    cv_rect(12, 6, 216, 2, BH_BORDER);
    cv_text((240 - text_w(&FONT_L, "BLACKHOLE")) / 2, 15,
            &FONT_L, "BLACKHOLE", BH_WHITE);
    cv_text((240 - text_w(&FONT_B, "REMOTE")) / 2, 43,
            &FONT_B, "REMOTE", BH_PURPLE);
    cv_text((240 - text_w(&FONT_XS, "MIDI CONTROLLER")) / 2, 65,
            &FONT_XS, "MIDI CONTROLLER", BH_CYAN);
    cv_rect(16, 89, 208, 2, BH_BORDER);
    cv_rect(16, 89, 49, 2, BH_CYAN);
    cv_blit(0, 144);
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
    bh_hole_pixels(120, 124, 28, 88, 100, 153, 144);
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
