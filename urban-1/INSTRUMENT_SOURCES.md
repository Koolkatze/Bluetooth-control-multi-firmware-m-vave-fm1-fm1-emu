# URBAN-1: verified instrument-source audit
Last checked: 2026-11-02. GPL-3.0-only compatibility applies to **code**
with corresponding source. Other authors' presets, samples and brand names
are not automatically free to redistribute. User-facing claims must reflect
the actual firmware binary, not the list of interesting repositories.

## Enabled in URBAN-1 now (actual compiled HYP-1 synthesis)

Source: [RustyPawz/fm1-HYP1](https://github.com/RustyPawz/fm1-HYP1),
`firmware/src/dsp/amb.c`, lines 153–159 at revision
`57c456db61826331d356458c9c1a136517da7cbf`.

| Real DSP resource | Number | Available names |
|---|---:|---|
| PAD, four simultaneous oscillator voices | 16 timbres | Rhodes, Voices, Strings, Glass, Organ, Warm, Stabs, Dark, Dub, Trance, Poly, Hollow, Pings, Hits, Rave, Sine |
| DRONE (bass) | 16 timbres | Saws, Warm, Grit, Dirty, Reese, Pulse, FM, Organ, Rumble, Sub, Square, Hoover, Fifths, Metal, Sync, Bright |
| ACID (lead) | 16 timbres | Saw, Square, Driven, Sync, Pluck, PWM, Chip, Sub, Trance, Bleep, Fifth, Bell, Organ, Laser, Fold, Soft |
| Drum engine | 8 kits, six lanes | Hypno, House, Techno, Acid, Break, Minimal, Hard, 808 |

**Implemented:** all 16 timbres of each of the three melodic banks and all 8
drum kits appear in at least one of the 56 playable genre-subgenre patches.
The chord engine has been upgraded to emit actual 2–4-note modal chords
through HYP-1's four PAD voices, including 7ths, 9th shells, suspensions,
fifths and quartal voicings. The bass and lead are still monophonic.

**Distinction:** 56 patches are combinations of existing timbres and settings,
not 56 additional synthesis engines.

## Engines found in other GPL-compatible FM-1 firmwares

### [TRIBE](https://github.com/apiel/fm1-tribe) (GPL-3.0-only)

Actual sources: `firmware/src/tribe/engines.c`,
`firmware/src/dsp/{fm4,lead,noise,tribe_syn,drum808,drum909,bass303,kicks}.c`,
`LICENSING.md`.

28 playable engine types: two kick synthesizers, eleven 909 voices, eleven
808 voices, acid (303), three-oscillator/unison LEAD, four-operator FM4,
noise (white/pink/brown/dust with filter, crusher and comb), and TRIBE
morphing wavetable synth. The FM4 module has eight FM algorithms and
operator envelopes; LEAD has detuned unison, a resonant ladder and glide;
TRIBE has wavetable morph/drive effects. The native engine registry owns
its voices and requires memory pools, parameter banks and audio ISR
integration.

**Status: studied and identified; NOT YET PORTED into URBAN-1.** Loading
the TRIBE firmware as a sound bank is impossible: each FM-1 firmware
replaces the executable, and the HYP-1 ABI is different.

### [X0X](https://github.com/charlesvestal/fm1-x0x) (GPL-3.0-only)

Real sources: `firmware/src/dsp/{drum808,drum909,bass303,breaks}.c`
and `LICENSING.md`. Provides 11 808 percussion voices, 11 909 voices,
two 303 synths, and break slicing, with tape delay and compressor.
Most are shared/reworked in TRIBE. Recorded breaks carry distinct rights;
import only properly licensed material.

**Status: researched, not imported.**

### [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) (GPL-3.0-only)

Real sources: `firmware/src/{fm6_core,eng_fm6,eng_cz,eng_quad,eng_va}.c`,
its FM6 and CZ banks, `LICENSING.md`, `docs/PRESETS.md`.
FM6 (DX7-like six-operator FM), a Casio CZ tone bank, virtual analog,
and other engines. It credits Melodee, MSFA, Dexed and sample sources.
Individual original license notices must be retained. Some CC0
community instrument samples are documented in its assets.

**Status: researched, not imported or advertised as installed.**

## Porting order and constraints

1. **Native HYP library:** fully map its current 48 timbres + eight kits,
   use PAD polyphony, implement modal/genre harmony, dynamic arrangements,
   groove and velocity behaviour. **Implemented and tested on host.**
2. **Modular engine ABI:** design a block-based audio API
   `note_on/note_off/render/set_patch` with per-engine budgets and
   bounded voice counts. Need verified 44.1-kHz DSP, memory and per-block CPU.
3. **TRIBE FM4 / 303 / 808–909:** port and test against fixed reference
   waveforms and host performance. Include complete corresponding source
   and copyright attributions.
4. **Sample/other synthesis:** only import explicitly licensed samples
   (e.g. CC0) and measure memory/bandwidth; do not bundle third-party
   sample banks on copyright assumptions.
5. Recalculate CPU and RAM headroom with native Android emulation and the
   physical JieLi target separately before adding concurrent engine voices.

Important: adding *every* independent engine from several firmwares at once
would exceed practical real-time budget without testing, even when all
source licenses permit code reuse. **Do not claim 28+ extra engines are
present until the actual audio implementations are linked and tested.**
