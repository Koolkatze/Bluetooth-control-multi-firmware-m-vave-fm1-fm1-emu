package com.koolkatze.fm1offline;

import android.app.Activity;
import android.webkit.WebView;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

/** Explicit, user-initiated updates of open-source firmware for the emulator.
 * Downloaded packages are private app files, never flashed to a hardware FM-1.
 * No periodic/background network access; JavaScript in the WebView remains
 * restricted to appassets.androidplatform.net and never receives remote URLs.
 */
final class FirmwareUpdater {
    private static final String TAG = "FM1FirmwareUpdate";
    private static final String CATALOG_ROOT =
            "https://raw.githubusercontent.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/master/web/";
    private static final int MAX_CATALOG = 1024 * 1024;
    private static final int MAX_PACKAGE = 15 * 1024 * 1024;
    private final Activity activity;
    private final WebView web;
    private final File updates;
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final AtomicBoolean busy = new AtomicBoolean(false);

    FirmwareUpdater(Activity activity, WebView web, File updates) {
        this.activity = activity;
        this.web = web;
        this.updates = updates;
    }

    private static boolean validDevice(String device) {
        return "fm1".equals(device) || "smk37".equals(device);
    }

    private static boolean validId(String id) {
        return id != null && id.matches("[a-zA-Z0-9_-]{1,90}");
    }

    private static String catalogPath(String device) {
        return device.equals("fm1") ? "firmwares.json" : "smk37/firmwares.json";
    }

    private File folder(String device) { return new File(updates, device); }

    // Metadata is stored separately from the bundled firmware and from the
    // emulated firmware's flash settings, which are keyed by package SHA-256.
    private File metadata(String device) { return new File(folder(device), "installed.json"); }

    private synchronized JSONObject installed(String device) {
        JSONObject empty = new JSONObject();
        try {
            File path = metadata(device);
            if (!path.isFile()) return empty;
            return new JSONObject(new String(readFile(path, MAX_CATALOG), StandardCharsets.UTF_8));
        } catch (Exception error) {
            Log.w(TAG, "Installed manifest could not be read", error);
            return empty;
        }
    }

    String installedJson(String device) {
        return validDevice(device) ? installed(device).toString() : "{}";
    }

    boolean start(String device, String selected) {
        if (!validDevice(device) || (!"".equals(selected) && !validId(selected))) return false;
        if (!busy.compareAndSet(false, true)) return false;
        worker.execute(() -> {
            try { update(device, selected); }
            catch (Exception error) {
                notifyPage("error", "", "", error.getMessage(), 0, 0, 0);
            } finally {
                busy.set(false);
                notifyPage("finished", "", "", "", 0, 0, 0);
            }
        });
        return true;
    }

    void close() { worker.shutdownNow(); }

    private void notifyPage(String kind, String id, String version,
                            String message, int changed, int checked, int total) {
        try {
            JSONObject info = new JSONObject();
            info.put("kind", kind);
            info.put("id", id);
            info.put("version", version);
            info.put("message", message == null ? "" : message);
            info.put("changed", changed);
            info.put("checked", checked);
            info.put("total", total);
            final String js = "window.fm1FirmwareUpdateProgress?.(" + info.toString() + ")";
            activity.runOnUiThread(() -> {
                if (!activity.isFinishing()) web.evaluateJavascript(js, null);
            });
        } catch (Exception error) {
            Log.w(TAG, "Update progress callback failed", error);
        }
    }

    private void update(String device, String selected) throws Exception {
        JSONObject packaged = new JSONObject(new String(
                readStream(activity.getAssets().open("site/" + catalogPath(device)), MAX_CATALOG),
                StandardCharsets.UTF_8));
        JSONObject available = new JSONObject(new String(
                download(CATALOG_ROOT + catalogPath(device), MAX_CATALOG), StandardCharsets.UTF_8));
        JSONArray original = packaged.getJSONArray("firmwares");
        Map<String, JSONObject> remote = new HashMap<>();
        JSONArray newest = available.getJSONArray("firmwares");
        for (int i = 0; i < newest.length(); i++) {
            JSONObject f = newest.getJSONObject(i);
            String id = f.optString("id");
            if (validId(id)) remote.put(id, f);
        }
        Set<String> known = new HashSet<>();
        for (int i = 0; i < original.length(); i++) known.add(original.getJSONObject(i).optString("id"));
        if (!selected.isEmpty() && !known.contains(selected))
            throw new IllegalArgumentException("El firmware seleccionado no pertenece al catálogo instalado");

        JSONObject versions = installed(device);
        int total = selected.isEmpty() ? original.length() : 1;
        int changed = 0, checked = 0;
        notifyPage("start", "", "", "Comprobando versiones en el catálogo oficial…", 0, 0, total);
        for (int i = 0; i < original.length(); i++) {
            if (Thread.currentThread().isInterrupted()) return;
            JSONObject current = original.getJSONObject(i);
            String id = current.optString("id");
            if (!selected.isEmpty() && !selected.equals(id)) continue;
            checked++;
            JSONObject candidate = remote.get(id);
            String label = current.optString("name", id);
            if (candidate == null || !eligible(current) || !eligible(candidate)) {
                notifyPage("skipped", id, "", label + ": sin actualización compatible", changed, checked, total);
                continue;
            }
            String version = candidate.optString("version");
            String previous = versions.optJSONObject(id) != null
                    ? versions.optJSONObject(id).optString("version")
                    : current.optString("version");
            if (version.isEmpty() || version.equals(previous)) {
                notifyPage("unchanged", id, previous, label + ": ya está actualizado", changed, checked, total);
                continue;
            }
            String address = candidate.optString("package");
            try {
                notifyPage("progress", id, version, "Descargando " + label + " (" + version + ")…", changed, checked, total);
                byte[] bytes = download(address, MAX_PACKAGE);
                if (bytes.length < 1024 || !isPackage(bytes))
                    throw new IllegalArgumentException("Archivo .fwsc inválido");
                File dir = folder(device);
                if (!dir.isDirectory() && !dir.mkdirs())
                    throw new IllegalStateException("No se pudo crear la carpeta de actualizaciones");
                File part = new File(dir, id + ".fwsc.tmp");
                File finalFile = new File(dir, id + ".fwsc");
                try (FileOutputStream out = new FileOutputStream(part)) { out.write(bytes); out.getFD().sync(); }
                if (!part.renameTo(finalFile)) throw new IllegalStateException("No se pudo guardar el firmware");
                JSONObject details = new JSONObject();
                details.put("version", version);
                details.put("name", label);
                details.put("url", address);
                versions.put(id, details);
                writeManifest(metadata(device), versions);
                changed++;
                notifyPage("updated", id, version, label + ": descargado " + version, changed, checked, total);
            } catch (Exception error) {
                notifyPage("error-item", id, version, label + ": " + error.getMessage(), changed, checked, total);
            }
        }
        notifyPage("summary", "", "", "Comprobados: " + checked + " · Actualizados: " + changed,
                changed, checked, total);
    }

    private static boolean eligible(JSONObject f) {
        String license = f.optString("license");
        return !f.optBoolean("byo") && !f.optString("repo").isEmpty()
                && license.matches("(?i)^(GPL|MIT|MPL|Apache|BSD|ISC|CC0|Unlicense)(?:$|[^A-Za-z].*)")
                && !license.toLowerCase(java.util.Locale.ROOT).contains("source is not published");
    }

    private static void writeManifest(File file, JSONObject manifest) throws Exception {
        File temp = new File(file.getParentFile(), "installed.json.tmp");
        try (FileOutputStream out = new FileOutputStream(temp)) {
            out.write(manifest.toString().getBytes(StandardCharsets.UTF_8));
            out.getFD().sync();
        }
        if (!temp.renameTo(file)) throw new IllegalStateException("No se pudo registrar la actualización");
    }

    private static boolean isPackage(byte[] bytes) {
        // Check that the UFW header has the required CRC16/CCITT unmasking.
        if (bytes.length < 20 * 0x30) return false;
        byte[] header = new byte[20 * 0x2f];
        for (int i = 0; i < 20; i++)
            System.arraycopy(bytes, i * 0x30, header, i * 0x2f, 0x2f);
        int key = 0xffff;
        for (int i = 0; i < 0x40; i++) {
            header[i] ^= (byte)(key & 0xff);
            key = ((key << 1) ^ ((key & 0x8000) != 0 ? 0x1021 : 0)) & 0xffff;
        }
        int crc = 0;
        for (int i = 2; i < 0x40; i++) {
            crc ^= (header[i] & 0xff) << 8;
            for (int k = 0; k < 8; k++)
                crc = ((crc & 0x8000) != 0 ? (crc << 1) ^ 0x1021 : crc << 1) & 0xffff;
        }
        int expected = (header[0] & 0xff) | ((header[1] & 0xff) << 8);
        return crc == expected;
    }

    private static byte[] readFile(File f, int limit) throws Exception {
        try (InputStream in = new FileInputStream(f)) { return readStream(in, limit); }
    }

    private static byte[] readStream(InputStream in, int limit) throws Exception {
        try (InputStream source = in; ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            byte[] buf = new byte[8192];
            int n;
            while ((n = source.read(buf)) != -1) {
                if (out.size() + n > limit) throw new IllegalArgumentException("Archivo demasiado grande");
                out.write(buf, 0, n);
            }
            return out.toByteArray();
        }
    }

    private static byte[] download(String address, int max) throws Exception {
        if (address == null || address.length() > 2048) throw new IllegalArgumentException("URL de firmware inválida");
        for (int redirect = 0; redirect < 5; redirect++) {
            URL url = new URL(address);
            String host = url.getHost().toLowerCase(java.util.Locale.ROOT);
            boolean allowed = host.equals("thefirmwarehub.com")
                    || host.equals("raw.githubusercontent.com")
                    || host.equals("github.com")
                    || host.endsWith(".github.io")
                    || host.endsWith(".githubusercontent.com");
            if (!"https".equals(url.getProtocol()) || !allowed ||
                    url.getUserInfo() != null || url.getPort() != -1)
                throw new IllegalArgumentException("Origen de descarga no autorizado");
            HttpURLConnection c = (HttpURLConnection)url.openConnection();
            c.setInstanceFollowRedirects(false);
            c.setConnectTimeout(12000);
            c.setReadTimeout(20000);
            c.setRequestProperty("User-Agent", "FM1-Mobile-Android-Updater/0.2");
            try {
                int status = c.getResponseCode();
                if (status >= 300 && status < 400) {
                    String location = c.getHeaderField("Location");
                    if (location == null) throw new IllegalArgumentException("Redirección sin destino");
                    address = new URL(url, location).toExternalForm();
                    continue;
                }
                if (status != 200) throw new IllegalStateException("HTTP " + status);
                if (c.getContentLengthLong() > max) throw new IllegalArgumentException("Descarga demasiado grande");
                return readStream(c.getInputStream(), max);
            } finally { c.disconnect(); }
        }
        throw new IllegalArgumentException("Demasiadas redirecciones");
    }
}
