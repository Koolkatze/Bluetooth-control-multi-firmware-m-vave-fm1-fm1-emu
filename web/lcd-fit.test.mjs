import test from "node:test";
import assert from "node:assert/strict";
import { computeLCDSize } from "./lcd-fit.js";

function fits(w, h, original = false) {
  const result = computeLCDSize(w, h, { original });
  assert.ok(result.width <= w - 16, "LCD exceeded horizontal viewport");
  assert.ok(result.height <= h - 64, "LCD exceeded safe vertical viewport");
  assert.ok(result.width > 0 && result.height > 0);
  const ratio = original ? 1 : 16 / 9;
  assert.ok(Math.abs(result.width / result.height - ratio) <= .012,
    "Firmware LCD aspect ratio must be preserved");
  return result;
}

test("landscape phone: full LCD fits above Android's bottom control toggle", () => {
  const horizontal = fits(1536, 590);
  const original = fits(1536, 590, true);
  assert.equal(horizontal.height, 526);
  assert.equal(original.width, original.height);
  assert.ok(original.width < horizontal.width);
});

test("portrait phone: fit by width and avoid clipping", () => {
  const horizontal = fits(390, 740);
  const original = fits(390, 740, true);
  assert.equal(horizontal.width, 374);
  assert.equal(original.width, 374);
});

test("rotation, short screens and split screen preserve bounds", () => {
  for (const [w, h] of [[848, 380], [600, 260], [320, 420], [1920, 900], [320, 180]]) {
    fits(w, h, true);
    fits(w, h, false);
  }
});
