// Fit the complete firmware LCD into the Android WebView's usable viewport.
// Geometry is calculated in CSS pixels after the native system-bar insets.
// A square original LCD remains square; horizontal split mode is 16:9.
// Keep bottom space for the small floating Android toolbar toggle.
export function computeLCDSize(viewportWidth, viewportHeight, {
  original = false, gapX = 16, gapTop = 8, gapBottom = 56,
} = {}) {
  const maxWidth = Math.max(1, Math.floor(Number(viewportWidth) - gapX) || 1);
  const maxHeight = Math.max(1, Math.floor(Number(viewportHeight) - gapTop - gapBottom) || 1);
  const ratio = original ? 1 : 16 / 9;
  if (maxWidth / ratio <= maxHeight) {
    return { width: maxWidth, height: Math.max(1, Math.floor(maxWidth / ratio)) };
  }
  return { width: Math.max(1, Math.floor(maxHeight * ratio)), height: maxHeight };
}
