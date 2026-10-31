# FM-1 Mobile Companion — multifirmware, pantalla ampliada y síntesis desde el móvil


## 🎹 Prueba el FM-1 directamente en tu navegador

### **[▶ Abrir FM-1 Online — GitHub Pages](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?screen=1&fit=1)**

**Sin instalar nada:** elige un firmware compatible, pulsa **Start** y utiliza la LCD ampliada desde tu navegador. Esta edición online necesita conexión para cargar los archivos y algunos firmwares.

### **[⬇ Descargar la aplicación Android sin Internet — M-VAVE FM1: Firmware bundle](https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/releases/tag/v0.2.0)**

Versión **0.2.0 (motor nativo experimental)**: APK con el nuevo icono FM-1, emulador y paquetes compatibles integrados. Tras instalarlo, los firmwares incluidos funcionan sin conexión. La integración con el FM-1 físico por MIDI sigue en fase de pruebas. Esta versión incluye **un motor nuevo en Rust nativo con salida AAudio**, independiente del AudioWorklet: la CPU/JIT y los periféricos del FM-1 se ejecutan fuera del callback de audio, mientras MIDI USB/BLE llega directamente a Rust. Conserva la pantalla y los firmwares offline. **Es experimental: la latencia real aún no está medida. Para tocar en directo, utiliza audio USB-C o cableado; Bluetooth añade retardo propio.**


**Objetivo del proyecto:** convertir un teléfono Android en la **pantalla principal y el motor de síntesis** del M-VAVE FM-1, para tocar y alternar entre firmwares compatibles **sin tener que flashear el FM-1 físico cada vez que se cambia de sintetizador**. El teléfono puede funcionar también de forma independiente. En el modo combinado, el FM-1 físico lleva un **firmware controlador MIDI como CTL-1**, que muestra su propia interfaz en la pantalla integrada: esa pantalla queda como **pantalla secundaria de control**, mientras el móvil muestra la pantalla del sintetizador emulado y genera el sonido. **No buscamos capturar ni duplicar la LCD física.**

**[Abrir el proyecto web](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?screen=1&fit=1)** · [Código fuente](https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu) · [Adaptación de la LCD](docs/screen-adaptation.md)

> **Estado:** prototipo basado en el emulador de código abierto [joakimeriksson/fm1-emu](https://github.com/joakimeriksson/fm1-emu). La ejecución virtual de firmwares, la LCD del sintetizador emulado, la salida de audio del navegador y la entrada Web MIDI están implementadas en la web. El firmware [CTL-1](https://github.com/charlesvestal/fm1-ctl) **sí permite convertir el FM-1 físico en un controlador MIDI USB y Bluetooth con pantalla propia**, según su autor. **Todavía falta verificar e integrar el enlace CTL-1 físico → Android/Web MIDI → sintetizador emulado**, incluida la compatibilidad con el navegador, los controles y la latencia. Nunca se requiere capturar la LCD del FM-1.

## Nuevo motor Android nativo — v0.2.0 experimental

La versión Android tiene ahora un motor de síntesis **Rust compilado para el
procesador del móvil**. La CPU emulada, periféricos y código JIT del FM-1 corren
en un hilo separado de la salida de audio. **AAudio** reproduce muestras PCM
mediante una cola lock-free; el hilo de audio no compila JIT, no dibuja pantallas
y no ejecuta JavaScript. El MIDI USB/BLE entra directamente desde Android al
hilo Rust.

La pantalla del móvil sigue siendo la del firmware emulado. El FM-1 físico
puede seguir utilizando CTL-1 como controlador con su propia pantalla. Si el
motor nativo no está disponible en la arquitectura, la app conserva el
motor WebAssembly de reserva.

**Estado:** nueva implementación en pruebas. No se ha medido aún una latencia
equivalente al FM-1 físico en un teléfono real. La salida Bluetooth sigue
añadiendo retardo; para tocar es preferible **USB-C o cable**. Los cambios de
flash del firmware nativo se guardan automáticamente en el almacenamiento
privado Android, pero su exportación al formato web `.fm1save` aún no existe.
[Arquitectura nativa, instalación y limitaciones](android/README.md).

## Aplicación Android: sintetizador sin conexión

**Ya existe un APK Android de prueba generado por GitHub Actions.** A diferencia
de la página web, el APK **incluye los archivos HTML/JS, el motor WebAssembly
y los paquetes de firmware admitidos**, de forma que no necesita conectarse a
Internet en el teléfono. La aplicación **no solicita permiso de Internet**.

**[Descargar APK de la release v0.1.0](https://github.com/Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/releases/tag/v0.2.0)** ·
**[Código Android, instrucciones y limitaciones](android/README.md)**

La primera compilación offline ha empaquetado **92 firmwares FM-1 más 9 de
SMK-37 (101 en total)** y ha pasado el arranque de un firmware con salida de
audio en la prueba automatizada. Esto verifica la construcción del APK,
**no reemplaza una prueba funcional en un teléfono Android real**. Los
firmwares incluidos se ejecutan sin conexión; también se pueden importar
archivos `.fwsc` compatibles desde el almacenamiento local.

- **Pantalla principal:** LCD del sintetizador ejecutado en Android (vista
  horizontal y vista original).
- **Pantalla secundaria física:** si conectas un M-VAVE FM-1 con **CTL-1**
  instalado, la pantalla del instrumento sigue mostrando la propia interfaz
  del controlador MIDI; **no hay captura ni duplicación de pantalla**.
- **MIDI:** botón nativo para seleccionar entradas **USB-MIDI** y buscar
  **BLE-MIDI** por Bluetooth. La integración específica con CTL-1 real todavía
  necesita pruebas con hardware y Android.
- **Audio:** generación local desde la CPU del móvil, con salida por la ruta
  activa de Android, como altavoz Bluetooth o una interfaz USB-C. Para
  interpretación en directo, USB-C suele ofrecer menor latencia.
- **Firmware oficial:** `FM-1.fwsc` es propietario y **no se incluye** en el
  APK público; impórtalo desde tu propia copia ya guardada en el móvil.
- **Instalación:** descarga el artefacto `FM1-Mobile-Offline-APK` de una
  ejecución exitosa de GitHub Actions, descomprime el archivo descargado e
  instala `app-debug.apk`. Es una compilación de prueba, no una versión
  publicada en Google Play.

## Qué queremos conseguir

1. **Cambiar entre firmwares del sintetizador sin reflashear el FM-1 físico.** Seleccionar y arrancar en el teléfono cualquier paquete `.fwsc` **compatible con el emulador**, incluidos firmwares comunitarios y el oficial de M-VAVE cuando el usuario aporte el archivo. Los firmwares de síntesis se ejecutan *virtualmente*. **CTL-1 se instala una vez en el FM-1 físico**, si se desea usarlo como controlador; alternar los firmwares de síntesis del móvil no vuelve a escribir su flash.
2. **Dos pantallas distintas y útiles.** La pantalla **principal del teléfono** muestra solo la LCD del firmware sintetizador **emulado**, ampliada y adaptada a horizontal sin dibujar el chasis. La **LCD física secundaria** del FM-1 conserva la interfaz del firmware controlador instalado (por ejemplo, CTL-1: notas, bancos MIDI, CC, ajustes, secuenciador). **No se transmite ni se espeja ninguna de las dos pantallas a la otra.**
3. **Usar el móvil como sintetizador.** Ejecutar la CPU emulada y el motor de audio en el teléfono, recibir notas y controles MIDI y producir el sonido desde el propio móvil. Así el móvil asume el procesamiento cuando se utiliza el motor emulado.
4. **FM-1 físico como controlador MIDI.** Utilizar **CTL-1** (o un firmware equivalente) en el hardware para enviar notas, controles, bancos y mensajes MIDI por **USB-C o Bluetooth** al sintetizador emulado del teléfono. La pantalla del FM-1 seguirá enseñando **la interfaz de CTL-1**, no una copia del firmware del móvil.
5. **Escuchar el sonido por un altavoz o interfaz externa.** Permitir el uso del audio del teléfono por **Bluetooth** o **USB-C** (por ejemplo, altavoz Bluetooth, interfaz de audio USB o DAC USB-C). Minimizar la latencia, especialmente al tocar en directo.

## Dos formas de uso

### A. Teléfono independiente: sintetizador FM-1 virtual

```text
Firmware .fwsc compatible
          │
          ▼
Emulador FM-1 (CPU/periféricos) en WebAssembly
          ├── LCD RGB565 240×240 ──► interfaz LCD móvil ──► pantalla del teléfono
          ├── motor de audio ──────► AudioWorklet ────────► salida del sistema
          └── MIDI emulado ◄──────── Web MIDI ◄──────────── teclado/controlador compatible
```

En este modo **no hace falta conectar el FM-1 físico ni flashearlo**. El móvil ejecuta el sintetizador emulado; el audio se genera en el móvil, no se transmite desde la CPU del FM-1 físico. La CPU del teléfono realiza el trabajo de emulación, de modo que el rendimiento depende del navegador, del dispositivo y del firmware elegido.

### B. FM-1 físico + móvil: controlador y sintetizador separados (**integración pendiente**)

```text
FM-1 físico — firmware CTL-1 instalado
  ├── Teclas, botones y potenciómetros
  │             │
  │             └── MIDI USB-C o Bluetooth ─────► Android / Web MIDI
  │                                                   │
  └── LCD propia: controles CTL-1                    ▼
      (bancos MIDI, CC, arpegiador...)     Firmware sintetizador EMULADO
                                             ├── LCD virtual ──► PANTALLA PRINCIPAL DEL MÓVIL
                                             └── CPU + audio ──► altavoz BT / interfaz USB-C
```

**Ambas pantallas son independientes y tienen funciones distintas.** La LCD del FM-1 muestra **el firmware controlador CTL-1 instalado en el hardware**; la pantalla del teléfono presenta **el firmware de síntesis que esté ejecutando su emulador**. **No hay captura, duplicación ni transmisión de imágenes desde el FM-1 físico**: únicamente necesitamos transmitir eventos de control **MIDI** hacia el móvil.

**El firmware controlador existe:** [CTL-1](https://github.com/charlesvestal/fm1-ctl) de Charles Vestal ofrece controles MIDI, cuatro bancos para los potenciómetros, secuenciación, arpegiador, MIDI USB y Bluetooth en la versión 1.0. [Manual y pantallas de CTL-1](https://charlesvestal.github.io/fm1-ctl/manual/) · [Instalador del autor](https://charlesvestal.github.io/fm1-ctl/install/). **La instalación de CTL-1 en el FM-1 real sí es un flasheo inicial voluntario**, y conlleva los riesgos habituales de instalar firmware de terceros. No es necesario volver a flashear al cambiar de sintetizador en el móvil.

La CPU del teléfono ejecutará la emulación y la síntesis. No se acelera físicamente la CPU del FM-1: el dispositivo seguirá ejecutando CTL-1 para sus controles, mientras la carga de sonido recae en Android. Hay que **comprobar el emparejamiento Bluetooth MIDI o el transporte USB MIDI en el móvil y mapear los mensajes de CTL-1 al firmware emulado**. No se considera aún probado en el conjunto hardware + Android + navegador.

## Estado actual y trabajo pendiente

| Función | Estado | Qué significa exactamente |
| --- | --- | --- |
| Ejecutar firmwares comunitarios compatibles sin flashear el dispositivo | **Implementado en el emulador web** | Se descargan paquetes de sus autores y se ejecutan virtualmente. No se instala firmware en el FM-1 físico. |
| Ejecutar el firmware oficial de M-VAVE | **Implementado, requiere archivo del usuario** | Abrir `FM-1.fwsc` desde el navegador; la web no lo descarga automáticamente por las restricciones del servidor de M-VAVE. |
| LCD aislada (`Solo LCD`) | **Implementado en la web** | Muestra el framebuffer del firmware emulado, sin el panel completo del instrumento. |
| Vista horizontal para teléfono | **Prototipo** | Lienzo 16:9; redistribución conservadora en dos zonas **solo si** existe una separación vacía segura. En otros casos se muestra la LCD cuadrada entera, sin distorsión. No es una remaquetación de todos los menús. |
| Síntesis y audio ejecutados en el móvil | **Implementado mediante WebAssembly + AudioWorklet** | El motor corre en el navegador del móvil; depende de capacidad del dispositivo, compatibilidad y latencia. |
| MIDI de entrada desde navegador | **Implementado con Web MIDI cuando está disponible** | La web acepta dispositivos que el sistema/navegador expone como entradas MIDI y permite elegir canal. |
| Teclado/controlador MIDI por USB-C | **Dependiente del sistema y navegador** | Puede funcionar si Android, el adaptador/OTG y el navegador reconocen el dispositivo MIDI. No está verificado para todos los móviles/controladores. |
| MIDI Bluetooth | **Objetivo de integración y pruebas** | No hay emparejamiento BLE-MIDI propio ni puente dedicado en la web. Solo podría aparecer ya como entrada si el sistema/navegador lo expone a Web MIDI. |
| Sonido en altavoz Bluetooth o interfaz/DAC USB-C | **Depende de la ruta de audio del sistema** | La web reproduce por la salida predeterminada del navegador/Android. La selección del altavoz se hace en el sistema; la web no incluye actualmente un selector de salidas ni control de latencia Bluetooth. |
| LCD del FM-1 físico como pantalla secundaria | **Disponible al ejecutar CTL-1 en el hardware; integración conjunta pendiente** | La LCD muestra exclusivamente la interfaz de CTL-1, independiente de la LCD emulada en Android. No se requiere captura de pantalla. |
| FM-1 físico como controlador MIDI USB/Bluetooth | **CTL-1 1.0 lo ofrece; pendiente validar con Android** | Instalar CTL-1 una vez en el hardware, conectar por USB o BLE MIDI y mapear notas/CC al emulador web. |
| Trasladar el trabajo de síntesis al teléfono con CTL-1 en el FM-1 | **Arquitectura prevista; integración pendiente** | El FM-1 procesa los controles y el teléfono ejecuta el sintetizador. El firmware virtual cambia sin reflashear el físico. |

**Importante:** «cualquier firmware» expresa el objetivo de compatibilidad; cada paquete de síntesis debe ser compatible con el emulador y algunos periféricos aún pueden no estar implementados. El FM-1 físico **no necesita cambiar su firmware controlador** para cambiar de sintetizador en Android, pero **sí debe tener instalado un firmware MIDI adecuado, como CTL-1**, si se quiere utilizar su Bluetooth y USB MIDI específicos. No se modifican los firmwares de terceros que se ejecutan virtualmente; una remaquetación de sus menús requiere perfiles validados o su código fuente.

## Cómo probar lo que ya funciona

1. Abre la [web de este proyecto](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?screen=1&fit=1) en un navegador compatible. La pantalla **Solo LCD** es el modo inicial.
2. Pulsa **Start** para arrancar el firmware elegido. Si necesitas escoger otro firmware, pulsa **Mostrar panel** para acceder a los ajustes y al catálogo, elígelo y vuelve a **Solo LCD**. Cambiar la vista no debería reiniciar el audio.
3. Utiliza **Original** para conservar íntegra la geometría de la LCD o **Horizontal** para activar la distribución panorámica cuando sea segura. Los textos de los botones indican la acción que efectuarán al pulsarlos.
4. Para un controlador MIDI, conéctalo al teléfono y comprueba si aparece como entrada en **MIDI in** dentro de los ajustes. El funcionamiento depende de Web MIDI y del soporte MIDI del sistema operativo.
5. Para salida de sonido externa, conecta o empareja tu altavoz Bluetooth, interfaz USB o DAC USB-C **desde los ajustes de audio de Android** y comprueba que el navegador utilice esa salida. Un dispositivo Bluetooth puede añadir latencia apreciable; USB-C suele ser preferible para tocar en tiempo real.
6. Para el firmware oficial, descarga tu propia copia de `FM-1.fwsc` de [M-VAVE](https://www.m-vave.com/download) y selecciónala mediante **Open .fwsc…**. **No utilices el actualizador del sintetizador**: este flujo solo lee el paquete en el navegador, sin enviarlo al aparato.

El navegador guarda los ajustes y cambios de flash emulada por firmware; se recomienda **Export…** para conservar una copia portátil `.fm1save`. Este guardado tampoco escribe en el hardware real.

## Arquitectura y próximos pasos

El proyecto parte de la emulación de la CPU JieLi pi32v2 y los periféricos del FM-1. El código Rust se compila a **WebAssembly** (`crates/fm1-web/`). `web/worklet.js` ejecuta la máquina en el hilo de audio; `web/main.js` coordina los firmwares, los mensajes MIDI y la vista; `web/mobile-lcd.js` convierte los píxeles de la LCD y compone la presentación móvil. Los firmwares comunitarios se obtienen según `web/firmwares.json`; los archivos de M-VAVE se importan localmente. [Detalles de la adaptación de pantalla](docs/screen-adaptation.md).

Las siguientes fases de desarrollo son: (1) **probar CTL-1 1.0** en el FM-1 físico como controlador (su instalación inicial es opcional y exige flashear una vez); (2) comprobar el **USB-MIDI hacia Android** mediante un adaptador USB-C/OTG; (3) validar la ruta **Bluetooth MIDI** de CTL-1 y cómo exponerla a Web MIDI en Android; (4) mapear los mensajes MIDI, bancos, notas y CC de CTL-1 al sintetizador emulado en la web; (5) evaluar la latencia del audio Bluetooth y USB-C; y (6) crear perfiles de interfaz horizontal específicos por firmware, sin perder menús ni parámetros. **No se desarrollará un sistema de captura/duplicación de la pantalla física:** esta permanece a cargo de CTL-1.

### English project summary

**FM-1 Mobile Companion** aims to run compatible M-VAVE FM-1 synthesizer firmware packages on an Android phone **without repeatedly flashing the physical device when changing synths**. In standalone mode, the phone runs the emulated firmware, enlarged LCD and audio engine. In combined mode, the physical FM-1 **runs the CTL-1 MIDI controller firmware**, showing CTL-1's control interface on **its own secondary display**, while the phone shows the **virtual synthesizer LCD as the primary display** and produces the sound. CTL-1 1.0 supports USB and Bluetooth MIDI, but the end-to-end Android browser link is **not yet validated**. **No physical LCD capture or mirroring is intended**. Installing CTL-1 on the real FM-1 requires an initial optional firmware flash; switching virtual synths thereafter does not.

## Proyecto de base y atribución

Este repositorio es una **adaptación del emulador abierto** [joakimeriksson/fm1-emu](https://github.com/joakimeriksson/fm1-emu). Se mantienen a continuación las descripciones, el catálogo autogenerado, las instrucciones de compilación y los créditos técnicos de la base original. Algunas referencias históricas del texto apuntan a la web del proyecto original; **para probar las modificaciones móviles usa siempre el [sitio de este fork](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/)**. Cada firmware pertenece a sus propios autores y conserva su licencia.

---

### FM-1 emulator (documentación técnica de la base original)

The **M-VAVE FM-1** is a pocket FM synthesizer, and a small community writes its own firmwares
for it: drum machines, grooveboxes, ambient generators, a chord harp. This project emulates the
FM-1 itself, its CPU, screen, keys, knobs and audio, so every one of those firmwares runs
unchanged in a browser, on a computer or as an instrument plugin in a DAW (AU, VST3). You can
try a firmware before you install it, write one without a synth on the desk, or just play.

**Play it: [joakimeriksson.github.io/fm1-emu](https://joakimeriksson.github.io/fm1-emu/)**:
pick a firmware, press Start, play with the computer keyboard, the mouse, a touch screen or
a MIDI keyboard.

[![The emulated FM-1's front panel running fifteen of the firmwares in turn](docs/media/firmwares.gif)](https://joakimeriksson.github.io/fm1-emu/)

With sound: nine of the firmwares in 50 seconds.

https://github.com/user-attachments/assets/6849316e-7239-497e-b617-be74317dc2b1

The browser keeps the firmware you run and automatically saves flash changes, including settings
and patches saved through the firmware's own controls. Each exact firmware version has its own
save. **Save & backup** offers a manual save, portable `.fm1save` backup export/import, reset,
and a check for firmware updates. This also works with locally opened packages and on the
SMK-37 page. Edits held only in firmware RAM need its normal save command first; browser saves
survive reloads but clearing browser data removes them. Export a backup to keep a separate copy.

The page hosts no firmware: it downloads each one from its author's own site when you start
it, the same update package you would install on a real FM-1. M-VAVE's own firmware can't be
fetched that way, so download `FM-1.fwsc` from M-VAVE and open it on the page.

And the **[M-VAVE SMK-37](https://joakimeriksson.github.io/fm1-emu/smk37/)**: a 37-key keyboard with
the FM-1's chip inside, whose community ports run here too ([below](#the-smk-37)).

Also here: **[FM-1 SID](https://github.com/joakimeriksson/fm1-sid)** (`fm1-sid/`, a submodule: clone with
`--recurse-submodules`), a firmware that turns the FM-1 into a Commodore 64 SID
synth, with a step sequencer and stereo chorus, delay and reverb, that also plays C64 tunes
and GoatTracker songs ([its page](https://joakimeriksson.github.io/fm1-emu/fm1-sid/) plays all of HVSC).
On a real FM-1 it is its own firmware on [fm1-dock](https://github.com/joakimeriksson/fm1-dock), which
keeps USB, the installers and the rescue.

## What runs

81 community firmwares: every one on [the FM-1 Firmware Hub](https://thefirmwarehub.com/) that
runs here (its packages, fetched from the Hub; `tools/hub_catalog.mjs` follows its catalog), and a
few more from [fm1-editor.com/firmware](https://fm1-editor.com/firmware/). Also FM-1 SID, and
M-VAVE's stock firmware (V13 and V15) with both CPU cores running. The ones the Hub doesn't list
are our own list, `web/firmwares-own.json`: add a firmware there, and the tool appends it. Each
name below opens the page with that firmware picked; press Start. (`?fw=` takes the Hub's ids
too; `&embed=1` shows only the device and its controls, for a window on another site.)

<!-- firmwares: node tools/readme_firmwares.mjs -->
| Firmware (opens it on the page) | By | What it is |
|---|---|---|
| [M-VAVE stock firmware](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=mvave) | [M-VAVE](https://github.com/joakimeriksson/fm1-emu/blob/master/docs/stock-firmware.md) | The firmware the FM-1 ships with: a DX7-style FM synth, both cores running. Bring your own `FM-1.fwsc`. |
| [Felucca 1.6.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=felucca) | [hugelton](https://github.com/hugelton/Felucca) | The original open firmware: a four-operator FM synth with sequencer, arpeggiator and effects. |
| [Felucca [Salt] 0.9-salt14](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=felucca-salt) | [ChanceTheMaker](https://github.com/ChanceTheMaker/Felucca) | A Felucca fork with its own sound and editor changes. |
| [386DX 1.0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=386dx) | [seajaysec](https://github.com/seajaysec/386-DX-FM1-KaraokeBot) | A karaoke robot with a band: it sings songs from karaoke and MIDI files, or lines you type, to the notes you play. |
| [AMB-1 1.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=amb) | [charlesvestal](https://github.com/charlesvestal/fm1-amb) | A generative ambient machine inspired by Music for Airports. |
| [April OS BETA 1.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=april) | [beashoko](https://github.com/beashoko/april-os) | A sampler inspired by the Casio SK-1: no synth engines, only samples, recorded through the installer page's microphone bridge. |
| [BEETLE 1.4.25](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=beetle) | [Joppe Sikken (SikkenWorks)](https://github.com/SikkenWorks/BEETLE) | A tracker for the FM-1: patterns written top to bottom, step by step, with note, instrument, chord and effect columns, the way trackers have always worked. |
| [Bunker Evolved 1.0.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=bunker) | [Kid Bunka](https://github.com/kidbee85-beep/bunker-evolved) | Generative ambient: four editable layers from 64 sound recipes, 24 worlds, 24 motifs and 32 scales. |
| [Bytemania 1.2.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=bytemania) | [lloyd (llozd)](https://github.com/llozd/bytemania) | A bytebeat synth: play one-line formulas, tweak them with the knobs, and write your own, with a random button in the formula creator that is really fun. |
| [Carnac OS 0.9.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=carnac) | [OrbVorgis](https://github.com/OwenKirby/carnac-os) | A dual-timbral wavetable synth after Carnac's standing stones: 15 morphing wavetable banks, seven voice architectures, a MAQ 16/3-style sequencer for modulated sequences and a chord sequencer for progressions, spring, tomb and tank reverbs. |
| [Chipbox 1.2.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=chipbox) | [Pasquale Barilla](https://github.com/pbarilla/chipbox) | A chiptune groovebox with replicas of the NES, Game Boy and Sega Master System sound chips, in three views: RACK for the 16 steps of a page, CHIP to set up each channel, SONG to chain the pages into a song. |
| [ChoralRoot 0.26.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=choralroot) | [Quixotic7](https://github.com/Quixotic7/ChoralRootFM1) | A port of the Choral Root synth. |
| [fm1-chord 0.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=chord) | [math0ne](https://github.com/math0ne/fm1-chord) | A chord machine after the HiChord, with extras: chords in a key under the white keys, slash chords and inversions, sounds, envelopes, effects and a looper. |
| [Clipper 1.1-experimental](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=clipper) | [isod89](https://github.com/isod89/clipper-fm1) | A deep synth grown out of SLOOP: 23 engines and 407 sounds, from HYBRID (three oscillators, two filters, a mod matrix) to six-operator FM, wavetables, wave sequencing, a Juno and physical models; arp, step sequencer, MPE and a web editor for every parameter. |
| [CoralFlower 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=coralflower) | [FicusFicusov](https://github.com/FicusFicusov/CoralFlower) | Chords after the Orchid, with a UX made for the FM-1: one key turns every note into a chord with a bass, 27 scales, an arpeggiator, a looper, a plate reverb, and a pixel flower that grows as you play. |
| [CS-1 2026.10.22](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=cs1) | [twochannels](https://github.com/twochannels/CS-1) | A Yamaha CS-80 in spirit: eight voices of two channels, high- and low-pass filters in series, ring modulator, the touch response and 22 tones from the owner's manual. |
| [CTL-1 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=ctl) | [charlesvestal](https://github.com/charlesvestal/fm1-ctl) | A MIDI controller: the keys play in a scale with its chords on the black keys, four banks of knobs, an arpeggiator and Euclidean drums, with General MIDI sounds of its own. |
| [Cut-A-Dor 1.2.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=cutador) | [Kid Bunka](https://github.com/kidbee85-beep/cut-a-dor-release) | A beat dojo for scratch DJs and beatmakers: boom bap, lo-fi and breaks from 16 rhythm families with swing and fills, an eight-lane 32-step sequencer, punch-in effects you record bar by bar, a bass voice. |
| [Del Rio 0.7](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=delrio) | [Jess Del Rio](https://github.com/jsopdebeeck/fm1-delrio) | A rhythm box of ceremony percussion: djembe, cajon, udu, clapsticks, maracas and triangle, their real strokes on the black keys, a step sequencer on the white keys and 31 rhythms from the literature. |
| [Dinghy 0.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=dinghy) | [zvenson](https://github.com/zvenson/dinghy) | Your base for your own FM-1 firmware: everything under it done and documented (updates, USB rescue, USB-MIDI, USB audio, keys, screen, settings), a 4-voice sine on top. |
| [DRIFT 0.8](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=drift) | [FBNFRTG](https://github.com/FBNFRTG/drift-fm1) | An ambient instrument for live sessions: warm synths, spacious pads and delicate plucks over a generative Euclidean sequencer on one track, with delays and reverbs from spring to shimmer. |
| [DUB-1 1.3.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=dub) | [sbegueria](https://github.com/sbegueria/fm1-dub) | A generative dub techno machine that plays itself from the moment it is on: chord stabs into a tape echo, FM pings, a sub bass on five grooves, a ducking kick, crackle, and an eight-bar arrangement with breaks and fills. |
| [DUELL 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=duell) | [OSCILL/R](https://github.com/oscillr/DUELL) | A multimorphing bitimbral synthesizer: two complete parts, three ways to play them together, and a morph that glides between two versions of every sound. |
| [Evander's Music Box 1.5.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=emb) | [Michail Tsakiroglou](https://github.com/MikeTsak/Evanders-Music-Box--FM-1-LARP-firmware) | A bard's instrument for LARP, medieval and high fantasy: recorder, hurdy-gurdy, harp and a tavern kit on four tracks, encounters and songs for the table. |
| [ETCH 1.61](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=etch) | [SikkenWorks](https://github.com/SikkenWorks/ETCH) | Draw the sound: a waveform drawn and changed with the knobs, morphed into another one, and a third modulating the result. |
| [Fart Machine One 1.1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fart) | [adamcharleswilsonmusic-code](https://github.com/adamcharleswilsonmusic-code/Fart-Machine-One) | A questionable musical instrument: eleven artisanal farts, reverb, delay and a sixteen-step sequencer. |
| [Felucca 8 Patterns 1.6.1_and_sequences](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=felucca8) | [echoe](https://github.com/echoe/Felucca_8_patterns) | Felucca 1.5.1 with eight sequences a track to switch between and chain one after another; the user samples are left out to make room. |
| [FiMba-1 0.6.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fimba) | [jadamsowers](https://github.com/jadamsowers/fm1-fimba) | A physically modelled kalimba (thumb piano), with mbira patterns, tape and grains. |
| [Flyfish 1.0.1-4e5efb8](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=flyfish) | [delirioux](https://github.com/delirioux/flyfish) | One piece on the FM-1: Chopin's Étude Op. |
| [FM-106 Beta4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fm106) | [PatttF](https://github.com/PatttF/fm-106) | The Juno-106 emulation of KR-106 ported to the FM-1: six voices on both cores, the BBD stereo chorus, 128 factory presets and a 1980s Roland look. |
| [fm1-dock 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fm1dock) | [joakimeriksson](https://github.com/joakimeriksson/fm1-dock) | One firmware, many synths: CS-1, Scintilla and OberVave as modules in flash, the sine built in; HOME opens the picker. |
| [FM-1 Doom 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fm1doom) | [zvenson](https://github.com/zvenson/fm1doom) | Doom on the FM-1: one arena from FreeDM, played on the keys and a knob, on the FM-1's screen, with synthesised sound effects and no music. |
| [FM1-GM 1.0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fm1gm) | [jartificer](https://github.com/jartificer/fm1-gm) | A General MIDI sequencer with 16 tracks, 128 General MIDI instruments to choose from and a drum kit (on track 10). |
| [FMEL-1 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fmel) | [Antonio Altea](https://github.com/AntonioAltea/fmel-1) | Mellotron-style tape sounds, coloured with effects and lo-fi tape: flutes, strings and choirs as on the old tape frames, played from the keys. |
| [FMini-1 1.8.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fmini) | [Ruppert](https://github.com/ruppert/fmini-1) | A three-oscillator monosynth in the spirit of the classic Moogs: a 24 dB/octave ladder filter, two contours, 100 factory patches, an arpeggiator, a sequencer and MIDI. |
| [FOMAJAM 0.1.6](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fomajam) | [konsumer](https://github.com/konsumer/fomajam) | A drum machine in the classic step-grid style: voices after the 909, 808, 707, 606, DMX and more, a machine per voice, accents and patterns. |
| [FoMni 0.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fomni) | [charlesvestal](https://github.com/charlesvestal/fm1-fomni) | A chord harp inspired by the Omnichord: hold a chord, strum the keys. |
| [FRASIL 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=frasil) | [Styves Goupil](https://github.com/styvesgoupil/frasil) | An ambient drone and texture instrument: tap a key to latch a drone, a note or a chord in your key, press to swell it. |
| [Frodhes 0.9.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=frodhes) | [Leonardo Mirabal](https://github.com/leonardoMirabal/frodhes) | A Rhodes electric piano, physically modelled, no samples: four piano types, compressor, drive, tremolo or wah, phaser, chorus, delay and reverb, tap tempo, MIDI clock, scales and presets. |
| [FuMi-1 1.1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=fumi) | [cartesive](https://github.com/cartesive/fumi-1) | Inspired by the Suiko ST-50 shigin conductor: koto and shakuhachi-style tones. |
| [Pocket GammaWave 1.0-GW](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=gammawave) | [ShadowTune (GammaWave Audio)](https://github.com/fvalente/GammaWave-FM1) | The companion to the GammaWave desktop synth: four industrial distortions, vowel and scream filters, an FFT spectrogram, a four-track tape deck screen and a song generator with 49 genres, trap and drill too. |
| [GHOULBOX 2.3.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=ghoulbox) | [jasonpersinger](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth) | Dungeon synth (a Felucca fork): cathedral reverb and dark drones. |
| [Golana 1.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=golana) | [Gadlele](https://github.com/Gadlele/Golana) | One analog-style synth laid out like the classic microKORG, with a screen that draws the sound. |
| [Golana DZ Edition 0.1.0-beta](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=golanadz) | [dzwerg](https://github.com/dzwerg/Golana-DZ) | Golana's synth with a sequencer for whole songs: 64 steps a pattern, 32 programs (up to 2,048 steps), a SONGMODE that plays from a first to a last program once or on repeat, and USB MIDI out to sequence other synths. |
| [GR!MM 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=grimm) | [GRiMMBIENT](https://github.com/GRiMMBIENT/grimm) | Dark ambient, dungeon synth and deathdream: four tracks, seven sound sources from VA and 6-op FM to formant voices and physical models, tape, granular freeze and reverbs up to infinite, a 64-step sequencer and song mode. |
| [HERA 1.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=hera) | [Ben Badio](https://github.com/benbadio/hera-fm1) | A chord groovebox after the Roland J-6: 100 chord sets, 9 phrase styles with 12 variations each and a 64-pattern step sequencer, on a four-voice Juno-style synth from Clipper with chorus, delay and reverb. |
| [Hortator 0.23.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=hortator) | [DeadActive](https://github.com/DeadActive/hortator) | A drum-focused firmware based on Felucca. |
| [HYP-1 1.5.54](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=hyp1) | [RedtailWorks](https://github.com/RustyPawz/fm1-HYP1) | Hypnotic techno and acid: a breathing pad, a deep drone, a generative 303 line and grain over drums you sequence. |
| [Jangada 1.0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=jangada) | [zednaked](https://github.com/zednaked/jangada) | Superwave analog voices and a modulation matrix. |
| [JIANT FM 0.9.9-beta4a-813869d](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=jiant) | [juanjiant-bit](https://github.com/juanjiant-bit/JIANT) | A bio-synthetic groovebox: six tracks drawn as living beings, eight engines (FM6 with Dexed, analog, phase, voice), mutating macros and SLOOP's song mode. |
| [KAWA 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=kawa) | [Omnipotard (Guizmoh)](https://github.com/Guizmoh/Kawa-Tracker-Fm1) | A pocket tracker for any style: six tracks with an engine each (pulse, 4-bit wave, noise, 6-operator FM, drum kit or sample), patterns chained into a song, 32 live effects and tools that write beats for you. |
| [Ketch 0.1-beta](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=ketch) | [risseraka](https://github.com/risseraka/fm1-tracker-ketch) | A tracker in the LSDJ / picoTracker / M8 line: song, chain and phrase screens, instruments, effects and a mixer, on the FM-1's own keys and screen. |
| [LOOM-8 0.7](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=loom) | [Charles Vestal](https://github.com/charlesvestal/fm1-loom) | A groovebox inspired by the OP-XY: eight tracks on nine engines, step tricks and locks, weave, a conductor and throws. |
| [MAKU 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=maku) | [okumurari](https://github.com/okumurari2/maku) | An ambience machine for DJ interludes: switch it on and a random world in key plays, four parts, buttons that are verbs, and the worlds you like breed new ones. |
| [MC-1 1.2.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=mc1) | [dspaudio](https://github.com/dspaudio/mc-1) | An original four-part groovebox with a 1990s workflow: three melodic parts and a 16-lane drum part, 128 melodic presets, eight drum kits, song arrangement, an arpeggiator and recorded knob moves. |
| [Melodee 1.4.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=melodee) | [keremimo](https://github.com/keremimo/melodee) | Total playing pleasure, far beyond Felucca: recording without quantize, microtonal scales, a Casio CZ-1, a bit-by-bit Dexed, scale modes made for an MPC Sample as its companion. |
| [MIN-1 0.2.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=min1) | [Reytian](https://github.com/Reytian/fm1-min1) | A tiny minimalist ensemble after Philip Glass: it starts playing when you switch on, you join whenever you like. |
| [Mini-Tone 1.0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=minitone) | [Stéphane Schmitz](https://github.com/eldontyrellcorp/Mini-Tone) | Felucca with six tracks and new engines: SID, Braids' 48 models, a TB-303 and engines after Elektron's Analog Four, SY Chip and Digitone 2, a ping-pong delay and a new reverb. |
| [MODELAR 0.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=modelar) | [Hirakos](https://github.com/hirakos999/MODELAR) | A semi-modular drone synth played like a Kaoss Pad: four oscillators modulating each other through eight routings, and keys that move a cursor on an XY pad instead of playing notes: white keys are points, black keys gestures. |
| [MONK-1 0.9.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=monk) | [Charles Vestal](https://github.com/charlesvestal/fm1-monk) | Twelve singers, from Jerry the monk to Snider the punk, each in its own voice: MonkSynth's formant voices, played with velocity and pressure from the FM-1's own keys. |
| [MORPHEUS 1.0.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=morpheus) | [Ben Badio](https://github.com/benbadio/morpheus-fm1) | A pattern sequencer after Reason's Matrix: up to four channels of MIDI sequence data out, and a few voices built in to build the patterns with. |
| [Mutable Dreams 1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=mutable) | [Kid Bunka](https://github.com/kidbee85-beep/mutable-dreams) | A generative instrument after a small modular setup: six interchangeable tracks on eight engines (selected Plaits algorithms, Rings' modal resonator, a Choir of its own), 64 Biomes, Euclidean rhythms with probability, four macros and a granular bus. |
| [MYCELIUM 1.0-test](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=mycelium) | [Mandalars](https://github.com/megagott/mycelium-fm1) | Forest, Psytrance, Goa and Hitech grooves, built on Felucca and SLOOP: original drums and icons and four factory songs. |
| [Niagara Drift 0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=niagara) | [evanyerburgh](https://github.com/evanyerburgh/niagara-drift) | A drone synth: two drifting virtual-analog drones, each with its own part, swells, LFOs, effects and a sequencer. |
| [No Man Sky FM 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=nmsfm) | [Hericdk](https://github.com/hericdk/nomanskyFM) | A space adventure while you make music, built on SLOOP 2.7.1: fly between planets with two knobs, land, mine, fight sentinels and warp between stars, every SLOOP screen floating over the game. |
| [NOIZ DEMONS 666](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=noizdemons) | [SPIKE the Percussionist](https://github.com/SPIKE-the-Percussionist/noiz_demons) | Felucca with randomization as a ritual: every new sound summons one of 30 demons of the Ars Goetia, its sigil takes the screen and its rank decides how wild the sound gets. |
| [OberVave 3.0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=obervave) | [zvenson](https://github.com/zvenson/obervave) | An 8-voice analog-style poly in the spirit of the classic American polys of 1979-84: two oscillators a voice, sync, cross-mod, a 12 / 24 dB filter with a SEM-style morph, an arpeggiator with hold, chorus, ping-pong delay and a plate reverb. |
| [OPN-1 2.0.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=opn1) | [putchdev](https://github.com/putchdev/opn-1) | The Sega Genesis / Mega Drive's Yamaha YM2612: six channels of 4-operator FM. |
| [ORBIT 0.7.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=orbit) | [dspaudio](https://github.com/dspaudio/orbit) | A four-track groovebox based on SLOOP: three synth parts and a drum machine, an event tape to rearrange them, OP-1-inspired screens. |
| [PIGEON 0.5.0-beta.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=pigeon) | [The Shrimp Blimp](https://github.com/TheShrimpBlimp/pigeon) | An old-school hip-hop and house sampler after the SP-404 and the PO-33: two song samples and a 16-pad kit, up to 32 chops a song, a 64-step sequencer, patterns and a song arrangement. |
| [Polly 0.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=polly) | [charlesvestal](https://github.com/charlesvestal/fm1-polly) | A pocket arranger after the Yamaha QY70: pick a style, play a chord and a band of eight follows, with General MIDI sounds. |
| [PurpleMonkey 0.11](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=purplemonkey) | [Quixotic7](https://github.com/Quixotic7/PurpleMonkeyFM1) | A firmware for kids. |
| [FM1 Quest 38](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=quest) | [hericdk](https://github.com/hericdk/fm1-quest) | A live RPG adventure inside the synth. |
| [Rainbow Mode 1.1.5-rb](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=rainbow) | [joshbartlettband-web](https://github.com/joshbartlettband-web/felucca-rainbow) | A music toy for little hands (a Felucca fork). |
| [Shine On 3.0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=shineon) | [zvenson](https://github.com/zvenson/shineon) | A string ensemble in the spirit of the late-70s machines, grown out of OberVave: divide-down keyers with 16', 8' and 4' footages, register filters, a three-tap BBD ensemble and a phaser, on two tracks with the sequencer. |
| [FM-1 SID 0.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sid) | [joakimeriksson](https://github.com/joakimeriksson/fm1-sid) | A SID chip on the FM-1, its own firmware on fm1-dock (fm1-dock's USB, installer and rescue; the sine as the fallback): a C64 tune player (reSID 0.16 and a 6510 running the tune's own code, PSID and RSID) starting with GoatTracker 2's example songs, Dojo first; a GoatTracker editor; and a synth: the keys play 12 instruments, KNOB1-4 are cutoff, resonance, attack, release; FX opens chorus, delay and reverb. |
| [SID-1 1.0.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sid1) | [putchdev](https://github.com/putchdev/sid-1) | The Commodore 64's SID chip, twice for six voices. |
| [SIDOU 0.3.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sidou) | [Denis Sablić](https://github.com/dsablic/sidou) | Turns the FM-1 into a C64 SID machine: two SIDs (6581 and 8580) with the chip's character dialled in, a GoatTracker 2 compatible tracker and step sequencer, and a live instrument to play over a song. |
| [Skiff 1.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=skiff) | [illuminus5](https://github.com/illuminus5/skiff) | A six-track groovebox built on Felucca 1.1.5: fourteen synth and drum engines, a 64-step sequencer with chords, ratchets and locks, songs and a performance layer. |
| [SLOOP 2.8.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sloop) | [isod89](https://github.com/isod89/sloop-fm1) | A live groovebox: four looping tracks. |
| [SLOOP ALG ALG05-TEST](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sloop-alg) | [shaw-core](https://github.com/shaw-core/Sloop_ALG02) | A SLOOP test build with extra FM algorithms. |
| [SLOOP DX7 Banks 2.5-b28-test](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sloop-banks) | [majnikool](https://github.com/majnikool/sloop-fm1) | SLOOP with up to twelve DX7 voice banks and presets grouped by kind. |
| [SLOOP Floyd FM 2.4](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sloop-floyd) | [renebohne](https://github.com/renebohne/sloop-fm1) | SLOOP with a four-operator FM engine after Floyd Steinberg's 4-OP idea: an envelope per operator and a live oscilloscope that draws the envelopes as you play. |
| [sloopDX 4.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=sloopdx) | [zvenson](https://github.com/zvenson/dxsloop) | SLOOP's live workflow with a six-operator DX7 engine from Dexed: 32 algorithms, your own .syx banks, every voice edited on the FM-1, and FM drums. |
| [Tessera 2.0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=tessera) | [Theo Dainis (Rastapopolix)](https://github.com/Rastapopolix/tessera) | Generative music on four tracks: each plays its own steps, an arpeggio or one of 13 generators (Turing, Euclid, Markov, Constellation, Chord and more), and the tracks evolve, follow and modulate each other. |
| [Tonitor 2026.10.23](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=tonitor) | [Nuno Zimas](https://github.com/nzimas/tonitor) | Six tracks for live ambient and experimental music: three synth engines, patches generated by algorithm, free-length loops with up to ten overdubs each, a granulator, filters and a lush reverb. |
| [TRIBE 0.3-beta](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=tribe) | [apiel](https://github.com/apiel/fm1-tribe) | A ten-track groovebox built on X0X where any engine plays on any track: kicks, 909 and 808 voices, a 303, sixteen clips a track and scatter effects. |
| [Tuna 1.0-fbc56d3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=tuna) | [delirioux](https://github.com/delirioux/tuna) | Generative music built on Felucca: composer profiles write the music as it plays, you steer them. |
| [Unholy Union 0.2.3](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=unholy) | [actuallysparky](https://github.com/actuallysparky/FM-1-Unholy-Union-generative-drone-firmware) | Generative ambient and drone: Immaculate Permutation rolls an arrangement in one key, a pad, a sequence and a chord arpeggio under a fourth voice for your hands. |
| [Vector37 0.3.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=vector37) | [genkigenki](https://github.com/genkigenki/Vector37) | An orbiting overtone synth: four corners of two overtone generators each, a sun on a path and a moon around it decide how much of each you hear; the knobs shape the selected planet. |
| [WaveLoop FM-1 1.0.5.2-midi23](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=waveloop) | [ELI7VH](https://github.com/ELI7VH/Felucca) | A Felucca mod: standalone Arturia MiniLab 3 control, fine track mixing, a DJ filter, momentary effects and MIDI feedback. |
| [winampFM Synth 0.12](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=winampfm) | [hericdk](https://github.com/hericdk/winampFM) | Winamp 2.x on the FM-1, built on SLOOP: your sounds as a playlist, four tracks recorded as loops, effects on sliders and a visualizer of lines. |
| [X0X 1.1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=x0x) | [charlesvestal](https://github.com/charlesvestal/fm1-x0x) | A groovebox: 909, 808, two 303s and a break player. |
| [X0X-RMX 1.1.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=x0xrmx) | [Mr-Roboto (Oliver)](https://github.com/Mr-Roboto/fm1-x0x-rmx) | X0X remixed after The Orb: banks and patterns reordered live per voice, Grids MAP and EUCLID drum generation, an extended TB-3PO with 22 scales and a morphing 303 waveform. |
| [ZEN-1 1.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=zen) | [aceyprime](https://github.com/aceyprime/zen-1) | Ambient as a living window: cherry petals on a river, koi in a pond, fireflies under a real moon, and every movement a note. |
| [zp12 2.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/?fw=zp12) | [zvenson](https://github.com/zvenson/zp12) | A 12-bit sampling drum machine after the E-mu SP-1200: 32 pads at 26 kHz with no interpolation, filtered channels on the knobs, loops and songs. |
<!-- /firmwares -->

MIDI in works through the emulated MIDI IN jack, so each firmware reads MIDI with its own
driver: Web MIDI on the page (Chrome, Edge, Firefox), any MIDI input natively. M-VAVE's
firmware listens on channel 2 in the emulator (not yet checked on a real FM-1); Felucca plays
its parts on channels 1–4.

## The SMK-37

M-VAVE's SMK-37 (Pro / Elite, the MKE-P37 and the Donner Starrykey 37 Play share its hardware) is a
37-key keyboard with 16 RGB pads, 8 knobs, 4 faders, pitch and mod wheels and a screen, around the
FM-1's chip: same flash layout, audio and update path, only the panel is wired differently.
genkigenki's [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37) kit maps that wiring and
ports FM-1 firmwares to it. The emulator models the SMK-37's panel from that map, and a package's
identity says which board it runs on.

**Play it: [joakimeriksson.github.io/fm1-emu/smk37](https://joakimeriksson.github.io/fm1-emu/smk37/)**.
The firmwares come from [genkigenki's install page](https://genkigenki.github.io/smk37/)
(`node tools/smk37_catalog.mjs` refreshes the list). M-VAVE's own SMK-37 firmware runs as the
MIDI controller it is (screen, buttons, pads, keys; it names the chord you hold), without sound (its
notes go to USB / Bluetooth MIDI) and slower than real time in the browser so far.

<!-- smk37-firmwares: node tools/readme_firmwares.mjs -->
| Firmware (opens it on the page) | By | What it is |
|---|---|---|
| [M-VAVE stock firmware 016](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=stock) | [M-VAVE](https://github.com/genkigenki/fm1-to-smk37) | The firmware the SMK-37 Pro ships with: a MIDI controller. Bring your own `SMK-37 Pro_016.fwsc`. |
| [Makkuro37 0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=makkuro37) | [genkigenki](https://github.com/genkigenki/Makkuro37) | A microtonal drone synth: eight voices as eight soot creatures, 96 presets. |
| [Vector37 0.2.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=vector37) | [genkigenki](https://github.com/genkigenki/Vector37) | An orbiting overtone synth: a moon circles a sun along a path, and where it is decides how much of each of four sounds you hear. |
| [Snoop37 2.6.5](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=snoop37) | [isod89 (port: genkigenki)](https://github.com/genkigenki/sloop-fm1) | SLOOP, the four-track looper groovebox, adapted for the SMK-37. |
| [X0X 0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=x0x37) | [Charles Vestal (port: genkigenki)](https://github.com/genkigenki/fm1-x0x) | A 909, an 808, two 303s with a line generator and a breakbeat player, all at once. |
| [FoMni 0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=fomni37) | [Charles Vestal (port: genkigenki)](https://github.com/genkigenki/fm1-fomni) | An Omnichord-style chord harp: the 16 pads become the strum plate, the keybed holds the chords. |
| [MAKU 0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=maku37) | [okumurari2 (port: genkigenki)](https://github.com/genkigenki/MAKU37) | An always-on ambience machine: power it on and a new, random, in-key world is already playing. |
| [Rainbow mode 0.2](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=rainbow37) | [joshbartlettband-web (port: genkigenki)](https://github.com/genkigenki/RAINBOW37) | A music toy for kids: colour-coded notes, 40 friends who sing along, no menus. |
| [GHOULBOX 2.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=ghoulbox) | [Jason Persinger (port: genkigenki)](https://github.com/genkigenki/Felucca/tree/ghoulbox-smk37) | Dungeon-synth firmware: dark ambient, medieval and fantasy sounds, with drones, a hurdy-gurdy crank and two sounds at once. |
| [ChoralRoot 0.21](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=choralroot) | [Quixotic7 (port: genkigenki)](https://github.com/genkigenki/ChoralRootFM1/tree/smk37) | A chord instrument in the manner of the Orchid: one hand plays roots, the other picks the chord. |
| [ORBIT 0.7.0](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=orbit) | [dspaudio (port: genkigenki)](https://github.com/genkigenki/orbit/tree/smk37) | A four-track groovebox with OP-1-inspired screens: three synth parts, a drum machine, an event tape and a mixer. |
| [AMB-1 1.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=amb) | [Charles Vestal (port: genkigenki)](https://github.com/genkigenki/fm1-amb/tree/smk37) | A generative ambient machine inspired by Music for Airports: loops of different lengths drift past each other and never repeat. |
| [Polly 0.4.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=polly) | [Charles Vestal (port: genkigenki)](https://github.com/genkigenki/fm1-polly/tree/smk37) | A pocket arranger inspired by the Yamaha QY70: a band of eight General MIDI instruments follows the chords you play. |
| [winampFM 0.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=winamp) | [Hericdk (port: genkigenki)](https://github.com/genkigenki/winampFM/tree/smk37) | A synth groovebox in the look of Winamp 2.x, built on SLOOP: player, playlist, effect sliders and a visualizer. |
| [OberVave 1.2.1](https://koolkatze.github.io/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu/smk37/?fw=obervave) | [zvenson](https://github.com/zvenson/obervave/blob/main/SMK37.md) | One 8-voice analog-style polysynth in the spirit of the American poly synths of 1979–84, with chorus, delay and a plate reverb. |
<!-- /smk37-firmwares -->

## On your computer

**Download** `fm1-live` for macOS (Apple silicon) or Linux (ARM64, x86-64) from
[the releases](https://github.com/joakimeriksson/fm1-emu/releases/latest), and play a firmware
package: `./fm1-live --fwsc felucca.fwsc`. Or build it from a checkout:

```sh
cargo run --release -p fm1-live -- --fw list           # the firmwares
node tools/fetch_firmwares.mjs                          # download them into reference/firmwares
cargo run --release -p fm1-live -- --fw felucca         # a window with the synth, audio out
cargo run --release -p fm1-live -- --fwsc FM-1.fwsc     # M-VAVE's own, from its package
```

`fm1-live` compiles the firmware to native code on Macs (Apple silicon and Intel), on Linux (ARM64
and x86-64) and on Windows (x86-64), and runs the real 240 MHz clock with it; elsewhere it interprets
(`--mhz` lowers the clock if that can't keep up). It plays every MIDI input of the computer (`--midi NAME`, `--midi none`,
`--midi-ch N`) and loads a DX7 bank into M-VAVE's firmware with `--syx bank.syx`.
`fm1-emu` is the batch runner for tests and debugging (WAV and screen output, breakpoints,
memory dumps).
Both take `--flaky-core1 RATE [--seed N]`: the second core misreads RAM now and then, as on
some units, to test a firmware's fallback to one core ([docs/stock-firmware.md](docs/stock-firmware.md#a-flaky-core-1---flaky-core1-fm1_socflaky)).
`fm1-live --flash FILE` keeps the flash chip (saved presets, samples) in FILE between runs: read
at the start if it is there, written back at the end. `FM1_FLASH_CUT=N` cuts the power half way
through the Nth erase or page program, to see what a firmware makes of a half-written save.
`fm1-live --usb` plugs in a USB cable: the FM-1's USB controller answers (without it, it stays
silent, as on a unit with no cable), a host enumerates it, and a USB-MIDI firmware shows up on the
computer (macOS and Linux; on Windows the bus runs, but no MIDI device appears) as a MIDI device
"<its product name> (emu)" that any program can play or talk to, Felucca's `fm1_install.py`
included. It goes away when the firmware drops off USB. The controller raises no interrupt yet:
firmwares that poll it (Felucca's family) enumerate, M-VAVE's own (interrupt-driven) does not. Headless, `--shot` with
`FM1_USB_SEND='MS:HEX;…'` sends MIDI over USB at those times and prints what comes back
(`FM1_USB_TRACE`: the bus's transactions; `FM1_USB_SIE_NS`: the controller's register access time,
an estimate, 1 µs by default).
USB audio: with `FM1_USB_WAV=FILE` (`--bench` or `--shot`) the host also records the firmware's
USB audio input as a computer's recording program would: it chooses the audio streaming
interface's setting that has the isochronous endpoint, asks for `FM1_USB_RATE` (44100 by
default) if the endpoint can be set to it and the format offers it (else the device's own rate),
takes one packet a millisecond and writes the samples to FILE (16-bit, interleaved as the device's
channels come, no header), printing how many packets came empty (nothing queued: an underrun).

The page builds with `web/build.sh` (Rust to WebAssembly; needs the `wasm32-unknown-unknown`
target), the whole site with `tools/build_site.sh`.

## In your DAW: AU and VST3

The emulator also comes as an **instrument plugin**, so a firmware plays in a DAW like any
other synth:

| Format | Loads in |
|---|---|
| **AU** | Logic Pro, GarageBand, MainStage, and most other Mac DAWs |
| **VST3** | Ableton Live, Cubase, FL Studio, Reaper, Bitwig, Studio One, Cakewalk… |
| **CLAP** | Bitwig, Reaper, FL Studio, Studio One… |

**What it does:**
- **The firmware:** **Load firmware / save…** in the plugin’s window picks a `.fwsc` package: any
  community firmware (the table above links each author's) or M-VAVE's own. An SMK-37 package
  gets the SMK-37's panel.
- **Playing it:** MIDI from the DAW plays it, as through the FM-1's MIDI IN. A *MIDI channel*
  setting moves all notes to one channel; M-VAVE's firmware listens on 2. The panel is the
  page's, played with the mouse: keys, buttons, knobs, and on the SMK-37 the pads, faders and
  wheels.
- **Automation:** *Master* (the MASTER pot) is a parameter the DAW can automate.
- **CPU clock:** the emulated clock, 240 MHz as on the chip by default. 120 or 160 let a heavy
  firmware keep up on a slower computer; 320 or 400 give a firmware that runs out of time more,
  at the cost of more of the computer's CPU.
- **Share setups with the browser or another DAW.** “Load firmware / save…” opens a `.fwsc`
  package or `.fm1save` setup. “Export save…” writes the firmware and its latest saved patches
  and settings to one portable file. Edits in RAM need the firmware’s own Save command first.
- **The project keeps the firmware and its patches.** The package and every flash sector the
  firmware has written are saved with the song, so it reopens with the same sounds.
- **Timing:** the emulator runs a little ahead of the DAW and reports that delay as the plugin's
  latency, so the DAW compensates and notes land where they were placed. The status bar shows
  how much faster than real time the firmware could run on this computer; most run at 3–6x,
  the heaviest at about 1.5x.

**Download it** from [the releases](https://github.com/joakimeriksson/fm1-emu/releases/latest),
for Macs with Apple silicon (M1 or later). The zip has the three plugins and an `INSTALL.txt` that
says where each goes. Until a release is signed with an Apple Developer ID, macOS blocks it until
you allow it, with one Terminal command that `INSTALL.txt` gives.

**Build and install it yourself (macOS, Apple silicon).** You need Rust, CMake and Ninja
(`brew install cmake ninja`), then:

```sh
tools/build_plugin.sh
```

This builds the plugin and installs `FM-1.component`, `FM-1.vst3` and `FM-1.clap` in
`~/Library/Audio/Plug-Ins`. It then runs Apple's AU validator on them.

**In GarageBand:**
1. *Settings → Audio/MIDI*: tick *Enable Audio Units*, then quit and reopen GarageBand.
2. Make a *Software Instrument* track and open its Smart Controls (**B**).
3. Click the *Instrument* slot and choose *AU Instruments → fm1-emu → FM-1*.
4. Click the slot to open its window, then *Load firmware / save…*.
5. Play it with **⌘K** (Musical Typing) or a MIDI keyboard.

**In Logic:** a Software Instrument track's instrument slot, *AU Instruments → fm1-emu → FM-1*.

**In VST3 or CLAP hosts:** add *FM-1* (by fm1-emu) as an instrument on a track.

**How it's built:** it's one plugin written in Rust as a CLAP with
[nih-plug](https://github.com/robbert-vdh/nih-plug). [clap-wrapper](https://github.com/free-audio/clap-wrapper)
makes the AU and VST3 of it, and the plugin stays MIT. All its parts are MIT, ISC or Apache-2.0,
including Apple's AudioUnitSDK and Steinberg's VST3 SDK, which is MIT since 3.8.

**So far it has passed:**
- Apple's AU validator (`auval`);
- Steinberg's VST3 validator (47 of 47 tests);
- GarageBand, as an AU;
- Steinberg's VST3 test host.

Logic, Ableton and the Windows / Linux builds haven't been tried yet. More in
[crates/fm1-plugin](crates/fm1-plugin/README.md).

## How it works

The FM-1 is built on a JieLi AC791N: two cores of JieLi's own 32-bit pi32v2 CPU, with no
public instruction set manual. The instruction set here was recovered from the firmwares
themselves (`isa/`, `docs/isa-notes.md`) and checked against test programs built with JieLi's
toolchain. On top of the CPU sit models of the chip's timers, interrupts, flash, the
240×240 screen, the key matrix and encoders, the knobs, the audio output and the MIDI UART.
Hot code is compiled, to AArch64 natively and to WebAssembly in the browser.
How it was built and checked, step by step, is told in a technical report:
[Emulating a Synthesizer Before It Arrives](https://joakimeriksson.github.io/fm1-emu/fm1-emu-report.pdf)
(PDF, 12 pages), with a section on the real FM-1: what releasing FM-1 SID before it ran on one cost
its first users, the rounds that made it work there, and what fm1-dock's probes measured on the chip.

- `crates/`: `fm1-isa` (decoder tables), `fm1-core` (CPU and JITs), `fm1-soc` (the chip and
  the board), `fm1-live` (the native synth), `fm1-emu` (batch runner), `fm1-web` (the wasm
  build), `fm1-gdbstub`
- `web/`: the page; `web/fwsc.js` unpacks the update packages
- `docs/`: the hardware notes (`recon.md`), the instruction set (`isa-notes.md`), M-VAVE's
  firmware (`stock-firmware.md`); `PLAN.md` is the work log
- `tools/`, `probes/`: ISA extraction, benchmarks, differential test programs
- `fm1-sid/`: the SID study and player firmware, [joakimeriksson/fm1-sid](https://github.com/joakimeriksson/fm1-sid) as a submodule
  (its own README; `git submodule update --init` after a plain clone)

## Licences

The emulator is MIT (`LICENSE`). The firmwares are their authors' and are not part of this
repository; the page fetches them from the authors' sites. `fm1-sid/` (its own repository) has
files under other licences, among them its player firmware, GPL-3.0 since it is built on
Felucca's code: its LICENSING.md lists them. The plugin (`crates/fm1-plugin`) is MIT too; the libraries and SDKs it
builds with are permissive: MIT, ISC, Apache-2.0, BSD, Zlib, BSL-1.0, Unicode-3.0 and Unlicense, egui's built-in
fonts OFL-1.1 and the Ubuntu font licence, and Steinberg's VST3 SDK (3.8, through clap-wrapper) MIT. nih-plug is built
without its VST3 bindings, which are GPL. What the emulator emulates it learned from the chips' behaviour, the
community's reverse engineering and the firmwares' own sources as references; no GPL code is in this repository
(the test probe `probes/x0x/d909.c` builds X0X's GPL drum against it from a checkout outside it).

## Credits

Built on the community's reverse engineering of the FM-1:
[AL-255/FM-1-RE](https://github.com/AL-255/FM-1-RE),
[ip2k/lunar-modulator](https://github.com/ip2k/lunar-modulator),
kagaimiq's [jielie](https://kagaimiq.github.io/jielie), the hardware layer of
[hugelton/Felucca](https://github.com/hugelton/Felucca) and
[awesome-fm-1](https://github.com/cicloid/awesome-fm-1). Thanks to every firmware author for publishing their work.
