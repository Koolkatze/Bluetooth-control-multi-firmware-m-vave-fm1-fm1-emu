const SIZE = 240;
const PIXEL_THRESHOLD = 28;
const ROW_GAP = 2;

function distance(a, b) {
  return Math.max(Math.abs(a[0] - b[0]), Math.abs(a[1] - b[1]), Math.abs(a[2] - b[2]));
}

function rgb565ToImageData(lcd) {
  const pixels = new Uint8ClampedArray(SIZE * SIZE * 4);
  for (let i = 0; i < lcd.length; i++) {
    const p = lcd[i];
    const o = i * 4;
    pixels[o] = Math.round(((p >> 11) & 31) * 255 / 31);
    pixels[o + 1] = Math.round(((p >> 5) & 63) * 255 / 63);
    pixels[o + 2] = Math.round((p & 31) * 255 / 31);
    pixels[o + 3] = 255;
  }
  return new ImageData(pixels, SIZE, SIZE);
}

function sampleBackground(data) {
  const points = [[0, 0], [SIZE - 1, 0], [0, SIZE - 1], [SIZE - 1, SIZE - 1]];
  const samples = points.map(([x, y]) => {
    const i = (y * SIZE + x) * 4;
    return [data[i], data[i + 1], data[i + 2]];
  });
  return [0, 1, 2].map((channel) =>
    [...samples].map((sample) => sample[channel]).sort((a, b) => a - b)[1]);
}

function isForeground(data, index, background) {
  return distance([data[index], data[index + 1], data[index + 2]], background) > PIXEL_THRESHOLD;
}

function getBounds(data, background) {
  let left = SIZE, top = SIZE, right = -1, bottom = -1;
  for (let y = 0; y < SIZE; y++) {
    for (let x = 0; x < SIZE; x++) {
      if (!isForeground(data, (y * SIZE + x) * 4, background)) continue;
      left = Math.min(left, x);
      top = Math.min(top, y);
      right = Math.max(right, x);
      bottom = Math.max(bottom, y);
    }
  }
  if (right < left || bottom < top) return { x: 0, y: 0, width: SIZE, height: SIZE };
  const pad = 4;
  left = Math.max(0, left - pad);
  top = Math.max(0, top - pad);
  right = Math.min(SIZE - 1, right + pad);
  bottom = Math.min(SIZE - 1, bottom + pad);
  return { x: left, y: top, width: right - left + 1, height: bottom - top + 1 };
}

/*
 * Pixel-only segmentation is deliberately advisory. It can find occupied horizontal bands,
 * but cannot know whether pixels represent text, a graph, an icon or a control. The renderer
 * must not rearrange these regions unless a firmware profile supplies semantic constraints.
 */
function detectRegions(data, background) {
  const rows = [];
  for (let y = 0; y < SIZE; y++) {
    let count = 0;
    let left = SIZE;
    let right = -1;
    for (let x = 0; x < SIZE; x++) {
      if (!isForeground(data, (y * SIZE + x) * 4, background)) continue;
      count++;
      left = Math.min(left, x);
      right = Math.max(right, x);
    }
    rows.push({ y, count, left, right });
  }

  const bands = [];
  let active = null;
  let emptyRows = 0;
  for (const row of rows) {
    if (row.count > 0) {
      if (!active || emptyRows > ROW_GAP) {
        if (active) bands.push(active);
        active = { top: row.y, bottom: row.y, left: row.left, right: row.right, pixels: 0 };
      }
      active.bottom = row.y;
      active.left = Math.min(active.left, row.left);
      active.right = Math.max(active.right, row.right);
      active.pixels += row.count;
      emptyRows = 0;
    } else if (active) {
      emptyRows++;
    }
  }
  if (active) bands.push(active);

  return bands
    .filter((band) => band.pixels >= 3)
    .map((band, index) => {
      const width = band.right - band.left + 1;
      const height = band.bottom - band.top + 1;
      const density = band.pixels / (width * height);
      const shape = width / Math.max(1, height);
      let kind = "unknown";
      if (height <= 18 && shape >= 1.5) kind = "text-or-horizontal-control";
      else if (shape <= 1.25 && height >= 12) kind = "icon-or-vertical-control";
      else if (width >= 48 && height >= 20) kind = "graphic-or-panel";
      return {
        id: index,
        x: band.left,
        y: band.top,
        width,
        height,
        pixelCount: band.pixels,
        density: Number(density.toFixed(3)),
        kind,
        confidence: "low",
      };
    });
}

export function analyzeScreenLayout(lcd) {
  const image = rgb565ToImageData(lcd);
  const background = sampleBackground(image.data);
  const bounds = getBounds(image.data, background);
  const regions = detectRegions(image.data, background);
  return {
    source: { width: SIZE, height: SIZE },
    bounds,
    regions,
    semanticConfidence: "insufficient",
    safeToReflow: false,
  };
}

export function renderAdaptiveLCD(ctx, lcd, width = SIZE, height = SIZE, adaptive = true) {
  const image = rgb565ToImageData(lcd);
  const bounds = adaptive ? getBounds(image.data, sampleBackground(image.data)) :
    { x: 0, y: 0, width: SIZE, height: SIZE };
  const source = document.createElement("canvas");
  source.width = SIZE;
  source.height = SIZE;
  source.getContext("2d").putImageData(image, 0, 0);
  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, width, height);
  const scale = Math.min(width / bounds.width, height / bounds.height);
  const drawWidth = bounds.width * scale;
  const drawHeight = bounds.height * scale;
  const x = (width - drawWidth) / 2;
  const y = (height - drawHeight) / 2;
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(source, bounds.x, bounds.y, bounds.width, bounds.height, x, y, drawWidth, drawHeight);
  return { bounds, cropped: bounds.width < SIZE || bounds.height < SIZE };
}

export class ScreenAnalyzer {
  constructor() {
    this.previous = null;
    this.samples = 0;
  }

  analyze(lcd) {
    const image = rgb565ToImageData(lcd);
    const background = sampleBackground(image.data);
    const bounds = getBounds(image.data, background);
    const regions = detectRegions(image.data, background);
    const changed = new Uint8Array(SIZE * SIZE);
    let changedPixels = 0;

    if (this.previous) {
      for (let i = 0; i < SIZE * SIZE; i++) {
        const p = i * 4;
        const different = distance(
          [image.data[p], image.data[p + 1], image.data[p + 2]],
          [this.previous[p], this.previous[p + 1], this.previous[p + 2]]
        ) > 12;
        if (different) {
          changed[i] = 1;
          changedPixels++;
        }
      }
    }

    const dynamicRegions = regions.map((region) => {
      let changedInRegion = 0;
      for (let y = region.y; y < region.y + region.height; y++) {
        for (let x = region.x; x < region.x + region.width; x++) {
          changedInRegion += changed[y * SIZE + x];
        }
      }
      const area = Math.max(1, region.width * region.height);
      const temporalRatio = changedInRegion / area;
      return {
        ...region,
        temporalChange: Number(temporalRatio.toFixed(3)),
        behavior: temporalRatio > 0.12 ? "dynamic" : "mostly-static",
      };
    });

    this.previous = image.data.slice();
    this.samples++;

    return {
      source: { width: SIZE, height: SIZE },
      bounds,
      regions: dynamicRegions,
      frame: {
        sample: this.samples,
        changedPixels,
        changedRatio: Number((changedPixels / (SIZE * SIZE)).toFixed(4)),
      },
      semanticConfidence: "insufficient",
      safeToReflow: false,
    };
  }

  reset() {
    this.previous = null;
    this.samples = 0;
  }
}
