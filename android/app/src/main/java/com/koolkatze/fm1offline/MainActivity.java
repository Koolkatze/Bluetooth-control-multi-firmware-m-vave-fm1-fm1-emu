package com.koolkatze.fm1offline;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.media.AudioDeviceInfo;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiReceiver;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.ConsoleMessage;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceError;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONTokener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;

import androidx.webkit.WebViewAssetLoader;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Fully offline FM-1 emulator shell.
 *
 * No INTERNET permission. WebViewAssetLoader serves the Rust WASM and all
 * firmware packages from the APK. Android MidiManager provides native USB MIDI
 * and BLE MIDI input even on WebView builds without navigator.requestMIDIAccess.
 */
public final class MainActivity extends Activity {
    private static final String TAG = "FM1Mobile";
    private static final String HOST = "appassets.androidplatform.net";
    private static final String HOME = "https://" + HOST + "/assets/site/index.html?screen=0&fit=1";
    private static final int FILE_PICKER = 1001;
    private static final int BLUETOOTH_PERMISSIONS = 1002;
    private static final UUID BLE_MIDI_UUID = UUID.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");

    private final Handler ui = new Handler(Looper.getMainLooper());
    private WebView web;
    private ValueCallback<Uri[]> fileCallback;
    private MidiManager midiManager;
    private MidiDevice midiDevice;
    private final List<MidiOutputPort> outputPorts = new ArrayList<>();
    private BluetoothLeScanner scanner;
    private ScanCallback scannerCallback;
    private final Map<String, BluetoothDevice> nearbyMidi = new LinkedHashMap<>();
    private boolean scanning;
    private Button midiButton;
    private Button startButton;
    private Button panelButton;
    private Button lcdButton;
    private Button audioButton;
    private TextView liveStatus;
    private boolean webReady = false;
    private int readyChecks = 0;
    private boolean shuttingDown = false;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(0xff08090b);
        getWindow().setNavigationBarColor(0xff08090b);
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        midiManager = (MidiManager) getSystemService(MIDI_SERVICE);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(0xff08090b);
        web = new WebView(this);
        web.setBackgroundColor(0xff08090b);
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(true); // The user selects .fwsc/.syx via Android's document picker.
        settings.setAllowFileAccessFromFileURLs(false);
        settings.setAllowUniversalAccessFromFileURLs(false);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        // User initiates the audio via the native Start toolbar.
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setSupportMultipleWindows(false);
        web.addJavascriptInterface(new NativeBridge(), "FM1Android");

        WebViewAssetLoader localAssets = new WebViewAssetLoader.Builder()
                .addPathHandler("/assets/", new WebViewAssetLoader.AssetsPathHandler(this))
                .build();
        web.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                updateStatus("Cargando emulador y firmwares locales…");
                readyChecks = 0;
                ui.postDelayed(() -> checkWebReady(), 600);
            }

            @Override
            public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                super.onReceivedError(view, request, error);
                if (isLocal(request.getUrl())) {
                    String msg = "No se pudo cargar " + request.getUrl().getLastPathSegment()
                            + ": " + error.getDescription();
                    Log.e(TAG, msg);
                    runOnUiThread(() -> updateStatus(msg));
                }
            }

            @Override
            public void onReceivedHttpError(WebView view, WebResourceRequest request,
                                            WebResourceResponse response) {
                super.onReceivedHttpError(view, request, response);
                if (isLocal(request.getUrl())) {
                    String msg = "Archivo local no disponible: "
                            + request.getUrl().getLastPathSegment() + " (" + response.getStatusCode() + ")";
                    Log.e(TAG, msg);
                    runOnUiThread(() -> updateStatus(msg));
                }
            }

            @Override
            public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
                Uri uri = request.getUrl();
                if (!"https".equals(uri.getScheme()) || !HOST.equals(uri.getHost())
                        || !uri.getPath().startsWith("/assets/site/")) {
                    // Blobs and data: URLs are internal WebView primitives.
                    if ("blob".equals(uri.getScheme()) || "data".equals(uri.getScheme())) return null;
                    return denied();
                }
                WebResourceResponse response = localAssets.shouldInterceptRequest(uri);
                return response != null ? response : denied();
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                Uri uri = request.getUrl();
                return !"https".equals(uri.getScheme()) || !HOST.equals(uri.getHost())
                        || !uri.getPath().startsWith("/assets/site/");
            }
        });
        web.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback,
                                             FileChooserParams params) {
                if (fileCallback != null) fileCallback.onReceiveValue(null);
                fileCallback = callback;
                try {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    startActivityForResult(intent, FILE_PICKER);
                    return true;
                } catch (Exception e) {
                    fileCallback = null;
                    callback.onReceiveValue(null);
                    show("No hay selector de archivos: " + e.getMessage());
                    return false;
                }
            }

            @Override
            public boolean onConsoleMessage(ConsoleMessage message) {
                if (message.messageLevel() == ConsoleMessage.MessageLevel.ERROR) {
                    Log.e(TAG, "WebView: " + message.message() + " (" + message.lineNumber() + ")");
                    runOnUiThread(() -> updateStatus("Error web: " + message.message()));
                }
                return true;
            }
        });

        FrameLayout.LayoutParams pageParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        pageParams.bottomMargin = dp(82);
        root.addView(web, pageParams);

        // Persistent Android toolbar. It is independent of the HTML and cannot
        // disappear when the emulator is in LCD-only mode or fails to initialize.
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        controls.setBackgroundColor(0xff171b22);
        controls.setPadding(dp(8), dp(2), dp(8), dp(2));
        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);
        buttons.setGravity(Gravity.CENTER_VERTICAL);
        startButton = toolbarButton("▶ Iniciar firmware", 13);
        panelButton = toolbarButton("Panel", 12);
        lcdButton = toolbarButton("Pantalla", 12);
        midiButton = toolbarButton("MIDI", 12);
        audioButton = toolbarButton("Audio", 12);
        buttons.addView(startButton, new LinearLayout.LayoutParams(0, dp(46), 2.0f));
        buttons.addView(panelButton, new LinearLayout.LayoutParams(0, dp(46), 0.8f));
        buttons.addView(lcdButton, new LinearLayout.LayoutParams(0, dp(46), 1.0f));
        buttons.addView(midiButton, new LinearLayout.LayoutParams(0, dp(46), 0.8f));
        buttons.addView(audioButton, new LinearLayout.LayoutParams(0, dp(46), 0.9f));
        controls.addView(buttons);

        liveStatus = new TextView(this);
        liveStatus.setText("Preparando emulador local…");
        liveStatus.setTextColor(0xffbdc5d3);
        liveStatus.setTextSize(11);
        liveStatus.setSingleLine(true);
        liveStatus.setEllipsize(android.text.TextUtils.TruncateAt.END);
        controls.addView(liveStatus, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(25)));
        FrameLayout.LayoutParams controlsParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(76), Gravity.BOTTOM);
        root.addView(controls, controlsParams);

        startButton.setOnClickListener(v -> invokeInWeb(
                "window.fm1StartFirmware ? (window.fm1StartFirmware(), 'ok') : 'notready'",
                "Iniciando firmware…"));
        panelButton.setOnClickListener(v -> invokeInWeb(
                "window.fm1SetDisplayMode ? (window.fm1SetDisplayMode(false), 'ok') : 'notready'",
                "Mostrando panel…"));
        lcdButton.setOnClickListener(v -> invokeInWeb(
                "window.fm1SetDisplayMode ? (window.fm1SetDisplayMode(true), 'ok') : 'notready'",
                "Mostrando LCD del firmware…"));
        midiButton.setOnClickListener(v -> showMidiPicker());
        audioButton.setOnClickListener(v -> showAudioOptions());
        setContentView(root);

        // Browser location is local HTTPS backed by packaged assets; no network is used.
        web.loadUrl(HOME);
    }

    private boolean isLocal(Uri uri) {
        return "https".equals(uri.getScheme()) && HOST.equals(uri.getHost())
                && uri.getPath() != null && uri.getPath().startsWith("/assets/site/");
    }

    private Button toolbarButton(String label, int fontSize) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextColor(0xffffffff);
        button.setTextSize(fontSize);
        button.setAllCaps(false);
        button.setMinWidth(0);
        button.setMinHeight(0);
        button.setPadding(0, 0, 0, 0);
        button.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xff343e4a));
        return button;
    }

    private void updateStatus(String message) {
        if (liveStatus != null && !shuttingDown) liveStatus.setText(message);
    }

    private void checkWebReady() {
        if (web == null || shuttingDown || webReady) return;
        web.evaluateJavascript("typeof window.fm1NativeStatus === 'function'", answer -> {
            if (web == null || shuttingDown) return;
            if ("true".equals(answer)) {
                webReady = true;
                updateStatus("Preparado. Pulsa Iniciar firmware. Panel y Pantalla disponibles.");
            } else if (++readyChecks < 35) {
                ui.postDelayed(this::checkWebReady, 500);
            } else {
                updateStatus("Error: interfaz no iniciada. Revisa el motor WebView de Android.");
            }
        });
    }

    private void invokeInWeb(String js, String message) {
        if (web == null) return;
        if (!webReady) {
            updateStatus("El emulador no está preparado. Comprueba el mensaje de diagnóstico.");
            checkWebReady();
            return;
        }
        web.evaluateJavascript(js, value -> {
            if ("\"ok\"".equals(value)) {
                updateStatus(message);
                ui.postDelayed(this::reportRunningStatus, 1300);
            } else {
                updateStatus("No se pudo ejecutar: " + value);
            }
        });
    }

    private void reportRunningStatus() {
        if (web == null || !webReady || shuttingDown) return;
        String js = "(function(){var s=window.fm1NativeStatus?.();"
                + "return s ? (s.firmware + ' · ' + s.mode + ' · '"
                + "+ (s.playing ? 'sonando' : 'sin iniciar') + ' · ' + s.status)"
                + " : 'Motor no preparado';})()";
        web.evaluateJavascript(js, value -> {
            if (shuttingDown || web == null) return;
            try {
                String statusText = String.valueOf(new JSONTokener(value).nextValue());
                updateStatus(statusText);
            } catch (Exception e) {
                updateStatus("Diagnóstico: " + value);
            }
        });
    }

    private static WebResourceResponse denied() {
        return new WebResourceResponse("text/plain", "UTF-8", 403, "Offline only",
                Collections.singletonMap("Cache-Control", "no-store"),
                new ByteArrayInputStream(new byte[0]));
    }

    private int dp(int n) {
        return Math.round(n * getResources().getDisplayMetrics().density);
    }

    private void show(String message) {
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    /** This bridge is available only to the packaged, origin-restricted local page. */
    public final class NativeBridge {
        @JavascriptInterface
        public void ready() {
            Log.i(TAG, "Local emulator UI ready");
            runOnUiThread(() -> {
                webReady = true;
                updateStatus("Preparado. Pulsa Iniciar firmware.");
            });
        }

        @JavascriptInterface
        public void error(String message) {
            Log.e(TAG, "JavaScript: " + message);
            runOnUiThread(() -> updateStatus("Error: " + message));
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == FILE_PICKER && fileCallback != null) {
            fileCallback.onReceiveValue(
                    WebChromeClient.FileChooserParams.parseResult(resultCode, data));
            fileCallback = null;
        }
    }

    private boolean canUseBluetooth() {
        if (Build.VERSION.SDK_INT >= 31) {
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED ||
                    checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                            != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[] {
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                }, BLUETOOTH_PERMISSIONS);
                return false;
            }
        } else if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                    BLUETOOTH_PERMISSIONS);
            return false;
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grants) {
        super.onRequestPermissionsResult(requestCode, permissions, grants);
        if (requestCode != BLUETOOTH_PERMISSIONS) return;
        boolean granted = grants.length > 0;
        for (int g : grants) if (g != PackageManager.PERMISSION_GRANTED) granted = false;
        if (granted) startBleScan();
        else show("Permiso Bluetooth denegado");
    }

    private void showAudioOptions() {
        AudioManager manager = (AudioManager) getSystemService(AUDIO_SERVICE);
        String nativeRate = manager != null
                ? manager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE) : null;
        StringBuilder description = new StringBuilder("Salida nativa Android: ")
                .append(nativeRate == null ? "automática" : nativeRate + " Hz")
                .append(".\n");
        boolean bluetooth = false;
        boolean usb = false;
        if (manager != null) {
            for (AudioDeviceInfo device : manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)) {
                final int type = device.getType();
                bluetooth |= type == AudioDeviceInfo.TYPE_BLUETOOTH_A2DP
                        || type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO;
                usb |= type == AudioDeviceInfo.TYPE_USB_DEVICE || type == AudioDeviceInfo.TYPE_USB_HEADSET;
            }
        }
        if (usb) description.append("Hay un dispositivo de audio USB disponible.\n");
        if (bluetooth) description.append("Hay audio Bluetooth disponible; puede añadir mucha latencia.\n");
        description.append("Para tocar en directo, usa cable/USB-C y perfil rápido. ")
                .append("Cambiar el perfil reinicia el firmware, preservando sus ajustes guardados.");
        String[] profiles = {
                "Rápido — baja latencia (recomendado)",
                "Equilibrado — más estabilidad, más retraso",
                "Reproducción — búfer grande, NO para tocar en directo"
        };
        String[] ids = { "interactive", "balanced", "playback" };
        new AlertDialog.Builder(this)
                .setTitle("Audio del sintetizador")
                .setMessage(description.toString())
                .setItems(profiles, (dialog, index) -> {
                    String expression = "window.fm1SetAudioProfile ? (window.fm1SetAudioProfile('"
                            + ids[index] + "') ? 'ok' : 'invalid') : 'notready'";
                    invokeInWeb(expression, "Perfil de audio: " + ids[index]);
                })
                .setNegativeButton("Cancelar", null)
                .show();
    }

    private void showMidiPicker() {
        if (midiManager == null) {
            show("Este Android no dispone del servicio MIDI");
            return;
        }
        List<MidiDeviceInfo> devices = new ArrayList<>();
        List<String> labels = new ArrayList<>();
        for (MidiDeviceInfo info : midiManager.getDevices()) {
            if (info.getOutputPortCount() <= 0) continue; // Output *from* keyboard to Android.
            devices.add(info);
            String name = info.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME);
            if (name == null || name.isEmpty()) name = "Dispositivo MIDI #" + info.getId();
            labels.add(name + " • " + info.getOutputPortCount() + " salida(s)");
        }
        int scanIndex = labels.size();
        labels.add("Buscar MIDI Bluetooth (BLE)");
        int disconnectIndex = labels.size();
        labels.add("Desconectar MIDI");
        labels.add("Audio externo: selecciona salida en Android");
        new AlertDialog.Builder(this)
                .setTitle("Entrada MIDI • FM-1 Mobile")
                .setItems(labels.toArray(new String[0]), (dialog, item) -> {
                    if (item < devices.size()) openMidiDevice(devices.get(item));
                    else if (item == scanIndex) startBleScan();
                    else if (item == disconnectIndex) closeMidi();
                    else show("Empareja el altavoz Bluetooth o conecta un DAC USB-C desde Android.");
                })
                .setNegativeButton("Cerrar", null)
                .show();
    }

    private void openMidiDevice(MidiDeviceInfo info) {
        if (midiManager == null) return;
        midiManager.openDevice(info, device -> {
            if (device == null) {
                show("No se pudo abrir el dispositivo MIDI");
                return;
            }
            connectMidi(device);
        }, ui);
    }

    private void connectMidi(MidiDevice device) {
        closeMidi();
        midiDevice = device;
        int connected = 0;
        for (int i = 0; i < device.getInfo().getOutputPortCount(); i++) {
            MidiOutputPort port = device.openOutputPort(i);
            if (port == null) continue;
            port.connect(new MidiReceiver() {
                @Override
                public void onSend(byte[] bytes, int offset, int count, long timestamp) {
                    if (count < 1 || count > 4096) return;
                    byte[] copy = Arrays.copyOfRange(bytes, offset, offset + count);
                    ui.post(() -> deliverMidi(copy));
                }
            });
            outputPorts.add(port);
            connected++;
        }
        if (connected == 0) {
            closeMidi();
            show("El dispositivo no expone salidas MIDI");
        } else {
            midiButton.setText("MIDI ✓");
            show("MIDI conectado: " + connected + " puerto(s)");
        }
    }

    private void deliverMidi(byte[] bytes) {
        if (web == null) return;
        StringBuilder js = new StringBuilder("window.fm1NativeMidi&&window.fm1NativeMidi([");
        for (int i = 0; i < bytes.length; i++) {
            if (i > 0) js.append(',');
            js.append(bytes[i] & 0xff);
        }
        js.append("]);");
        web.evaluateJavascript(js.toString(), null);
    }

    private void closeMidi() {
        for (MidiOutputPort port : outputPorts) {
            try { port.close(); } catch (IOException ignored) {}
        }
        outputPorts.clear();
        if (midiDevice != null) {
            try { midiDevice.close(); } catch (IOException ignored) {}
            midiDevice = null;
        }
        if (midiButton != null) midiButton.setText("MIDI");
    }

    private void startBleScan() {
        if (!canUseBluetooth()) return;
        if (scanning) return;
        BluetoothManager bluetooth = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = bluetooth != null ? bluetooth.getAdapter() : null;
        if (adapter == null || !adapter.isEnabled()) {
            show("Activa Bluetooth en Android antes de buscar");
            return;
        }
        scanner = adapter.getBluetoothLeScanner();
        if (scanner == null) {
            show("Escáner BLE no disponible");
            return;
        }
        nearbyMidi.clear();
        scanning = true;
        scannerCallback = new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                BluetoothDevice device = result.getDevice();
                if (device == null) return;
                try {
                    String key = device.getAddress();
                    ui.post(() -> nearbyMidi.put(key, device));
                } catch (SecurityException ignored) {}
            }

            @Override
            public void onScanFailed(int errorCode) {
                ui.post(() -> {
                    stopBleScan();
                    show("Error de búsqueda MIDI BLE: " + errorCode);
                });
            }
        };
        try {
            ScanFilter midiFilter = new ScanFilter.Builder()
                    .setServiceUuid(new ParcelUuid(BLE_MIDI_UUID)).build();
            ScanSettings options = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
            scanner.startScan(Collections.singletonList(midiFilter), options, scannerCallback);
            show("Buscando controladores Bluetooth MIDI…");
            ui.postDelayed(this::showBleResults, 9000);
        } catch (SecurityException | IllegalStateException ex) {
            stopBleScan();
            show("No se pudo iniciar Bluetooth MIDI: " + ex.getMessage());
        }
    }

    private void stopBleScan() {
        if (!scanning) return;
        scanning = false;
        try {
            if (scanner != null && scannerCallback != null) scanner.stopScan(scannerCallback);
        } catch (SecurityException | IllegalStateException ignored) {}
        scannerCallback = null;
    }

    private void showBleResults() {
        if (!scanning) return;
        stopBleScan();
        List<BluetoothDevice> choices = new ArrayList<>(nearbyMidi.values());
        if (choices.isEmpty()) {
            show("No se encontró ningún dispositivo Bluetooth MIDI. Enciende CTL-1 BLE MIDI.");
            return;
        }
        List<String> labels = new ArrayList<>();
        for (BluetoothDevice d : choices) {
            String name = null;
            try { name = d.getName(); } catch (SecurityException ignored) {}
            labels.add((name == null || name.isEmpty() ? "Controlador BLE MIDI" : name)
                    + " • " + d.getAddress());
        }
        new AlertDialog.Builder(this)
                .setTitle("Elegir Bluetooth MIDI")
                .setItems(labels.toArray(new String[0]), (dialog, which) -> {
                    if (!canUseBluetooth()) return;
                    BluetoothDevice chosen = choices.get(which);
                    midiManager.openBluetoothDevice(chosen, device -> {
                        if (device == null) show("No se pudo conectar por Bluetooth MIDI");
                        else connectMidi(device);
                    }, ui);
                })
                .setNegativeButton("Cancelar", null)
                .show();
    }

    @Override
    protected void onDestroy() {
        shuttingDown = true;
        stopBleScan();
        closeMidi();
        if (fileCallback != null) {
            fileCallback.onReceiveValue(null);
            fileCallback = null;
        }
        if (web != null) {
            web.loadUrl("about:blank");
            web.destroy();
            web = null;
        }
        super.onDestroy();
    }
}
