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
typedef struct {
    uint16_t mask;
    uint8_t gate,octave,accent,slide,motion;
} UrbanBassGrammar;
/* 56 bass partitures, not one family pattern with a couple of extra
 * hi-hats. A style defines real onset grid, note length, octave, attack,
 * glide chance and interval grammar:
 * motion 0=tonic/fifth, 1=melodic sliding 808, 2=walking/jazz,
 * 3=octave-funk groove, 4=sub drone, 5=mid-bass/Reese movement.
 */
static const UrbanBassGrammar urban_bass_profiles[URBAN_GENRES][4]={
 /* Trap: 808 / Drill / Ambient / Roll */
 {{0x0429,11,0,101,32,0},{0x0a81,9,0,108,86,1},
  {0x0101,16,0,78,6,4},{0x1429,5,0,105,55,1}},
 /* Goth: Witch / Industrial / Darkwave / Cloud */
 {{0x0821,12,0,91,25,4},{0x0921,5,0,107,30,5},
  {0x1111,4,1,88,13,3},{0x0101,16,0,76,8,4}},
 /* Sad: Emo / Piano / Lo-fi / Guitar */
 {{0x0109,13,0,83,20,4},{0x0101,14,0,78,8,4},
  {0x0809,8,0,74,4,0},{0x0009,12,0,80,16,4}},
 /* Hip-hop: deeper, steadier bass notes with no incidental octave kicks.
  * Boom-bap low fifth, jazz approach, G-Funk syncopation, lo-fi sustain. */
 {{0x0489,7,0,83,0,0},{0x0489,6,0,83,0,2},
  {0x0889,8,0,90,12,3},{0x0109,12,0,75,0,4}},
 /* Trip-hop: Bristol / Downtempo / Abstract / Dub */
 {{0x0441,8,0,79,0,4},{0x0109,11,0,74,0,4},
  {0x0a21,4,0,88,9,2},{0x0441,12,0,81,22,4}},
 /* Underground: low sample-loop foundations, deliberate passing notes. */
 {{0x0489,7,0,82,0,0},{0x0489,6,0,83,0,2},
  {0x0809,10,0,79,0,4},{0x048b,6,0,91,0,3}},
 /* Horrorcore: Memphis / Cinematic / Industrial / Dark */
 {{0x0481,10,0,96,30,4},{0x0101,16,0,75,0,4},
  {0x0889,6,0,107,32,5},{0x0a81,11,0,96,38,1}},
 /* Reggaeton: Dembow / Perreo / Romantic / Futuro */
 {{0x1119,4,1,96,0,3},{0x1199,3,1,102,12,3},
  {0x1109,9,0,75,0,2},{0x1299,4,1,99,16,3}},
 /* Drum & Bass: Neurofunk / Jump-up / Darkstep / Techstep */
 {{0x4a41,3,1,105,22,5},{0x4444,4,1,109,33,3},
  {0x0401,13,0,99,25,4},{0x2a49,4,1,104,15,5}},
 /* Dubstep: UK 140 / Deep / Riddim / Brostep */
 {{0x0401,12,0,106,37,1},{0x0101,16,0,81,10,4},
  {0x2021,5,0,104,39,5},{0x0c81,4,1,113,58,5}},
 /* Techno: Detroit / Hypnotic / Dub / Minimal */
 {{0x4444,3,1,83,0,3},{0x4444,4,1,78,8,0},
  {0x0101,14,0,77,0,4},{0x4444,2,1,78,0,0}},
 /* Hardcore: Gabber / Industrial / Frenchcore / Uptempo */
 {{0x5555,2,1,108,0,3},{0x1111,4,1,112,25,5},
  {0x4444,3,1,110,0,3},{0x5555,2,1,115,0,5}},
 /* Trance: Progressive / Uplifting / Psy / Tech */
 {{0x4444,4,1,91,0,3},{0x4444,5,1,90,0,3},
  {0x5555,2,1,104,13,5},{0x4444,3,1,101,0,0}},
 /* Liquid Jungle: Soulful / Atmos / Jazz / Deep */
 {{0x2a49,6,1,83,0,2},{0x0101,15,0,75,0,4},
  {0x2689,5,1,89,0,2},{0x0109,12,0,77,0,4}}
};
static UrbanBassGrammar urban_bass_style(const UrbanTrack *t) {
    return urban_bass_profiles[t->genre%URBAN_GENRES][t->subgenre%4u];
}
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
    UrbanBassGrammar preset=urban_bass_style(track);
    const UrbanBassGrammar *style=&preset;
    uint32_t root=seed ^ 0x5f356495u;
    uint8_t phrase_note[4];
    for(unsigned i=0;i<4u;i++) {
        unsigned degree=(urban_rng(&root)%3u)*2u;
        phrase_note[i]=urban_scale_intervals[song->scale%URBAN_SCALE_COUNT][degree];
    }
    for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++) {
        uint32_t r=root ^ ((sec+1u)*0x85ebca6bu) ^
                 ((uint32_t)track->subgenre*0xc2b2ae35u);
        uint16_t mask=urban_bass_section_mask(style->mask,sec);
        /* Distinct bass motif even at the same genre/subgenre. Keep the
         * original root downbeat while selecting different syncopations
         * and turnaround pickups for each newly generated song. */
        uint32_t riff=urban_mix_seed(seed ^ song->seed ^
              ((uint32_t)sec*0xa511e9b3u));
        static const uint8_t pickup[7]={3,5,6,7,9,11,14};
        if(track->genre==3u || track->genre==5u) {
            /* No independent random bass skips in a sample-like loop.
             * The selected pattern stays recognizable in every bar. */
            if(song->complexity>=3u && (urban_rng(&riff)&1u))
                mask|=0x2000u;
        } else for(unsigned n=0;n<1u+(song->complexity>1u);n++) {
            unsigned st=pickup[urban_rng(&riff)%7u];
            if(urban_rng(&riff)&1u)mask|=(uint16_t)(1u<<st);
            else mask&=(uint16_t)~(1u<<st);
        }
        /* Preserve characteristic offbeats in techno/trance (2,6,10,14)
         * rather than forcing a genre-agnostic tonic on step zero. */
        if(track->genre!=10u && track->genre!=12u)
            mask|=1u;
        for(unsigned step=0;step<URBAN_STEPS;step++) {
            UrbanBassStep *n=&out->seq[sec].steps[step];
            uint32_t v=urban_rng(&r);
            uint8_t hit=(uint8_t)((mask>>step)&1u);
            unsigned root_note=36u+(song->key%12u);
            /* Strong beats take tonic. Offbeats use notes established in the
             * SONG'S modal palette; all tracks share the chord transposition. */
            const uint8_t *scale=urban_scale_intervals[song->scale%URBAN_SCALE_COUNT];
            unsigned degree=0u;
            switch(style->motion) {
            case 1u: /* Sliding 808: tonic, minor-third, fifth, turnaround */
                degree=(step>=12u && v%3u==0u)?4u:
                       step%4u==0u?0u:(step%3u==0u?2u:4u);
                break;
            case 2u: /* Jazz/walking: diatonic passing tones */
                degree=(step/4u+((v>>4)&1u))%4u==0u?0u:
                       (step/4u+((v>>4)&1u))%4u==1u?2u:
                       (step/4u+((v>>4)&1u))%4u==2u?4u:5u;
                break;
            case 3u: /* Bouncy funk: roots, fifth and occasional octave */
                degree=(step%4u==0u?0u:4u);
                break;
            case 4u: /* Drone/sub: near-static anchor, rare turnaround */
                degree=(step>=12u && v%7u==0u)?4u:0u;
                break;
            case 5u: /* Reese/wobble: root, fourths, fifth, modal sevenths */
                degree=(step/4u+((v>>5)&1u))%4u==0u?0u:
                       (step/4u+((v>>5)&1u))%4u==1u?4u:
                       (step/4u+((v>>5)&1u))%4u==2u?2u:6u;
                break;
            default: /* 808 or simple sub: tonic and tasteful fifth */
                degree=(step%4u==0u)?0u:
                       (v%4u==0u?4u:0u);
                break;
            }
            /* Hip-hop should sit beneath the chords, not leap to random
             * high fifths, 9ths and octave stabs. Keep the sampled loop
             * anchored in the low register; jazz still walks diatonically. */
            if(track->genre==3u || track->genre==5u) {
                if(style->motion==2u)
                    degree=step>=12u?5u:step>=8u?4u:step>=4u?2u:0u;
                else if(style->motion==3u)
                    degree=step==10u || step==14u?4u:0u;
                else if(style->motion==0u)
                    degree=step==10u?4u:0u;
            }
            unsigned interval=scale[degree];
            if(track->genre!=3u && track->genre!=5u &&
               style->motion==2u && v%5u==0u)
                interval=phrase_note[(step/4u)%4u];
            if(track->genre!=3u && track->genre!=5u &&
               style->octave && v%6u==0u)interval+=12u;
            if(song->complexity>=2u && track->subgenre%4u==3u && step==15u &&
               (sec==URBAN_HOOK || sec==URBAN_DROP))hit=1u;
            n->note=(uint8_t)(root_note+interval);
            n->velocity=hit?(uint8_t)(style->accent+(v%12u)):0u;
            unsigned gate=(unsigned)style->gate+(v%3u);
            n->gate=(uint8_t)(gate>16u?16u:gate);
            n->glide=(uint8_t)(hit&&style->slide&&step>=7u &&
                              v%100u<(unsigned)style->slide);
            /* Correctly scale-quantized slide target, including Locrian. */
            if(n->glide && interval==0u)
                n->note=(uint8_t)(root_note+scale[4]);
        }
    }
}
#endif
