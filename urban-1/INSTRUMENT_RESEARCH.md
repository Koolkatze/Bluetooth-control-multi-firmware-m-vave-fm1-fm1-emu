# URBAN-1 — instrument research and integration boundaries

Research date: 2026-11-02. The goal is a **single FM-1 `.fwsc`** loaded in
the existing Android emulator, not several simultaneously running firmwares.

## Actually usable today in URBAN-1

HYP-1 (GPL-3.0-only) has **48 real synthesizer sound modes** in its three
existing engines. URBAN-1 now has access to all 16 in each engine, via
`urban_instruments.h` and `urban_style.h`:

- PAD, 4-voice polyphonic: Rhodes, Voices, Strings, Glass, Organ, Warm,
  Stabs, Dark, Dub, Trance, Poly, Hollow, Pings, Hits, Rave, Sine
- DRONE / BASS, monophonic: Saws, Warm, Grit, Dirty, Reese, Pulse, FM,
  Organ, Rumble, Sub, Square, Hoover, Fifths, Metal, Sync, Bright
- ACID / LEAD, monophonic: Saw, Square, Driven, Sync, Pluck, PWM, Chip,
  Sub, Trance, Bleep, Fifth, Bell, Organ, Laser, Fold, Soft
- Eight drum sound kits: Hypno, House, Techno, Acid, Break, Minimal, Hard, 808
- Eight synthesized texture colours: White, Pink, Brown, Blue, Violet,
  Grey, Black, Yellow

These are **64 sound/kit/noise options** (48 + 8 + 8), not 64 independent
audio voices. They can be combined with different envelopes, filters,
effects, voicings, note registers and generated score patterns. The program
selector varies timbres on every new composition within a genre-specific
palette; locked tracks preserve their sound until manually changed.

## Other open-source firmware engines identified

### FM1-GM — 128 additional General MIDI programs, sampled

Source: https://github.com/jartificer/fm1-gm
GPL-3.0-only firmware, with approximately 424 KiB reduced FluidR3 samples
under MIT (see their sample LICENSE). GM instrument families include acoustic
and electric pianos, guitars, electric/synth basses, strings, brass, winds,
mallets, pads and percussion. A **full FM1-GM soundbank cannot be enabled in
URBAN-1 just by adding the patch IDs**. Their sample decoder, RAM cache,
sample bank, voice allocator, mixer, rate conversion, and flash layout would
have to be ported and profiled with HYP-1's existing DSP and display. The
424 KiB data bank may exceed the flash budget available alongside HYP-1.
Do not bundle it without a verified build, licence notices and profiler.

Important: FM1-GM's upstream README warns of a dangerous loud-output bug
in versions <= 0.9.8. Any port must use current audited sources, gain limiting
and audio stress testing; never copy old release binaries.

### TRIBE — 28 engines / types, including modern synths

Source: https://github.com/apiel/fm1-tribe
GPL-3.0-only. The source includes:
- FM4: monophonic four-operator FM synthesizer, 8 algorithms, ratio,
  feedback, operator and amp envelopes (candidate for bells, plucks,
  mallets and keys)
- LEAD: three-oscillator unison lead with filter, drive/distortion and glide
- TRIBE wavetable synth: wavemorph, harmonics, fold, modulation, filter and FX
- NOISE synth, ACID, modelled/sampled 808/909 percussion and kick engines
- FX bus including chorus, flanger, phaser, crusher, ring modulation,
  wavefolder, tremolo etc.

Each additional engine needs a **real audio-render bridge**, per-track voice
routing, CPU-budget testing and a compatible build; no raw TRIBE binary is
usable as an HYP plug-in. TRIBE's engines are future candidates, not installed
by `urban_instruments.h`.

### X0X — 808/909 + two 303s + sliced breaks

Source: https://github.com/charlesvestal/fm1-x0x
GPL-3.0-only firmware. 11 909 voices, 11 808 percussion voices, two Open303
synths and a break-slice engine. The 909 uses samples from the GPL-3.0 ER-99
project; third-party recorded breaks do **not** share the firmware's licence
and must not be copied without permission. This is the best candidate for
more authentic Trap, Hip-hop, Reggaeton, Dubstep and Drum & Bass percussion,
but also requires porting the renderer/mixer, not just the sequencer.

### LOF-1, ECO-1, GRAV-1 (RedtailWorks)

Source: https://github.com/RustyPawz/fm1-HYP1
The same repository includes LOF-1 (twenty lead instruments, jazz comping),
GRAV-1 (16 space/ambient sounds) and ECO-1 (generative ambient). Their sound
architecture and resource footprint require assessment before integrating.

## Recommended future engine integration order

1. Add TRIBE's FM4 and its associated parameter model in a separate FM-1
   build, with tests and explicit CPU budget, rather than breaking v0.3.0
2. Add TRIBE LEAD / wavetable engine for rock-hardcore, trance and synth leads
3. Port X0X modelled 808/909 and break generator to replace simplified HYP
   drum kits where fast hat rolls / ghost notes require higher temporal
   resolution than the current 16-step grid
4. Evaluate FM1-GM's 128-sample data/decoder only after checking firmware
   image/flash limits, CPU and ear-safety gain behaviour

Source licenses must remain attached to any transplanted code or sample.
**This document is a validated inventory and port plan, not a claim that
another firmware's synth engine already runs inside URBAN-1.**
