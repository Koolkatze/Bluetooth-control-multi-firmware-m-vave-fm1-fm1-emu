#!/usr/bin/env node
/**
 * Prepare a complete OFFLINE WebView asset tree from a freshly built web/
 * directory. The APK includes the wasm, worklet, JS, CSS, and compatible
 * redistributed firmware packages. No URL is fetched when the app runs.
 *
 * Usage: node tools/package_offline_android.mjs _offline_web android/app/src/main/assets/site
 *
 * The upstream authors retain their firmware copyrights. We only bundle
 * catalog entries carrying an explicit open-source license and source URL.
 * M-VAVE's proprietary package and items with unconfirmed terms are NEVER
 * bundled. Their .fwsc files can be imported locally, without Internet.
 */
import { readFile, writeFile, cp, rm, mkdir, stat } from "node:fs/promises";
import { resolve, join, dirname, relative } from "node:path";
import { fetchPackage, appFromPackage } from "../web/fwsc.js";

const built = resolve(process.argv[2] || "_offline_web");
const out = resolve(process.argv[3] || "android/app/src/main/assets/site");

async function available(path) {
  try { return (await stat(path)).isFile(); } catch { return false; }
}
if (!(await available(join(built, "fm1_web.wasm")))) {
  throw new Error("Missing compiled fm1_web.wasm — run web/build.sh first");
}
if (built === out || out.startsWith(built + "/")) {
  throw new Error("Output must not be inside the source directory");
}
await rm(out, { recursive: true, force: true });
await mkdir(dirname(out), { recursive: true });
await cp(built, out, { recursive: true });

const permitted = (f) =>
  !f.byo && Boolean(f.repo) &&
  /^(GPL(?:-|\b)|MIT(?:\b|-)|MPL(?:-|\b)|Apache(?:-|\b)|BSD(?:-|\b)|ISC(?:-|\b)|CC0(?:-|\b)|Unlicense(?:\b))/i.test(f.license || "") &&
  !/source is not published/i.test(f.license || "");
const slug = (id) => {
  if (!/^[a-zA-Z0-9_-]{1,90}$/.test(id)) throw new Error("Unsafe firmware id: " + id);
  return id;
};
// The built-in packages are downloaded at build time. Optional, user-triggered
// Android firmware updates are handled separately by the native Java updater.
// An individual mirror may be down: mark that firmware unavailable rather
// than claiming it works offline when it has not actually been packaged.
const download = (uri) => fetch(uri, {
  signal: AbortSignal.timeout(30000),
  headers: { "User-Agent": "FM1-Mobile-Offline-Build/0.1 (GitHub Actions)" }
});
const logs = [];
const catalogs = [
  { path: "firmwares.json", namespace: "FM-1" },
  { path: "smk37/firmwares.json", namespace: "SMK-37" }
];

async function prepareCatalog(spec) {
  const target = join(out, spec.path);
  const catalog = JSON.parse(await readFile(target, "utf8"));
  const folder = dirname(target);
  const firmwareDir = join(folder, "offline-fw");
  await mkdir(firmwareDir, { recursive: true });

  const results = new Array(catalog.firmwares.length);
  const work = catalog.firmwares.map((firmware, i) => async () => {
    const f = { ...firmware };
    if (f.byo) {
      // A proprietary factory firmware may be imported via Android Files; do
      // not ship it inside a public APK or silently fetch it at runtime.
      results[i] = f;
      logs.push({ device: spec.namespace, id: f.id, bundled: false, reason: "bring your own .fwsc" });
      return;
    }
    if (!permitted(f)) {
      logs.push({ device: spec.namespace, id: f.id, bundled: false, reason: "licence/source not verified for redistribution" });
      return;
    }
    try {
      const got = await fetchPackage(f, download);
      if (!got.bytes || got.bytes.length < 1024 || got.bytes.length > 15 * 1024 * 1024) {
        throw new Error("unexpected package size");
      }
      // Validate the real update container before claiming it is available.
      const app = appFromPackage(got.bytes);
      if (app.length < 512) throw new Error("no valid firmware app");
      const id = slug(f.id);
      const name = `offline-fw/${id}.fwsc`;
      await writeFile(join(firmwareDir, `${id}.fwsc`), got.bytes);
      f.package = name;
      f.version = got.version || f.version;
      delete f.hub; delete f.installer; delete f.manifest; delete f.embedded;
      // The UI's links to firmware source remain for attribution but are not
      // used by the boot path. Android blocks all external requests.
      results[i] = f;
      logs.push({ device: spec.namespace, id, bundled: true, bytes: got.bytes.length, originalUrl: got.url, source: f.repo, license: f.license });
      console.log(`[offline] ${spec.namespace} ${id}: ${got.bytes.length} bytes`);
    } catch (error) {
      logs.push({ device: spec.namespace, id: f.id, bundled: false, reason: String(error?.message || error) });
      console.warn(`[offline] ${spec.namespace} ${f.id}: excluded — ${error?.message || error}`);
    }
  });

  // Bounded concurrent downloads: avoid overloading the firmware authors.
  let position = 0;
  await Promise.all(Array.from({ length: 5 }, async () => {
    for (;;) {
      const index = position++;
      if (index >= work.length) return;
      await work[index]();
    }
  }));

  catalog.firmwares = results.filter(Boolean);
  if (!catalog.firmwares.length) throw new Error(spec.namespace + ": no offline firmware was obtained");
  const prior = catalog.default;
  if (!catalog.firmwares.some(f => f.id === prior && f.package?.startsWith("offline-fw/"))) {
    catalog.default = catalog.firmwares.find(f => f.package?.startsWith("offline-fw/"))?.id ?? prior;
  }
  if (!catalog.firmwares.some(f => f.package?.startsWith("offline-fw/"))) {
    throw new Error(spec.namespace + ": no redistributable firmware to boot fully offline");
  }
  catalog.note = `Built-in OFFLINE ${spec.namespace} catalog. Packages use assets/offline-fw/. Offline playback uses these packaged files; optional updater buttons fetch new releases on user request. To add unsupported firmware manually, import a local .fwsc with Open .fwsc. Third-party rights remain with their authors.`;
  await writeFile(target, JSON.stringify(catalog, null, 2) + "\n");
}

for (const cat of catalogs) await prepareCatalog(cat);

// Human-readable notice inside the APK. The original authors' source links
// and licences remain available without fetching any runtime resources.
const bundled = logs.filter(x => x.bundled);
const report = [
  "# Offline firmware licence and provenance manifest",
  "",
  "Firmware files are the original authors' packages. Their code, names and",
  "licences remain theirs. See the links below for corresponding source and",
  "licence terms. Proprietary M-VAVE stock firmware is NOT bundled.",
  "",
  ...bundled.map(x =>
    `- ${x.device} / ${x.id}: ${x.license}; source: ${x.source}; original package: ${x.originalUrl}`),
  "", "## Not packaged",
  ...logs.filter(x => !x.bundled).map(x =>
    `- ${x.device} / ${x.id}: ${x.reason}`),
  ""
].join("\n");
await writeFile(join(out, "OFFLINE-FIRMWARES.md"), report);
await writeFile(join(out, "offline-bundle-report.json"),
  JSON.stringify({ created: new Date().toISOString(), bundled: bundled.length, excluded: logs.length - bundled.length, entries: logs }, null, 2) + "\n");

// Mark the application as fully local. External page links are non-functional
// in the APK by design, but all of its own imported modules and wasm resolve.
for (const htmlFile of ["index.html", "smk37/index.html"]) {
  const page = join(out, htmlFile);
  let html = await readFile(page, "utf8");
  html = html.replace("<head>", `<head>
<script>
  document.documentElement.setAttribute('data-offline-app','');
  window.addEventListener('error', function (event) {
    try { window.FM1Android?.error?.('JS ' + (event.message || event.target?.src || 'resource error')); } catch {}
  }, true);
  window.addEventListener('unhandledrejection', function (event) {
    try { window.FM1Android?.error?.('Promise ' + String(event.reason)); } catch {}
  });
</script>`);
  html = html.replace(/<title>[^<]+<\/title>/, "<title>FM-1 Mobile Offline</title>");
  await writeFile(page, html);
}
const info = logs.reduce((a, x) => { a[x.bundled ? "bundled" : "excluded"]++; return a; },
  { bundled: 0, excluded: 0 });
console.log("[offline] APK resources ready at", relative(process.cwd(), out), info);
if (!bundled.some(x => x.device === "FM-1")) {
  throw new Error("No FM-1 firmware included: an offline synthesizer needs a runnable local firmware");
}
