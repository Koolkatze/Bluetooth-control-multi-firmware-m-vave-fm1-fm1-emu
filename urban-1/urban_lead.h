/* SPDX-License-Identifier: GPL-3.0-only
 * Genre-authored lead phrases for the real HYP-1 monophonic ACID synth.
 * Silence is intentional: verses leave space for vocals/drums, breaks
 * can omit the lead entirely and hooks have identifiable repeatable motifs.
 * 16th-note gates; no audio HAL dependency, deterministic host tests.
 */
#ifndef URBAN_LEAD_H
#define URBAN_LEAD_H
#include "urban_music.h"

typedef struct {
    uint16_t verse,hook,drop;
    uint8_t gate; int8_t register_shift; uint8_t verse_bar_mask,hook_bar_mask,velocity;
} UrbanLeadStyle;
/* Masks are 16-step grid. Bar masks select bars 0..3 that contain lead. */
static const UrbanLeadStyle urban_lead_styles[URBAN_GENRES]={
 /*         VERSE  HOOK  DROP    gate reg  verse hook vel */
 /* Trap */ {0x0101,0x0841,0x0849, 4,-1, 0x5,0xf, 82},
 /* Goth */ {0x0001,0x0801,0x0901, 8,-1, 0x1,0x5, 66},
 /* Sad  */ {0x0081,0x0101,0x0901,10,-1, 0x5,0x7, 68},
 /* Hip  */ {0x0849,0x2489,0x2489, 3,-1, 0x7,0xf, 83},
 /* Trip */ {0x0021,0x1281,0x1481, 6,-1, 0x5,0x7, 66},
 /* Undg */ {0x1089,0x1489,0x3489, 3,-1, 0x7,0xf, 78},
 /* Horr */ {0x0001,0x0901,0x0901, 7,-1, 0x1,0xb, 72},
 /* Reg  */ {0x0249,0x2229,0x2a29, 3,-1, 0x7,0xf, 84},
 /* DnB  */ {0x0081,0x9249,0x9249, 2, 0, 0x5,0xf, 88},
 /* Dub  */ {0x0001,0x0841,0x0881, 5,-1, 0x1,0xf, 73},
 /* Tech */ {0x0020,0x2222,0x2222, 2,-1, 0x5,0xf, 74},
 /* Hard */ {0x0101,0x5555,0x5555, 2, 0, 0x7,0xf, 90},
 /* Trnc */ {0x0880,0x8888,0x8888, 5, 1, 0x7,0xf, 87},
 /* Liqd */ {0x0841,0x2a49,0x2a49, 6, 0, 0x7,0xf, 77}
};
/* Genre-tailored HYP ACID envelope/gesture parameters. This changes
 * the actual synth's filter/envelopes (not just the note names):
 * trap = mellow short pluck, goth = dark long tail, reggaeton = bright
 * pluck, dnb = short articulate sync, techno = bleep, trance = legato
 * resonant lead, liquid = soft expressive.
 */
typedef struct {
    uint8_t filter_env,decay,slide,accent,resonance;
} UrbanLeadTone;
static const UrbanLeadTone urban_lead_tones[URBAN_GENRES]={
    {28,29, 9,24,27}, /* Trap */
    {21,76,45,17,41}, /* Goth trap */
    {18,84,19,13,23}, /* Sad trap */
    {34,27,11,28,29}, /* Hip-hop */
    {22,64,21,18,31}, /* Trip-hop */
    {38,40,25,31,40}, /* Underground */
    {36,61,43,38,49}, /* Horrorcore */
    {45,24, 8,39,26}, /* Reggaeton */
    {51,23,19,59,51}, /* Drum & Bass */
    {62,46,61,58,65}, /* Dubstep */
    {53,22,24,44,55}, /* Techno */
    {65,19,38,71,62}, /* Hardcore */
    {69,65,63,49,47}, /* Trance */
    {29,65,37,24,24}  /* Liquid */
};
static uint8_t urban_lead_active(const UrbanSong *s,unsigned section,
                                  unsigned bar) {
    const UrbanTrack *t=&s->tracks[3];
    const UrbanLeadStyle *p=&urban_lead_styles[t->genre%URBAN_GENRES];
    unsigned b=bar%URBAN_BARS_PER_PHRASE;
    uint32_t variation=urban_mix_seed(s->seed^
           t->seq[section%URBAN_SEQUENCES].seed^
           (uint32_t)(s->melodic_variant*0x7f4a7c15u));
    if(section==URBAN_INTRO)
        return (uint8_t)((t->genre==12u||t->genre==13u) &&
                         b==3u && s->complexity>=2u);
    if(section==URBAN_BREAK)
        return (uint8_t)(t->genre==13u && b==3u &&
                         s->complexity>=2u);
    if(section==URBAN_OUTRO)return (uint8_t)(b==0u);
    if(section==URBAN_BRIDGE)
        return (uint8_t)((t->genre==12u||t->genre==13u) && b==3u);
    if(section==URBAN_VERSE_A||section==URBAN_VERSE_B) {
        uint8_t mask=p->verse_bar_mask;
        /* Some verses are intentionally instrumental-free, others
         * contain an answer only on alternate four-bar periods. */
        if((variation&3u)==0u && t->genre!=12u)
            mask&=0x5u;
        return (uint8_t)((mask>>b)&1u);
    }
    return (uint8_t)((p->hook_bar_mask>>b)&1u);
}
static UrbanNote urban_lead_score(const UrbanSong *s,unsigned section,
                                   unsigned bar,unsigned step,
                                   UrbanNote input) {
    const UrbanTrack *t=&s->tracks[3];
    const UrbanLeadStyle *p=&urban_lead_styles[t->genre%URBAN_GENRES];
    input.velocity=0u;
    if(step>=URBAN_STEPS || !urban_lead_active(s,section,bar))return input;
    uint16_t mask=section==URBAN_HOOK?p->hook:
                  section==URBAN_DROP?p->drop:p->verse;
    uint32_t r=urban_mix_seed(s->seed ^
            t->seq[section%URBAN_SEQUENCES].seed ^
            ((uint32_t)(bar+1u)*0x85ebca6bu) ^
            ((uint32_t)(step+1u)*0x9e3779b9u));
    /* Three deterministic melody-score choices per genre & song,
     * keeping the strong genre motif while changing weaker syncopations. */
    static const uint8_t weak[6]={3,5,6,9,11,14};
    if(s->complexity>1u && t->genre!=1u && t->genre!=2u) {
        unsigned loc=weak[(r>>6)%6u];
        if((r&3u)==0u)mask^=(uint16_t)(1u<<loc);
    }
    if(!(mask&(uint16_t)(1u<<step)))return input;
    /* The input pitch comes from the song's diatonic motif. Score rhythms
     * make audible note onsets instead of blindly preserving the overly
     * dense base generated step sequence. */
    input.velocity=(uint8_t)(p->velocity+(r%11u));
    input.gate=(uint8_t)(p->gate+(t->subgenre%3u));
    if(input.gate>12u)input.gate=12u;
    /* Use one curated high register for uplifting trance, a less shrill
     * central register for the other styles. */
    if(p->register_shift<0 && input.note>=48u+12u)
        input.note=(uint8_t)(input.note-12u);
    else if(p->register_shift>0 && input.note<=100u-12u)
        input.note=(uint8_t)(input.note+12u);
    return input;
}
#endif
