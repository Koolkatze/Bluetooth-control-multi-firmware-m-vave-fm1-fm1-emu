/* SPDX-License-Identifier: GPL-3.0-only
 * Curated programs over all the 48 actual HYP-1 synthesis sounds,
 * 8 drum kits and 8 noise colours. These are NOT imported X0X/TRIBE
 * engines or samples. Those require a separate DSP/mixer port.
 * One generated song selects reproducible timbres, and each regenerated
 * track may select a fresh timbre without affecting unrelated tracks.
 */
#ifndef URBAN_INSTRUMENTS_H
#define URBAN_INSTRUMENTS_H
#include "urban_style.h"
static const char *const urban_pad_names[16]={
    "Rhodes","Voices","Strings","Glass","Organ","Warm","Stabs","Dark",
    "Dub","Trance","Poly","Hollow","Pings","Hits","Rave","Sine"
};
static const char *const urban_bass_names[16]={
    "Saws","Warm","Grit","Dirty","Reese","Pulse","FM","Organ",
    "Rumble","Sub","Square","Hoover","Fifths","Metal","Sync","Bright"
};
static const char *const urban_lead_names[16]={
    "Saw","Square","Driven","Sync","Pluck","PWM","Chip","Sub",
    "Trance","Bleep","Fifth","Bell","Organ","Laser","Fold","Soft"
};
static const char *const urban_kit_names[8]={
    "Hypno","House","Techno","Acid","Break","Minimal","Hard","808"
};
static const char *const urban_noise_names[8]={
    "White","Pink","Brown","Blue","Violet","Grey","Black","Yellow"
};
/* Alternative sound choices follow each genre's musical palette.
 * These 42 references are real HYP-1 sound-bank IDs, not fake GM patches. */
static const uint8_t urban_alt_sounds[URBAN_GENRES][3][3]={
  /*                        bass        lead        pad */
  {{9,8,10},            {4,9,15},    {5,3,11}}, /* trap */
  {{8,3,9},             {15,7,2},    {7,11,3}}, /* goth */
  {{9,1,7},             {11,4,15},   {0,2,5}},  /* sad */
  {{1,7,9},             {15,12,5},   {0,4,5}},  /* hip-hop */
  {{8,2,6},             {15,9,4},    {7,3,8}},  /* trip */
  {{1,7,9},             {15,12,5},   {0,4,5}},  /* underground */
  {{8,13,9},            {2,13,9},    {7,11,14}},/* horror */
  {{9,6,1},             {4,11,9},    {6,0,10}}, /* reggaeton */
  {{4,14,15},           {2,6,3},     {6,10,1}}, /* dnb */
  {{4,11,9},            {2,14,7},    {7,8,14}}, /* dubstep */
  {{5,0,1},             {0,9,1},     {6,8,13}}, /* techno */
  {{11,13,14},          {2,14,13},   {14,7,6}}, /* hardcore */
  {{5,12,6},            {8,10,3},    {9,10,14}},/* trance */
  {{1,7,9},             {15,11,8},   {0,1,5}}   /* liquid */
};
/* Bass timbres must be selected INSIDE the subgenre's sound palette:
 * previously a single generic genre-wide 3-preset pool overrode the
 * subgenre patch on 75% of Generate presses, making drill, cloud,
 * neuro and jump-up effectively share the same bass.
 * 4 real HYP DRONE programs per named substyle; the per-style
 * filter, envelope, octave, LFO and actual note score also change. */
static const uint8_t urban_subgenre_bass_sounds[URBAN_GENRES][4][4]={
 /* trap: 808 / drill / ambient / rolled */
 {{9,10,8,9},{9,3,2,8},{9,1,8,7},{10,9,5,8}},
 /* goth: witch / industrial / darkwave / cloud */
 {{8,9,3,8},{3,2,13,11},{5,1,8,7},{9,8,1,6}},
 /* sad: emo / piano / lo-fi / guitar */
 {{9,1,8,9},{1,9,7,1},{1,7,9,8},{1,9,5,7}},
 /* Hip-hop: deep fundamentals, no metallic FM/Fifths/bright Saws.
  * Keep four audible variations within the right acoustic/analog family. */
 {{1,7,9,1},{7,1,9,7},{1,7,5,10},{1,9,7,1}},
 /* trip-hop: Bristol / down / abstract / dub */
 {{8,9,2,6},{1,8,9,7},{6,13,2,9},{8,9,1,4}},
 /* Underground: acoustic-like bass or dark analog; no bell-like FM. */
 {{1,7,9,1},{7,1,9,7},{1,8,7,9},{1,7,5,9}},
 /* horror: Memphis / cinematic / industrial / dark */
 {{8,9,3,8},{9,8,1,7},{3,13,2,11},{8,9,11,4}},
 /* reggaeton: dembow / perreo / romantic / future */
 {{9,10,1,9},{10,9,5,6},{1,7,9,1},{6,5,14,9}},
 /* DnB: neuro / jump-up / darkstep / techstep */
 {{4,14,2,4},{11,15,4,11},{8,3,4,13},{14,4,2,15}},
 /* dubstep: 140 / deep / riddim / brostep */
 {{4,11,9,4},{9,8,1,9},{11,4,14,3},{4,13,14,11}},
 /* techno: Detroit / hypnotic / dub / minimal */
 {{5,0,6,5},{0,1,5,7},{1,8,9,5},{5,0,1,5}},
 /* hardcore: gabber / industrial / frenchcore / uptempo */
 {{11,13,14,11},{13,3,14,2},{11,13,15,14},{14,11,13,3}},
 /* trance: progressive / uplifting / psy / tech */
 {{5,12,6,5},{12,5,14,12},{6,14,4,15},{14,5,13,6}},
 /* liquid: soulful / atmosphere / jazz / deep */
 {{1,7,9,1},{9,1,8,7},{7,6,1,12},{9,1,8,9}}
};
typedef struct {
    UrbanStylePatch patch;
    uint8_t noise_color,noise_amount,program_variant;
    uint8_t jazz_voice; /* 0 = native HYP, 1..5 = real jazz DSP */
    uint8_t jazz_bass;  /* 0 = HYP electronic DRONE, 1 = upright, 2 = finger bass */
} UrbanInstrumentProgram;
static UrbanInstrumentProgram urban_instrument_program(
    const UrbanSong *s,unsigned track,unsigned section) {
    UrbanInstrumentProgram p;
    const UrbanTrack *t=&s->tracks[track%URBAN_TRACKS];
    unsigned genre=t->genre%URBAN_GENRES;
    uint32_t seed=urban_mix_seed(t->seq[0].seed ^ s->seed ^
             ((uint32_t)(track+1u)*0x9e3779b9u));
    p.patch=urban_style_patch(genre,t->subgenre);
    /* 4 related timbres per selected subgenre; same selection in the
     * repeated hook. A fresh 'Generate' changes the sound as well as
     * the notes, while staying within the genre's curated family. */
    unsigned variation=(seed>>5)&3u;
    unsigned column=track==2u?0u:track==3u?1u:2u;
    p.program_variant=(uint8_t)variation;
    p.jazz_voice=0u;
    p.jazz_bass=0u;
    /* A warm drone is NOT an acoustic bass. Jazz subgenres require a
     * distinct string-pluck instrument, never the Reese/FM/Organ bank.
     * Only the bass track owns this program, so independent overrides work. */
    if(track==2u && ((genre==3u && t->subgenre%4u==1u) ||
                    (genre==5u && t->subgenre%4u==1u) ||
                    (genre==13u && t->subgenre%4u==2u))) {
        p.jazz_bass=(uint8_t)(genre==13u && variation==3u?2u:1u);
    }
    /* Dedicated modelled instrument voices, NOT fake HYP sound IDs.
     * The lead and chord track each honour its own named subgenre. */
    if((track==3u||track==4u) &&
       ((genre==3u&&t->subgenre%4u==1u) ||
        (genre==5u&&t->subgenre%4u==1u) ||
        (genre==13u&&t->subgenre%4u==2u) ||
        (genre==2u&&t->subgenre%4u==1u))) {
        static const uint8_t jazz_chords[4][4]={
            {2,1,2,1}, /* Jazz Rap: Rhodes, acoustic grand */
            {1,2,1,2}, /* Jazz Underground: pianist-led */
            {2,4,2,1}, /* Jazz Liquid: Rhodes, vibraphone */
            {1,1,2,1}  /* Sad Piano: mostly acoustic */
        };
        static const uint8_t jazz_leads[4][4]={
            {3,4,3,5}, /* Jazz Rap: flute, vibes, bells */
            {4,3,5,4}, /* Underground: vibes, flute, bells */
            {3,4,5,3}, /* Liquid: airy flute and mallets */
            {4,3,4,5}  /* Sad piano: soft mallet responses */
        };
        unsigned row=genre==3u?0u:genre==5u?1u:genre==13u?2u:3u;
        p.jazz_voice=track==3u?
            jazz_leads[row][variation]:jazz_chords[row][variation];
    }
    if(track==2u) {
        /* The bass sound is now chosen from the named SUBGENRE, never
         * from the old genre-global alternative pool. */
        p.patch.bass=urban_subgenre_bass_sounds[genre][t->subgenre%4u][variation];
    } else if(genre==12u) {
        /* Progressive, Uplifting, Psytrance and Tech Trance no longer
         * share the same three stochastic chip/Fifth/Sync timbres.
         * Give each style a coherent lead AND a separate pad family. */
        static const uint8_t trance_leads[4][4]={
            {15u,8u,5u,15u}, /* Progressive: soft analog / warm PWM */
            {8u,5u,8u,0u},   /* Uplifting: bright sustained analog */
            {3u,2u,3u,5u},   /* Psy: filtered sync/acid plucks */
            {5u,3u,5u,0u}    /* Tech: rhythmic short synth stabs */
        };
        static const uint8_t trance_pads[4][4]={
            {5u,9u,5u,1u},{9u,10u,9u,6u},
            {7u,8u,7u,14u},{6u,9u,5u,6u}
        };
        unsigned sub=t->subgenre%4u;
        if(track==3u)p.patch.lead=trance_leads[sub][variation];
        if(track==4u)p.patch.pad=trance_pads[sub][variation];
    } else if(genre==3u || genre==5u) {
        /* Never let a generic, RNG-selected Chip/Bell/Pluck overwrite
         * a laid-back hip-hop patch. Map the melodic layer by SUBSTYLE.
         * G-Funk alone may select warm analog PWM, while boom-bap and
         * lo-fi stay soft/organ-like; pad remains Rhodes, warm or organ. */
        static const uint8_t hip_leads[4][4]={
            {15,12,15,12},{15,12,15,12},{5,15,5,12},{15,12,15,12}
        };
        static const uint8_t hip_pads[4][4]={
            {0,5,0,4},{0,4,5,0},{4,5,0,4},{0,5,0,4}
        };
        unsigned sub=t->subgenre%4u;
        if(track==3u)p.patch.lead=hip_leads[sub][variation];
        if(track==4u)p.patch.pad=hip_pads[sub][variation];
    } else if(variation) {
        unsigned alt=urban_alt_sounds[genre][column][variation-1u];
        if(track==3u)p.patch.lead=(uint8_t)alt;
        if(track==4u)p.patch.pad=(uint8_t)alt;
    }
    /* Section colouring: bridges are lighter, drops stronger, but not
     * a different random instrument every single bar. */
    if(section==URBAN_BRIDGE||section==URBAN_BREAK) {
        if(track==3u)p.patch.lead_cut=(uint8_t)(p.patch.lead_cut>16u?
                                  p.patch.lead_cut-16u:0u);
    }
    if(section==URBAN_DROP) {
        unsigned cut=(unsigned)p.patch.lead_cut+8u;
        p.patch.lead_cut=(uint8_t)(cut>100u?100u:cut);
    }
    /* Eight native HYP noise colours supply genre-specific atmosphere:
     * crackle/tape-like textures, wind, hiss etc, NOT imported samples. */
    static const uint8_t noise[URBAN_GENRES]={
        2,6,2,6,7,6,4,1,3,1,0,4,5,2
    };
    static const uint8_t level[URBAN_GENRES]={
        4,15,6,7,17,12,18,2,5,8,3,6,11,15
    };
    p.noise_color=noise[genre];
    p.noise_amount=level[genre];
    return p;
}
#endif
