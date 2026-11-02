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
    {20,24,44,17, 3,16, 4, 2, 9,27, 1}, /* dry boom-bap */
    {45,60,85,17, 2,22,12, 2,18,17, 3}, /* dub trip-hop */
    {30,37,61,26, 3,25, 6, 3,22,34, 1}, /* underground jazz */
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
    f.pad_lfo_rate=(uint8_t)((v[4]+sub)%8u);
    f.pad_lfo_depth=(uint8_t)urban_fx_clamp(v[5]+(int)sub*5,6,60);
    f.pad_detune=(uint8_t)urban_fx_clamp(v[6]+(int)sub*3,3,28);
    f.pad_attack=(uint8_t)urban_fx_clamp(
         (g==1u||g==12u||g==13u?32:14)+(int)sub*3,7,50);
    f.pad_release=(uint8_t)urban_fx_clamp(
         (g==1u||g==2u||g==12u||g==13u?65:38)+(int)sub*4,25,83);
    f.bass_lfo_rate=(uint8_t)((v[7]+sub)%8u);
    f.bass_lfo_depth=(uint8_t)urban_fx_clamp(v[8]+(int)sub*4,6,54);
    f.bass_resonance=(uint8_t)urban_fx_clamp(
        g==0u||g==2u||g==7u?19:36+(int)sub*8,16,68);
    const UrbanLeadTone *lead=&urban_lead_tones[s->tracks[3].genre%URBAN_GENRES];
    f.lead_env=(uint8_t)urban_fx_clamp((int)lead->filter_env+
                                                  (int)sub*3,12,82);
    f.lead_decay=(uint8_t)urban_fx_clamp((int)lead->decay+
                                                  (int)sub*4,12,88);
    f.lead_slide=(uint8_t)urban_fx_clamp((int)lead->slide+
                                                  (int)sub*4,4,78);
    f.lead_accent=(uint8_t)urban_fx_clamp((int)lead->accent+
                                                  (int)sub*3,8,78);
    f.lead_resonance=(uint8_t)urban_fx_clamp((int)lead->resonance+
                                                  (int)sub*3,12,79);
    f.snare_decay=(uint8_t)urban_fx_clamp(g==8u||g==13u?
                  31+(int)sub*4:42+(int)sub*3,20,72);
    f.hat_decay=(uint8_t)urban_fx_clamp(g==0u||g==8u?
                  19+(int)sub*4:36+(int)sub*3,14,65);
    f.kick_decay=(uint8_t)urban_fx_clamp(g<=2u||g==9u?
                  67+(int)sub*3:44+(int)sub*3,30,82);
    f.perc_decay=(uint8_t)urban_fx_clamp(g==7u?
                  24+(int)sub*4:40+(int)sub*2,18,62);
    f.noise_tone=(uint8_t)urban_fx_clamp((int)v[9]+
                                             (int)sub*5,12,78);
    f.noise_pump=(uint8_t)urban_fx_clamp(
         g==8u||g==10u||g==12u?57+(int)sub*5:26+(int)sub*5,20,80);
    return f;
}
#endif
