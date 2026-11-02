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
static const UrbanChordGrammar urban_chord_grammars[URBAN_GENRES]={
    {0x0101u, 6u,1},  /* trap: half-time minor keys, plucked */
    {0x0001u,15u,0},  /* goth: extended held dark pads */
    {0x0001u,15u,0},  /* sad trap: piano / emotional sustained voicings */
    {0x1101u, 3u,2},  /* hip-hop: offbeat Rhodes stabs */
    {0x0881u, 6u,2},  /* trip-hop: lazy offbeats */
    {0x2101u, 3u,2},  /* underground: jazzy comp */
    {0x0001u,13u,0},  /* horrorcore: dissonant atmosphere */
    {0x0841u, 4u,2},  /* reggaeton: syncopated montuno / chords */
    {0x1041u, 3u,3},  /* DnB: syncopated stabs */
    {0x0101u, 4u,1},  /* dubstep: halftime spacious */
    {0x4444u, 2u,3},  /* techno: eighth-note offbeat chords */
    {0x1111u, 2u,3},  /* hardcore: four-on-the-floor stabs */
    {0x4444u, 3u,3},  /* trance: offbeat supersaw */
    {0x2201u, 6u,2}   /* liquid: soulful chords */
};
static UrbanChordScore urban_chord_score(const UrbanSong *s,
    unsigned section,unsigned bar) {
    UrbanChordScore out;
    const UrbanTrack *tr=&s->tracks[4];
    const UrbanChordGrammar *g=&urban_chord_grammars[tr->genre%URBAN_GENRES];
    uint32_t rng=urban_mix_seed(
        tr->seq[section%URBAN_SEQUENCES].seed ^
        s->seed ^ ((bar%4u+1u)*0xa511e9b3u) ^
        ((uint32_t)s->harmony_variant*0x9e3779b9u));
    out.onsets=g->mask;
    out.gate=g->gate;
    out.color=s->chord_colors[bar%4u];
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
#endif
