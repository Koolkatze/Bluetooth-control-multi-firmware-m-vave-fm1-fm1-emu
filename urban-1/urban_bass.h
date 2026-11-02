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
/* A genuinely authored 4-bar bass rhythm for each named substyle.
 * Four masks per style = 224 distinct bar positions, not the old
 * one-bar loop pasted four times. The clock remains strict 16ths.
 * Sparse/ambient styles intentionally leave gaps rather than adding
 * arbitrary 32nd ratchets or speed from unrelated genres. */
static const uint16_t urban_bass_fourbar[URBAN_GENRES][4][4]={
  /* 0 */ {{0x0429u,0x0489u,0x1429u,0x04a9u},{0x0a81u,0x0a09u,0x2a81u,0x0ac1u},{0x0101u,0x0001u,0x0101u,0x0001u},{0x1429u,0x34a9u,0x14a9u,0x1c29u}},
  /* 1 */ {{0x0821u,0x0809u,0x0a21u,0x0829u},{0x0921u,0x0a21u,0x2921u,0x0929u},{0x1111u,0x0101u,0x1111u,0x1011u},{0x0101u,0x0001u,0x0101u,0x0801u}},
  /* 2 */ {{0x0109u,0x0009u,0x0101u,0x0109u},{0x0101u,0x1101u,0x0101u,0x0001u},{0x0809u,0x0409u,0x0801u,0x0c09u},{0x0009u,0x0109u,0x0001u,0x0809u}},
  /* 3 */ {{0x0489u,0x0409u,0x0489u,0x04c9u},{0x1111u,0x1111u,0x1111u,0x9111u},{0x0889u,0x28c9u,0x0ac9u,0x18c9u},{0x0109u,0x0009u,0x0109u,0x0101u}},
  /* 4 */ {{0x0441u,0x0401u,0x0441u,0x0641u},{0x0109u,0x0009u,0x0101u,0x0809u},{0x0a21u,0x2a21u,0x0a89u,0x8a21u},{0x0441u,0x0401u,0x0c41u,0x0449u}},
  /* 5 */ {{0x0489u,0x0409u,0x0489u,0x0c89u},{0x1111u,0x1111u,0x1111u,0x5111u},{0x0809u,0x0801u,0x0a09u,0x0809u},{0x048bu,0x24c9u,0x0c8bu,0x04cbu}},
  /* 6 */ {{0x0481u,0x0401u,0x0489u,0x0c81u},{0x0101u,0x0001u,0x0101u,0x0401u},{0x0889u,0x0ac9u,0x2889u,0x0c89u},{0x0a81u,0x0801u,0x0a89u,0x0a01u}},
  /* 7 */ {{0x1119u,0x1199u,0x1119u,0x1999u},{0x1199u,0x1999u,0x11b9u,0x1999u},{0x1109u,0x1009u,0x0109u,0x1109u},{0x1299u,0x12b9u,0x1a99u,0x12d9u}},
  /* 8 */ {{0x4a41u,0x6a41u,0x4b41u,0x4a61u},{0x4444u,0x4644u,0x444cu,0x4464u},{0x0401u,0x0241u,0x0601u,0x0a41u},{0x2a49u,0x6a49u,0x2b49u,0x3a49u}},
  /* 9 */ {{0x0401u,0x0409u,0x0401u,0x0601u},{0x0101u,0x0001u,0x0101u,0x0001u},{0x2021u,0x2221u,0x2029u,0x2229u},{0x0c81u,0x2c81u,0x0e81u,0x2c89u}},
  /* 10 */ {{0x4444u,0x4544u,0x4446u,0x4464u},{0x4444u,0x4444u,0x4444u,0x4444u},{0x0101u,0x0001u,0x0101u,0x0401u},{0x4444u,0x0404u,0x4444u,0x0404u}},
  /* 11 */ {{0x5555u,0x5555u,0x5555u,0x5d55u},{0x1111u,0x1311u,0x1119u,0x1319u},{0x4444u,0x444cu,0x4644u,0x4444u},{0x5555u,0x5d55u,0x5755u,0x5f55u}},
  /* 12 */ {{0x4444u,0x4444u,0x4444u,0x4444u},{0x4444u,0x444cu,0x4444u,0x4464u},{0x5555u,0x5555u,0x5555u,0x5555u},{0x4444u,0x4444u,0x444cu,0x4444u}},
  /* 13 */ {{0x2a49u,0x6a49u,0x2a41u,0x2ac9u},{0x0101u,0x0001u,0x0101u,0x0001u},{0x1111u,0x1111u,0x1111u,0x9111u},{0x0109u,0x0101u,0x0809u,0x0109u}}
};
static uint16_t urban_bass_bar_mask(const UrbanTrack *t,unsigned sec,
                                    unsigned bar) {
    const unsigned g=t->genre%URBAN_GENRES,sub=t->subgenre%4u;
    uint16_t mask=urban_bass_fourbar[g][sub][bar%4u];
    if(sec==URBAN_INTRO || sec==URBAN_OUTRO) {
        if(g==12u || g==10u)mask&=0x4444u;
        else mask&=0x0101u;
    } else if(sec==URBAN_BREAK || sec==URBAN_BRIDGE) {
        mask&=(g==12u?0x4444u:0x0101u);
    }
    return mask;
}
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
        phrase_note[i]=(uint8_t)urban_scale_pc(song->scale,
            urban_scale_root_degree(song->scale,degree));
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
            unsigned scale=song->scale;
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
            unsigned interval=urban_scale_pc(scale,
                          urban_scale_root_degree(scale,degree));
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
                n->note=(uint8_t)(root_note+urban_scale_pc(scale,
                                     urban_scale_root_degree(scale,4u)));
        }
    }
}
/* At playback choose the authored bar score. The original 16th base
 * notes retain their synth envelopes/seed; only deterministic, styled
 * onset and melodic response are varied. All 4 bars are audible.
 * Keep chord transposition downstream in urban_runtime_tick(). */
static UrbanBassStep urban_bass_bar_score(const UrbanSong *s,
             const UrbanBassSong *score,unsigned section,
             unsigned bar,unsigned step) {
    UrbanBassStep n=score->seq[section%URBAN_SEQUENCES].steps[step%URBAN_STEPS];
    const UrbanTrack *t=&s->tracks[2];
    unsigned sub=t->subgenre%4u;
    uint16_t mask=urban_bass_bar_mask(t,section,bar);
    if(!(mask&(1u<<(step%16u)))) {
        n.velocity=0u;n.glide=0u;return n;
    }
    UrbanBassGrammar g=urban_bass_style(t);
    uint32_t v=urban_mix_seed(t->seq[section%URBAN_SEQUENCES].seed ^
                    ((bar+1u)*0x9e3779b9u) ^ ((step+1u)*0x85ebca6bu));
    unsigned accent=step%4u==0u?6u:step%2u==0u?3u:0u;
    unsigned vel=(unsigned)g.accent+accent+(v%8u);
    n.velocity=(uint8_t)(vel>124u?124u:vel);
    /* Different phrase replies: walking scale degrees for jazz,
     * sustained root/fifth for lofi/ambient, and funk slides only
     * for slide-enabled substyles. No global random octave jumps. */
    unsigned degree=0u;
    if(g.motion==2u) {
        static const uint8_t walk[4][4]={
            {0,2,4,5},{0,1,2,4},{4,5,2,1},{0,2,4,6}
        };
        degree=walk[bar%4u][(step/4u)%4u];
    } else if(g.motion==3u) {
        degree=(step%4u==2u || (bar==3u && step>=12u))?4u:0u;
    } else if(g.motion==5u) {
        degree=(bar==1u && step>=8u)?2u:
               (bar==2u && step>=8u)?4u:
               (bar==3u && step>=12u)?6u:0u;
    } else if(g.motion==1u) {
        degree=(bar==3u && step>=12u)?4u:
               (step==6u || step==14u)?2u:0u;
    } else if(g.motion==0u && (step==10u || (bar==3u && step==14u)))
        degree=4u;
    else if(g.motion==4u && bar==3u && step>=12u && sub!=2u)
        degree=4u;
    unsigned semitone=urban_scale_pc(s->scale,
                              urban_scale_root_degree(s->scale,degree));
    unsigned note=36u+s->key%12u+semitone;
    if(g.octave && g.motion==5u && (step==10u||step==14u))note+=12u;
    if(note>71u)note-=12u;
    n.note=(uint8_t)note;
    unsigned gate=g.gate+(v%3u);
    n.gate=(uint8_t)(gate>16u?16u:gate);
    n.glide=(uint8_t)(g.slide && step>=7u && (v%100u)<g.slide);
    return n;
}
#endif
