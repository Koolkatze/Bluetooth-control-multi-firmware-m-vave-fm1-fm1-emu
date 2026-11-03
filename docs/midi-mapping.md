# M-VAVE FM-1 físico ↔ Workbench: MIDI USB-C y BLE

## Objetivo

El FM-1 físico ejecuta su firmware controlador (por ejemplo, **CTL-1**), y
**la misma APK Workbench** ejecuta cualquiera de los firmwares FM-1 emulados
(URBAN-1 incluido) con su pantalla y audio propios. El FM-1 real es controlador;
**no se clona ni captura su LCD, ni se flashea al seleccionar un firmware en Workbench**.

El receptor `MidiPanelMapper.java` alimenta la matriz original del FM-1
virtual: **27 teclas + 14 botones (IDs 0–40), 7 encoders, MASTER**.
El motor nativo Rust recibe los controles directamente, y la WebView solamente
refleja el estado. El motor WebAssembly de reserva recibe exactamente las mismas
acciones. BLE-MIDI y USB-MIDI tienen el mismo decodificador, sin depender de
los ajustes de "Transferencia USB → MIDI" del teléfono.

El botón **▲ → MIDI** abre conexión USB o búsqueda BLE, **Modo de mapeo**,
**Ver botones y knobs mapeados**, diagnóstico y desconexión.

## Perfiles disponibles

1. **Automático**: CTL-1 si el nombre del dispositivo contiene
   `FM-1`, `CTL-1` o `M-VAVE`; cualquier otro dispositivo usa MIDI
   convencional. Se puede cambiar manualmente y el ajuste persiste.
2. **Solo instrumento MIDI**: pasa notas/CC/program changes/reloj MIDI al
   puerto del firmware emulado sin intervenir en su panel.
3. **CTL-1 instalado**: transforma los mensajes que **sí emite** el CTL-1
   público en controles del panel virtual (tabla siguiente).
4. **Workbench Remote**: asignación 1:1 para TODOS los controles físicos,
   **solo si el firmware del FM-1 físico implementa el protocolo reservado
   descrito abajo**. Seleccionar este modo no cambia mágicamente CTL-1.

## CTL-1 público: mapa funcional (sin reflashear)

| FM-1 con CTL-1 | Mensaje que emite | Workbench |
|---|---|---|
| Teclas cromáticas F3–G5, octava original | Note on/off, canal ≠ 10, notas 53–79 | Teclas virtuales 14–40 |
| KNOB1 en banco A/B/C/D | CC 74/73/16/20 | Encoder virtual **KNOB1**, hardware ID 2 |
| KNOB2 en banco A/B/C/D | CC 71/72/17/21 | **KNOB2**, hardware ID 3 |
| KNOB3 en banco A/B/C/D | CC 1/75/18/22 | **KNOB3**, hardware ID 4 |
| KNOB4 en banco A/B/C/D | CC 7/76/19/23 | **KNOB4**, hardware ID 5 |
| PRESETS | Program Change | Encoder PRESETS, hardware ID 6 (cambios relativos) |
| PLAY | MIDI Start / Stop | Pulso de PLAY, tecla ID 12 |
| ENV | CC64 | Botón ENV, tecla ID 4 |
| REC (panic) | CC123 | Pulso de REC, tecla ID 13 |
| MASTER (configurable) | En CTL-1: EDIT → Sound → MASTER → CC11 | MASTER 0–1023 |
| Secuenciador / drums, canal 10 | Notas | Siguen entrando como MIDI musical |

- CTL-1 solo emite CC de **KNOB1–4 en la página Play/HOME**.
  Sus otras páginas editan los valores del propio CTL-1. Hay **cuatro bancos
  de cuatro CC**; cualquiera de los cuatro bancos apunta al correspondiente
  knob virtual, no a dieciséis knobs diferentes.
- Los encoders virtuales solo admiten **incrementos relativos**. Para
  prevenir saltos al conectar, el primer valor de cada CC establece su
  referencia; los movimientos siguientes giran el knob correspondiente.
- CTL-1 usa **CC1 también para LFO/mod wheel**, y el banco A usa CC1 para
  KNOB3: los dos eventos son indistinguibles por MIDI. No se puede garantizar
  ese control aislado con el firmware CTL-1 sin modificar.
- La selección de escalas/acordes, arpegiador y cambio de octava de CTL-1
  alteran las notas MIDI transmitidas. En ese caso NO se conoce qué tecla
  física se pulsó a partir del tono: las notas fuera de 53–79 pasan a MIDI
  musical; en el interior del rango se corresponden por **nota**, no por
  posición física. La asignación física exacta exige Workbench Remote.
- Otros botones (OCT±, FX, SCL/SEL, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ)
  y encoders (SELECT, ALGORITHM) **no transmiten automáticamente su pulsación
  o giro** en el firmware CTL-1 público. Una APK no puede recuperar eventos
  que el hardware no envía. Tampoco se puede inferir de forma fiable
  "qué botón se pulsó" desde las notas musicales.

Consulta el manual oficial del controlador:
https://charlesvestal.github.io/fm1-ctl/manual/

## Protocolo Workbench Remote v1: panel completo

Extensión MIDI **unidireccional**, de FM-1 físico → APK. Utiliza **canal 16**
reservado a control del panel, para no confundirlo con las notas de los
canales 1–15. No usa SysEx, funciona con el mismo perfil sobre USB MIDI y
BLE MIDI, sin esperar confirmaciones de UI y sin modificar el firmware
emulado.

| Elemento | MIDI enviado por firmware físico | Resultado virtual |
|---|---|---|
| 27 teclas (matriz 14–40) | Note on/off canal 16, notas **48–74** | ID 14 + nota − 48, pressed/released |
| 14 botones (matriz 0–13) | CC canal 16, números **80–93** | ID CC − 80; 127=pressed, 0=released |
| SELECT, PRESETS, ALGORITHM, KNOB1..4 | CC canal 16, números **94–100** | Encoder 0, 6, 1, 2, 3, 4, 5 |
| MASTER | CC canal 16, número **101** | ADC 0–1023 |
| Otros mensajes, canales 1–15 | Note/CC/Clock/Program Change | MIDI musical sin alterar |

Los encoders son relativos: CC valor **1..6** = +1..+6 clicks;
CC valor **127..122** = −1..−6 clicks; 0 o 64 = sin movimiento.
Para evitar saltos/mensajes erróneos se rechazan cambios de magnitud >12.
Las posiciones absolutas y el cambio de banco local no necesitan
sincronizarse con el firmware del sintetizador emulado.

### Correspondencia de los 14 botones

| CC ch16 | ID del panel | Botón |
|---|---|---|
| 80 | 0 | OCT− |
| 81 | 1 | OCT+ |
| 82 | 2 | FX |
| 83 | 3 | SCL |
| 84 | 4 | ENV |
| 85 | 5 | LFO |
| 86 | 6 | EDIT |
| 87 | 7 | GLO |
| 88 | 8 | HOME |
| 89 | 9 | SAVE |
| 90 | 10 | ARP |
| 91 | 11 | SEQ |
| 92 | 12 | PLAY |
| 93 | 13 | REC |

Encoder 94 = SELECT; 95 = PRESETS; 96 = ALGORITHM;
97 = KNOB1; 98 = KNOB2; 99 = KNOB3; 100 = KNOB4.

**Requisito en firmware físico**: su scanner de matriz debe emitir
Note on/off + CC press/release para todos los botones/teclas, y mensajes
relativos de encoder y valores MASTER. Para evitar eventos duplicados,
en modo Remote el firmware puede optar por **no enviar** MIDI musical
de las teclas; alternativamente puede enviarlo en canales 1–15 y
la app lo tratará como performance MIDI independiente.
Enviar únicamente los CC del CTL-1 original **NO equivale a Remote**.

El protocolo es una especificación de Workbench, **no** una funcionalidad
afirmada del CTL-1 publicado. Adaptar el firmware hardware sería un proyecto
separado; para probar la APK no hace falta flashearlo.

### Adaptador de código fuente del CTL-1 (opcional, sin flasheo automático)

El repositorio incluye `tools/patch_ctl1_workbench.py` para transformar
**un clon local** del código fuente `charlesvestal/fm1-ctl` en un controlador
compatible con el modo **Workbench Remote**:

```sh
git clone --recursive https://github.com/charlesvestal/fm1-ctl.git
python3 tools/patch_ctl1_workbench.py ./fm1-ctl --dry-run
python3 tools/patch_ctl1_workbench.py ./fm1-ctl
```

Este script modifica **solo** `fm1-ctl/app/ui.c`, conserva
`ui.workbench-backup.c`, verifica cada punto de inserción y se detiene si
la estructura del archivo cambió. NO descarga un firmware, NO compila, NO
flashea y NO se conecta al FM-1.

La extensión envía transiciones reales de las 27 teclas físicas en el
canal 16 antes del procesamiento específico de página, 14 botones
`CC80–93`, 7 encoders `CC94–100` y el potenciómetro MASTER `CC101`.
Separa las notas del teclado físico del MIDI de su secuenciador para evitar
que una misma pulsación suene por duplicado: las notas físicas siguen
sonando en el sintetizador GM local, pero ya no se mandan como notas
convencionales simultáneamente por USB/BLE. El MIDI generado por el
secuenciador/arpegiador no se altera. En modo Remote, deja el canal musical
CTL-1 entre 1 y 15 para reservar el canal 16 a los controles.

**Para usar el modo Remote con un FM-1 real**, además de seleccionar ese
perfil en la APK, es imprescindible compilar `ctl.fwsc` con el toolchain
JieLi, las bibliotecas BLE y las dependencias descritas por el autor en
[BUILDING.md de CTL-1](https://github.com/charlesvestal/fm1-ctl/blob/main/BUILDING.md),
y **flashear voluntariamente** dicho firmware en el dispositivo físico.
No existe un binario validado adjunto a Workbench: el parche es código
fuente sin una prueba con el dispositivo real y podría necesitar ajustes
para cambios de versión upstream. No flashees hasta tener el método
de recuperación preparado. El funcionamiento de USB/BLE debe comprobarse
en hardware.

El firmware local del FM-1 conserva sus propias páginas y pantalla,
pero cuando se usa Workbench Remote **las funciones que editen ajustes
locales del CTL-1 no necesariamente cambian esos mismos parámetros en el
firmware emulado**: el evento físico acciona el botón/knob equivalente
en el panel virtual, que determina el comportamiento de su propio firmware.

## Verificación

La suite `tests/MidiPanelMapperTest.java` comprueba la matriz de teclas,
los cuatro bancos CTL-1, los 7 encoders, potenciómetro, realtime MIDI,
running status entre paquetes, paso de batería y la liberación de notas
en desconexión. No sustituye la prueba USB/BLE en hardware Android real:
que haya una sesión BLE emparejada no implica que lleguen mensajes MIDI;
utiliza **MIDI → Diagnóstico** para ver contadores reales.

Al sustituir una APK de desarrollo firmada con distinta clave puede ser
necesario desinstalar la anterior, lo que elimina datos privados.
Guarda/exporta primero los ajustes y proyectos que puedas.
