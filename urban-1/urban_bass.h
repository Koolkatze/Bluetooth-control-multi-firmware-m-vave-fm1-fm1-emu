/* SPDX-License-Identifier: GPL-3.0-only
 * 14 genre bass archetypes. The four-bar chord timeline is applied by
 * urban_runtime_tick, so every bar advances harmony without re-rolling the
 * underlying motif. 808 slide is retained as synthesis metadata.
 */
#ifndef URBAN_BASS_H
#define URBAN_BASS_H
#include "urban_song.h"
typedef struct { uint8_t note,velocity,gate,glide; } UrbanBassStep;
typedef struct { UrbanBassStep steps[URBAN_STEPS]; } UrbanBassPhrase;
typedef struct { UrbanBassPhrase seq[URBAN_SEQUENCES]; } UrbanBassSong;
typedef struct { uint16_t mask; uint8_t gate,octave,accent,slide; } UrbanBassGrammar;
static const UrbanBassGrammar urban_bass_grammars[URBAN_GENRES]={
    {0x0429,8,0,100,42}, /* Trap long 808s and end-of-bar slides */
    {0x0821,9,0, 93,40}, /* Goth trap: sparse sub */
    {0x0109,12,0,88,34}, /* Sad trap: long 808s */
    {0x1249,4,1, 85, 0}, /* Hip-hop: syncopated electric bass */
    {0x0441,7,0, 78, 0}, /* Trip-hop: pulsing low end */
    {0x1289,4,1, 83, 0}, /* Underground: busy live-style */
    {0x0481,9,0, 93,31}, /* Horrorcore: sub and low slide */
    {0x1119,3,1, 93, 0}, /* Reggaeton: kick/dembow sync */
    {0x4a41,2,1, 95, 0}, /* DnB: syncopated sub/reese accents */
    {0x0401,11,0,104,36},/* Dubstep: long sub, drop movement */
    {0x2222,3,1, 85, 0}, /* Techno: offbeat bass after kick */
    {0x5555,2,1,104, 0}, /* Hardcore techno: driving eighths */
    {0x2222,4,1, 92, 0}, /* Trance: rolling offbeats */
    {0x2a49,5,1, 81, 0}  /* Liquid jungle: smooth syncopation */
};
static uint16_t urban_bass_section_mask(uint16_t mask,unsigned sec) {
    if(sec==URBAN_INTRO || sec==URBAN_OUTRO) return (uint16_t)(mask&0x1111u);
    if(sec==URBAN_BRIDGE || sec==URBAN_BREAK)return (uint16_t)(mask&0x0101u);
    if(sec==URBAN_DROP)return (uint16_t)(mask|0x0404u);
    if(sec==URBAN_HOOK)return (uint16_t)(mask|0x1000u);
    if(sec==URBAN_VERSE_B)return (uint16_t)(mask|0x0800u);
    return mask;
}
static void urban_generate_bass(const UrbanSong *song,uint32_t seed,UrbanBassSong *out) {
    const UrbanTrack *track=&song->tracks[2];
    const UrbanBassGrammar *style=&urban_bass_grammars[track->genre%URBAN_GENRES];
    uint32_t root=seed ^ 0x5f356495u;
    uint8_t mode=song->scale%4u;
    static const uint8_t passing[4][3]={
        {0,3,7},{0,1,7},{0,3,7},{0,3,7}
    };
    uint8_t phrase_note[4];
    for(unsigned i=0;i<4u;i++)
        phrase_note[i]=passing[mode][urban_rng(&root)%3u];
    for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++) {
        uint32_t r=root ^ ((sec+1u)*0x85ebca6bu) ^
                 ((uint32_t)track->subgenre*0xc2b2ae35u);
        uint16_t mask=urban_bass_section_mask(style->mask,sec);
        /* Subgenre bass arrangement: preserve the parent genre's bass
         * identity, but alter its syncopation and phrasing audibly. */
        switch(track->subgenre%4u) {
        case 1u:
            if(track->genre==0u||track->genre==7u||track->genre==8u)
                mask|=0x0808u; /* drill/perreo/jump-up */
            else if(track->genre>=10u)mask|=0x4444u;
            else mask|=0x0404u;
            break;
        case 2u:
            if(track->genre==9u||track->genre<=2u||track->genre==13u)
                mask&=0x1111u; /* deep/ambient, leave harmonic space */
            else mask|=0x0101u;
            break;
        case 3u:
            if(track->genre==0u||track->genre==9u||track->genre==11u)
                mask|=0x2222u;
            else mask|=0x1010u;
            break;
        default: break;
        }
        for(unsigned step=0;step<URBAN_STEPS;step++) {
            UrbanBassStep *n=&out->seq[sec].steps[step];
            uint32_t v=urban_rng(&r);
            uint8_t hit=(uint8_t)((mask>>step)&1u);
            unsigned root_note=36u+(song->key%12u);
            /* Strong beats take tonic. Offbeats use notes established in the
             * SONG'S modal palette; all tracks share the chord transposition. */
            unsigned interval=step%4u==0u?0u:phrase_note[(step/4u)%4u];
            if(style->octave && (v%5u==0u))interval+=12u;
            if(song->complexity>=2u && track->subgenre%4u==3u && step==15u &&
               (sec==URBAN_HOOK || sec==URBAN_DROP))hit=1u;
            n->note=(uint8_t)(root_note+interval);
            n->velocity=hit?(uint8_t)(style->accent+(v%15u)):0u;
            n->gate=(uint8_t)(style->gate+(v%4u));
            n->glide=(uint8_t)(hit&&style->slide&&step>=8u &&
                               v%100u<(unsigned)style->slide+(track->subgenre%4u)*3u);
            /* Slides need a target pitch other than the sustained tonic. */
            if(n->glide && interval==0u)n->note=(uint8_t)(root_note+7u);
        }
    }
}
#endif
