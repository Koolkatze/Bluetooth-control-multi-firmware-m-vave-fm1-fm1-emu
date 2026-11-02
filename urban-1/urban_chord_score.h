/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 rhythmic chord score. One synchronized chord per 4-bar harmonic
 * timeline, retriggered according to 14 genre grammars. No new DSP voice
 * allocation: HYP-1 already has a 4-note polyphonic PAD synthesizer.
 */
#ifndef URBAN_CHORD_SCORE_H
#define URBAN_CHORD_SCORE_H
#include "urban_harmony.h"
typedef struct { uint16_t onsets; uint8_t gate; uint8_t color; } UrbanChordScore;
typedef struct { uint16_t mask; uint8_t gate; uint8_t variation; } UrbanChordGrammar;
/* Sixteenth grid, bit n = onset on 16th n, in one bar. */
/* Per-subgenre chord rhythm and note length in 16ths.
 * Not all styles need the same pluck/pad comping: perreo has offbeat
 * syncopation, witch house long washes, lo-fi warm holds, neurofunk
 * metallic short stabs, romantic reggaeton sustained triads, psy
 * trance pumping pulses and jazz-liquid syncopated 9th voicings.
 */
static const UrbanChordGrammar urban_chord_grammars[URBAN_GENRES][4]={
 /* Classic trap, drill, ambient, roll */
 {{0x0101,6,1},{0x0801,7,2},{0x0001,16,0},{0x0501,3,2}},
 /* Witch / industrial / darkwave / cloud */
 {{0x0001,16,0},{0x1101,3,2},{0x0101,12,1},{0x0001,16,0}},
 /* Emo / piano / lo-fi / guitar */
 {{0x0001,15,0},{0x0101,8,1},{0x0001,16,0},{0x1011,6,2}},
 /* Boom-bap two-bar sample / jazz 9th syncs / G-funk sustain / lo-fi wash.
  * Longer chords replace toy-like 16th staccato chord retriggers. */
 {{0x0101,8,1},{0x0001,15,0},{0x0001,15,0},{0x0001,16,0}},
 /* Bristol / downtempo / abstract / dub */
 {{0x0881,7,2},{0x0101,9,1},{0x2401,5,3},{0x0401,12,1}},
 /* Underground: looped sevenths; syncopation only on funk/break variants. */
 {{0x0101,9,1},{0x0001,15,0},{0x0001,14,0},{0x0881,7,1}},
 /* Memphis / cinematic / industrial / dark */
 {{0x0101,8,1},{0x0001,16,0},{0x1101,3,2},{0x0801,12,1}},
 /* Dembow / perreo / romantic / futuro */
 {{0x0841,4,2},{0x2841,3,3},{0x0001,15,0},{0x2251,4,3}},
 /* Neuro / jump-up / darkstep / techstep */
 {{0x1041,3,3},{0x0101,6,1},{0x0801,8,2},{0x2101,3,3}},
 /* UK140 / deep / riddim / brostep */
 {{0x0101,4,1},{0x0001,16,0},{0x1111,2,3},{0x2201,4,3}},
 /* Detroit / hypnotic / dub / minimal */
 {{0x4444,2,3},{0x1011,2,3},{0x0001,15,0},{0x0101,4,1}},
 /* Gabber / industrial / frenchcore / uptempo */
 {{0x1111,2,3},{0x5555,2,3},{0x4444,2,3},{0x2222,2,3}},
 /* Progressive / uplifting / psy / tech */
 {{0x4444,4,3},{0x4444,6,3},{0x5555,2,3},{0x2222,3,3}},
 /* Soulful / atmospheric / jazzy liquid / deep */
 {{0x2201,6,2},{0x0001,16,0},{0x0001,14,0},{0x0101,12,1}}
};
static UrbanChordScore urban_chord_score(const UrbanSong *s,
    unsigned section,unsigned bar) {
    UrbanChordScore out;
    const UrbanTrack *tr=&s->tracks[4];
    unsigned genre=tr->genre%URBAN_GENRES,sub=tr->subgenre%4u;
    const UrbanChordGrammar *g=&urban_chord_grammars[genre][sub];
    uint32_t rng=urban_mix_seed(
        tr->seq[section%URBAN_SEQUENCES].seed ^
        s->seed ^ ((bar%4u+1u)*0xa511e9b3u) ^
        ((uint32_t)sub*0x85ebca6bu) ^
        ((uint32_t)s->harmony_variant*0x9e3779b9u));
    out.onsets=g->mask;
    out.gate=g->gate;
    /* Genuine jazz/piano comping: one voiced seventh/ninth held
     * through most of the bar; only an occasional intentional answer.
     * Never treat a pianist as a 16th-note pad retrigger loop. */
    unsigned pianist=(genre==3u && sub==1u) ||
                     (genre==5u && sub==1u) ||
                     (genre==13u && sub==2u) ||
                     (genre==2u && sub==1u);
    if(pianist) {
        out.onsets=(uint16_t)((bar%4u==3u &&
                (section==URBAN_HOOK||section==URBAN_VERSE_B))?
                0x0101u:0x0001u);
        out.gate=bar%4u==3u?8u:15u;
    }
    /* Track 4 may be independently overridden: its rhythm and voicing
     * follow its own subgenre, while the song's shared harmonic roots
     * remain in the global key/mode so mixed tracks cannot clash. */
    out.color=(genre==s->genre%URBAN_GENRES &&
               sub==s->subgenre%4u)?
                s->chord_colors[bar%4u]:
                urban_substyle_chord_colors[genre][sub][bar%4u];
    if(section==URBAN_INTRO||section==URBAN_BRIDGE||
       section==URBAN_BREAK||section==URBAN_OUTRO) {
        out.onsets=(uint16_t)(out.onsets&0x1111u);
        out.gate=out.gate<12u?12u:out.gate;
        if(!out.onsets)out.onsets=1u;
    }else if(section==URBAN_HOOK||section==URBAN_DROP){
        if(g->variation>=2u && s->complexity>=2u)
            out.onsets|=0x0001u;
    }
    /* One real per-take rhythmic decision: alter a weaker on/offbeat.
     * Anchor notes remain characteristic of the selected genre. */
    /* Prevent generic stochastic chord edits from turning jazz back
     * into hurried staccato; ambient's existing sustained wash stays. */
    if(pianist) {
        if(out.gate>16u)out.gate=16u;
        return out;
    }
    static const uint8_t position[8]={3,5,6,7,9,11,13,14};
    if(g->variation && s->complexity) {
        unsigned step=position[urban_rng(&rng)%8u];
        if((urban_rng(&rng)%100u)<15u*s->complexity)
            out.onsets^=(uint16_t)(1u<<step);
    }
    /* Chord colour changes by bar and section, but always follows the
     * song's selected scale degrees via urban_build_chord(). */
    if(section==URBAN_BRIDGE && s->complexity>=2u)
        out.color=URBAN_CH_SUS;
    if(section==URBAN_DROP && s->complexity>=2u &&
       (tr->genre==10u||tr->genre==11u))
        out.color=URBAN_CH_POWER;
    if(out.gate>16u)out.gate=16u;
    return out;
}
/* How a player depresses chord keys: simultaneous synth block chords
 * for ambient, authentic ascending/descending 12–30 ms rolls for piano
 * and jazz, tighter smaller offsets for soul/funk. All notes retain
 * one shared tonal progression and the original chord envelope. */
enum { URBAN_CH_TOUCH_BLOCK=0, URBAN_CH_TOUCH_UP=1,
       URBAN_CH_TOUCH_DOWN=2 };
typedef struct { uint8_t touch,spacing_ms; } UrbanChordExpression;
static UrbanChordExpression urban_chord_expression(const UrbanSong *s,
              unsigned section,unsigned bar) {
    const UrbanTrack *t=&s->tracks[4];
    unsigned g=t->genre%URBAN_GENRES,sub=t->subgenre%4u;
    UrbanChordExpression x={URBAN_CH_TOUCH_BLOCK,0u};
    /* These ambient substyles intentionally retain unrolled,
     * long-form pads/washes exactly as before. */
    if((g==1u && (sub==0u||sub==3u)) ||
       (g==2u && sub==2u) ||
       (g==4u && sub==1u) ||
       (g==5u && sub==2u) ||
       (g==7u && sub==2u) ||
       (g==9u && sub==1u) ||
       (g==10u && sub==2u) ||
       (g==13u && sub==1u))return x;
    uint32_t v=urban_mix_seed(t->seq[section%URBAN_SEQUENCES].seed ^
                          ((bar+1u)*0x85ebca6bu));
    if((g==3u && sub==1u)||(g==5u && sub==1u)||
       (g==13u && sub==2u)||(g==2u && sub==1u)) {
        x.touch=(uint8_t)(v&1u?URBAN_CH_TOUCH_UP:URBAN_CH_TOUCH_DOWN);
        x.spacing_ms=(uint8_t)(14u+(v>>4u)%11u);
    } else if((g==3u && sub==2u)||(g==7u && sub==0u)||
              (g==13u && sub==0u)) {
        x.touch=URBAN_CH_TOUCH_UP;
        x.spacing_ms=(uint8_t)(7u+v%5u);
    }
    return x;
}
#endif
