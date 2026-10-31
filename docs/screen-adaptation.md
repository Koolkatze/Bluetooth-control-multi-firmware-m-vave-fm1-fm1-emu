# Firmware screen adaptation

## Current contract

The emulator exposes a 240 × 240 RGB565 framebuffer. The browser receives pixels, not a tree of
labels, controls, text runs or drawing commands. Consequently, pixel analysis can identify occupied
areas, but it cannot reliably tell a label from a waveform, icon, meter or decorative border.

The default renderer therefore preserves the original screen geometry. Auto fit may remove only
the empty outer margin and scales the remaining image proportionally with nearest-neighbour
sampling. It does not stretch a square screen into a landscape rectangle.

## Obtaining the official firmware without installing it

The FM-1 stock package does not need to be extracted from the physical synth. M-VAVE publishes
its PC firmware package on the official [Downloads page](https://www.m-vave.com/download), under
**PC Firmware → FM-1 → Download**. The current download catalogue lists FM-1 V16 (dated
2026-10-21). Select the download link itself; do not open M-UPGRADE or Firmware Hub and do not
press an install/update action. The expected package is named `FM-1.fwsc`.

The official file is a firmware update package, not a read-back of the exact flash contents of a
particular unit. It is suitable for loading the matching stock firmware in the emulator, but it
does not contain the factory user patch bank from a physical FM-1. Do not attempt to obtain it by
starting an update or sending SysEx to the device: that is a write/update protocol, not a passive
backup procedure.

For reproducibility, record the downloaded version, file size and SHA-256 before using it. A
community-verified V15 package reference is 699,956 bytes with SHA-256
`db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`; do not apply that
checksum to V16. Keep the original file unchanged and use a separate copy for any analysis.

### Uploaded stock package verified

A user-supplied package was inspected locally without sending it to the FM-1 or changing the device.
Its package markers identify it as `FM-1_016` (V16). The UFW header CRC-16/CCITT validates;
the flash-image range is within the package; the JLFS directory's `isd_config.ini` checksum
validates; and the app-area directory contains an `app.bin` entry with the expected flags and
an in-range payload. The app payload can be extracted using the same package format implemented
by `web/fwsc.js`.

| Property | Verified value |
|---|---|
| Package identity | `FM-1_016` |
| File size | 718,196 bytes |
| SHA-256 | `d017fb65e70aca97ee29d944470c3515a74581df5c0576bf70de917c7b2b61d5` |
| Flash image | 692,224 bytes |
| App entry | `app.bin`, offset `0x140` in the app area |
| App payload after trailing 0xFF padding | 597,928 bytes |

This validates the package structure and its compatibility with the documented FM-1 package
format; it is not yet a runtime boot test on Android or in the browser. The firmware binary is
user-supplied and is deliberately not committed to this repository.

To try this exact package in the emulator, open the FM-1 page, select the M-VAVE stock firmware
entry, choose **Open .fwsc…**, and select the downloaded `FM-1.fwsc`. The page recognises it as
the stock package, keeps it in that browser's local storage, and uses it on the next **Start**.
If the emulator is already running, selecting the file triggers a reboot with that package.
Screen-only mode is the default; add `&layout=1` to the page URL to enable the diagnostic
**Export frame** control while collecting representative screens.

## Analysis layer

`web/screen-adapter.js` exports `analyzeScreenLayout(lcd)`. It currently:

1. estimates the background from the four corner pixels;
2. finds the occupied screen bounds;
3. groups nearby occupied scanlines into horizontal bands;
4. reports each band’s bounding box, pixel density and a deliberately tentative visual category.

The output always has `safeToReflow: false` and `semanticConfidence: "insufficient"`. These
regions are diagnostics, not a licence to rearrange pixels. A generic heuristic must never move a
waveform, split a glyph, hide a value or change the meaning of a firmware screen.

## Android MIDI validation notes

The existing browser page uses the Web MIDI API for MIDI input. On Android, test it in Chrome over
HTTPS and grant the site's MIDI permission. Google's Chrome help confirms that Chrome on Android
can request access to connected MIDI devices; Android itself supports USB and BLE MIDI transports.
These platform capabilities do not, by themselves, prove that a particular Chrome build exposes a
paired BLE-MIDI peripheral to `navigator.requestMIDIAccess()`.

Validate transports independently on the target phone:

1. **USB-C / OTG:** connect a class-compliant USB-MIDI interface or controller, open the page's
   Settings view, grant MIDI permission and confirm the device appears in the input selector.
2. **Bluetooth LE MIDI:** pair the controller in Android Bluetooth settings, reopen MIDI access and
   check whether it appears in the same selector. Do not assume ordinary Bluetooth audio pairing
   makes it a MIDI endpoint.
3. Send notes and controller messages and confirm the emulator receives them. Record the exact
   Android build, Chrome version, controller firmware and transport for each successful test.

If USB MIDI works but the paired BLE-MIDI device is absent, the next implementation should be a
small native Android MIDI bridge using Android's MIDI APIs, rather than a browser-side BLE
workaround. Keep this as a separate transport layer so the emulator's existing MIDI message path
remains unchanged.

## Profile format (planned)

A profile is keyed by firmware identity and version range. It describes semantic regions only when
they are known from firmware source, a verified drawing trace or a manually validated reference
screen. Coordinates are normalized to the 240 × 240 source.

Example shape:

```json
{
  "schemaVersion": 1,
  "firmware": {
    "id": "example-firmware",
    "version": "*"
  },
  "source": {
    "width": 240,
    "height": 240
  },
  "regions": [
    {
      "id": "header",
      "role": "text",
      "rect": [0.04, 0.03, 0.92, 0.14],
      "layout": "reflowable",
      "minScale": 1.0
    },
    {
      "id": "waveform",
      "role": "graphic",
      "rect": [0.05, 0.22, 0.90, 0.58],
      "layout": "preserve-aspect"
    }
  ],
  "landscape": {
    "strategy": "profile-guided",
    "minimumConfidence": 0.98,
    "fallback": "original"
  }
}
```

This is a design example, not an active profile. The example ID and coordinates do not describe a
real firmware.

## Safe adaptation pipeline

1. Identify the exact firmware package and version.
2. Prefer instrumentation of the emulator’s LCD drawing API to recover text, rectangles, lines,
   bitmaps and clipping operations. Preserve draw order and z-order.
3. Where drawing metadata is unavailable, combine OCR, connected-component analysis and temporal
   comparison across frames. Treat every inferred label or control as uncertain.
4. Match detected content against a verified firmware profile.
5. Reflow only semantic text and explicitly reflowable controls. Preserve waveforms, meters,
   pixel art and other graphics unless the profile defines a safe transformation.
6. Check clipping, overlap, minimum text size and information loss.
7. If any check fails or confidence is below the profile threshold, render the untouched source
   framebuffer.

## Acceptance criteria

A profile is not ready for normal use until it has been checked against multiple screens from the
same firmware: boot, home, menus, parameter editing, long labels, numeric values and animated
graphics. Test both static and rapidly changing screens. A transformed screen must preserve every
visible value and state indicator, avoid overlap and remain legible at the target phone’s actual
landscape viewport size.

No profile may be inferred solely from the firmware name, and no third-party firmware should be
bundled unless its licence and redistribution terms permit it.


## Emulator instrumentation findings

The current browser-facing API reads the emulated LCD framebuffer through `ui.panel_lcd()`.
The emulator's SoC LCD model is a ST7789-style SPI peripheral: firmware sends controller
commands, address-window arguments and RGB565 pixel bytes. The LCD model writes those pixels
into a 240 × 240 framebuffer. By the time the browser reads `panel_lcd()`, text and shapes have
already been rasterized into pixels.

This means the existing interface does not expose the firmware's original text strings, font
glyphs, widget types or drawing-call boundaries. Intercepting the LCD SPI stream would provide
write windows and pixel payloads, not semantic drawing primitives. Recovering semantic content
would require a separate firmware-aware layer (for example, tracing known graphics-library
routines or analysing firmware symbols/source), and must be evaluated per firmware family.

The browser diagnostics now compare consecutive framebuffer samples. Each detected horizontal
band includes a temporal-change ratio and a coarse dynamic/static hint. This can help locate
animated waveforms, meters or blinking indicators, but it still cannot identify their meaning.
The diagnostics remain opt-in with `?layout=1`; they do not alter the rendered image.

### Diagnostic capture workflow

The page now provides a diagnostic capture control when opened with `?layout=1`. After starting
a firmware, the **Export frame** control downloads:

- a PNG of the current 240 × 240 rendered LCD;
- a JSON report with firmware identity/version, board, capture time, current fit setting,
  occupied bounds, detected bands and temporal-change estimates.

Capture representative states independently: boot/home, menus, parameter editing, sequencers,
waveforms and animated pages. Let animated pages run before capture so the temporal comparison has
multiple samples. The capture is evidence for profile development, not a semantic interpretation;
`safeToReflow` remains false.

The next step is to collect captures from named firmware versions and compare them with firmware
source or drawing-library traces. Only then can a profile define which elements may be rearranged.
