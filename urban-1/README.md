# URBAN-1 — experimental generative groovebox for M-VAVE FM-1

**v0.4.9 – Live MIDI recorder, modulation pages and tape transport:**

- **ENV** opens real knob-controlled synth envelopes, and **LFO** opens real LFO rate/depth/shape/destination. In Tracks these start at the current instrument; in Song they start at Master, and ALGORITHM navigates across tracks. The Lead LFO is now a separate active DSP oscillator with RATE/DEPTH modulating acid-filter cutoff; native HYP Bass and Pad LFO controls are retained. On Tracks, `SEL+ENV` locks and `SEL+LFO` mutes, so those actions no longer steal the primary ENV/LFO buttons.
- In **FX → MASTER → TAPE / REPEAT**, the first knob enables/disables **PLAY TAPE** (ON by default). With it on, PLAY performs an accelerating Tape Start using the existing master tape memory, while PAUSE/PLAY while running triggers HYP's pitch-bending Tape Stop instead of abruptly cutting the audio. With it off, PLAY/PAUSE uses ordinary transport. Pressing and holding PLAY for at least 650 ms in Song mode restarts at INTRO. A brief PLAY tap retains the previous behavior.
- Changing song genre/subgenre, or a track's selected genre/subgenre, resets previous manual FX/EDIT parameter overrides and applies the new style's FX, filters, envelopes and instrument palette. The Tape transport ON/OFF preference is independent.
- **REC** from Song or Tracks opens **REC ARM**. K1 chooses **Bass / Lead / Chords**, K2 selects **1/2/4/8 bars**, K3 toggles quantization, K4 selects **1/4, 1/8, 1/16, 1/32** when enabled. Press REC again to start recording the physical emulator's **27 playable piano keys**. The second cycle enters **automatic overdub**; keep playing without stopping. Press REC once more to stop recording and open the **MIDI piano-roll editor**. There, K1 moves the editor cursor, K2 changes MIDI pitch, K3 changes velocity, and K4 toggles notes on the grid. Another REC resumes overdub. `SAVE` in the editor retains the loop in the running session and exits to Song/Tracks.
- While recording, **SELECT / ENV / LFO / EDIT** switch the four-knob views between MIDI, envelopes, LFO and sound/filter editing. Notes, chords and knob automation share one per-loop timeline. Recorded knob changes replay in subsequent passes. The generated background arrangement continues on other tracks while the selected recorded voice takes over from its generated part.
- **Modelled jazz modulation:** the new piano/Rhodes/mallet synthesis also responds to PAD attack/release and brightness + PAD LFO rate/depth; jazz flute/vibraphone/bell respond to the independent LEAD cutoff-rate/depth LFO and lead envelope, while the upright/finger bass responds to native Bass cutoff, LFO rate/depth and attack/decay. These controls affect the real procedural PCM voices; they are not just display labels.\n- **Current limitations:** captured loops are **in RAM** and do not survive a power cycle or firmware switch; the device project store does not yet have a dedicated recorder slot. Quantization OFF preserves timing at the **32nd-note edit grid**, while UI-thread MIDI playback is emitted on the next available 16th polling boundary (not sample-accurate 32nd playback). Bass/Lead/Chords are the three currently playable keyboard instruments, not the six separate drum lanes or Sound FX. The tape spin-up reuses the existing HYP circular buffer and performs a short final crossfade. These are working prototype behaviors, not a commercial DAW. Source tests, FM-1 host interaction and cross-compiled firmware packaging are required before publishing.

**v0.4.8 – Echo FX audio fix:** Master FX **Echo** now sends an adjustable 0–100 amount of the actual complete stereo music mix to the original HYP tempo-synced tape/Echoplex delay, just like the momentary **Echo Throw** performance effect. Previous Echo control changed the P_ECHO value, but this only multiplied tiny per-instrument sends (as low as 0.04 on bass); the delayed signal was practically inaudible, particularly on dry bass/drums. Now Echo generates noticeable repeated tails and Throw still acts as its strong temporary boost. Total input is capped so activating both simultaneously will not overload the tape send. Existing Echo Time, shared reverb, FX mute/solo and master dry dynamics remain in place. Dedicated host audio regression tests verify real delayed PCM taps, zero Echo send at 0, and independent Throw functionality.

**v0.4.7 – Half-time Jazz bass:** Jazz Rap (Hip-hop/1), Jazz Underground (Underground/1) and Jazz Liquid (Liquid Jungle/2) now have **two contrabass attacks per ordinary bar on beats 1 and 3**, replacing four quarter-note attacks, halving the note density without changing the song BPM or any other track. Their walking note choice advances at half-time and their natural plucked gate sustains slightly longer. Jazz bass track overrides work independently; no other bass subgenre has been changed. 192 native score takes check all eight song sections, exact two attacks per regular bar, zero extra pickups, and 53 untouched substyles.

**v0.4.6 – Jazz acoustic bass:** Jazz Rap, Jazz Underground and Jazz Liquid bass tracks now use a dedicated note-triggered *upright pizzicato* bass synthesis model with softened string plucks, damped harmonics, faint finger scrape and body resonance. Jazz Liquid may also choose a fingered bass. This removes HYP's electronic Drone/Reese/Organ timbres and glide/LFO from jazz bass only, maintains the full four-bar walking score and independent track overrides, and shows the actual instrument name on the emulator. Non-jazz bass/ambient tracks remain unchanged. Tested through new real HYP host PCM frame regression; no imported acoustic samples.

**v0.4.5 – Jazz instruments:** Dedicated five-voice DSP sound engine (four polyphonic chords plus one mono lead) introduces modelled acoustic piano, Rhodes/tine electric piano, breathy flute, vibraphone and soft bells, with per-voice attacks/decays and subtle modulation. Jazz Rap, Jazz Underground, Jazz Liquid and Sad Piano select curated, per-track and per-take acoustic-like programs; roll attacks stay sample-synchronous with HYP DSP, and effects/faders use the existing bus. **These are original procedural models, not acoustic samples, soundfonts or preset renames.** Other genres and ambient washes retain their previous HYP voices. Real HYP host and firmware CI test new PCM rendering and native-style isolation.


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
- Actual HYP DSP voices: six drum lanes, bass/drone, lead/acid,
  chord/pad, atmospheric noise (track 6), and now a **separate procedural
  Sound FX voice (track 7)**. Track 8 ("Extra 2") remains reserved.
- **Sound FX** obeys individual genre, subgenre, regeneration, mute and solo.
  No existing drum, bass, lead, pad or atmosphere track was removed.
- On **FX** or **EDIT**, turn **ALGORITHM** to page **7/8 Sound FX**
  (master is page 0), then adjust four live knobs: **Sound** (-1 AUTO
  palette / ten manual types), **Level**, **Length**, **Density**.
  Automatic style selection follows the chosen genre/subgenre.

## Sound FX track (v0.4.0)

Track 7 is now an actual audible synth layer with its **own HYP DSP**
voice, not a sample name in a playlist. Its ten procedural FX are:
vinyl scratch, tape stop, reverse swell, sub impact, dub siren,
glitch zap, noise riser, downlifter, vinyl pop, and air whoosh.
There are **56 dedicated primary/secondary FX choices** across the
14 genre families and 4 subgenres. For example Boom Bap uses scratch /
vinyl pop, G-Funk dub siren / tape stop, underground dusty scratches,
Trip-hop reversed sweeps, Trap drops/risers and Dubstep impacts/downlifters.

The cues are quantized to the 16-step grid and placed deliberately at
phrase boundaries, transitions and selected late-bar moments. They are
not a stream of random fills. A track's sequence seed determines its
variations and regenerates with SAVE; its LOCK prevents regeneration.
FX/EDIT expose manual SOUND, LEVEL, LENGTH and DENSITY control on
Sound FX's separate track page (ALGORITHM). Level or density 0 silences
new effects; mute and solo work as on the other tracks.

**Limitation:** these are *synthesized* approximations of effects, not
commercial scratch/crowd/foley recordings. One dedicated monophonic SFX
voice plays at a time to stay within the FM-1's resources. SFX timing
is tied to the HYP audio transport; the native FM-1 host and emulator
still need subjective listening validation on the user's device.

## Expanded scale engine (v0.4.3)

URBAN-1 now offers **124 named scale entries** across the 12-tone
equal-tempered system, not merely its original 10 seven-note scales.
The catalogue covers Western major modes, harmonic and melodic minor
families and all their rotations, harmonic major, double harmonic,
Hungarian, Romanian, Neapolitan, Persian and Enigmatic mode families;
major/minor/suspended and Japanese-derived **pentatonic** forms;
**six-note** blues, whole-tone and augmented scales; **eight-note**
diminished and bebop scales; Messiaen modes and the **12-note**
chromatic scale. Several traditional names denote equivalent pitch
collections, so the 124 *named entries* are not 124 unique pitch sets.

Every **56 subgenres** has an independently assigned **primary scale
plus two genre-compatible alternatives**. Selecting a genre/subgenre
explicitly restores its primary musical centre; pressing SAVE may
choose one of its related alternatives for a new take, while keeping
the four-bar chord framework. All bass, chord, lead, voice-leading
and harmonization routines now use the chosen scale's actual count of
degrees per octave (5–12) rather than silently assuming seven.
Every note is generated from scale pitch classes; pattern timing is
separate from theory and is not affected.

To explore the full catalogue manually:
**EDIT → ALGORITHM: MASTER → SONG THEORY → K3: SCALE**.
The LCD displays each selected scale name. User-selected scales survive
SAVE; explicitly switching song genre/subgenre restores that style's
curated scale selection.

**Scope / limitation:** this library uses 12-tone equal temperament.
It is **not** a complete index of every scale or tuning system in world
music; that would be unbounded and would require microtonal pitch,
unequal intervals, melodic ascent/descent rules and ornamentation
for many traditions (e.g. maqamat, ragas and gamelan). Japanese-derived
scale names here represent standard piano-note approximations and
must not be mistaken for faithful traditional intonation.

## Groove realism: four-bar bass, pianist chords and drums (v0.4.4)

**Bass:** each of the **56 named subgenres** now has four authored bar
rhythms, rather than the former one-bar bass mask repeated four times
under a new chord. The 16th-step note-on grid remains synced to the
same song clock as the kick/snare. Chord-root transposition, modal
passing tones and pitch/slide articulations are handled independently
from the attack pattern, so Jazz Rap walking bass, G-Funk pocket,
Reggaeton tumbao, Neuro DnB syncopation, Psytrance root pulse,
Dubstep half-time, Minimal Techno and ambient drones differ in
real scores rather than only a new synth patch. SAVE still makes
deterministic new takes, never unpredictable per-frame hits.

**Chords:** natural piano/jazz accompaniment deliberately plays
open seventh/ninth chords for longer than EDM stabs. Jazz Rap,
Underground Jazz, Jazzy Liquid and Sad Piano do **not** trigger all
piano keys at precisely the same instant: ascending or descending
voicings are pressed 14–24 ms apart by HYP's **audio callback**
(sample-level scheduling, not WebView/LCD polling). Soul/funk
accompaniments use subtler, faster 7–11 ms spreads. Other genres
retain their purposeful short rhythmic stabs/pulsing patterns.
**Ambient/darkwave/cloud/downtempo/deep/dub atmospheric pads remain
long, simultaneous block chords**, with no artificial strum.

**Drums:** kick/snare and hat/perc tracks keep their own independent
subgenre settings. The common random kick edits, generic eighth-hat
overlays and automatic clap doubling have been removed. Each named
style has authored four-bar kick replies, snare ghosts, hi-hat
responses and turnaround fills while retaining the kick/backbeat
identity. Boom Bap still has **zero clap-lane events**, and the
clean normal Hip-hop kick patch stays uncrushed.

**Limitations:** piano realism is produced from HYP's synthesized pad
timbres and multi-note timing, not multisampled acoustic recordings.
Rhythms are 16th-grid based except the humanized intra-chord
press spacing. FM-1 physical device performance and subjective
mix quality must still be judged by listening on the target emulator.

## Genre implementation (14 genre families, four named substyles each)

Trap, Goth Trap, Sad Trap, Hip-hop, Trip-hop, Underground Hip-hop,
Horrorcore, Reggaeton, Drum and Bass, Dubstep, Techno, Hardcore Techno,
Trance, Liquid Jungle. Every genre has a specific:
- BPM suggestion and three scale palettes per named subgenre, with 124 available named 12-TET choices
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

**Hip-hop overhaul (v0.3.7):** Boom Bap, Jazz Rap, G-Funk and Lo-fi
(and four Underground variants) now use warm curated synthesis palettes,
low-register bass, longer Rhodes-style comping, reduced lead density,
drier break/808 drum kits and actual HYP drum micro-swing. The music
engine no longer applies randomized hip-hop kick rearrangements or
stacked clap doubling. See [the detailed production audit](HIP_HOP_PRODUCTION_AUDIT.md).

This is a **genre-aware rule-based prototype**, not sample-based genre
reconstruction or a trained music model. More accurate per-substyle
programming, richer chord voicings and multi-voice instrument synthesis
remain ongoing tasks.

## Trance composition and clock-sync correction (v0.4.2)

Progressive Trance, Uplifting, Psytrance and Tech Trance now have four
separate rhythmic grammars, melodic figures and real HYP ACID/PAD tone
banks. The previous **0x8888** pattern placed a note on sixteenth
steps **3, 7, 11, 15**, just before every quarter note. This is
replaced by deliberate **offbeat eighths on steps 2, 6, 10, 14** for
Progressive/Uplifting, a straight eighth root/fifth pulse for Psy and
short quarter-note stabs for Tech. Melodic development follows each
four-bar progression; random weak-note insertions and arbitrary
rotation of the entire Trance motif have been removed.

Most importantly, Trance notes no longer wait for UI/WebView polling:
a precomputed, double-buffered phrase score is consumed directly in
the HYP-1 **audio tick that triggers the drums**. It releases and
restarts notes at audio clock edges in both Song Mode and manual Loop
Mode, with no LCD frame quantization, while respecting the lead track
genre, mute and solo. The score is refreshed when a musical parameter
changes. Progressive is softer, Uplifting more open, Psy rhythmic and
short, and Tech staccato. These are synthesized melodies, not recorded
Trance loops; subjective musical evaluation on the user's emulator
remains necessary.

Validation includes 384 seeded takes, distinct masks and note range
checks, plus host audio tests observing actual DSP Trance note-on
events and zero off-grid onsets.

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
