# Firmware screen adaptation

## Current contract

The emulator exposes a 240 × 240 RGB565 framebuffer. The browser receives pixels, not a tree of
labels, controls, text runs or drawing commands. Consequently, pixel analysis can identify occupied
areas, but it cannot reliably tell a label from a waveform, icon, meter or decorative border.

The default renderer therefore preserves the original screen geometry. Auto fit may remove only
the empty outer margin and scales the remaining image proportionally with nearest-neighbour
sampling. It does not stretch a square screen into a landscape rectangle.

## Analysis layer

`web/screen-adapter.js` exports `analyzeScreenLayout(lcd)`. It currently:

1. estimates the background from the four corner pixels;
2. finds the occupied screen bounds;
3. groups nearby occupied scanlines into horizontal bands;
4. reports each band’s bounding box, pixel density and a deliberately tentative visual category.

The output always has `safeToReflow: false` and `semanticConfidence: "insufficient"`. These
regions are diagnostics, not a licence to rearrange pixels. A generic heuristic must never move a
waveform, split a glyph, hide a value or change the meaning of a firmware screen.

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
