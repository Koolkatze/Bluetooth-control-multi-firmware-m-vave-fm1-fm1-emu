# URBAN-1 — firmware development (prototype)

Status: **architecture / initial implementation**, not a flashable FM-1 firmware.

URBAN-1 is a separate multigenre generative groovebox project. JIANT Genre Engine is deferred.

## Pages

### SONG
- Generate Song creates one shared musical identity (tempo, key, harmonic plan) and 8 related sequences A–H.
- Each sequence contains parts for all tracks. Song arrangement is an ordered list of sequence references with repeats.
- Sequence performance mode: turn **NEXT** knob to queue A–H; while playing, the queued sequence begins **only at the end of the current sequence**. When stopped, selection takes effect immediately.
- Editing the arrangement does not regenerate any track.

### TRACKS
- Shows every track and its genre/subgenre.
- Select a track and generate **all eight sequence parts for that track only**.
- Every other track, its settings, patterns and seeds remain byte-for-byte unchanged; song arrangement and global harmony remain unchanged.
- Track regeneration follows global tempo/key/harmony; a later optional Free mode may relax harmonic constraints.
- Each track supports lock, mute, solo, and per-track regeneration seed.
- Regeneration supports undo (one snapshot minimum).

## Initial track roles
Drums, hats, bass, melody, chords, atmosphere. Up to 8 logical tracks, with active synthesis voices constrained by CPU/RAM measurements.

## Genre families
Trap, Goth trap, Sad trap, Hip-hop, Trip-hop, Underground hip-hop, Horrorcore, Reggaeton, Drum and bass, Dubstep, Techno, Hardcore techno, Trance, Liquid jungle.

Genre and subgenre are selected separately at song and track levels. A song genre establishes overall musical constraints; track genre selects rhythmic vocabulary and instrument palette.

## Engineering milestones
1. Deterministic shared song state and isolated track regeneration.
2. Pattern/song playback and NEXT knob bar/sequence boundary queue.
3. Import/fork HYP-1 GPL-3.0 source with attribution and licensing intact; adapt its platform, renderer, sequencer and simulator.
4. Implement distinct genre grammars, sound banks, bass/808 synthesis and section-aware motif development.
5. Host simulator tests, CPU/audio profiling, then firmware toolchain build and physical FM-1 validation.

Source inspiration: https://github.com/RustyPawz/fm1-HYP1 (GPL-3.0-only). **No HYP-1 source has yet been copied into this directory.**
