// Mobile LCD compositor. Firmware stays byte-identical: this operates on the
// 240x240 RGB565 pixels published by the emulated display controller.
//
// A square framebuffer contains no semantic widget information. We only place
// two horizontal strips side by side when a genuinely empty row separates them.
// Otherwise the complete, unmodified LCD is shown, never the instrument body.
const SIDE = 240;

export function findSafeSplit(lcd) {
  if (!lcd || lcd.length !== SIDE * SIDE) return null;
  const corners = [lcd[0], lcd[SIDE - 1], lcd[(SIDE - 1) * SIDE], lcd[SIDE * SIDE - 1]];
  // Avoid guessing from dark foregrounds: pick the most common corner colour.
  const bg = corners.sort((a, b) => corners.filter(x => x === a).length - corners.filter(x => x === b).length).at(-1);
  const dist = (a, b) => Math.max(
    Math.abs(((a >> 11) & 31) - ((b >> 11) & 31)) * 255 / 31,
    Math.abs(((a >> 5) & 63) - ((b >> 5) & 63)) * 255 / 63,
    Math.abs((a & 31) - (b & 31)) * 255 / 31
  );
  const empty = [];
  for (let y = 0; y < SIDE; y++) {
    let n = 0;
    for (let x = 0; x < SIDE; x++) if (dist(lcd[y * SIDE + x], bg) > 24) n++;
    empty.push(n === 0);
  }
  // Require a 4-row blank band; do not cut text, knobs or waveforms.
  let best = null, nearest = Infinity;
  for (let y = 65; y <= 175; y++) {
    if (!empty[y - 2] || !empty[y - 1] || !empty[y] || !empty[y + 1]) continue;
    const delta = Math.abs(y - 120);
    if (delta < nearest) { nearest = delta; best = y; }
  }
  return best;
}

export function drawMobileLCD(ctx, lcd, width, height, { horizontal = true } = {}) {
  if (!lcd || lcd.length !== SIDE * SIDE) return { mode: "missing" };
  const pixels = new Uint8ClampedArray(SIDE * SIDE * 4);
  let visible = false;
  for (let i = 0; i < lcd.length; i++) {
    const p = lcd[i], j = i * 4;
    pixels[j] = ((p >> 11) & 31) * 255 / 31;
    pixels[j + 1] = ((p >> 5) & 63) * 255 / 63;
    pixels[j + 2] = (p & 31) * 255 / 31;
    pixels[j + 3] = 255;
    if (p !== 0) visible = true;
  }
  const source = document.createElement("canvas");
  source.width = source.height = SIDE;
  source.getContext("2d").putImageData(new ImageData(pixels, SIDE, SIDE), 0, 0);
  ctx.fillStyle = "#06080c";
  ctx.fillRect(0, 0, width, height);
  ctx.imageSmoothingEnabled = false;

  function place(sx, sy, sw, sh, dx, dy, dw, dh) {
    const scale = Math.min(dw / sw, dh / sh);
    const w = sw * scale, h = sh * scale;
    ctx.drawImage(source, sx, sy, sw, sh, dx + (dw - w) / 2, dy + (dh - h) / 2, w, h);
  }
  const split = horizontal && width > height * 1.3 ? findSafeSplit(lcd) : null;
  if (split !== null) {
    const pad = Math.max(8, Math.round(width * 0.018));
    const gap = pad;
    const columnWidth = (width - pad * 2 - gap) / 2;
    const columnHeight = height - pad * 2;
    ctx.fillStyle = "#0d1117";
    ctx.fillRect(pad, pad, columnWidth, columnHeight);
    ctx.fillRect(pad + columnWidth + gap, pad, columnWidth, columnHeight);
    place(0, 0, SIDE, split, pad, pad, columnWidth, columnHeight);
    place(0, split, SIDE, SIDE - split, pad + columnWidth + gap, pad, columnWidth, columnHeight);
    return { mode: "horizontal", split, visible };
  }
  place(0, 0, SIDE, SIDE, 0, 0, width, height);
  return { mode: "original", visible };
}
