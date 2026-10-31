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
import android.content.res.Configuration;
import android.graphics.Insets;
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
import android.util.Base64;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
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
import java.io.File;
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
    private FirmwareUpdater firmwareUpdater;
    private FrameLayout safeAreaRoot;
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
    private Button toggleControlsButton;
    private Button infoButton;
    private boolean webReady = false;
    private int readyChecks = 0;
    private boolean shuttingDown = false;
    private volatile boolean nativeActive = false;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(0xff08090b);
        getWindow().setNavigationBarColor(0xff08090b);
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // Android 15+/17 draws targetSdk 35 apps edge-to-edge, so the system
        // clock/battery/notch can cover the HTML header unless the *native*
        // root handles WindowInsets. Make the behavior consistent on older
        // Android versions, too: reserve the system-bar area in our root.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(false);
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
        }
        midiManager = (MidiManager) getSystemService(MIDI_SERVICE);
        Log.i(TAG, "Rust AAudio native library loaded: " + NativeSynth.AVAILABLE);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        FrameLayout root = new FrameLayout(this);
        safeAreaRoot = root;
        root.setBackgroundColor(0xff08090b);
        // A single safe-area inset on the outermost Android view protects
        // *every* embedded page (FM-1, SMK-37, firmware browser, settings,
        // panel and LCD-only) and the permanent native control bar.
        // The upper band is intentionally empty and matches the background.
        root.setPadding(0, dp(36), 0, 0); // safe fallback until insets arrive
        root.setOnApplyWindowInsetsListener((view, insets) -> {
            int left, top, right, bottom;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                Insets safe = insets.getInsets(WindowInsets.Type.systemBars()
                        | WindowInsets.Type.displayCutout());
                left = safe.left;
                top = safe.top;
                right = safe.right;
                // Keep text fields visible when the Android keyboard appears.
                // Root consumes insets, so it must handle the IME as well.
                bottom = Math.max(safe.bottom,
                        insets.getInsets(WindowInsets.Type.ime()).bottom);
            } else {
                left = insets.getSystemWindowInsetLeft();
                top = insets.getSystemWindowInsetTop();
                right = insets.getSystemWindowInsetRight();
                bottom = insets.getSystemWindowInsetBottom();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P &&
                        insets.getDisplayCutout() != null) {
                    android.view.DisplayCutout cutout = insets.getDisplayCutout();
                    left = Math.max(left, cutout.getSafeInsetLeft());
                    top = Math.max(top, cutout.getSafeInsetTop());
                    right = Math.max(right, cutout.getSafeInsetRight());
                    bottom = Math.max(bottom, cutout.getSafeInsetBottom());
                }
            }
            // A little more breathing space below the status bar; on devices
            // that report zero insets, reserve at least 24dp for the clock.
            view.setPadding(left, Math.max(top, dp(24)) + dp(10), right, bottom);
            // We've applied these insets to the parent: don't apply them
            // again inside WebView via CSS env(safe-area-inset-top).
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
                return WindowInsets.CONSUMED;
            WindowInsets consumed = insets.consumeSystemWindowInsets();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
                consumed = consumed.consumeDisplayCutout();
            return consumed;
        });
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
        // The WebView remains fully local. Only native, explicit updater
        // requests can reach the public firmware sources over HTTPS.
        File updateFolder = getDir("fm1_firmware_updates", MODE_PRIVATE);
        firmwareUpdater = new FirmwareUpdater(this, web, updateFolder);
        web.addJavascriptInterface(new NativeBridge(), "FM1Android");

        WebViewAssetLoader localAssets = new WebViewAssetLoader.Builder()
                .addPathHandler("/assets/", new WebViewAssetLoader.AssetsPathHandler(this))
                .addPathHandler("/updates/",
                        new WebViewAssetLoader.InternalStoragePathHandler(this, updateFolder))
                .build();
        web.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                // The browser's sound/CPU status remains up to date, but the
                // Android shell presents it through the small info button.
                // Apply this to FM-1, SMK-37, and all local subpages.
                view.evaluateJavascript("(function(){"
                        + "if(document.getElementById('fm1-android-status-style'))return;"
                        + "var css=document.createElement('style');"
                        + "css.id='fm1-android-status-style';"
                        + "css.textContent='#status{display:none!important}';"
                        + "document.head.appendChild(css);"
                        + "})()", null);
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
                        || !(uri.getPath().startsWith("/assets/site/")
                             || uri.getPath().startsWith("/updates/"))) {
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

        // The emulated LCD uses the ENTIRE usable display. Native controls
        // float above the bottom edge only when explicitly expanded.
        FrameLayout.LayoutParams pageParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        root.addView(web, pageParams);

        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        controls.setBackgroundColor(0xe6171b22);
        controls.setPadding(dp(7), dp(3), dp(7), dp(3));
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
        controls.setVisibility(View.GONE);
        FrameLayout.LayoutParams controlsParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        controlsParams.bottomMargin = dp(45);
        root.addView(controls, controlsParams);

        // Keep diagnostics in memory without taking LCD space.
        liveStatus = new TextView(this);
        liveStatus.setText("Preparando emulador local…");

        // Two small, translucent, permanently reachable floating buttons.
        LinearLayout quickActions = new LinearLayout(this);
        quickActions.setOrientation(LinearLayout.HORIZONTAL);
        quickActions.setGravity(Gravity.CENTER);
        quickActions.setPadding(dp(3), 0, dp(3), 0);
        toggleControlsButton = toolbarButton("▲", 17);
        toggleControlsButton.setContentDescription("Mostrar controles");
        toggleControlsButton.setBackgroundTintList(
                android.content.res.ColorStateList.valueOf(0x99343e4a));
        infoButton = toolbarButton("ⓘ", 19);
        infoButton.setContentDescription("Datos de sonido");
        infoButton.setBackgroundTintList(
                android.content.res.ColorStateList.valueOf(0x99343e4a));
        LinearLayout.LayoutParams quickButtonParams =
                new LinearLayout.LayoutParams(dp(42), dp(38));
        quickButtonParams.setMargins(dp(2), 0, dp(2), 0);
        quickActions.addView(toggleControlsButton, quickButtonParams);
        LinearLayout.LayoutParams infoParams =
                new LinearLayout.LayoutParams(dp(42), dp(38));
        infoParams.setMargins(dp(2), 0, dp(2), 0);
        quickActions.addView(infoButton, infoParams);
        FrameLayout.LayoutParams quickParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(40),
                Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        quickParams.bottomMargin = dp(4);
        root.addView(quickActions, quickParams);

        toggleControlsButton.setOnClickListener(v -> {
            boolean expand = controls.getVisibility() != View.VISIBLE;
            controls.setVisibility(expand ? View.VISIBLE : View.GONE);
            toggleControlsButton.setText(expand ? "▼" : "▲");
            toggleControlsButton.setContentDescription(
                    expand ? "Ocultar controles" : "Mostrar controles");
        });
        infoButton.setOnClickListener(v -> showSoundDiagnostics());

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
        audioButton.setOnClickListener(v -> {
            if (nativeActive) {
                new AlertDialog.Builder(this).setTitle("Audio nativo AAudio")
                        .setMessage("El motor Rust nativo utiliza AAudio en modo baja latencia. "
                                + "La latencia de un altavoz Bluetooth sigue dependiendo del dispositivo. "
                                + "Para tocar utiliza USB-C o salida de audio por cable.")
                        .setPositiveButton("Aceptar", null).show();
            } else {
                showAudioOptions();
            }
        });
        setContentView(root);
        root.requestApplyInsets();

        // Browser location is local HTTPS backed by packaged assets; no network is used.
        web.loadUrl(HOME);
    }

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);
        // Status and navigation bars can change sides in landscape/rotation.
        if (safeAreaRoot != null) safeAreaRoot.requestApplyInsets();
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

    private void showSoundDiagnostics() {
        StringBuilder details = new StringBuilder();
        details.append(liveStatus != null ? liveStatus.getText() : "Sin datos")
                .append("\n\nMotor: ")
                .append(NativeSynth.AVAILABLE ? "Rust / AAudio nativo" : "WebAssembly");
        if (NativeSynth.AVAILABLE) {
            try {
                details.append("\nEstado nativo: ").append(NativeSynth.stats());
                details.append("\nDiagnóstico PCM: ").append(NativeSynth.diagnostics());
            } catch (Exception | UnsatisfiedLinkError e) {
                details.append("\nNo se pudo consultar AAudio: ").append(e.getMessage());
            }
        }
        if (web == null) {
            new AlertDialog.Builder(this).setTitle("Información y datos de sonido")
                    .setMessage(details.toString())
                    .setPositiveButton("Cerrar", null).show();
            return;
        }
        web.evaluateJavascript("(function(){var s=document.getElementById('status');"
                + "return s ? (s.textContent || '') : '';})()", result -> {
            if (shuttingDown || isFinishing()) return;
            String webStatus = "";
            try {
                Object parsed = new JSONTokener(result).nextValue();
                if (parsed instanceof String) webStatus = (String) parsed;
            } catch (Exception ignored) {
                // The native diagnostic remains useful even without the page.
            }
            if (!webStatus.trim().isEmpty()) {
                details.append("\n\nEmulador: ").append(webStatus);
            }
            new AlertDialog.Builder(this).setTitle("Información y datos de sonido")
                    .setMessage(details.toString())
                    .setPositiveButton("Cerrar", null).show();
        });
    }

    private void checkWebReady() {
        if (web == null || shuttingDown || webReady) return;
        web.evaluateJavascript("typeof window.fm1NativeStatus === 'function'", answer -> {
            if (web == null || shuttingDown) return;
            if ("true".equals(answer)) {
                webReady = true;
                updateStatus(NativeSynth.AVAILABLE
                        ? "Preparado. Motor Rust + AAudio nativo. Pulsa Iniciar firmware."
                        : "Preparado. Motor WebAssembly de reserva. Pulsa Iniciar firmware.");
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
                updateStatus(NativeSynth.AVAILABLE
                        ? "Preparado. Motor Rust/AAudio nativo disponible. Pulsa Iniciar firmware."
                        : "Preparado. Motor WebAssembly de reserva. Pulsa Iniciar firmware.");
            });
        }

        @JavascriptInterface
        public String installedFirmwareUpdates(String device) {
            return firmwareUpdater == null ? "{}" : firmwareUpdater.installedJson(device);
        }

        @JavascriptInterface
        public boolean updateFirmwares(String device, String selectedId) {
            return firmwareUpdater != null
                    && firmwareUpdater.start(device, selectedId == null ? "" : selectedId);
        }

        @JavascriptInterface
        public boolean nativeAvailable() { return NativeSynth.AVAILABLE; }

        /**
         * Firmware was loaded from bundled APK resources or the Android document
         * picker by the offline web selector. This is called once per boot, NOT
         * from the audio callback. No network access is needed.
         */
        @JavascriptInterface
        public String bootNative(String base64, int clockMhz, String fingerprint) {
            if (!NativeSynth.AVAILABLE) return "ERROR:Biblioteca AAudio no disponible";
            if (base64 == null || base64.length() > 24000000) return "ERROR:Firmware demasiado grande";
            try {
                byte[] packageBytes = Base64.decode(base64, Base64.DEFAULT);
                String result = NativeSynth.boot(packageBytes, clockMhz, fingerprint,
                        getFilesDir().getAbsolutePath());
                nativeActive = result != null && result.startsWith("OK:");
                runOnUiThread(() -> {
                    if (nativeActive) {
                        updateStatus("Rust + AAudio: " + result.substring(3) + " Hz · audio nativo activo");
                        audioButton.setText("AAudio");
                    } else {
                        updateStatus(result);
                        audioButton.setText("Audio");
                    }
                });
                return result;
            } catch (Throwable error) {
                nativeActive = false;
                Log.e(TAG, "Native synth start failed", error);
                return "ERROR:" + error.getMessage();
            }
        }

        @JavascriptInterface
        public String nativeLcd() {
            if (!nativeActive || !NativeSynth.AVAILABLE) return "";
            try {
                byte[] pixels = NativeSynth.lcd();
                return pixels == null ? "" : Base64.encodeToString(pixels, Base64.NO_WRAP);
            } catch (Throwable error) {
                return "";
            }
        }

        @JavascriptInterface
        public String nativeStats() {
            if (!nativeActive || !NativeSynth.AVAILABLE) return "0";
            return Long.toString(NativeSynth.stats());
        }

        @JavascriptInterface
        public String nativeDiagnostics() {
            if (!nativeActive || !NativeSynth.AVAILABLE) return "0,0,0";
            return NativeSynth.diagnostics();
        }

        @JavascriptInterface
        public void nativeControl(int kind, int key, int value) {
            if (nativeActive && NativeSynth.AVAILABLE)
                NativeSynth.control(kind, key, value);
        }

        @JavascriptInterface
        public void nativeMidi(int[] values) {
            if (!nativeActive || !NativeSynth.AVAILABLE || values == null
                    || values.length == 0 || values.length > 3) return;
            byte[] b = new byte[values.length];
            for (int i = 0; i < b.length; i++) b[i] = (byte) values[i];
            NativeSynth.midi(b);
        }

        @JavascriptInterface
        public void stopNative() {
            if (NativeSynth.AVAILABLE) NativeSynth.stop();
            nativeActive = false;
            runOnUiThread(() -> {
                if (audioButton != null) audioButton.setText("Audio");
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
                    if (nativeActive && NativeSynth.AVAILABLE) {
                        // Native MIDI should never queue behind JavaScript/UI painting.
                        deliverMidi(copy);
                    } else {
                        ui.post(() -> deliverMidi(copy));
                    }
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
        if (nativeActive && NativeSynth.AVAILABLE) {
            // Pass MIDI directly from Android MidiManager to the Rust worker.
            // No JavaScript/main-thread scheduling is involved in note delivery.
            for (int i = 0; i < bytes.length;) {
                int status = bytes[i] & 0xff;
                if (status < 0x80) { i++; continue; }
                int len = status >= 0xf8 ? 1 :
                        status == 0xf1 || status == 0xf3 ? 2 :
                        status == 0xf2 ? 3 :
                        status >= 0xf0 ? 1 :
                        (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 2 : 3;
                if (i + len > bytes.length) break;
                NativeSynth.midi(Arrays.copyOfRange(bytes, i, i + len));
                i += len;
            }
            return;
        }
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
        if (firmwareUpdater != null) firmwareUpdater.close();
        if (NativeSynth.AVAILABLE) NativeSynth.stop();
        nativeActive = false;
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
