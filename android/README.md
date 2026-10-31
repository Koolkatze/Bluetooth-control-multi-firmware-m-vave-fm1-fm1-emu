# FM-1 Mobile — sintetizador Android sin conexión, actualizaciones opcionales por Internet

Esta aplicación contiene **dos implementaciones del emulador FM-1**:
el **motor nativo Rust/AAudio** de baja latencia para Android, y el
motor WebAssembly/AudioWorklet como reserva y para el modo web. También
incluye la interfaz móvil y los firmwares comunitarios que pudieron
empaquetarse legal y técnicamente durante la compilación. **No es una página web remota ni necesita
GitHub Pages.** Android carga todos los componentes del emulador desde el APK con
`WebViewAssetLoader` bajo la dirección HTTPS local reservada
`appassets.androidplatform.net`. La síntesis, las pantallas y los firmwares
instalados siguen funcionando sin conexión. **Solo los botones de
actualización solicitan acceso a Internet** para consultar versiones nuevas.

## Motor nativo nuevo (v0.2.0 experimental)

**La app Android ya no depende de WebAudio para producir el sonido cuando hay
una biblioteca nativa compatible.** Ahora contiene un segundo motor construido
a partir del **núcleo Rust real** (`fm1-core` + `fm1-soc`), que ejecuta el
firmware del FM-1 en un hilo de trabajo independiente. Utiliza **AAudio** de
Android para entregar PCM con una cola **SPSC sin bloqueos**: el callback solo
copia muestras y nunca ejecuta código del firmware, compila JIT, toca la LCD o
llama a JavaScript. El worker hace una conversión continua desde 44,1 kHz hacia
la frecuencia de salida nativa del teléfono (normalmente 48 kHz).

La app empaqueta bibliotecas Android para **arm64-v8a y x86_64**. Si la
biblioteca no se carga, utiliza el motor WebAssembly/AudioWorklet de reserva.
En v0.2.3 la barra inferior empieza **plegada**: la pantalla del sintetizador
ocupa todo el espacio disponible. Una flecha semitransparente **▲** abre los
controles «Iniciar firmware», «Panel», «Pantalla», «MIDI» y «Audio», y cambia a
**▼** para plegarlos. El botón **ⓘ** muestra los datos del motor Rust/AAudio,
el estado del emulador y las estadísticas de audio sin ocupar espacio permanente.
La zona superior continúa reservada para la barra de estado de Android.

**Ajuste de pantalla v0.2.4:** en la vista «Solo LCD», tanto
«Original» (imagen 1:1) como «Horizontal» (imagen 16:9) calculan
su tamaño a partir del área útil real del WebView. La altura nunca
sobrepasa el área comprendida entre la barra superior del sistema
y los botones flotantes inferiores. Al girar el teléfono o cambiar
las dimensiones de la pantalla, se recalcula sin deformar el firmware.

Las notas del MIDI USB-C/BLE entran directamente desde `MidiManager` en la
cola del motor Rust, **sin esperar a que la pantalla termine de redibujarse**.
Los controles del panel táctil también llegan a Rust. La LCD sigue apareciendo
en la interfaz del móvil, pero ahora procede de la CPU nativa; no se captura
ninguna pantalla del FM-1 físico.

**Guardado y advertencias:** el motor Rust almacena periódicamente los sectores
de flash modificados por cada firmware en el almacenamiento privado Android
(`fm1-native-<hash>.bin`). Este formato todavía **no es compatible con el
botón Exportar .fm1save de la web**; no desinstales la app sin una copia de
seguridad externa de los datos que necesites conservar. Las importaciones
`.syx` y la correspondencia completa de todas las funciones del motor web
siguen pendientes de validación en la ruta nativa.

**No está verificada todavía una latencia equivalente a la del FM-1 físico en
un teléfono real.** AAudio puede reducir mucho el retraso del motor, pero
Bluetooth A2DP añade su propia latencia; para interpretación en directo utiliza
USB-C/cable. La latencia final requiere medición física con MIDI, DAC y
firmwares concretos.

## Instalar un APK

1. Entra en [GitHub Actions → Android offline APK](https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/actions/workflows/android-offline.yml).
2. Abre una ejecución completada correctamente, descarga el artefacto
   **FM1-Mobile-Offline-APK** y extrae `app-debug.apk` del ZIP.
3. Copia el APK al teléfono e instálalo permitiendo temporalmente la instalación
   desde esa fuente. No necesitas una cuenta ni conexión una vez instalado.
4. Abre **FM-1 Mobile** en horizontal. Pulsa **▲** para desplegar los
   controles y **Iniciar firmware** para arrancar. Después usa **▼** para
   recuperar toda la pantalla. El botón **ⓘ** abre los datos de sonido.
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
Desde la versión **v0.2.5**, el panel **Firmware** ofrece:
- **Actualizar seleccionado:** comprueba en el catálogo público la versión del
  firmware elegido y descarga solo ese paquete si hay una versión distinta.
- **Actualizar todos:** revisa los firmwares libres instalados en el catálogo
  y descarga únicamente los que hayan cambiado.
- Ambos botones muestran el progreso y los errores en la pestaña Firmware.
  Requieren Internet solo cuando se pulsan; sin conexión el emulador funciona igual.

La APK consulta el catálogo del repositorio en GitHub por HTTPS. Descarga
firmwares de sus fuentes públicas autorizadas, comprueba el formato de
los paquetes y conserva las descargas en el almacenamiento privado de Android.
**No modifica ni flashea el FM-1 físico**. Tampoco descarga automáticamente
el firmware oficial propietario de M-VAVE ni sustituye los presets antiguos,
que siguen asociados al identificador criptográfico de su versión.
**Para aplicar una descarga hay que volver a seleccionar o iniciar ese firmware**:
no se interrumpe el audio mientras se actualiza. El selector muestra la versión
local disponible al cargarla. Los firmwares nuevos que no figuran en el catálogo
de la APK requieren una aplicación actualizada o importación manual.

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
