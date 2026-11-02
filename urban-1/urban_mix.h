/* SPDX-License-Identifier: GPL-3.0-only
 * HYP-1 mixing has quadratic layer gain (v*v/10000), not a linear
 * 0..100 fader. Sum of high layer settings plus distorted drums and
 * reverb can overload the fixed output. Make coherent genre-specific
 * balances with bounded headroom; this is not a true-peak DSP limiter.
 */
#ifndef URBAN_MIX_H
#define URBAN_MIX_H
#include "urban_lead.h"
#include "urban_instruments.h"
typedef struct {
    uint8_t drums,bass,lead,chords,noise;
    uint8_t kick,snare,clap,hat,open,perc;
    uint8_t drive,reverb,echo;
} UrbanMixPlan;
typedef struct {
    uint8_t drums,bass,lead,chords,noise;
    uint8_t kick,hat;
} UrbanGenreBalance;
static const UrbanGenreBalance urban_balance[URBAN_GENRES]={
 /*               DRUM BASS LEAD PADS NOISE KICK HAT */
 /* trap       */ {74,72,34,40, 3,78,36},
 /* goth trap  */ {62,68,29,44,10,70,32},
 /* sad trap   */ {61,66,30,51, 5,69,34},
 /* hip-hop    */ {73,70,24,52, 3,80,30},
 /* trip-hop   */ {62,63,33,49,10,70,35},
 /* underground*/ {72,69,25,51, 4,80,33},
 /* horrorcore */ {70,72,30,38,10,76,36},
 /* reggaeton  */ {76,66,39,41, 2,82,42},
 /* dnb        */ {75,70,41,38, 5,83,42},
 /* dubstep    */ {72,76,38,40, 5,78,41},
 /* techno     */ {76,62,41,41, 2,80,45},
 /* hardcore   */ {80,68,39,36, 3,82,42},
 /* trance     */ {69,62,49,49, 5,71,43},
 /* liquid     */ {68,65,35,51, 8,74,38}
};
static uint8_t urban_mix_cap(unsigned n,unsigned hi) {
    return (uint8_t)(n>hi?hi:n);
}
static unsigned urban_mix_energy(const UrbanMixPlan *m) {
    /* HYP-1 maps channel controls to squared amplitude. Summed proxy
     * treats no lane as a guaranteed perfectly uncorrelated source. */
    return (unsigned)m->drums*m->drums+
           (unsigned)m->bass*m->bass+
           (unsigned)m->lead*m->lead+
           (unsigned)m->chords*m->chords+
           (unsigned)m->noise*m->noise;
}
static UrbanMixPlan urban_mix_plan(const UrbanSong *s,
                        unsigned section,unsigned bar) {
    UrbanMixPlan m;
    const UrbanGenreBalance *b=&urban_balance[s->genre%URBAN_GENRES];
    const UrbanStylePatch fx=urban_style_patch(s->genre,s->subgenre);
    static const uint8_t dynamics[URBAN_SEQUENCES][5]={
      /* Drum  Bass Lead Pad  Noise */
        {42,45,  0, 59,45}, /* Intro */
        {91,92, 78, 91,84}, /* Verse A */
        {100,98,100,100,91},/* Hook */
        {93,94, 82, 91,86}, /* Verse B */
        {42,64, 25,104,80}, /* Bridge */
        {104,104,108, 95,72},/* Drop */
        {27,48,  0, 81,55}, /* Breakdown */
        {44,57, 22, 72,45}  /* Outro */
    };
    section%=URBAN_SEQUENCES;
    m.drums=urban_mix_cap((unsigned)b->drums*dynamics[section][0]/100u,82);
    m.bass=urban_mix_cap((unsigned)b->bass*dynamics[section][1]/100u,78);
    m.lead=urban_mix_cap((unsigned)b->lead*dynamics[section][2]/100u,55);
    m.chords=urban_mix_cap((unsigned)b->chords*dynamics[section][3]/100u,55);
    m.noise=urban_mix_cap((unsigned)b->noise*dynamics[section][4]/100u,13);
    if(!urban_lead_active(s,section,bar))m.lead=0u;
    if(s->genre==3u || s->genre==5u) {
        /* G-Funk makes room for the rare held mono analog lead;
         * in boom-bap and lo-fi the harmony is the main pitched layer. */
        if(s->subgenre%4u==2u && m.lead)
            m.lead=urban_mix_cap((unsigned)m.lead+13u,53u);
        if(s->subgenre%4u==3u) {
            m.lead=(uint8_t)(m.lead*3u/4u);
            m.noise=urban_mix_cap((unsigned)m.noise+2u,13u);
        }
    }
    uint8_t solo=0u;
    for(unsigned t=0;t<URBAN_TRACKS;t++)solo|=s->tracks[t].solo;
    if(s->tracks[0].muted||((solo!=0u)&&!s->tracks[0].solo))
        m.drums=(uint8_t)(m.drums*2u/5u);
    if(s->tracks[2].muted||(solo&&!s->tracks[2].solo))m.bass=0u;
    if(s->tracks[3].muted||(solo&&!s->tracks[3].solo))m.lead=0u;
    if(s->tracks[4].muted||(solo&&!s->tracks[4].solo))m.chords=0u;
    if(s->tracks[5].muted||(solo&&!s->tracks[5].solo))m.noise=0u;
    /* Reserve at least ~35% per-fader headroom in the loudest section,
     * especially for simultaneous four-note pad chords and driven kits. */
    while(urban_mix_energy(&m)>15500u) {
        m.drums=(uint8_t)(m.drums*95u/100u);
        m.bass=(uint8_t)(m.bass*95u/100u);
        m.lead=(uint8_t)(m.lead*95u/100u);
        m.chords=(uint8_t)(m.chords*95u/100u);
        m.noise=(uint8_t)(m.noise*95u/100u);
    }
    m.kick=urban_mix_cap(b->kick,82u);
    m.snare=urban_mix_cap((unsigned)b->kick*76u/100u,65);
    m.clap=urban_mix_cap((unsigned)b->kick*65u/100u,55);
    m.hat=b->hat;
    m.open=urban_mix_cap((unsigned)b->hat*85u/100u,42);
    m.perc=urban_mix_cap((unsigned)b->hat*77u/100u,38);
    /* Drives beyond ~60 can force the HYP drum envelope and the
     * subsequent FX into drastic saturation at peak density. */
    m.drive=urban_mix_cap(fx.drive,60);
    m.reverb=urban_mix_cap(fx.reverb,62);
    m.echo=urban_mix_cap(fx.echo,53);
    if(s->genre==3u || s->genre==5u) {
        /* Dry vinyl/break-kit emphasis. HYP's native synth drum
         * clap is not a sampled snare, so don't stack it at full gain. */
        m.snare=urban_mix_cap((unsigned)b->kick*86u/100u,70u);
        m.clap=urban_mix_cap((unsigned)b->kick*20u/100u,18u);
        m.hat=urban_mix_cap(b->hat,35u);
        m.open=urban_mix_cap(b->hat*60u/100u,25u);
        m.perc=urban_mix_cap(b->hat*55u/100u,24u);
        m.drive=urban_mix_cap(fx.drive,27u);
        m.reverb=urban_mix_cap(fx.reverb,38u);
        m.echo=urban_mix_cap(fx.echo,30u);
    }
    /* Strong compression in hardcore / neuro is a *deliberate* colour,
     * but kick + hat + clap should not all operate at 100% gain. */
    return m;
}
#endif
