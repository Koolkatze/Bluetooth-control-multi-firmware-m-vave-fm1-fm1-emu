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
/* Relative scale-degree figures, not a generic rising arpeggio.
 * Notes follow chord-degree roots in the song's CURRENT modal scale.
 * Trap is nearly static, boom-bap answers briefly, reggaeton favours
 * thirds and nearby steps, DnB is angular, trance climbs towards the
 * hook, liquid is consonant and lyrical. */
static const int8_t urban_lead_degrees[URBAN_GENRES][8]={
    {0,2,1,0,-1,0,2,0},      /* trap */
    {0,-1,-2,0,1,-1,-2,0},   /* goth */
    {0,1,2,1,0,-1,0,-2},     /* sad */
    {2,0,1,-1,2,0,-1,0},     /* hip hop */
    {0,-2,0,1,-1,0,2,-1},    /* trip hop */
    {2,4,3,1,2,0,-1,1},     /* underground */
    {0,-1,1,-2,0,1,-1,-2},   /* horrorcore */
    {0,2,1,2,0,1,-1,0},     /* reggaeton */
    {0,3,-1,2,-2,1,4,-1},   /* DnB */
    {0,4,0,-1,0,3,-1,0},    /* dubstep */
    {0,0,1,0,0,-1,0,1},     /* techno */
    {0,4,0,4,-1,4,0,4},     /* hardcore */
    {0,1,2,4,5,4,2,1},      /* trance */
    {2,1,3,2,0,-1,2,1}      /* liquid */
};
/* Probability of there being a separate foreground lead AT ALL.
 * A beat can be complete with its bass, drums and pad alone.
 * Substyles without foreground leads retain their character rather
 * than adding a compulsory generic synth solo. */
static const uint8_t urban_lead_chance[URBAN_GENRES][4]={
    {25,45,12,35},   /* Trap: often leaves room for vocals / 808 */
    {20,35,18,12},   /* Goth: sparse dark distant motifs */
    {35,20,12,30},   /* Sad: pluck/piano chord main layer */
    {12,28,35,10},   /* Hip-hop: boom-bap especially vocal-forward */
    {12,15,22,12},   /* Trip-hop: atmosphere and bass dominate */
    {15,40,25,35},   /* Underground: sample/chord front */
    {25,32,20,25},   /* Horrorcore */
    {45,55,24,42},   /* Reggaeton: hook, not continuous lead */
    {15,35,18,18},   /* DnB: Reese/breaks dominate */
    {30, 8,30,48},   /* Dubstep: bass leads on deep variants */
    { 8,24,12, 3},   /* Techno: hypnotic and minimal may need no solo */
    { 4, 7,30,18},   /* Hardcore: gabber = kick is the lead */
    {72,94,62,70},   /* Trance: one of the few genuinely lead-forward genres */
    {28,20,45,15}    /* Liquid: soulful pads and chords also foreground */
};
static uint8_t urban_lead_presence(const UrbanSong *s) {
    const UrbanTrack *t=&s->tracks[3];
    unsigned g=t->genre%URBAN_GENRES,sub=t->subgenre%4u;
    uint32_t selection=urban_mix_seed(s->seed ^
        t->seq[URBAN_HOOK].seed ^ 0x529dc91bu);
    return (uint8_t)(selection%100u<urban_lead_chance[g][sub]);
}
static uint8_t urban_lead_active(const UrbanSong *s,unsigned section,
                                  unsigned bar) {
    const UrbanTrack *t=&s->tracks[3];
    const UrbanLeadStyle *p=&urban_lead_styles[t->genre%URBAN_GENRES];
    unsigned b=bar%URBAN_BARS_PER_PHRASE;
    uint32_t variation=urban_mix_seed(s->seed^
           t->seq[section%URBAN_SEQUENCES].seed^
           (uint32_t)(s->melodic_variant*0x7f4a7c15u));
    if(!urban_lead_presence(s))return 0u;
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
        if((variation&3u)==0u && t->genre!=12u)
            mask&=0x5u;
        return (uint8_t)((mask>>b)&1u);
    }
    return (uint8_t)((p->hook_bar_mask>>b)&1u);
}
static uint16_t urban_lead_bar_mask(const UrbanSong *s,unsigned section,
                                   unsigned bar) {
    const UrbanTrack *t=&s->tracks[3];
    const UrbanLeadStyle *p=&urban_lead_styles[t->genre%URBAN_GENRES];
    uint16_t mask=section==URBAN_HOOK?p->hook:
                  section==URBAN_DROP?p->drop:p->verse;
    uint32_t r=urban_mix_seed(s->seed ^
            t->seq[section%URBAN_SEQUENCES].seed ^
            ((uint32_t)(bar+1u)*0x85ebca6bu) ^
            ((uint32_t)(s->melodic_variant+1u)*0x9e3779b9u));
    static const uint8_t weak[6]={3,5,6,9,11,14};
    if(s->complexity>1u && t->genre!=1u && t->genre!=2u) {
        unsigned edits=s->complexity==3u?2u:1u;
        for(unsigned i=0;i<edits;i++) {
            unsigned loc=weak[urban_rng(&r)%6u];
            if(urban_rng(&r)%100u<45u)
                mask^=(uint16_t)(1u<<loc);
        }
    }
    return mask;
}
static UrbanNote urban_lead_score(const UrbanSong *s,unsigned section,
                                   unsigned bar,unsigned step,
                                   UrbanNote input) {
    const UrbanTrack *t=&s->tracks[3];
    const UrbanLeadStyle *p=&urban_lead_styles[t->genre%URBAN_GENRES];
    unsigned g=t->genre%URBAN_GENRES;
    input.velocity=0u;
    if(step>=URBAN_STEPS || !urban_lead_active(s,section,bar))return input;
    if(!(urban_lead_bar_mask(s,section,bar)&(uint16_t)(1u<<step)))return input;
    uint32_t r=urban_mix_seed(s->seed ^
            t->seq[section%URBAN_SEQUENCES].seed ^
            ((uint32_t)(bar+1u)*0x85ebca6bu) ^
            ((uint32_t)(step+1u)*0x9e3779b9u));
    /* Strong onsets outline chord functions; weak onsets use a genuine
     * genre contour, whose pitches live within the selected mode. */
    unsigned contour_pos=(step/2u+(s->melodic_variant%8u))%8u;
    int relative=urban_lead_degrees[g][contour_pos];
    if((step%4u)==0u && (g==0u||g==1u||g==2u||g==9u))
        relative=0;
    int degree=(int)s->chord_degrees[bar%4u]+relative;
    int octave=(g==12u?5:g==8u||g==11u?4:4)+p->register_shift;
    while(degree<0){degree+=7;octave--;}
    while(degree>=7){degree-=7;octave++;}
    int pitch=12*octave+(int)(s->key%12u)+
             (int)urban_scale_intervals[s->scale%URBAN_SCALE_COUNT][degree];
    while(pitch<48)pitch+=12;
    while(pitch>96)pitch-=12;
    input.note=(uint8_t)pitch;
    input.velocity=(uint8_t)(p->velocity+(r%11u));
    input.gate=(uint8_t)(p->gate+(t->subgenre%3u));
    if(input.gate>12u)input.gate=12u;
    return input;
}
#endif
