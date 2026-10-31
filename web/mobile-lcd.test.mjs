import test from "node:test";
import assert from "node:assert/strict";
import { findSafeSplit, drawMobileLCD } from "./mobile-lcd.js";

const SIZE = 240;
function screen() { return new Uint16Array(SIZE * SIZE); }
function bar(lcd, y0, y1) {
  for (let y = y0; y < y1; y++) for (let x = 0; x < SIZE; x++) lcd[y * SIZE + x] = 0xffff;
}

test("splits at a truly empty horizontal band without cutting content", () => {
  const lcd = screen(); bar(lcd, 10, 110); bar(lcd, 130, 220);
  const split = findSafeSplit(lcd);
  assert.ok(split >= 111 && split <= 128, String(split));
});

test("does not split when all center rows contain content", () => {
  const lcd = screen(); bar(lcd, 10, 220);
  assert.equal(findSafeSplit(lcd), null);
});

test("renderer never includes a synthesizer panel and preserves source pixels", () => {
  globalThis.ImageData = class ImageData {
    constructor(data, width, height) {
      this.data = data; this.width = width; this.height = height;
    }
  };
  const lcd = screen(); bar(lcd, 10, 110); bar(lcd, 130, 220);
  const calls = [];
  const sourceContext = { putImageData: img => calls.push(["lcd", img.width, img.height]) };
  const source = { width: 0, height: 0, getContext: () => sourceContext };
  globalThis.document = { createElement: () => source };
  const ctx = {
    fillRect: (...args) => calls.push(["fill", ...args]),
    drawImage: (...args) => calls.push(["draw", args[1], args[2], args[3], args[4]]),
  };
  const result = drawMobileLCD(ctx, lcd, 960, 540);
  assert.equal(result.mode, "horizontal");
  assert.equal(calls.filter(c => c[0] === "draw").length, 2);
  assert.deepEqual(calls.find(c => c[0] === "lcd"), ["lcd", 240, 240]);
});
