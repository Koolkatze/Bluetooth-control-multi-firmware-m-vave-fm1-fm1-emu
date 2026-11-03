package com.koolkatze.fm1offline;

import android.app.AlertDialog;
import android.content.Context;
import android.hardware.usb.UsbManager;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.Log;
import android.webkit.WebView;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/**
 * EXPLICIT physical FM-1 firmware installer via USB MIDI, never BLE:
 *    selected .fwsc -> checksum / auto-detect FM-1 over USB MIDI ->
 *    probe current model -> user risk confirmation -> M-UPGRADE check ->
 *    loader re-enumeration -> block writing -> reboot identity verification.
 *
 * Ported from charlesvestal/fm1-base tools/fm1_install.py and web/fm1ota.js
 * (MPL-2.0, Charles Vestal, Leo Kuroshita and contributors).
 *
 * The app's in-emulator firmware updater never calls this code. Nothing is
 * written to hardware merely by selecting or starting a virtual firmware.
 */
final class PhysicalFm1Installer {
    private static final String TAG = "PhysicalFm1Installer";
    private static final int CHUNK_LIMIT = 48 * 1024;
    private static final long DISCOVERY_TIMEOUT_MS = 90_000L;
    private static final long REPROBE_INTERVAL_MS = 4_000L;
    private static final byte[] SUCCESS = new byte[]{'s','u','c','c','e','s','s',0};

    private final MainActivity activity;
    private final MidiManager midi;
    private final WebView web;
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final Object uploadLock = new Object();

    private File stage;
    private FileOutputStream stageOutput;
    private MessageDigest stageDigest;
    private long stagedBytes;
    private int stagedParts;
    private int expectedBytes;
    private String expectedDigest, selectedName;
    private volatile boolean occupied;
    private volatile boolean writing;
    private volatile boolean flashing; // true only AFTER explicit risk consent
    private volatile boolean closed;
    private volatile int discoveryGeneration;
    private AlertDialog discoveryDialog;

    PhysicalFm1Installer(MainActivity activity, MidiManager midi, WebView web) {
        this.activity = activity;
        this.midi = midi;
        this.web = web;
    }

    boolean begin(String device, String name, int total, String hash) {
        synchronized (uploadLock) {
            if (closed || occupied || !"fm1".equals(device) || midi == null ||
                    total < 1024 || total > Fm1OtaProtocol.MAX_PACKAGE ||
                    hash == null || !hash.matches("[a-fA-F0-9]{64}"))
                return false;
            occupied = true;
            try {
                selectedName = name == null ? "Sin nombre" : name.replaceAll("[\\r\\n]", " ").substring(
                        0, Math.min(100, name.length()));
                stage = File.createTempFile("fm1-physical-", ".fwsc", activity.getCacheDir());
                stageOutput = new FileOutputStream(stage);
                stageDigest = MessageDigest.getInstance("SHA-256");
                stagedBytes = 0;
                stagedParts = 0;
                expectedBytes = total;
                expectedDigest = hash.toLowerCase(java.util.Locale.ROOT);
                return true;
            } catch (Exception error) {
                failUpload("No se pudo preparar el archivo: " + error.getMessage());
                return false;
            }
        }
    }

    boolean append(int sequence, String base64) {
        synchronized (uploadLock) {
            if (!occupied || closed || writing || stageOutput == null ||
                    sequence != stagedParts || base64 == null ||
                    base64.length() > (CHUNK_LIMIT * 4 / 3 + 8))
                return false;
            try {
                byte[] bytes = Base64.decode(base64, Base64.NO_WRAP);
                if (bytes.length == 0 || bytes.length > CHUNK_LIMIT ||
                        stagedBytes + bytes.length > expectedBytes)
                    throw new IOException("Tamaño de bloque inválido");
                stageOutput.write(bytes);
                stageDigest.update(bytes);
                stagedBytes += bytes.length;
                stagedParts++;
                return true;
            } catch (Exception error) {
                failUpload("Error al transferir firmware: " + error.getMessage());
                return false;
            }
        }
    }

    boolean abortUpload() {
        synchronized (uploadLock) {
            if (!occupied || writing) return false;
            cleanupUpload();
            return true;
        }
    }

    boolean finish() {
        final Fm1OtaProtocol.Package pkg;
        synchronized (uploadLock) {
            if (closed || !occupied || stageOutput == null || writing) return false;
            try {
                stageOutput.close();
                stageOutput = null;
                if (stagedBytes != expectedBytes)
                    throw new IOException("Archivo incompleto: " + stagedBytes + "/" + expectedBytes);
                if (!Fm1OtaProtocol.hex(stageDigest.digest()).equals(expectedDigest))
                    throw new IOException("El checksum SHA-256 del archivo no coincide");
                byte[] raw = Files.readAllBytes(stage.toPath());
                pkg = Fm1OtaProtocol.readPackage(raw); // FM-1 identity + loader validation
            } catch (Exception error) {
                failUpload("Archivo rechazado: " + error.getMessage());
                return false;
            }
            writing = true; // no second transfer while picker, confirmation, or write is active
        }
        ui.post(() -> detectPhysicalFm1(pkg));
        return true;
    }

    private void failUpload(String message) {
        Log.w(TAG, message);
        notifyPage("error", message, -1);
        cleanupUpload();
    }

    private void cleanupUpload() {
        synchronized (uploadLock) {
            if (stageOutput != null) {
                try { stageOutput.close(); } catch (IOException ignored) {}
                stageOutput = null;
            }
            if (stage != null) {
                if (!stage.delete()) Log.w(TAG, "Unable to remove temporary FWSC file");
                stage = null;
            }
            stagedBytes = 0;
            stageDigest = null;
            writing = false;
            occupied = false;
        }
    }

    private void notifyPage(String kind, String message, int percent) {
        try {
            JSONObject event = new JSONObject();
            event.put("kind", kind);
            event.put("message", message);
            event.put("percent", percent);
            String js = "window.fm1PhysicalFirmwareProgress?.(" + event.toString() + ")";
            ui.post(() -> {
                if (!closed && !activity.isFinishing()) {
                    web.evaluateJavascript(js, null);
                    activity.updateStatusForPhysicalFirmware(message);
                }
            });
        } catch (Exception error) {
            Log.w(TAG, "Cannot report physical firmware progress", error);
        }
    }

    /**
     * Detect a USB-MIDI FM-1 already connected or connected after the install
     * button is pressed. Never use the name alone as proof that the device is
     * an FM-1: the device must answer the non-destructive SysEx identity
     * handshake. A user must still approve the risk dialog after discovery.
     */
    private void detectPhysicalFm1(Fm1OtaProtocol.Package pkg) {
        if (closed) { cleanupUpload(); return; }
        if (midi == null) {
            notifyPage("error", "Este Android no ofrece el servicio MIDI USB.", -1);
            cleanupUpload();
            return;
        }
        // Release Workbench's existing MIDI performance stream to the FM-1,
        // otherwise the USB host might not grant exclusive input/output ports.
        activity.releaseMidiForPhysicalInstall();
        final int generation = ++discoveryGeneration;
        AlertDialog dialog = new AlertDialog.Builder(activity)
                .setTitle("Detectando FM-1 por USB automáticamente")
                .setMessage("Si el M-VAVE FM-1 ya está conectado, Workbench "
                        + "buscará su puerto MIDI y comprobará su identidad. "
                        + "Si aún no lo está, conecta ahora un cable USB de datos.\\n\\n"
                        + "No hace falta seleccionar manualmente un dispositivo. "
                        + "No se instalará nada sin tu confirmación posterior. "
                        + "Para salir de la búsqueda pulsa Cancelar.")
                .setNegativeButton("Cancelar", (d, which) -> cancelDiscovery(generation))
                .create();
        dialog.setOnCancelListener(d -> cancelDiscovery(generation));
        discoveryDialog = dialog;
        dialog.show();
        notifyPage("searching", "Buscando FM-1 USB-MIDI, conectado o recién enchufado…", 0);
        worker.execute(() -> searchUntilConnected(pkg, generation));
    }

    private void dismissDiscoveryDialog() {
        if (discoveryDialog != null) {
            AlertDialog dialog = discoveryDialog;
            discoveryDialog = null;
            if (dialog.isShowing()) dialog.dismiss();
        }
    }

    private void cancelDiscovery(int generation) {
        if (generation != discoveryGeneration) return;
        ++discoveryGeneration;
        dismissDiscoveryDialog();
        notifyPage("cancelled", "Búsqueda USB cancelada: no se ha escrito ningún firmware.", -1);
        cleanupUpload();
    }

    private void searchUntilConnected(Fm1OtaProtocol.Package pkg, int generation) {
        final long startedAt = System.currentTimeMillis();
        final Map<Integer, Long> probeTimes = new HashMap<>();
        boolean foundAnotherUsbDevice = false;
        boolean reportedNotMidi = false;
        try {
            while (!closed && discoveryGeneration == generation
                    && System.currentTimeMillis() - startedAt < DISCOVERY_TIMEOUT_MS) {
                List<MidiDeviceInfo> devices = usbDevices();
                if (devices.isEmpty()) {
                    // Android may see a USB data device without exposing any
                    // MIDI ports. Explain this rather than showing a useless
                    // manual picker or claiming the controller was detected.
                    UsbManager usb = (UsbManager)activity.getSystemService(Context.USB_SERVICE);
                    boolean usbAttached = usb != null && !usb.getDeviceList().isEmpty();
                    if (usbAttached && !reportedNotMidi) {
                        notifyPage("searching", "Se detecta un USB, pero aún no aparece "
                                + "un dispositivo MIDI bidireccional. Revisa cable OTG "
                                + "y permisos USB del FM-1; conexión USB-MIDI del teléfono "
                                + "no equivale a USB-MIDI del controlador.", 0);
                        reportedNotMidi = true;
                    }
                } else {
                    MidiDeviceInfo selected = null;
                    Fm1OtaProtocol.Identity selectedIdentity = null;
                    int matched = 0;
                    long now = System.currentTimeMillis();
                    for (MidiDeviceInfo info : devices) {
                        if (closed || generation != discoveryGeneration) return;
                        long last = probeTimes.containsKey(info.getId())
                                ? probeTimes.get(info.getId()) : 0;
                        if (now - last < REPROBE_INTERVAL_MS) continue;
                        probeTimes.put(info.getId(), now);
                        // The handshake is read-only. Device names are a
                        // discovery hint, not a trust or compatibility check.
                        try (Link link = Link.open(midi, info, ui)) {
                            Fm1OtaProtocol.Identity id = handshake(link, 2);
                            if (id == null) continue;
                            if (!"FM-1".equals(id.model)) continue;
                            if (selected == null) {
                                selected = info;
                                selectedIdentity = id;
                            }
                            matched++;
                        } catch (Exception error) {
                            Log.d(TAG, "USB-MIDI identity probing failed", error);
                        }
                    }
                    if (matched > 1) {
                        notifyPage("searching", "Se detectaron varios FM-1. "
                                + "Desconecta todos salvo el que quieras actualizar.", 0);
                    } else if (matched == 1) {
                        final MidiDeviceInfo verifiedDevice = selected;
                        final Fm1OtaProtocol.Identity verifiedIdentity = selectedIdentity;
                        ui.post(() -> {
                            if (closed || discoveryGeneration != generation) return;
                            dismissDiscoveryDialog();
                            try {
                                validateTarget(verifiedIdentity, pkg);
                                notifyPage("detected", "FM-1 detectado por USB: "
                                        + verifiedIdentity.text + ". Esperando confirmación.", 0);
                                confirmInstall(verifiedDevice, verifiedIdentity, pkg);
                            } catch (IOException error) {
                                notifyPage("error", "Firmware no compatible: "
                                        + error.getMessage(), -1);
                                cleanupUpload();
                            }
                        });
                        return;
                    } else if (!foundAnotherUsbDevice) {
                        foundAnotherUsbDevice = true;
                        notifyPage("searching", "Buscando respuesta del FM-1 por USB-MIDI. "
                                + "No se instalará nada hasta reconocer su identidad.", 0);
                    }
                }
                Thread.sleep(700L);
            }
            ui.post(() -> {
                if (closed || discoveryGeneration != generation) return;
                dismissDiscoveryDialog();
                notifyPage("error", "No se detectó un M-VAVE FM-1 con MIDI USB "
                        + "bidireccional. Comprueba cable USB de datos/OTG, "
                        + "permisos y que el firmware físico exponga USB MIDI. "
                        + "No se ha escrito nada.", -1);
                cleanupUpload();
            });
        } catch (InterruptedException error) {
            Thread.currentThread().interrupt();
            ui.post(() -> {
                if (closed || discoveryGeneration != generation) return;
                dismissDiscoveryDialog();
                notifyPage("cancelled", "Búsqueda USB interrumpida; no se escribió firmware.", -1);
                cleanupUpload();
            });
        }
    }

    private static void validateTarget(Fm1OtaProtocol.Identity target,
                                       Fm1OtaProtocol.Package pkg) throws IOException {
        if (!"FM-1".equals(target.model))
            throw new IOException("Modelo incompatible: " + target.text);
        if (target.open && !pkg.stock)
            throw new IOException("El FM-1 tiene firmware Open de M-VAVE. "
                    + "Primero restaura un paquete stock válido; no admite firmware personalizado directamente.");
    }

    private void confirmInstall(MidiDeviceInfo selected, Fm1OtaProtocol.Identity target,
                                Fm1OtaProtocol.Package pkg) {
        if (closed) { cleanupUpload(); return; }
        String text = "¿Seguro que quieres instalar firmware personalizado?\n\n"
                + "Firmware seleccionado: " + selectedName + "\n"
                + "Identidad del paquete: " + pkg.product + "\n"
                + "FM-1 detectado: " + target.text + "\n"
                + "Transporte: USB MIDI (nunca Bluetooth)\n\n"
                + "ATENCIÓN: esta operación modifica la memoria del FM-1 físico. "
                + "Un corte de conexión o un firmware defectuoso puede dejarlo inutilizable. "
                + "El procedimiento será bajo tu propia responsabilidad. "
                + "Necesitas tener preparado un método de recuperación.\n\n"
                + "IMPORTANTE: al sustituir Workbench Remote por otro firmware, "
                + "el FM-1 perderá el control completo de la pantalla y los "
                + "knobs del móvil hasta reinstalar Workbench Remote.\n\n"
                + "No desconectes el USB ni apagues el FM-1 hasta la verificación final.";
        new AlertDialog.Builder(activity)
                .setTitle("CONFIRMAR INSTALACIÓN · BAJO TU RESPONSABILIDAD")
                .setMessage(text)
                .setPositiveButton("Sí, instalar bajo mi responsabilidad", (dialog, which) -> {
                    flashing = true;
                    notifyPage("starting", "Iniciando instalación física USB de " + pkg.product, 0);
                    worker.execute(() -> install(selected, pkg));
                })
                .setNegativeButton("Cancelar (no instalar)", (dialog, which) -> cancel())
                .setOnCancelListener(dialog -> cancel())
                .show();
    }

    private void cancel() {
        ++discoveryGeneration;
        dismissDiscoveryDialog();
        notifyPage("cancelled", "Instalación física cancelada. No se escribió firmware.", -1);
        cleanupUpload();
    }

    private void install(MidiDeviceInfo selected, Fm1OtaProtocol.Package pkg) {
        try {
            // The FM-1's loader can re-enumerate under a generic USB MIDI
            // descriptor. Save USB IDs that existed before the OTA, and also
            // accept devices which appear ONLY after its reboot. Never probe
            // arbitrary pre-existing MIDI gear just because it is connected.
            Set<Integer> usbBaseline = new HashSet<>();
            for (MidiDeviceInfo dev : usbDevices()) usbBaseline.add(dev.getId());
            Fm1OtaProtocol.Identity target;
            Link link = Link.open(midi, selected, ui);
            try {
                target = handshake(link, 2);
                if (target == null) throw new IOException("El FM-1 dejó de responder antes de comenzar.");
                validateTarget(target, pkg);
                if (target.open) armStockUpdate(link, pkg);
                if (!target.loader) {
                    notifyPage("checking", "FM-1 comprobando el firmware seleccionado…", 1);
                    link.send(Fm1OtaProtocol.UPGRADE);
                    sleep(2000);
                    serve(link, pkg.image, Fm1OtaProtocol.FINISH_CHECK, 8000, false);
                }
            } finally { link.close(); }
            if (!target.loader) {
                notifyPage("loader", "Esperando al cargador de actualización del FM-1…", 8);
                sleep(3000);
            } else {
                notifyPage("loader", "FM-1 ya está en modo actualización: reanudando…", 8);
            }
            try (Link ota = target.loader
                    ? waitFor(id -> id.loader, 15000, selected, usbBaseline)
                    : waitFor(id -> id.loader, 30000, selected, usbBaseline)) {
                if (ota == null) throw new IOException("No apareció el cargador USB de actualización. "
                        + "Reconecta el FM-1 en modo actualización y reintenta con el mismo archivo.");
                notifyPage("writing", "Escribiendo firmware en FM-1. NO desconectes el cable.", 10);
                ota.send(Fm1OtaProtocol.UPGRADE);
                sleep(2000);
                serve(ota, pkg.image, Fm1OtaProtocol.FINISH_WRITE, 180000, true);
            }
            notifyPage("reboot", "Esperando el reinicio y verificando la identidad…", 99);
            sleep(3000);
            try (Link restarted = waitFor(id -> !id.loader, 40000, selected, usbBaseline)) {
                if (restarted == null)
                    throw new IOException("El FM-1 no volvió a conectarse. "
                            + "No se puede confirmar la instalación; revisa el modo de recuperación.");
                Fm1OtaProtocol.Identity actual = handshake(restarted, 2);
                if (actual == null || !pkg.product.equals(actual.text))
                    throw new IOException("El FM-1 regresó pero la identidad no coincide: "
                            + (actual == null ? "sin respuesta" : actual.text)
                            + "; esperada: " + pkg.product);
            }
            notifyPage("done", "Instalado y verificado: el FM-1 físico ejecuta " + pkg.product, 100);
        } catch (Exception error) {
            Log.e(TAG, "Physical FM-1 USB update failed", error);
            notifyPage("error", "Instalación física no completada: " + error.getMessage()
                    + ". Si el equipo quedó en el cargador de actualización, "
                    + "reintenta con el mismo paquete .fwsc y USB; evita apagarlo.", -1);
        } finally {
            flashing = false;
            cleanupUpload();
        }
    }

    private void armStockUpdate(Link link, Fm1OtaProtocol.Package pkg) throws Exception {
        if (!pkg.stock) throw new IOException("El firmware M-VAVE Open solo admite restaurar stock");
        byte[] cap = query(link, 49, new byte[0]);
        if (cap == null || cap.length != 47 || cap[0] != 'F' ||
                cap[1] != 'M' || cap[2] != 'C' || cap[3] != '1' || cap[6] != 0)
            throw new IOException("Firmware Open: el FM-1 no autorizó una restauración stock");
        byte[] request = new byte[38];
        request[0] = 1;
        request[1] = cap[5];
        for (int i = 0; i < 4; i++) request[2 + i] = (byte)(pkg.image.length >>> (8 * i));
        System.arraycopy(pkg.digest, 0, request, 6, 32);
        byte[] ack = query(link, 50, request);
        if (ack == null || ack.length != 1 || ack[0] != 0)
            throw new IOException("Firmware Open: restauración rechazada por el FM-1");
    }

    private static byte[] query(Link link, int command, byte[] request) throws Exception {
        link.clear();
        link.send(Fm1OtaProtocol.frame(command, request));
        long until = System.currentTimeMillis() + 1500L;
        while (System.currentTimeMillis() < until) {
            byte[] response = link.read(Math.max(1, until - System.currentTimeMillis()));
            if (response == null) continue;
            byte[] payload = Fm1OtaProtocol.queryPayload(response, command);
            if (payload != null) return payload;
        }
        return null;
    }

    private void serve(Link link, byte[] image, long expectedFinish,
                       long timeoutIdle, boolean writePhase) throws Exception {
        long last = System.currentTimeMillis();
        int served = 0;
        while (true) {
            byte[] message = link.read(850);
            if (message == null) {
                if (System.currentTimeMillis() - last > timeoutIdle)
                    throw new IOException("FM-1 dejó de solicitar bloques por USB MIDI");
                continue;
            }
            Fm1OtaProtocol.ReadRequest req = Fm1OtaProtocol.readRequest(message);
            if (req == null) continue;
            last = System.currentTimeMillis();
            if (req.address == Fm1OtaProtocol.FINISH_CHECK ||
                    req.address == Fm1OtaProtocol.FINISH_WRITE) {
                link.send(Fm1OtaProtocol.response(req.address, SUCCESS, 0, SUCCESS.length, req.flags));
                if (req.address == expectedFinish) return;
                continue;
            }
            if (req.length < 0 || req.length > Fm1OtaProtocol.MAX_REQUEST ||
                    req.address > image.length || req.length > (long)image.length - req.address)
                throw new IOException("El FM-1 solicitó un bloque fuera del archivo .fwsc; operación detenida.");
            sleep(10);
            link.send(Fm1OtaProtocol.response(req.address, image, (int)req.address,
                    req.length, req.flags));
            served++;
            if (writePhase && served % 12 == 0) {
                int percent = (int)Math.min(98L,
                        10L + (req.address + req.length) * 88L / image.length);
                notifyPage("writing", "Escribiendo firmware físico: " + percent
                        + "% · NO desconectes el USB", percent);
            }
        }
    }

    private static Fm1OtaProtocol.Identity handshake(Link link, int tries) throws Exception {
        link.clear();
        for (int n = 0; n < tries; ++n) {
            link.send(Fm1OtaProtocol.HANDSHAKE);
            long until = System.currentTimeMillis() + 1000L;
            while (System.currentTimeMillis() < until) {
                byte[] msg = link.read(Math.max(1L, until - System.currentTimeMillis()));
                if (msg == null) continue;
                Fm1OtaProtocol.Identity id = Fm1OtaProtocol.identity(msg);
                if (id != null) return id;
            }
        }
        return null;
    }

    private interface IdentityFilter {
        boolean accepts(Fm1OtaProtocol.Identity identity);
    }

    private Link waitFor(IdentityFilter filter, long timeout, MidiDeviceInfo selected,
                         Set<Integer> usbBaseline) throws Exception {
        long until = System.currentTimeMillis() + timeout;
        while (System.currentTimeMillis() < until) {
            for (MidiDeviceInfo candidate : usbDevices()) {
                // Only a user-selected USB FM-1 and devices appearing at the
                // same USB port during reboot are allowed. Identity validation
                // below remains mandatory before any firmware bytes are sent.
                if (candidate.getId() != selected.getId()
                        && usbBaseline.contains(candidate.getId())
                        && !looksLikeFm1(candidate)) continue;
                try {
                    Link link = Link.open(midi, candidate, ui);
                    Fm1OtaProtocol.Identity id;
                    try { id = handshake(link, 1); }
                    catch (Exception err) { link.close(); continue; }
                    if (id != null && filter.accepts(id)) return link;
                    link.close();
                } catch (Exception error) {
                    Log.d(TAG, "Waiting for USB re-enumeration", error);
                }
            }
            sleep(750);
        }
        return null;
    }

    private static boolean looksLikeFm1(MidiDeviceInfo info) {
        String name = info.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME, "");
        String maker = info.getProperties().getString(MidiDeviceInfo.PROPERTY_MANUFACTURER, "");
        String text = (name + " " + maker).toLowerCase(java.util.Locale.ROOT);
        return text.contains("fm-1") || text.contains("m-vave")
                || text.contains("felucca") || text.contains("jieli")
                || text.contains("ota-") || text.contains("ctl-1");
    }

    private List<MidiDeviceInfo> usbDevices() {
        List<MidiDeviceInfo> result = new ArrayList<>();
        if (midi == null) return result;
        for (MidiDeviceInfo info : midi.getDevices()) {
            if (info.getType() == MidiDeviceInfo.TYPE_USB &&
                    info.getInputPortCount() > 0 && info.getOutputPortCount() > 0)
                result.add(info);
        }
        return result;
    }

    private static void sleep(long ms) throws InterruptedException { Thread.sleep(ms); }

    void close() {
        closed = true;
        ++discoveryGeneration;
        // Only the user-confirmed flash must continue safely after the UI
        // closes. Waiting for a USB device can be stopped without risk.
        dismissDiscoveryDialog();
        worker.shutdown();
        if (!flashing) cleanupUpload();
    }

    /**
     * A MIDI port pair. Android's physical MIDI device's output port goes
     * TO the phone; its input port receives our SysEx FROM the phone.
     */
    private static final class Link implements AutoCloseable {
        final MidiDevice device;
        final MidiInputPort input;
        final MidiOutputPort output;
        final ArrayBlockingQueue<byte[]> messages = new ArrayBlockingQueue<>(256);
        final ByteArrayOutputStream partial = new ByteArrayOutputStream();
        boolean inSysex;
        final MidiReceiver receiver = new MidiReceiver() {
            @Override
            public void onSend(byte[] bytes, int offset, int count, long timestamp) {
                synchronized (partial) {
                    for (int i = offset; i < offset + count; i++) {
                        int value = bytes[i] & 255;
                        if (value >= 0xf8) continue;
                        if (value == 0xf0) {
                            partial.reset();
                            partial.write(value);
                            inSysex = true;
                        } else if (inSysex && value == 0xf7) {
                            partial.write(value);
                            if (!messages.offer(partial.toByteArray())) {
                                messages.poll();
                                messages.offer(partial.toByteArray());
                            }
                            partial.reset();
                            inSysex = false;
                        } else if (inSysex && value < 0x80) {
                            if (partial.size() < 8192) partial.write(value);
                            else { partial.reset(); inSysex = false; }
                        } else if (value >= 0x80) {
                            partial.reset();
                            inSysex = false;
                        }
                    }
                }
            }
        };

        private Link(MidiDevice device, MidiInputPort input, MidiOutputPort output) {
            this.device = device; this.input = input; this.output = output;
            output.connect(receiver);
        }

        static Link open(MidiManager manager, MidiDeviceInfo info, Handler callback)
                throws Exception {
            CountDownLatch ready = new CountDownLatch(1);
            MidiDevice[] opened = new MidiDevice[1];
            manager.openDevice(info, d -> { opened[0] = d; ready.countDown(); }, callback);
            if (!ready.await(4500, TimeUnit.MILLISECONDS) || opened[0] == null)
                throw new IOException("Android no pudo abrir el FM-1 como MIDI USB");
            MidiInputPort input = opened[0].openInputPort(0);
            MidiOutputPort output = opened[0].openOutputPort(0);
            if (input == null || output == null) {
                if (input != null) input.close();
                if (output != null) output.close();
                opened[0].close();
                throw new IOException("El dispositivo USB no tiene puertos MIDI bidireccionales");
            }
            Link link = new Link(opened[0], input, output);
            // Some USB-MIDI stacks drop SysEx immediately after a fresh
            // port opens. fm1_install.py waits 300 ms for exactly this.
            Thread.sleep(300L);
            return link;
        }

        void send(byte[] message) throws IOException { input.send(message, 0, message.length); }

        byte[] read(long millis) throws InterruptedException {
            return messages.poll(Math.max(1L, millis), TimeUnit.MILLISECONDS);
        }

        void clear() { messages.clear(); }

        @Override public void close() {
            try { output.disconnect(receiver); }
            catch (RuntimeException ignored) { /* USB may have disappeared */ }
            try { output.close(); } catch (IOException ignored) {}
            try { input.close(); } catch (IOException ignored) {}
            try { device.close(); } catch (IOException ignored) {}
            messages.clear();
        }
    }
}
