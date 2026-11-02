# URBAN-1 — experimental generative groovebox for M-VAVE FM-1

**Current source:** HYP-1 GPL-3.0 synthesis integrated with the URBAN-1
composition engine. **This release is firmware-only (`.fwsc`)**: load the
package through your existing Android FM-1 emulator's firmware selector.
No Android APK changes or reinstallations are necessary. **Not a finished commercial music generator.**

## Song Mode (automatic)

- **Four-bar A–H phrases / 56-bar arrangement:** A Intro (4), B Verse 1 (8), C Hook (8), D Verse 2 (8),
  C Hook reprise (8), E Bridge (4), F Drop (8), G Breakdown (4), H Outro (4).
  Loops at the end; the hook's motif remains recognizable on the reprise.
- Tracks share one four-bar harmonic timeline, key, tempo and modal scale.
- Arrangement progress is visible on the LCD as `BAR 01/56` and progress bar.
- **HOME**: Song page; **ARP**: toggle Song Mode / Sequence Mode.
- **SAVE**: generate a new song in selected genre; **REC**: restart at bar 1.
- **KNOB 1**: tempo; **KNOB 2**: one of 14 genres; **KNOB 3**: **subgenre**
  (four named styles for the selected genre); **SELECT** (left encoder): key.
- **ALGORITHM** (left): alternative control for the same four named substyles.
- **PRESETS** (left): complexity 0–3 (melodic ornaments, drum fills).
- New genre, key and substyle are reflected in the engine; press SAVE to
  regenerate a fresh arrangement in the selected style.

## Sequence Mode (manual)

- **ARP** on Song page switches to **LOOP MODE**: the currently selected
  four-bar A–H phrase repeats indefinitely until you request another one.
  The current bar (1/4) and pending A–H destination appear on the LCD.
- **KNOB 4** clockwise / anticlockwise queues the next/previous section.
  The queued section starts only on the NEXT bar boundary (not mid-bar),
  **even if the current four-bar phrase has not finished**.
- **PLAY** starts/stops, **SAVE** regenerates the selected song.

## Tracks page

- **SEQ**: Tracks page; **HOME**: Song page.
- **KNOB 1**: select any of 8 logical tracks. **KNOB 2**: track genre.
- **KNOB 3**: one of four named track substyles.
- **SAVE**: regenerate A–H **only for that track**.
- **FX**: mute, **GLO**: solo, **EDIT**: lock track against regeneration.
- Song harmonic progression and all other track-generation seeds remain
  unchanged during a one-track regeneration.
- Current FM-1/HYP DSP voices: 6 drum lanes, bass/drone, lead/acid,
  chord/pad. Tracks 5–7 are logical tracks but **not independent audible
  synth voices**. Independent voice count is a hardware/audio-engine task.

## Genre implementation (14 genre families, four named substyles each)

Trap, Goth Trap, Sad Trap, Hip-hop, Trip-hop, Underground Hip-hop,
Horrorcore, Reggaeton, Drum and Bass, Dubstep, Techno, Hardcore Techno,
Trance, Liquid Jungle. Every genre has a specific:
- BPM suggestion, minor / Phrygian / Dorian / harmonic minor palette
- four-bar chord progression, sixteen steps **per bar** and six drum lane
  mapping, for **64 steps per A–H phrase** with tonal and rhythmic variation
- lead rhythmic grammar, articulation and section-specific variations
- bass grammar (808 sustain/slide, dembow bass, DnB syncopation,
  techno/trance offbeats, etc.)
- **56 specific sound/effect patches** mapping actual HYP-1 DRONE/ACID/PAD
  instruments and eight built-in drum kits (bass, lead, chords, kicks, hats),
  filters, drive, reverb, echo and swing. These are synthesized timbres,
  **not licensed genre-specific samples**.
- repeated hook and song-form transitions, controlled four-bar drum fills.

This is a **genre-aware rule-based prototype**, not sample-based genre
reconstruction or a trained music model. More accurate per-substyle
programming, richer chord voicings and multi-voice instrument synthesis
remain ongoing tasks.

## Build and verification

```sh
make -C urban-1 test
git clone https://github.com/RustyPawz/fm1-HYP1 hyp1
git -C hyp1 checkout 57c456db61826331d356458c9c1a136517da7cbf
python3 urban-1/tools/apply_hyp1.py --hyp-root hyp1
(cd hyp1 && sh host/build_host.sh)
(cd hyp1 && ./build.sh --release 0.1.0)
```

The source tree's GitHub Actions compiles and validates a standalone
experimental **firmware package**. New URBAN-1 commits do **not** trigger
Android APK builds. Install this `.fwsc` from your existing Android emulator's
firmware picker; do not reinstall the Android app.
Real-device audio and latency still require hands-on testing.

GPL-3.0 base: [RustyPawz/fm1-HYP1](https://github.com/RustyPawz/fm1-HYP1).
URBAN-1 source: [feature/urban-1](../tree/feature/urban-1).
