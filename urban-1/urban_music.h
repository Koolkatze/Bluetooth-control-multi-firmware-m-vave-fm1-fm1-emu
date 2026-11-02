/* SPDX-License-Identifier: GPL-3.0-only
 * Genre-aware melodic composition with one common 4-bar harmony.
 * Core rhythm + motif remain recognizable across repeated hooks;
 * section variation is arrangement-aware rather than independent noise.
 */
#ifndef URBAN_MUSIC_H
#define URBAN_MUSIC_H
#include "urban_rhythm.h"
typedef struct { uint8_t note, velocity, gate; } UrbanNote;
typedef struct { UrbanNote steps[URBAN_STEPS]; uint32_t seed; } UrbanPhrase;
typedef struct { UrbanPhrase sequence[URBAN_SEQUENCES]; } UrbanVoice;
static const uint8_t urban_minor_scale[7]={0,2,3,5,7,8,10};
/* Shared 10-mode scale table and harmonic plan live in urban_song.h. */
typedef struct {
    uint16_t lead, chord, arp, texture;
    uint8_t density, accent, gate, octave;
} UrbanMelodyGrammar;
/* Each lead mask encodes a deliberate rhythmic archetype, not genre-independent
 * random notes. Four onsets at structurally important times survive sections.
 */
static const UrbanMelodyGrammar urban_melodic[URBAN_GENRES]={
    {0x0921,0x0101,0x5555,0x0101,25,86,4,0}, /* Trap: spacious 808-friendly melody */
    {0x0841,0x0001,0x5454,0x0101,21,77,7,0}, /* Goth trap: distant sparse minor */
    {0x0881,0x0001,0x4444,0x0101,19,68,8,0}, /* Sad trap: held emotive figures */
    {0x2489,0x0101,0x5555,0x0111,38,82,3,0}, /* Hip-hop: syncopated phrasing */
    {0x1221,0x0001,0x4444,0x0101,27,71,7,0}, /* Trip-hop: broken/downtempo */
    {0x1489,0x0101,0x4545,0x0111,34,74,4,0}, /* Underground jazz-influenced */
    {0x0901,0x0001,0x5555,0x0101,26,88,4,0}, /* Horrorcore: tense repetitions */
    {0x2229,0x1111,0x5555,0x1010,43,84,3,0}, /* Reggaeton: tresillo melodic */
    {0x9249,0x0001,0xaaaa,0x0101,58,91,2,0}, /* DnB: syncopated stabs */
    {0x0841,0x0001,0x5555,0x0101,25,94,5,0}, /* Dubstep: space around drops */
    {0x2222,0x1111,0xaaaa,0x0101,48,85,4,0}, /* Techno: cyclical stabs */
    {0x5555,0x1111,0x5555,0x1111,66,98,2,0}, /* Hardcore: relentless rhythm */
    {0x8888,0x0101,0xaaaa,0x0101,51,82,5,1}, /* Trance: soaring offbeat/arps */
    {0x2a49,0x0101,0xaaaa,0x0101,47,77,6,0}  /* Liquid: flowing intervals */
};
static uint8_t urban_density(uint8_t genre,unsigned track) {
    unsigned d=urban_melodic[genre%URBAN_GENRES].density;
    if(track==2u)d=d/2u;
    if(track==4u||track==5u)d=d/3u;
    return (uint8_t)d;
}
static uint16_t urban_section_mask(uint16_t base,unsigned section) {
    switch(section) {
    case URBAN_INTRO: return (uint16_t)(base&0x1111u);
    case URBAN_VERSE_A: return base;
    case URBAN_HOOK: return (uint16_t)(base|0x1010u);
    case URBAN_VERSE_B: return (uint16_t)(base|0x0404u);
    case URBAN_BRIDGE: return (uint16_t)(base&0x4445u);
    case URBAN_DROP: return (uint16_t)(base|0x5555u);
    case URBAN_BREAK: return (uint16_t)(base&0x0101u);
    case URBAN_OUTRO: return (uint16_t)(base&0x1111u);
    default: return base;
    }
}
/* Genre grammar gives a recognizable rhythm; these rhythmic gestures
 * are chosen from a song-dependent family, so repeated SAVE presses change
 * the actual note onsets, not just velocities. The same seed repeats exactly
 * when a hook returns within one song.
 */
static uint16_t urban_melodic_rhythm(const UrbanSong *song,
     unsigned track,unsigned section,uint16_t base,uint32_t seed) {
    uint32_t rng=urban_mix_seed(seed ^ song->seed ^
       ((uint32_t)track*0xa511e9b3u) ^
       ((uint32_t)(song->melodic_variant+1u)*0x7f4a7c15u));
    uint16_t mask=urban_section_mask(base,section);
    if(track<=1u)return mask;
    /* Vary the offbeats, not the primary pulse: note rhythm is still
     * recognizably boom bap, trap, trance or breakbeat. */
    static const uint8_t ornament_positions[8]={2,3,5,6,7,9,11,14};
    unsigned density=urban_density(song->tracks[track].genre,track);
    unsigned picks=track==4u?1u:(song->complexity+1u);
    for(unsigned i=0;i<picks;i++) {
        unsigned step=ornament_positions[urban_rng(&rng)%8u];
        uint16_t bit=(uint16_t)(1u<<step);
        if((urban_rng(&rng)%100u)<density+24u)
            mask^=bit;
    }
    /* Never erase all the rhythmic identity. Bass uses a separate grammar;
     * for chords, the first beat always establishes harmonic function. */
    if(track==4u) mask|=1u;
    if(!mask)mask=1u;
    return mask;
}
static void urban_generate_voice(const UrbanSong *song,unsigned track,
                                  uint32_t seed,UrbanVoice *out) {
    if(track>=URBAN_TRACKS)return;
    const UrbanTrack *cfg=&song->tracks[track];
    const UrbanMelodyGrammar *g=&urban_melodic[cfg->genre%URBAN_GENRES];
    uint32_t motif_rng=urban_mix_seed(seed ^
               ((uint32_t)song->melodic_variant*0xc2b2ae35u) ^
               ((uint32_t)(track+1u)*0x9e3779b9u));
    uint8_t motif[8];
    uint8_t anchor=(uint8_t)(urban_rng(&motif_rng)%7u);
    motif[0]=anchor;
    /* Family of contour gestures: descending, ascending, question/answer,
     * angular, hypnotic repetition, scalar, leaps, arpeggiation.
     * 8 independent slots give a real composition rather than four canned
     * root/third/fifth notes repeatedly assigned to the same 16 steps.
     */
    int contour=(int)(song->melodic_variant%8u);
    for(unsigned i=1u;i<8u;i++) {
        unsigned r=urban_rng(&motif_rng)%7u;
        int step=contour==0?1:
                 contour==1?-1:
                 contour==2?(i%2u?2:-1):
                 contour==3?(i%2u?-2:3):
                 contour==4?0:
                 contour==5?(int)(r%3u)-1:
                 contour==6?(int)(r%5u)-2:
                 (i%2u?2:-2);
        int next=(int)motif[i-1]+step;
        if(next<0)next+=7;
        motif[i]=(uint8_t)(next%7);
    }
    for(unsigned section=0;section<URBAN_SEQUENCES;section++){
        UrbanPhrase *phrase=&out->sequence[section];
        uint32_t rng=urban_mix_seed(seed ^
            ((section+1u)*0x85ebca6bu) ^
            ((uint32_t)cfg->subgenre*0xc2b2ae35u));
        phrase->seed=rng;
        uint16_t base=track<=1u?0u:track==3u?g->lead:
            track==4u?g->chord:track==5u?g->texture:g->arp;
        uint16_t mask=urban_melodic_rhythm(song,track,section,base,seed);
        unsigned density=urban_density(cfg->genre,track);
        for(unsigned step=0;step<URBAN_STEPS;step++){
            UrbanNote *n=&phrase->steps[step];
            uint32_t v=urban_rng(&rng);
            unsigned motif_index=(step/2u)%8u;
            unsigned degree=motif[motif_index];
            /* A repeated chorus uses the same motif with a contrasting
             * register. Bridge and outro answer it in a lower register.
             * At each strong onset, select a root/3rd/5th/7th chord tone.
             */
            if(step%4u==0u) {
                static const uint8_t chord_degrees[4]={0,2,4,6};
                degree=chord_degrees[(step/4u+motif[0])%4u];
            } else if((v%13u)==0u && song->complexity>1u)
                degree=(degree+1u)%7u;
            if((cfg->genre==3u||cfg->genre==5u||
                cfg->genre==13u) && step%2u)
                degree=(degree+2u)%7u; /* jazz/neo-soul turns */
            unsigned octave=track==4u?48u:track==5u?60u:
                track==2u?36u:60u+(g->octave*12u);
            if((section==URBAN_BRIDGE||section==URBAN_OUTRO) && octave>=60u)
                octave-=12u;
            /* Genre and song both constrain note choice to its mode. */
            n->note=(uint8_t)(octave+song->key%12u+
                 urban_scale_intervals[song->scale%URBAN_SCALE_COUNT][degree]);
            uint8_t on=(uint8_t)((mask>>step)&1u);
            if(!on && (section==URBAN_HOOK||section==URBAN_DROP) &&
                 step>=12u && v%100u<
                 (density/9u*song->complexity + cfg->subgenre%4u))
                on=1u;
            n->velocity=on?(uint8_t)(g->accent+(v%22u)):0u;
            n->gate=(uint8_t)(track==4u?12u:track==5u?10u:
                             g->gate+(v%3u));
            if(track<=1u) {
                n->note=urban_drum_note(cfg->genre,track,step);
                n->velocity=urban_drum_hit(cfg->genre,cfg->subgenre,
                              track,step,section,&rng)?108u:0u;
                n->gate=1u;
            }
        }
    }
}
#endif
