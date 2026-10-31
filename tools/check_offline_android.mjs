#!/usr/bin/env node
// Static regression checks for an APK that never fetches network content.
import { readFile, stat } from "node:fs/promises";
import assert from "node:assert/strict";
import { resolve, join, dirname } from "node:path";

const root = resolve(process.argv[2] || "android/app/src/main/assets/site");
for (const name of ["index.html", "style.css", "main.js", "worklet.js",
  "jit.js", "fwsc.js", "screen-adapter.js", "mobile-lcd.js", "lcd-fit.js", "fm1_web.wasm"]) {
  const file = await stat(join(root, name));
  assert(file.size > 0, name + " is empty");
}
let total = 0, fm1Count = 0;
for (const rel of ["firmwares.json", "smk37/firmwares.json"]) {
  const cat = JSON.parse(await readFile(join(root, rel), "utf8"));
  assert(cat.firmwares.some(f => f.id === cat.default), rel + ": no default");
  for (const f of cat.firmwares) {
    if (f.byo) {
      assert(!f.package, "Do not bundle proprietary stock firmware: " + f.id);
      continue;
    }
    assert(f.package?.startsWith("offline-fw/"), "Remote boot URL detected: " + f.id);
    assert(!f.hub && !f.installer && !f.embedded && !f.manifest,
      "Remote firmware fetch fallback still enabled: " + f.id);
    const file = await stat(join(dirname(join(root, rel)), f.package));
    assert(file.size > 1024, f.id + " invalid package size");
    total++;
    if (rel === "firmwares.json") fm1Count++;
  }
}
assert(fm1Count >= 1, "At least one actual FM-1 synthesizer package is required");
const manifest = await readFile("android/app/src/main/AndroidManifest.xml", "utf8");
assert(manifest.includes('android.permission.INTERNET'),
  "Firmware update actions require the Android INTERNET permission");
const main = await readFile(join(root, "main.js"), "utf8");
assert(main.includes("nativeWindow.fm1NativeMidi") || main.includes("window.fm1NativeMidi"),
  "Native MIDI integration missing");
const smk = await readFile(join(root, "smk37/mobile-lcd.js"), "utf8");
assert(smk.includes("drawMobileLCD"), "SMK-37 offline imports incomplete");
const page = await readFile(join(root, "index.html"), "utf8");
const smkPage = await readFile(join(root, "smk37/index.html"), "utf8");
for (const html of [page, smkPage]) {
  assert(html.includes('id="update-selected-firmware"'), "Selected firmware update button missing");
  assert(html.includes('id="update-all-firmwares"'), "Bulk firmware update button missing");
}
assert(main.includes("installedFirmwareUpdates"), "Persistent native firmware update loading missing");
assert(main.includes("updateFirmwares"), "Native updater action missing");
console.log(`Offline verification passed: ${fm1Count} FM-1 firmware(s), ${total} total, user-initiated HTTPS firmware updates only`);
