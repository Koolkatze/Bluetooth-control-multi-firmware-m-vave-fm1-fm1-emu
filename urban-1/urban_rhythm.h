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
      /* 3 hip-hop     */ {0x0489,0x1010,0x5555,0x0000,0x0000,0x0040, 6,29,4},
      /* 4 trip-hop    */ {0x0441,0x1010,0x4444,0x1000,0x0200,0x0008,13,33,4},
      /* 5 underground */ {0x0889,0x1010,0x5555,0x0000,0x0000,0x0800, 9,31,4},
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
      /* Hip-hop: jazz break / G-Funk's 808 and lightly open hat / dusty lo-fi */
      {{0x04c9,0x1010,0x5555,0x0000,0x2000,0x0044, 6,25,4},
       {0x0889,0x1010,0x5555,0x1000,0x2000,0x0000, 7, 7,7},
       {0x0109,0x1010,0x5555,0x0000,0x0000,0x0000, 2,22,4}},
      /* Trip-hop: Downtempo / Abstract / Dub */
      {{0x0109,0x1010,0x4444,0x1000,0x0000,0x0040, 6,24,4},
       {0x0a21,0x1010,0x2222,0x0000,0x0000,0x2222,19,18,5},
       {0x0441,0x1010,0x4444,0x0800,0x0200,0x2828,13,20,4}},
      /* Underground: spacious jazz, abstract cuts, funky breakbeat */
      {{0x0489,0x1010,0x5555,0x0000,0x0000,0x0040, 8,32,4},
       {0x0889,0x1010,0x4444,0x0000,0x0000,0x0100, 9,11,4},
       {0x048b,0x1010,0x5555,0x0000,0x0000,0x0080,12,17,4}},
      /* Horrorcore: Cinematic / Industrial / Dark */
      {{0x0101,0x0100,0x1111,0x0000,0x0000,0x0020, 6, 0,7},
       {0x0889,0x1010,0xaaaa,0x1010,0x2000,0x2222,25, 2,6},
       {0x0a81,0x0100,0x5555,0x0100,0x2020,0x0888,18, 6,7}},
      /* Reggaeton: Perreo / Romantico / Futurista */
      {{0x1199,0x0444,0xaaaa,0x0888,0x2000,0x2222,18, 0,3},
       {0x1109,0x0444,0x5555,0x0888,0x0000,0x0008, 4, 0,3},
       {0x1199,0x0444,0x5555,0x0888,0x2020,0x2222,19, 0,3}},
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
      {{0x1111,0x1010,0xaaaa,0x1010,0x4444,0xaaaa,30, 0,6},
       {0x1111,0x1010,0x5555,0x1010,0x4444,0x2222,16, 0,6},
       {0x1111,0x1090,0xffff,0x1010,0x4444,0xffff,34, 0,6}},
      /* Trance: Uplifting / Psy / Tech */
      {{0x1111,0x1010,0xeeee,0x1010,0xeeee,0x0000,12, 0,1},
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
/* Authentic, individually arranged drum phrase responses per subgenre.
 * Lane ownership stays separate: kick/snare = drum track, hats/perc =
 * hats track, so a mixed-genre arrangement does not overwrite either.
 * 0 means a consciously sparse style, not a missing sample or fallback.
 * bar2 kick | bar3 snare ghost | bar2 hat | bar3 perc | bar4 snare fill */
typedef struct {
    uint16_t kick_response,snare_ghost,hat_response,perc_response,turnaround;
} UrbanDrumPocket;
static const UrbanDrumPocket urban_drum_pockets[URBAN_GENRES][4]={
  /* 0 */ {{0x0800u,0x0000u,0x0880u,0x0000u,0x8000u},{0x0200u,0x0000u,0x8080u,0x0040u,0x2000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x1000u,0x0000u,0xaaaau,0x0080u,0x4000u}},
  /* 1 */ {{0x0800u,0x0000u,0x8080u,0x0020u,0x4000u},{0x0200u,0x0020u,0xaaaau,0x2222u,0x8000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u}},
  /* 2 */ {{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0800u,0x0000u,0x0000u,0x0000u,0x0000u}},
  /* 3 */ {{0x0008u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0040u,0x0020u,0x0000u,0x0040u,0x0000u},{0x0200u,0x0000u,0x8000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u}},
  /* 4 */ {{0x0020u,0x0000u,0x0000u,0x0008u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0800u,0x0020u,0x8888u,0x0200u,0x0000u},{0x0020u,0x0000u,0x0000u,0x0800u,0x0000u}},
  /* 5 */ {{0x0008u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0020u,0x0000u,0x0020u,0x0000u},{0x0200u,0x0000u,0x0000u,0x0800u,0x0000u},{0x0020u,0x0040u,0x0000u,0x0080u,0x0000u}},
  /* 6 */ {{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0200u,0x0000u,0xaaaau,0x2222u,0x4000u},{0x0800u,0x0000u,0x0000u,0x0800u,0x0000u}},
  /* 7 */ {{0x0800u,0x0000u,0x0000u,0x0080u,0x0000u},{0x0200u,0x0000u,0xaaaau,0x2222u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0200u,0x0000u,0xaaaau,0x0040u,0x0000u}},
  /* 8 */ {{0x0200u,0x0020u,0x8888u,0x0080u,0x8000u},{0x0800u,0x0000u,0x0000u,0x0040u,0x4000u},{0x0200u,0x0040u,0xffffu,0x4444u,0x2000u},{0x0800u,0x0020u,0xaaaau,0x0080u,0x2000u}},
  /* 9 */ {{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x2000u,0x0000u,0x0000u,0x0000u,0x8000u},{0x0800u,0x0000u,0xaaaau,0x8888u,0x4000u}},
  /* 10 */ {{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u}},
  /* 11 */ {{0x0000u,0x0000u,0xaaaau,0x0000u,0x0000u},{0x0000u,0x0000u,0xaaaau,0x4444u,0x0000u},{0x0000u,0x0000u,0xaaaau,0x0000u,0x0000u},{0x0000u,0x0000u,0xffffu,0x8888u,0x0000u}},
  /* 12 */ {{0x0000u,0x0000u,0xaaaau,0x0000u,0x0000u},{0x0000u,0x0000u,0xeeeeu,0x0000u,0x0000u},{0x0000u,0x0000u,0xffffu,0x8888u,0x0000u},{0x0000u,0x0000u,0xaaaau,0x2222u,0x0000u}},
  /* 13 */ {{0x0200u,0x0020u,0xaaaau,0x0040u,0x8000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u},{0x0800u,0x0020u,0xaaaau,0x4444u,0x4000u},{0x0000u,0x0000u,0x0000u,0x0000u,0x0000u}}
};
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
    /* NO universal random kick/snare mutation: that previously made
     * all drum subgenres converge on one generic syncopated rhythm. Only
     * explicitly authored bar-by-bar ghost notes and fill candidates
     * may be added. Preserve the baseline kick/snare/hat signature. */
    const UrbanDrumPocket dp=urban_drum_pockets[d->genre%URBAN_GENRES]
                                                  [d->subgenre%4u];
    const UrbanDrumPocket hp=urban_drum_pockets[h->genre%URBAN_GENRES]
                                                  [h->subgenre%4u];
    uint32_t figure=urban_mix_seed(s->seed ^
         d->seq[section%URBAN_SEQUENCES].seed ^
         ((uint32_t)s->drum_variant*0x85ebca6bu));
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
        /* The hook uses this SUBSTYLE's authored lanes. Never force
         * 0xaaaa hats, doubled claps or 0x0101 kicks into unrelated
         * styles such as jazz breaks, drill or half-time dubstep. */
        if(section==URBAN_DROP && s->complexity>=2u)
            out[5]|=(uint16_t)(hp.perc_response&0x8888u);
    }
    /* Independently authored four-bar pocket, with intentional
     * response/ghost/turnaround rather than identical universal fills.
     * Keep Intro/Break/Outro dynamics and any bass pocket separate. */
    const unsigned phrase_bar=section_bar%URBAN_BARS_PER_PHRASE;
    if(s->complexity && section!=URBAN_INTRO &&
       section!=URBAN_BREAK && section!=URBAN_OUTRO) {
        if(phrase_bar==1u) {
            out[0]|=dp.kick_response;
            out[3]|=hp.hat_response;
        } else if(phrase_bar==2u && s->complexity>=2u) {
            out[1]|=dp.snare_ghost;
            out[5]|=hp.perc_response;
        } else if(phrase_bar==3u && s->complexity>=1u) {
            /* Same named fill identity on every take; seeded probability
             * changes whether a sparse extra is voiced, never its lane. */
            if((urban_rng(&figure)&3u)!=0u)
                out[1]|=dp.turnaround;
            if(s->complexity>=3u && (urban_rng(&figure)&1u))
                out[3]|=(uint16_t)(hp.hat_response&0xc000u);
        }
    }
    /* Boom Bap is SNARE ONLY on the backbeat, never the synthetic
     * clap lane. Preserve this through hooks, drops, fills, complexity
     * changes and independently selected drum-track subgenres. */
    if(d->genre%URBAN_GENRES==3u && d->subgenre%4u==0u)
        out[2]=0u;
    uint8_t solo=0;
    for(unsigned t=0;t<URBAN_TRACKS;t++)solo|=s->tracks[t].solo;
    if(d->muted || (solo&&!d->solo))
        out[0]=out[1]=out[2]=0;
    if(h->muted || (solo&&!h->solo))
        out[3]=out[4]=out[5]=0;
}
#endif
