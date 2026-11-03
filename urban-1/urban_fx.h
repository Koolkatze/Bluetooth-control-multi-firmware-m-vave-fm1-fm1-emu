/* SPDX-License-Identifier: GPL-3.0-only
 * Genre/substyle timbral processing presets using actual HYP-1 controls.
 * HYP-1 has a SHARED stereo echo and reverb bus. It does not support
 * per-track delay/reverb sends; per-instrument differentiation is achieved
 * with each synth's filter envelope, LFO and articulation, plus six
 * separate drum damping/tuning controls.
 */
#ifndef URBAN_FX_H
#define URBAN_FX_H
#include "urban_mix.h"
typedef struct {
    uint8_t echo,echo_time,reverb,reverb_size,drive;
    uint8_t pad_lfo_rate,pad_lfo_depth,pad_detune,pad_attack,pad_release;
    uint8_t bass_lfo_rate,bass_lfo_depth,bass_resonance;
    uint8_t lead_env,lead_decay,lead_slide,lead_accent,lead_resonance;
    uint8_t snare_decay,hat_decay,kick_decay,perc_decay;
    uint8_t noise_tone,noise_pump;
} UrbanFxPlan;
/* 14 distinct genre effect architectures. Substyle offsets are applied
 * below and all required FX remain strictly above zero. Echo-time
 * enums are native HYP 0..5, not milliseconds. */
static const uint8_t urban_fx_genre[URBAN_GENRES][11]={
 /* echo rev size drive  PAD rate depth detune BASS rate depth  lead env   echo time */
    {24,36,61,25, 2,15, 8, 2,12,28, 1}, /* trap, pluck and wide tails */
    {42,60,82,28, 1,31,14, 1,25,17, 3}, /* gothic dark distant */
    {36,61,81,13, 1,16, 5, 1, 9,15, 4}, /* sad piano */
    {12,17,42,12, 2, 8, 4, 1, 6,20, 1}, /* muted hip-hop / Rhodes */
    {45,60,85,17, 2,22,12, 2,18,17, 3}, /* dub trip-hop */
    {15,22,52,16, 2,10, 4, 2, 9,22, 1}, /* dry underground breaks */
    {43,59,78,37, 1,28,10, 2,20,36, 4}, /* horrorcore */
    {23,31,49,19, 3,12, 6, 3,14,44, 1}, /* perreo punchy */
    {24,27,48,39, 3,18, 9, 4,29,48, 0}, /* neuro/DnB */
    {49,55,76,40, 2,31,11, 2,27,55, 4}, /* dubstep wobble/echo */
    {32,39,64,36, 3,33,12, 3,23,51, 2}, /* techno motion */
    {20,28,46,54, 4,24,10, 5,32,65, 0}, /* hardcore clipped tone */
    {47,58,84,26, 5,31,18, 4,20,65, 3}, /* trance synth/phaser-ish LFO */
    {45,62,86,14, 2,34, 8, 2,16,25, 3}  /* liquid soulful reverb */
};
static unsigned urban_fx_clamp(int v,int lo,int hi) {
    if(v<lo)return (unsigned)lo;
    if(v>hi)return (unsigned)hi;
    return (unsigned)v;
}
static UrbanFxPlan urban_fx_plan(const UrbanSong *s) {
    const unsigned g=s->genre%URBAN_GENRES;
    const unsigned sub=s->subgenre%4u;
    const uint8_t *v=urban_fx_genre[g];
    /* Every track's independent genre/subgenre now owns the audible
     * processing applied to THAT instrument rather than inheriting an
     * unrelated preset from the global song genre. The HYP stereo
     * echo/reverb bus remains shared across voices. */
    const unsigned pg=s->tracks[4].genre%URBAN_GENRES;
    const unsigned ps=s->tracks[4].subgenre%4u;
    const unsigned bg=s->tracks[2].genre%URBAN_GENRES;
    const unsigned bs=s->tracks[2].subgenre%4u;
    const unsigned lg=s->tracks[3].genre%URBAN_GENRES;
    const unsigned ls=s->tracks[3].subgenre%4u;
    const unsigned dg=s->tracks[0].genre%URBAN_GENRES;
    const unsigned ds=s->tracks[0].subgenre%4u;
    const unsigned hg=s->tracks[1].genre%URBAN_GENRES;
    const unsigned hs=s->tracks[1].subgenre%4u;
    const unsigned ng=s->tracks[5].genre%URBAN_GENRES;
    const unsigned ns=s->tracks[5].subgenre%4u;
    const uint8_t *pv=urban_fx_genre[pg];
    const uint8_t *bv=urban_fx_genre[bg];
    const uint8_t *nv=urban_fx_genre[ng];
    UrbanFxPlan f;
    UrbanMixPlan mix=urban_mix_plan(s,s->current,urban_phrase_bar(s));
    /* Substyle variations: e.g. atmospheric vs industrial vs minimal.
     * Every one of the 56 choices differs in envelope/effect parameters. */
    static const int8_t wet[4]={0,6,12,-5};
    static const int8_t drive[4]={0,8,-6,4};
    f.echo=(uint8_t)urban_fx_clamp((int)v[0]+wet[sub],8,mix.echo);
    f.echo_time=(uint8_t)((v[10]+(sub==2u?1u:sub==3u?2u:0u))%6u);
    f.reverb=(uint8_t)urban_fx_clamp((int)v[1]+wet[sub],8,mix.reverb);
    f.reverb_size=(uint8_t)urban_fx_clamp((int)v[2]+wet[sub],35,90);
    f.drive=(uint8_t)urban_fx_clamp((int)v[3]+drive[sub],8,mix.drive);
    f.pad_lfo_rate=(uint8_t)((pv[4]+ps)%8u);
    f.pad_lfo_depth=(uint8_t)urban_fx_clamp(pv[5]+(int)ps*5,6,60);
    f.pad_detune=(uint8_t)urban_fx_clamp(pv[6]+(int)ps*3,3,28);
    f.pad_attack=(uint8_t)urban_fx_clamp(
         (pg==1u||pg==12u||pg==13u?32:14)+(int)ps*3,7,50);
    f.pad_release=(uint8_t)urban_fx_clamp(
         (pg==1u||pg==2u||pg==12u||pg==13u?65:38)+(int)ps*4,25,83);
    f.bass_lfo_rate=(uint8_t)((bv[7]+bs)%8u);
    f.bass_lfo_depth=(uint8_t)urban_fx_clamp(bv[8]+(int)bs*4,6,54);
    f.bass_resonance=(uint8_t)urban_fx_clamp(
        bg==0u||bg==2u||bg==7u?19:36+(int)bs*8,16,68);
    const UrbanLeadTone *lead=&urban_lead_tones[lg];
    f.lead_env=(uint8_t)urban_fx_clamp((int)lead->filter_env+
                                                  (int)ls*3,12,82);
    f.lead_decay=(uint8_t)urban_fx_clamp((int)lead->decay+
                                                  (int)ls*4,12,88);
    f.lead_slide=(uint8_t)urban_fx_clamp((int)lead->slide+
                                                  (int)ls*4,4,78);
    f.lead_accent=(uint8_t)urban_fx_clamp((int)lead->accent+
                                                  (int)ls*3,8,78);
    f.lead_resonance=(uint8_t)urban_fx_clamp((int)lead->resonance+
                                                  (int)ls*3,12,79);
    f.snare_decay=(uint8_t)urban_fx_clamp(dg==8u||dg==13u?
                  31+(int)ds*4:42+(int)ds*3,20,72);
    f.hat_decay=(uint8_t)urban_fx_clamp(hg==0u||hg==8u?
                  19+(int)hs*4:36+(int)hs*3,14,65);
    f.kick_decay=(uint8_t)urban_fx_clamp(dg<=2u||dg==9u?
                  67+(int)ds*3:44+(int)ds*3,30,82);
    f.perc_decay=(uint8_t)urban_fx_clamp(dg==7u?
                  24+(int)ds*4:40+(int)ds*2,18,62);
    f.noise_tone=(uint8_t)urban_fx_clamp((int)nv[9]+
                                             (int)ns*5,12,78);
    f.noise_pump=(uint8_t)urban_fx_clamp(
         ng==8u||ng==10u||ng==12u?57+(int)ns*5:26+(int)ns*5,20,80);
    /* The synthetic HYP patch is very different from a vintage sampled
     * record. Soften its envelopes/filter movement for hip-hop, while
     * allowing a single more legato PWM lead specifically in G-Funk. */
    if(g==3u || g==5u) {
        static const uint8_t rev[4]={13,19,19,26};
        static const uint8_t del[4]={ 8,12,17,14};
        f.reverb=(uint8_t)urban_fx_clamp(rev[sub],8,mix.reverb);
        f.echo=(uint8_t)urban_fx_clamp(del[sub],8,mix.echo);
        f.reverb_size=(uint8_t)(sub==3u?58u:42u);
    }
    if(pg==3u || pg==5u) {
        f.pad_lfo_depth=(uint8_t)(8u+ps*3u);
        f.pad_detune=(uint8_t)(3u+ps*2u);
        f.pad_attack=(uint8_t)(ps==3u?12u:7u);
        f.pad_release=(uint8_t)(ps==2u?63u:ps==3u?70u:50u);
    }
    if(bg==3u || bg==5u) {
        f.bass_lfo_depth=(uint8_t)(6u+bs*2u);
        f.bass_resonance=(uint8_t)(16u+bs*2u);
    }
    if(lg==3u || lg==5u) {
        f.lead_env=(uint8_t)(15u+ls*3u);
        f.lead_decay=(uint8_t)(ls==2u?76u:58u);
        f.lead_slide=(uint8_t)(ls==2u?62u:6u);
        f.lead_accent=(uint8_t)(14u+ls*3u);
        f.lead_resonance=(uint8_t)(14u+ls*2u);
    }
    /* Lo-fi is not just a lowpass flag: use HYP's actual Rhodes/piano
     * attack, small wow/flutter-rate LFO, low pad detune, muted drums,
     * and quiet pink/brown noise without a club sidechain pump. Every
     * flavor differs but remains recognizably mellow. */
    if(pg==3u && ps==3u) {
        f.pad_lfo_rate=2u;
        f.pad_lfo_depth=(uint8_t)(urban_lofi_mood(s)==1u?12u:9u);
        f.pad_detune=6u;
        f.pad_attack=(uint8_t)(urban_lofi_mood(s)==3u?20u:12u);
        f.pad_release=(uint8_t)(urban_lofi_mood(s)==3u?82u:71u);
    }
    if(bg==3u && bs==3u) {
        f.bass_lfo_rate=1u;
        f.bass_lfo_depth=6u;
        f.bass_resonance=16u;
    }
    if(lg==3u && ls==3u) {
        f.lead_env=14u;
        f.lead_decay=67u;
        f.lead_slide=4u;
        f.lead_accent=12u;
        f.lead_resonance=13u;
    }
    if(ng==3u && ns==3u) {
        f.noise_tone=(uint8_t)(urban_lofi_mood(s)==1u?11u:17u);
        f.noise_pump=20u;
    }
    if(g==3u && sub==3u) {
        static const uint8_t rev[4]={22u,29u,21u,30u};
        static const uint8_t echo[4]={10u,14u,18u,11u};
        unsigned mood=urban_lofi_mood(s);
        f.reverb=rev[mood]; f.echo=echo[mood];
        f.reverb_size=58u;
        f.echo_time=2u;
    }
    if(lg==12u) {
        /* Trance lead engine is not one resonant legato patch for all.
         * Psy/Tech require short plucks, Progressive a softer offbeat,
         * and Uplifting broad but distinct note attacks. Long slide and
         * huge release formerly smudged the note attacks across drums. */
        static const uint8_t env[4]={34u,56u,42u,36u};
        static const uint8_t dec[4]={43u,59u,20u,27u};
        static const uint8_t slide[4]={8u,15u,5u,6u};
        static const uint8_t acc[4]={23u,40u,52u,38u};
        static const uint8_t res[4]={25u,35u,44u,32u};
        f.lead_env=env[ls];
        f.lead_decay=dec[ls];
        f.lead_slide=slide[ls];
        f.lead_accent=acc[ls];
        f.lead_resonance=res[ls];
    }
    if(dg==3u || dg==5u) {
        f.snare_decay=(uint8_t)(31u+ds*3u);
        /* Exact HYP Bristol Trip-hop kick envelope and clean gain;
         * only the Lo-fi Hip-hop substyle opts into saturation and
         * the longer decaying kick. P_DRIVE is KICK ONLY in HYP. */
        f.kick_decay=(uint8_t)(dg==3u && ds==3u?55u:44u);
        f.drive=(uint8_t)(dg==3u && ds==3u?22u:8u);
        if(dg==3u && ds==3u)f.snare_decay=29u;
    }
    if(hg==3u || hg==5u) {
        f.hat_decay=(uint8_t)(hg==3u && hs==3u?20u:18u+hs*4u);
        f.perc_decay=(uint8_t)(hg==3u && hs==3u?20u:24u+hs*3u);
    }
    return f;
}
#endif
