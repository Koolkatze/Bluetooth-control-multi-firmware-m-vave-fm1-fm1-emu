# FM-1 Mobile — APK Android 100 % sin Internet durante su uso

Esta aplicación contiene el **emulador FM-1 completo compilado a WebAssembly**, la
interfaz web y los firmwares comunitarios que pudieron empaquetarse legal y
técnicamente durante la compilación. **No es una página web remota ni necesita
GitHub Pages.** Android carga todo desde el APK con
`WebViewAssetLoader` bajo la dirección HTTPS local reservada
`appassets.androidplatform.net`. El manifiesto no solicita el permiso
`android.permission.INTERNET`.

## Instalar un APK

1. Entra en [GitHub Actions → Android offline APK](https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/actions/workflows/android-offline.yml).
2. Abre una ejecución completada correctamente, descarga el artefacto
   **FM1-Mobile-Offline-APK** y extrae `app-debug.apk` del ZIP.
3. Copia el APK al teléfono e instálalo permitiendo temporalmente la instalación
   desde esa fuente. No necesitas una cuenta ni conexión una vez instalado.
4. Abre **FM-1 Mobile** en horizontal y pulsa **Start**. La pantalla muestra
   la LCD del firmware emulado (no la carcasa física).
5. El botón **MIDI** de la app (esquina izquierda) permite escoger dispositivos
   USB-MIDI detectados por Android o buscar controladores **BLE-MIDI**.
6. El audio sale por la ruta activa de Android. Para latencia más baja, conecta
   un DAC/interfaz USB-C; la salida Bluetooth también puede funcionar pero
   puede añadir latencia elevada.

El APK debug sirve para probarlo y se firma automáticamente con una
**clave de desarrollo**: no es una publicación firmada de producción.

## Firmwares 100 % locales

El proceso `tools/package_offline_android.mjs` se ejecuta en el ordenador de
compilación (GitHub Actions), **no en el teléfono**:

- genera `fm1_web.wasm` y copia HTML, CSS, JS, worklet y adaptador de LCD;
- obtiene paquetes `.fwsc` que el catálogo identifica con **licencias de código
  abierto explícitas y enlaces a código fuente**; comprueba su estructura y
  los guarda como archivos locales en `assets/site/offline-fw/`;
- sustituye las URL de descarga del catálogo por rutas locales;
- si un archivo no está disponible o su licencia no está clara, **no finge
  que está instalado**: se excluye y se deja constancia en
  `offline-bundle-report.json`.

**El firmware oficial propietario de M-VAVE no se distribuye dentro del APK.**
Se puede importar, sin conexión, desde un `FM-1.fwsc` que tengas guardado en
el teléfono usando **Mostrar panel → Open .fwsc…**. También puedes importar
cualquier otro firmware compatible `.fwsc` desde el almacenamiento local,
sin flashear el sintetizador físico. Los cambios guardados y presets quedan
en el almacenamiento interno de la app; exporta `.fm1save` para hacer copia.

Si quieres **empaquetar firmwares adicionales en tu propia compilación**, deben
estar disponibles en la carpeta de assets o importarse mediante el selector.
La app no tiene descarga de firmware en segundo plano ni hace peticiones web.

## M-VAVE físico: MIDI y dos pantallas

El modo deseado es **CTL-1 en el FM-1 real** (su pantalla propia muestra los
controles MIDI) y un **firmware sintetizador emulado en Android** (la pantalla
principal del móvil muestra ese sintetizador y el móvil genera el sonido).
No se captura ni se duplica la LCD del FM-1 físico. Instalar CTL-1 en el FM-1
real **sí requiere un flasheo inicial voluntario**; cambiar después el
sintetizador del móvil no modifica el FM-1.

La aplicación usa `MidiManager` para entradas USB MIDI y
`MidiManager.openBluetoothDevice` para dispositivos BLE MIDI anunciados con
el UUID estándar `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`. Pide permisos
Bluetooth al buscar. El **enlace extremo a extremo con CTL-1 en el hardware**
todavía necesita pruebas reales de puerto, canal y mapeo de mensajes.

## Compilación propia

Requisitos: Android SDK (plataforma 35), JDK 17, Gradle 8.10.2, Rust estable
con objetivo `wasm32-unknown-unknown` y Node.js 22 o superior.

```sh
rustup target add wasm32-unknown-unknown
web/build.sh _offline_web
node tools/package_offline_android.mjs _offline_web android/app/src/main/assets/site
node tools/check_offline_android.mjs android/app/src/main/assets/site
gradle -p android :app:assembleDebug
```

El APK se encuentra en
`android/app/build/outputs/apk/debug/app-debug.apk`. El archivo
`android/app/src/main/assets/site/` se genera en cada compilación y **no debe
subirse a GitHub**, ya que contiene paquetes de terceros.

### Limitaciones

- Requiere Android con un sistema WebView que admita WebAssembly y
  AudioWorklet (la mayoría de instalaciones recientes, pero no todas).
- Algunos firmwares emulados pueden ser más exigentes que la CPU del teléfono.
- No se garantiza que todos los firmwares del catálogo se empaqueten: las
  fuentes pueden estar temporalmente caídas y algunos no admiten redistribución.
- No hay selector de salida de audio propio: Android decide la ruta activa.
- Las notas/controladores MIDI dependen de la detección de dispositivos y su
  configuración, y BLE puede requerir permisos o configuraciones específicas.
- Solo funciona con la app abierta; el audio continuo en segundo plano y
  sincronización avanzada son trabajo futuro.
