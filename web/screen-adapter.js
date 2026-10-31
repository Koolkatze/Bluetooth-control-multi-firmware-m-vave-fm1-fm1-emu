const SIZE = 240;

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

function detectContentBounds(image) {
  const { data } = image;
  const corner = (x, y) => {
    const i = (y * SIZE + x) * 4;
    return [data[i], data[i + 1], data[i + 2]];
  };
  const samples = [corner(0, 0), corner(SIZE - 1, 0), corner(0, SIZE - 1), corner(SIZE - 1, SIZE - 1)];
  const background = [0, 1, 2].map((channel) =>
    [...samples].map((sample) => sample[channel]).sort((a, b) => a - b)[1]);
  let left = SIZE, top = SIZE, right = -1, bottom = -1;
  for (let y = 0; y < SIZE; y++) {
    for (let x = 0; x < SIZE; x++) {
      const i = (y * SIZE + x) * 4;
      if (distance([data[i], data[i + 1], data[i + 2]], background) <= 28) continue;
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

export function renderAdaptiveLCD(ctx, lcd, width = SIZE, height = SIZE, adaptive = true) {
  const image = rgb565ToImageData(lcd);
  const bounds = adaptive ? detectContentBounds(image) : { x: 0, y: 0, width: SIZE, height: SIZE };
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
