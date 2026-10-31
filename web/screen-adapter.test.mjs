import test from "node:test";
import assert from "node:assert/strict";

globalThis.ImageData = class ImageData {
  constructor(data, width, height) {
    this.data = data;
    this.width = width;
    this.height = height;
  }
};

const { ScreenAnalyzer } = await import("./screen-adapter.js");

function frameWithRect(x, y, width, height, color = 0xffff) {
  const frame = new Uint16Array(240 * 240);
  frame.fill(0x0000);
  for (let row = y; row < y + height; row++) {
    for (let col = x; col < x + width; col++) {
      frame[row * 240 + col] = color;
    }
  }
  return frame;
}

test("screen analyzer finds occupied bounds and keeps reflow disabled", () => {
  const analyzer = new ScreenAnalyzer();
  const result = analyzer.analyze(frameWithRect(30, 40, 80, 12));

  assert.deepEqual(result.source, { width: 240, height: 240 });
  assert.equal(result.bounds.x, 26);
  assert.equal(result.bounds.y, 36);
  assert.equal(result.bounds.width, 88);
  assert.equal(result.bounds.height, 20);
  assert.equal(result.regions.length, 1);
  assert.equal(result.semanticConfidence, "insufficient");
  assert.equal(result.safeToReflow, false);
});

test("screen analyzer reports temporal changes between frames", () => {
  const analyzer = new ScreenAnalyzer();
  analyzer.analyze(frameWithRect(20, 30, 60, 10));
  const result = analyzer.analyze(frameWithRect(20, 30, 60, 10, 0xf800));

  assert.equal(result.frame.sample, 2);
  assert.ok(result.frame.changedPixels > 0);
  assert.equal(result.regions.length, 1);
  assert.equal(result.regions[0].behavior, "dynamic");
  assert.equal(result.safeToReflow, false);
});

test("screen analyzer reset clears temporal history", () => {
  const analyzer = new ScreenAnalyzer();
  analyzer.analyze(frameWithRect(20, 30, 60, 10));
  analyzer.reset();
  const result = analyzer.analyze(frameWithRect(20, 30, 60, 10));

  assert.equal(result.frame.sample, 1);
  assert.equal(result.frame.changedPixels, 0);
});
