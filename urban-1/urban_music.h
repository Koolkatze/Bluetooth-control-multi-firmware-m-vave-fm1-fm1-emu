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
static const uint8_t urban_scale_intervals[4][7] = {
  {0,2,3,5,7,8,10}, /* natural minor */
  {0,1,3,5,7,8,10}, /* Phrygian */
  {0,2,3,5,7,9,10}, /* Dorian */
  {0,2,3,5,7,8,11}  /* harmonic minor */
};
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
static void urban_generate_voice(const UrbanSong *song,unsigned track,
                                  uint32_t seed,UrbanVoice *out) {
    if(track>=URBAN_TRACKS)return;
    const UrbanTrack *cfg=&song->tracks[track];
    const UrbanMelodyGrammar *g=&urban_melodic[cfg->genre%URBAN_GENRES];
    uint32_t root=seed ^ ((track+1u)*0x9e3779b9u);
    uint8_t motif[4];
    for(unsigned i=0;i<4;i++)motif[i]=(uint8_t)(urban_rng(&root)%7u);
    for(unsigned s=0;s<URBAN_SEQUENCES;s++){
        UrbanPhrase *phrase=&out->sequence[s];
        uint32_t r=root ^ ((s+1u)*0x85ebca6bu) ^
                  ((uint32_t)cfg->subgenre*0xc2b2ae35u);
        phrase->seed=r;
        uint16_t base=track<=1u?0u:track==3u?g->lead:
            track==4u?g->chord:track==5u?g->texture:g->arp;
        uint16_t mask=urban_section_mask(base,s);
        unsigned density=urban_density(cfg->genre,track);
        for(unsigned step=0;step<URBAN_STEPS;step++){
            UrbanNote *n=&phrase->steps[step];
            uint32_t v=urban_rng(&r);
            unsigned idx=(step/4u)%4u;
            unsigned degree=motif[idx];
            /* On the strongest beats outline tonic/third/fifth, then borrow
             * passing scale tones from the song's *shared* modal vocabulary. */
            if(step%4u==0u)degree=(idx==0u?0u:idx==1u?2u:idx==2u?4u:motif[idx]);
            else if(step%2u && (v%7u)==0u)degree=(degree+1u)%7u;
            unsigned pitch=(unsigned)urban_scale_intervals[song->scale%4u][degree];
            unsigned octave=track==4u?48u:track==5u?60u:
                            track==2u?36u:60u+(g->octave*12u);
            n->note=(uint8_t)(octave+song->key%12u+pitch);
            /* Strong genre onsets are preserved, sparse optional ornaments
             * are deterministic and bounded by subgenre and section energy. */
            uint8_t on=(uint8_t)((mask>>step)&1u);
            if(!on && (s==URBAN_HOOK || s==URBAN_DROP) &&
               step>8u && v%100u < (density/10u*song->complexity + cfg->subgenre%4u))
                on=1u;
            n->velocity=on?(uint8_t)(g->accent+(v%24u)):0u;
            n->gate=(uint8_t)(track==4u?10u:track==5u?12u:
                             g->gate+(v%3u));
            if(track<=1u) {
                n->note=urban_drum_note(cfg->genre,track,step);
                n->velocity=urban_drum_hit(cfg->genre,cfg->subgenre,
                                           track,step,s,&r)?108u:0u;
                n->gate=1u;
            }
        }
    }
}
#endif
