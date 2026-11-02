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
      /*10 techno      */ {0x1111,0x1010,0x5555,0x1010,0x4444,0x0000,10, 0,6},
      /*11 hardcore    */ {0x1111,0x1010,0xffff,0x1010,0x4444,0x4444,19, 0,6},
      /*12 trance      */ {0x1111,0x1010,0xaaaa,0x1010,0x4444,0x0000, 7, 0,6},
      /*13 liquid jung */ {0x0241,0x1010,0xaaaa,0x1000,0x2020,0x0440,15,19,5}
    };
    /* Full six-lane rhythm transcriptions for EACH named substyle.
     * Values are 16th-step masks, not cosmetic random fill percentages:
     * kick/snare/accent structure and hat/percussion density change
     * together. First subgenre uses the family baseline above. */
    static const UrbanGroove variants[URBAN_GENRES][3]={
      /* Trap: Drill / Ambient Trap / Rolling Hats */
      {{0x0a81,0x0100,0xaaaa,0x0100,0x2010,0x0040,26, 7,7},
       {0x0101,0x0100,0x1111,0x0000,0x0000,0x0000, 4, 0,7},
       {0x1429,0x0100,0xffff,0x0100,0x2020,0x0080,31, 6,7}},
      /* Goth Trap: Industrial / Darkwave / Cloud */
      {{0x0921,0x0100,0xaaaa,0x0100,0x2000,0x2222,23, 4,6},
       {0x1111,0x1010,0x5555,0x1010,0x2020,0x0000,14, 0,5},
       {0x0101,0x0100,0x4444,0x0000,0x2000,0x0000, 6, 0,7}},
      /* Sad Trap: Piano / Lo-fi / Guitar */
      {{0x0101,0x0100,0x1111,0x0000,0x0000,0x0000, 3, 0,7},
       {0x0809,0x1010,0x4444,0x1000,0x0000,0x0040, 8, 8,4},
       {0x0009,0x0100,0x4444,0x0000,0x0000,0x0000, 2, 0,7}},
      /* Hip-hop: Jazz Rap / G-Funk / Lo-fi */
      {{0x0849,0x1010,0x4444,0x1010,0x0000,0x4444,16,31,4},
       {0x2089,0x1010,0xaaaa,0x1010,0x2020,0x0080,17, 6,4},
       {0x0109,0x1010,0x4444,0x1000,0x0000,0x0040, 7,22,4}},
      /* Trip-hop: Downtempo / Abstract / Dub */
      {{0x0109,0x1010,0x4444,0x1000,0x0000,0x0040, 6,24,4},
       {0x0a21,0x1010,0x2222,0x0000,0x0000,0x2222,19,18,5},
       {0x0441,0x1010,0x4444,0x1000,0x0200,0x2828,13,20,4}},
      /* Underground Hip-hop: Jazz / Experimental / Break */
      {{0x0849,0x1010,0x4444,0x1000,0x0000,0x4444,13,32,4},
       {0x0a89,0x1010,0xaaaa,0x1010,0x2020,0x1111,20,11,5},
       {0x2449,0x1090,0xaaaa,0x1010,0x0000,0x0480,26,17,4}},
      /* Horrorcore: Cinematic / Industrial / Dark */
      {{0x0101,0x0100,0x1111,0x0000,0x0000,0x0020, 6, 0,7},
       {0x0889,0x1010,0xaaaa,0x1010,0x2000,0x2222,25, 2,6},
       {0x0a81,0x0100,0x5555,0x0100,0x2020,0x0888,18, 6,7}},
      /* Reggaeton: Perreo / Romantico / Futurista */
      {{0x1199,0x0444,0xaaaa,0x0888,0x2000,0x2222,18, 0,3},
       {0x1109,0x0444,0x5555,0x0888,0x0000,0x0008, 4, 0,3},
       {0x1199,0x0444,0xaaaa,0x0888,0x2020,0x2222,19, 0,3}},
      /* DnB: Jump-up / Darkstep / Techstep */
      {{0x0c41,0x1010,0xaaaa,0x1010,0x2000,0x0040,18, 0,4},
       {0x0241,0x1090,0xaaaa,0x1010,0x2020,0x4444,28, 3,6},
       {0x0641,0x1010,0xffff,0x1010,0x0020,0x0880,24, 0,5}},
      /* Dubstep: Deep / Riddim / Brostep */
      {{0x0101,0x0100,0x4444,0x0000,0x0000,0x0008, 5, 0,7},
       {0x2021,0x0100,0x5555,0x0100,0x2020,0x0000,19, 0,6},
       {0x0c81,0x0100,0xffff,0x0100,0x2020,0x8888,32, 0,6}},
      /* Techno: Hypnotic / Dub / Minimal */
      {{0x1111,0x1010,0x5555,0x0000,0x4444,0x2222, 8, 0,5},
       {0x1111,0x0000,0x5555,0x0000,0x4444,0x0000, 4, 0,5},
       {0x1111,0x1000,0x4444,0x0000,0x0000,0x0000, 2, 0,5}},
      /* Hardcore: Industrial / Frenchcore / Uptempo */
      {{0x1111,0x1010,0xffff,0x1010,0x4444,0xaaaa,30, 0,6},
       {0x1111,0x1010,0xaaaa,0x1010,0x4444,0x2222,16, 0,6},
       {0x1111,0x1010,0xffff,0x1010,0x4444,0xffff,34, 0,6}},
      /* Trance: Uplifting / Psy / Tech */
      {{0x1111,0x1010,0xaaaa,0x1010,0xeeee,0x0000,12, 0,1},
       {0x1111,0x1010,0xffff,0x1010,0x4444,0x8888,24, 0,3},
       {0x1111,0x1010,0x5555,0x1010,0xaaaa,0x2222,16, 0,2}},
      /* Liquid Jungle: Atmospheric / Jazzy / Deep */
      {{0x0109,0x1010,0x5555,0x0000,0x2020,0x0008, 6, 0,4},
       {0x0641,0x1010,0xaaaa,0x1000,0x2020,0x4444,12,24,4},
       {0x0101,0x1010,0xaaaa,0x0000,0x2020,0x0000, 5, 0,4}}
    };
    unsigned gidx=genre%URBAN_GENRES,sub=subgenre%4u;
    UrbanGroove g=sub?variants[gidx][sub-1u]:grooves[gidx];
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
    /* Two recordings in the same genre should NOT share the same drum
     * partitura: derive a 4-bar riff and extra accents from the song seed.
     * Keep canonical downbeats, dembow/snare backbeats and 4-to-floor
     * anchors; genre identity survives every generation. */
    uint32_t figure=urban_mix_seed(s->seed ^
         (d->seq[section%URBAN_SEQUENCES].seed*0x7f4a7c15u) ^
         ((uint32_t)s->drum_variant*0x85ebca6bu) ^
         ((section_bar/4u)*0x9e3779b9u));
    static const uint8_t accents[8]={2,3,5,6,7,9,11,14};
    unsigned edits=1u+(s->complexity>=2u);
    for(unsigned n=0;n<edits;n++) {
        unsigned step=accents[urban_rng(&figure)%8u];
        uint16_t bit=(uint16_t)(1u<<step);
        if((urban_rng(&figure)&1u)!=0u) out[0]|=bit;
        else out[0]&=(uint16_t)~bit;
        if((urban_rng(&figure)%4u)==0u)out[5]^=bit;
    }
    /* Signature kick beats must be present, avoiding a new song that
     * accidentally loses the reggaeton, techno or DnB foundation. */
    out[0]|=(uint16_t)(g.kick&0x1111u);
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
    /* Four-bar phrase development: bar 1 establishes the riff, bar 2
     * answers the beat, bar 3 varies percussion/hat placement, bar 4
     * performs a controlled turnaround or fill. All 6 lanes participate.
     * Important: no random rearrangement of kick/snare backbeat anchors.
     */
    unsigned phrase_bar=section_bar%URBAN_BARS_PER_PHRASE;
    if(s->complexity && section!=URBAN_INTRO &&
       section!=URBAN_BREAK && section!=URBAN_OUTRO) {
        if(phrase_bar==1u) {
            switch(d->genre%URBAN_GENRES) {
            case 0: case 1: case 2: case 6: /* trap / horror */
                out[3]|=0x0808u; break;
            case 3: case 4: case 5: /* boom bap / trip hop */
                out[5]|=0x0080u; break;
            case 7: /* dembow response */
                out[2]|=0x0080u; break;
            case 8: case 13: /* breaks */
                out[0]|=0x0200u; break;
            case 9: /* dubstep: half-time space */
                out[4]|=0x0400u; break;
            default: /* four on the floor */
                out[4]|=0x4444u; break;
            }
        } else if(phrase_bar==2u && s->complexity>=2u) {
            if(d->genre<=2u || d->genre==9u)out[3]|=0x8080u;
            else if(d->genre==7u)out[5]|=0x0404u;
            else if(d->genre==8u || d->genre==13u)out[1]|=0x0020u;
            else out[5]|=0x2020u;
        }
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
