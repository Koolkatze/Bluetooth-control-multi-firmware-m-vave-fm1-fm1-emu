# URBAN-1 — Hip-hop production audit (v0.3.7)

This audit applies to **Hip-hop** (Boom Bap, Jazz Rap, G-Funk, Lo-fi)
and **Underground Hip-hop** (Raw/Dusty, Jazz Underground, Experimental,
Breakbeat) in the existing FM-1/HYP-1 synthesis backend.

## What was sounding synthetic or toy-like

1. Boom Bap and Raw/Dusty were assigned HYP's **Techno** kit, although
   HYP has a dedicated **Break** kit and a separate 808 kit.
2. Hip-hop bass patches could select metallic FM, sync, high fifths and
   bright presets on regeneration; the bass score also inserted random
   octave jumps and unstructured passing notes.
3. The default ACID lead bank let bright Bell, Pluck, Chip and other
   electronic synth voices randomly replace a selected substyle patch.
4. A lead was often built as a staccato 16th-note pattern regardless of
   whether the beat needed a foreground melody. Chords had too many
   retriggers, especially on boom-bap and underground sample-like parts.
5. The HYP-1 port always set **Swing = 0**, even though genre patterns
   declared hip-hop swing amounts; kick/snare edits and stacked claps
   could reinforce a mechanical or glitchy feel.
6. The drum kit's HYP stochastic chop had previously been disabled in the
   URBAN port; that safeguard stays enabled.

## Production decisions

| Style | Rhythm and feel | Bass | Harmony / melody |
| --- | --- | --- | --- |
| Boom Bap | Break kit, eighth-note hats, dry 2/4 snare, around 24 ms late off-sixteenths at 90 BPM | Warm or organ-like low register, tonic and occasional fifth | Two-bar i–iv minor-seventh loop, brief chord-tone answer, usually no foreground lead |
| Jazz Rap | Airier break with an offbeat kick and optional light percussion, around 21 ms | Rounded bass with structured diatonic approaches | Rhodes, seventh/ninth shells, syncopated comp, infrequent mellow organ melody |
| G-Funk | 808-style kit, relatively straight syncopation and restrained clap | Rounded funk bass without random octave leaps | Sustained organ/analog pads, the only hip-hop variant featuring a regularly present PWM lead with slower decay/slide |
| Lo-fi | Sparse break beat, less bright hats and mild timing lag | Sustained low fundamental | Darker Rhodes chords, fewer retriggers, filtered/soft motion and subtle noise |
| Underground | Break-derived 2/4 beats with style-specific fills and restrained accents | Low warm sounds, no involuntary high jumps | Sample-loop style sevenths, Rhodes/organ as default; occasional deliberate melodic fills |

HYP's *Swing* control delays odd sixteenth drum events by
`0.005 × parameter × sixteenth duration`; at 90 BPM the Boom Bap
value 29 produces about 24 ms delay. Main snares remain on the even
sixteenth positions (2 and 4). This does **not** generate extra drum hits.

All styles retain four-bar harmonically synchronized phrases, independent
track patch choices, and headroom-controlled mixing.

## What this does not claim

The built-in HYP PAD, DRONE and ACID engines are synthetic. In particular,
**Rhodes**, **Break** and **808** are the names of HYP synthesis presets;
they are not recorded Rhodes pianos or licensed drum samples. The sound is
closer to the reference production method within these limits, but cannot
perfectly reproduce sampled soul/jazz records without integrating a real
sampler and licensed sounds.

The HYP backend has one shared reverb/echo path; it cannot independently
send each track to a full DAW effects chain. HYP also ignores per-note MIDI
velocity in its `en_note()` synth API. This revision therefore avoids
claims of real sample dynamics or sample slicing.

## Reproducible technical checks

Run `make -C urban-1 test`, including `urban_hiphop_test`. The latter
validates 2 genre families × 4 substyles × 16 seeds: stable drum masks,
low-register bass without random octave spikes, restrained lead timbres,
proper harmonic gating, filtered FX, and per-substyle program assignments.
The host smoke test builds the real pinned HYP-1 DSP and synthesizes output,
and the FM-1 firmware CI cross-compiles/tests the `.fwsc` package.

These are algorithmic regressions, **not** listening tests. Subjective
approval in the user's Android emulator remains necessary.

## Reference production characteristics

- Native Instruments, *What is boom bap? How to produce a 90s-style hip
  hop beat*: https://www.native-instruments.com/blogs/blog/what-is-boom-bap/
- Boom Bap rhythm and backbeat reference:
  https://www.rhythmdictionary.com/p/boom-bap
- Lo-fi hip-hop reference (swing, warm bass, minor sevenths, band-limiting):
  https://ableton.onefootstep.us/genres/lofi-hiphop.html
