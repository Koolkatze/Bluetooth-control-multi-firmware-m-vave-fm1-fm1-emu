/* SPDX-License-Identifier: GPL-3.0-only
 * 14 hand-authored genre drum grammars (not random density).
 * FM-1 / HYP-1 six lanes: kick, snare, clap, closed hat, open hat, perc.
 * Patterns repeat coherently within section and add controlled 4-bar fills.
 */
#ifndef URBAN_RHYTHM_H
#define URBAN_RHYTHM_H
#include "urban_song.h"
typedef struct {
    uint16_t kick,snare,hat,clap,open,perc;
    uint8_t extras, swing, kit;
} UrbanGroove;
static UrbanGroove urban_groove(uint8_t genre,uint8_t subgenre) {
    static const UrbanGroove grooves[URBAN_GENRES]={
      /*                  KICK  SNARE HATS  CLAP  OPEN  PERC +F SW KIT */
      /* 0 trap        */ {0x0429,0x0100,0x5555,0x0100,0x2000,0x0000,18, 7,7},
      /* 1 goth trap   */ {0x0821,0x0100,0x5155,0x0100,0x2000,0x0400,16,10,7},
      /* 2 sad trap    */ {0x0109,0x0100,0x5555,0x0100,0x0000,0x0000, 9, 8,7},
      /* 3 hip-hop     */ {0x1049,0x1010,0x5555,0x1010,0x0000,0x0040,15,29,2},
      /* 4 trip-hop    */ {0x0441,0x1010,0x4444,0x1000,0x0200,0x0008,13,33,4},
      /* 5 underground */ {0x1089,0x1010,0x5555,0x1000,0x0000,0x0800,19,31,2},
      /* 6 horrorcore  */ {0x0481,0x0100,0x5555,0x0100,0x2000,0x0280,20,16,7},
      /* 7 reggaeton   */ {0x1119,0x0444,0x5555,0x0888,0x2000,0x0888,11, 6,3},
      /* 8 dnb         */ {0x0441,0x1010,0xffff,0x1010,0x2020,0x0080,24, 7,5},
      /* 9 dubstep     */ {0x0401,0x0100,0x5555,0x0100,0x2000,0x0800,21, 7,7},
      /*10 techno      */ {0x1111,0x1010,0x5555,0x1010,0x2222,0x0000,10, 0,6},
      /*11 hardcore    */ {0x1111,0x1010,0xffff,0x1010,0x2222,0x4444,19, 0,6},
      /*12 trance      */ {0x1111,0x1010,0xaaaa,0x1010,0x2222,0x0000, 7, 0,6},
      /*13 liquid jung */ {0x0241,0x1010,0xaaaa,0x1000,0x2020,0x0440,15,19,5}
    };
    UrbanGroove g=grooves[genre%URBAN_GENRES];
    /* Subgenre controls decoration rather than replacing genre's identity. */
    g.extras=(uint8_t)(g.extras+(subgenre%5u)*2u);
    g.swing=(uint8_t)(g.swing+(subgenre%3u)*2u);
    return g;
}
static uint8_t urban_drum_hit(uint8_t genre,uint8_t subgenre,
    unsigned track,unsigned step,unsigned section,uint32_t *rng) {
    UrbanGroove g=urban_groove(genre,subgenre);
    uint16_t m=track==0u?(uint16_t)(g.kick|g.snare):g.hat;
    uint8_t hit=(uint8_t)((m>>step)&1u);
    uint32_t v=urban_rng(rng)%100u;
    if(!hit && (section==URBAN_HOOK || section==URBAN_DROP)
       && v<g.extras/3u && step>=12u)hit=1u;
    if(section==URBAN_INTRO && step>=8u && track==1u)hit=0u;
    return hit;
}
static uint8_t urban_drum_note(uint8_t genre,unsigned track,unsigned step) {
    if(track==1u)return 42u;
    UrbanGroove g=urban_groove(genre,0);
    return (uint8_t)(((g.snare>>step)&1u)?38u:36u);
}
/* Musical pattern writer. 16-bit bitplanes remain suitable for HYP's
 * pattern engine; variation is stable from the track seed and bar index.
 * Drum track 0 controls kick/snare/clap; track 1 controls hat/open/percs.
 */
static void urban_drum_lanes(const UrbanSong *s,unsigned section,unsigned section_bar,
                             uint16_t out[6]) {
    const UrbanTrack *d=&s->tracks[0],*h=&s->tracks[1];
    UrbanGroove g=urban_groove(d->genre,d->subgenre);
    UrbanGroove hg=urban_groove(h->genre,h->subgenre);
    uint32_t r=d->seq[section%URBAN_SEQUENCES].seed ^
               (h->seq[section%URBAN_SEQUENCES].seed*0x9e3779b9u) ^
               ((section_bar/4u)*0x7f4a7c15u);
    out[0]=g.kick; out[1]=g.snare; out[2]=g.clap;
    out[3]=hg.hat; out[4]=hg.open; out[5]=hg.perc;
    if(section==URBAN_INTRO) {
        out[0]&=0x0101u;
        out[1]=0; out[2]=0; out[3]&=0x1111u; out[4]=0; out[5]=0;
    } else if(section==URBAN_BREAK || section==URBAN_BRIDGE) {
        out[0]&=0x0001u;
        out[1]&=0x1000u;
        out[2]=0; out[3]&=0x4444u; out[4]=0;
    } else if(section==URBAN_OUTRO) {
        out[0]&=0x1111u; out[1]&=0x1010u;
        out[3]&=0x5555u; out[4]=0; out[5]=0;
    } else if(section==URBAN_HOOK || section==URBAN_DROP) {
        /* stronger hats and claps on hooks, 4-bar buildup into drops */
        out[3]|=hg.hat&0xaaaa;
        out[2]|=g.snare;
        if(section==URBAN_DROP)out[0]|=0x0101u;
    }
    if(s->complexity>0u && section_bar%4u==3u &&
       section!=URBAN_INTRO && section!=URBAN_OUTRO) {
        /* fill one bar in four, with genre-specific probabilities */
        for(unsigned step=12u;step<16u;step++) {
            if((urban_rng(&r)%100u)<g.extras*s->complexity/2u) out[1]|=(uint16_t)(1u<<step);
            if((urban_rng(&r)%100u)<hg.extras*s->complexity)out[3]|=(uint16_t)(1u<<step);
        }
    }
    uint8_t solo=0;
    for(unsigned t=0;t<URBAN_TRACKS;t++)solo|=s->tracks[t].solo;
    if(d->muted || (solo&&!d->solo))
        out[0]=out[1]=out[2]=0;
    if(h->muted || (solo&&!h->solo))
        out[3]=out[4]=out[5]=0;
}
#endif
