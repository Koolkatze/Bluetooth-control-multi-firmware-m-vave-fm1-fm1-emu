// FM-1 finite rotary control geometry, shared by mobile and web emulators.
// Exactly 300 degrees: 7 o'clock = 0, 12 o'clock = 50, 5 o'clock = 100.
// Physical rotary encoders are *relative*: finger rotation emits delta
// detents to the running firmware, which keeps its own numeric min/max.
// The forbidden 60-degree bottom sector has no wrap from 100 to 0.
export const KNOB_SWEEP_LIMIT = Math.PI * 5 / 6;
export const FM1_KNOB_CENTERS = [
  [250, 135], [110, 295], [250, 295],
  [935, 120], [1027, 120], [1119, 120], [1211, 120],
  [110, 135], // MASTER, analogue rather than encoder
];
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
export function knobAngle(cx, cy, x, y) {
  return Math.atan2(x - cx, cy - y); // 12 o'clock zero, clockwise positive
}
export function knobPercent(angle) {
  return Math.round(100 * (clamp(angle, -KNOB_SWEEP_LIMIT, KNOB_SWEEP_LIMIT)
    + KNOB_SWEEP_LIMIT) / (2 * KNOB_SWEEP_LIMIT));
}
export function beginKnobGesture(i, x, y) {
  const [cx, cy] = FM1_KNOB_CENTERS[i];
  const raw = knobAngle(cx, cy, x, y);
  return { cx, cy, angle: raw, raw, percent: knobPercent(raw) };
}
export function moveKnobGesture(g, x, y) {
  // Ignore unstable points at the centre of the dial.
  if ((x - g.cx) ** 2 + (y - g.cy) ** 2 < 90) return 0;
  const raw = knobAngle(g.cx, g.cy, x, y);
  // Unwrap the +/-pi boundary by choosing the SHORTEST change in
  // angular position. Dragging across the 5->7 bottom dead zone stays
  // clamped to 100; it never jumps to zero.
  const delta = Math.atan2(Math.sin(raw - g.raw), Math.cos(raw - g.raw));
  g.angle += delta;
  g.raw = raw;
  const next = knobPercent(g.angle);
  const diff = next - g.percent;
  g.percent = next;
  return diff;
}
