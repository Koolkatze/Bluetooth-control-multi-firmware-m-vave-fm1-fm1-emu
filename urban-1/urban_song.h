/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 deterministic song composition and 8-section transport.
 * Each arrangement entry is one 4/4 bar. Song mode plays the full form;
 * Sequence mode changes A-H only at bar boundaries. No dynamic allocation.
 */
#ifndef URBAN_SONG_H
#define URBAN_SONG_H
#include <stdint.h>
#include "urban_scales.h"
#if !defined(URBAN_EMBEDDED)
#include <string.h>
#endif
#define URBAN_TRACKS 8
#define URBAN_SEQUENCES 8
#define URBAN_STEPS 16
#define URBAN_BARS_PER_PHRASE 4u
#define URBAN_ARRANGEMENT 64
#define URBAN_GENRES 14u
/* Scale count is defined by the 12-TET named-scale registry. */

enum { URBAN_INTRO, URBAN_VERSE_A, URBAN_HOOK, URBAN_VERSE_B,
       URBAN_BRIDGE, URBAN_DROP, URBAN_BREAK, URBAN_OUTRO };
enum { URBAN_NATURAL_MINOR, URBAN_PHRYGIAN, URBAN_DORIAN, URBAN_HARMONIC_MINOR,
       URBAN_IONIAN, URBAN_MIXOLYDIAN, URBAN_PHRYGIAN_DOMINANT,
       URBAN_LYDIAN, URBAN_MELODIC_MINOR, URBAN_LOCRIAN };

typedef struct {
    uint8_t steps[URBAN_STEPS];
    uint32_t seed;
} UrbanPattern;
typedef struct {
    uint8_t genre, subgenre, locked, muted, solo;
    UrbanPattern seq[URBAN_SEQUENCES];
} UrbanTrack;
typedef struct {
    UrbanTrack tracks[URBAN_TRACKS];
    uint8_t arrangement[URBAN_ARRANGEMENT];
    uint8_t arrangement_len, key, tempo, current, queued, playing;
    uint32_t seed;
    uint8_t genre, subgenre, scale, complexity, progression[4];
    uint8_t harmony_variant, melodic_variant, drum_variant, chord_colors[4];
    uint8_t chord_degrees[4];
    uint8_t song_mode, arrangement_pos, section_bar; /* song_mode=1 AUTO, 0 LOOP */
    uint8_t scale_manual; /* preserve explicit EDIT scale through SAVE */
} UrbanSong;

static const uint8_t urban_genre_bpm[URBAN_GENRES] = {
    142, 135, 128, 94, 82, 90, 92, 96, 174, 140, 132, 180, 138, 170
};
/* Lo-fi Hip-hop is a distinct slow pocket inside Hip-hop, not the
 * 94 BPM Boom-Bap/G-Funk clock. An explicit tempo edited by the player
 * is still preserved when regenerating songs. */
static uint8_t urban_style_tempo(unsigned genre,unsigned sub) {
    return (uint8_t)((genre%URBAN_GENRES==3u && sub%4u==3u)?
                     80u:urban_genre_bpm[genre%URBAN_GENRES]);
}
/* Four arrangement colours, varied on SAVE, all *original* performances:
 * study/Rhodes, rainy piano, jazz/Rhodes-flute and sleepy minimal. */
static unsigned urban_lofi_mood(const UrbanSong *s) {
    return (unsigned)(s->melodic_variant&3u);
}
static const uint8_t urban_genre_scale[URBAN_GENRES] = {
    0,1,0,2,0,0,1,0,0,1,0,1,2,2
};
/* Degrees in semitones from tonic: genre-sensitive but ALWAYS one
 * harmonic timeline shared by all tracks even when tracks mix genres.
 */
static const uint8_t urban_progressions[URBAN_GENRES][4] = {
    {0,8,10,5},  /* trap: i-VI-VII-iv */
    {0,1,8,10},  /* goth trap: Phrygian tension */
    {0,8,3,10},  /* sad trap: i-VI-III-VII */
    {0,5,3,10},  /* hip-hop: i-iv-III-VII */
    {0,10,5,8},  /* trip-hop: descending darkness */
    {0,3,5,10},  /* underground: jazzy III-iv-VII */
    {0,1,5,10},  /* horrorcore: dark flat-II */
    {0,10,8,10}, /* reggaeton: minor-VII-VI-VII */
    {0,8,3,10},  /* DnB: harmonic lift */
    {0,1,10,8},  /* dubstep: halfstep tension */
    {0,0,10,8},  /* techno: pedal with shifts */
    {0,1,0,10},  /* hardcore: tonal pedal */
    {0,8,3,10},  /* trance: i-VI-III-VII */
    {0,5,3,10}   /* liquid: i-iv-III-VII */
};
/* True genre/substyle musical affinity. The first mode is the
 * default; the other two are coherent optional colours, selected by
 * SAVE's harmony variation. All 56 styles have independent choices;
 * no random unrestricted choice of chromatic or experimental modes. */
static const uint8_t urban_genre_mode_options[URBAN_GENRES][4]={
  /* Trap */ {  0,  1, 85,  3},
  /* Goth Trap */ {  1, 35,  0, 85},
  /* Sad Trap */ {  0,  4,  2, 85},
  /* Hip-hop */ { 85,  2,  5,  2}, /* Lo-fi primary: Dorian 7-note jazz palette */
  /* Trip-hop */ {  0,  2,  9,  1},
  /* Underground */ {101,  2,  8,  5},
  /* Horrorcore */ {  1, 28, 35, 70},
  /* Reggaeton */ {  0,  6,  4,  2},
  /* Drum & Bass */ {  1,  0,  9,110},
  /* Dubstep */ {  0, 85,  1,  3},
  /* Techno */ {  2,  0,  5, 85},
  /* Hardcore */ {  1,  9,  3,  0},
  /* Trance */ {  0,  4,  6,  2},
  /* Liquid */ {  2,  7,114, 85}
};
static const uint8_t urban_style_scale_alternatives[URBAN_GENRES][4][2]={
  /* 0 */ {{85,101},{49,35},{0,90},{8,35}},
  /* 1 */ {{49,28},{110,9},{2,3},{90,88}},
  /* 2 */ {{85,3},{84,107},{8,101},{101,90}},
  /* 3 */ {{0,101},{114,8},{94,100},{0,4}}, /* Lo-fi alt: minor / major warmth */
  /* 4 */ {{3,85},{8,106},{102,111},{6,85}},
  /* 5 */ {{85,0},{114,101},{20,77},{100,85}},
  /* 6 */ {{49,35},{70,3},{110,9},{77,49}},
  /* 7 */ {{85,2},{3,28},{84,7},{5,8}},
  /* 8 */ {{9,35},{85,101},{111,49},{102,20}},
  /* 9 */ {{85,1},{2,101},{9,110},{28,35}},
  /* 10 */ {{8,5},{85,2},{17,102},{91,84}},
  /* 11 */ {{35,9},{110,49},{35,28},{1,85}},
  /* 12 */ {{2,3},{7,84},{28,3},{8,5}},
  /* 13 */ {{114,8},{84,4},{112,2},{0,101}}
};
/* 56 substyle-specific four-bar chord progressions as modal DEGREES.
 * Each subgenre gets its own functional harmony. This is not a table of
 * alternative genres selected randomly on SAVE; the user chooses the
 * substyle and SAVE varies the cadence/voicings within that style.
 * Chord roots never leave the selected modal scale. */
static const uint8_t urban_chord_degrees[URBAN_GENRES][4][4]={
 /* Classic trap / drill / cloud / rolling hats */
 {{0,5,6,3},{0,1,5,0},{0,3,5,6},{0,6,5,3}},
 /* Witch house / industrial / darkwave / cloud */
 {{0,1,5,6},{0,1,0,6},{0,5,3,6},{0,3,1,5}},
 /* Emo / piano / lo-fi / guitar */
 {{0,5,2,6},{0,3,5,2},{0,3,5,6},{0,5,3,6}},
 /* Boom-bap / jazz / G-funk / lo-fi */
 {{0,3,0,3},{0,3,2,4},{0,0,3,0},{0,3,5,4}}, /* i-IV-vi-VII mellow jazz loop */
 /* Bristol / downtempo / abstract / dub */
 {{0,6,3,5},{0,3,5,2},{0,1,6,5},{0,3,6,0}},
 /* Dusty / jazz underground / experimental / break */
 {{0,3,6,0},{0,2,3,5},{0,4,2,3},{0,6,3,2}},
 /* Memphis / cinematic / industrial / horror */
 {{0,1,6,0},{0,5,1,3},{0,1,0,6},{0,4,1,6}},
 /* Dembow / perreo / romantic major / futuristic Dorian */
 {{0,6,5,6},{0,3,6,5},{0,4,5,3},{0,3,4,6}},
 /* Neuro / jump-up pedal / darkstep / techstep */
 {{0,5,1,6},{0,0,6,0},{0,1,5,0},{0,4,1,6}},
 /* UK 140 / deep / riddim / brostep */
 {{0,5,6,0},{0,3,0,6},{0,1,0,1},{0,6,5,0}},
 /* Detroit / hypnotic / dub / minimal */
 {{0,3,4,0},{0,0,6,0},{0,3,0,6},{0,0,0,0}},
 /* Gabber / industrial / frenchcore / uptempo */
 {{0,0,6,0},{0,1,0,6},{0,4,0,5},{0,0,1,0}},
 /* Progressive / uplifting major / psy Phrygian dom / tech Dorian */
 {{0,5,2,6},{0,4,5,3},{0,1,4,0},{0,3,4,6}},
 /* Soulful / atmospheric / jazzy / deep liquid */
 {{0,3,5,6},{0,5,3,0},{0,2,3,6},{0,6,3,0}}
};
enum { URBAN_CH_TRIAD, URBAN_CH_SEVENTH, URBAN_CH_NINTH,
       URBAN_CH_SUS, URBAN_CH_POWER, URBAN_CH_QUARTAL };
/* Distinct chord voicings, not just shifted roots. 4 bars per substyle.
 * E.g. Jazz rap uses 7th/9th shells, deep dubstep uses suspended pads,
 * gabber uses power chords and romantic reggaeton uses pop triads. */
static const uint8_t urban_substyle_chord_colors[URBAN_GENRES][4][4]={
 {{4,0,4,0},{4,3,4,3},{2,1,2,1},{4,0,3,4}},
 {{1,3,1,3},{4,3,4,0},{1,2,1,2},{2,1,3,2}},
 {{1,2,1,1},{0,1,2,0},{2,1,2,1},{0,3,1,0}},
 {{1,1,2,1},{1,2,1,2},{1,0,1,0},{2,1,2,1}},
 {{1,2,1,2},{1,3,1,0},{5,3,2,5},{1,3,1,3}},
 {{1,1,2,1},{1,2,1,2},{5,2,3,5},{1,0,3,0}},
 {{4,3,4,0},{1,2,3,1},{4,4,3,4},{3,4,1,3}},
 {{0,1,0,1},{0,3,0,3},{0,0,1,0},{1,2,1,0}},
 {{4,3,4,3},{4,4,0,4},{3,4,3,4},{4,3,5,3}},
 {{4,3,4,3},{3,1,3,1},{4,4,3,4},{4,0,4,0}},
 {{3,4,0,3},{3,4,3,4},{3,1,3,1},{4,4,4,4}},
 {{4,4,3,4},{4,3,4,4},{4,4,4,0},{4,4,4,4}},
 {{1,2,1,2},{0,1,0,1},{3,4,3,4},{4,3,1,4}},
 {{1,2,1,2},{2,1,2,1},{2,1,5,2},{1,3,1,0}}
};
/* Legacy family palette stays available for genre-level lookups and
 * diagnostic compatibility; the real score uses the 56-style table. */
static const uint8_t urban_chord_palettes[URBAN_GENRES][4]={
  {0,4,1,0},{0,1,4,3},{1,2,0,1},{1,2,0,3},
  {2,1,3,5},{2,1,5,0},{4,0,3,1},{0,1,3,0},
  {1,3,4,2},{4,3,5,0},{4,3,5,0},{4,4,3,0},
  {1,3,2,0},{2,1,3,5}
};
/* A tiny fixed-state mixer, not a platform RNG dependency. */
static uint32_t urban_mix_seed(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return x ^ (x >> 16);
}

static uint32_t urban_rng(uint32_t *x) {
    uint32_t a = *x ? *x : 0x6d2b79f5u;
    a ^= a << 13; a ^= a >> 17; a ^= a << 5;
    return *x = a;
}
/* Applying a new subgenre must update the actual score WITHOUT
 * resetting playback. The global song still owns one harmonic timeline;
 * independently overridden tracks get their own comping/groove, not an
 * incompatible second key. */
static void urban_song_harmony(UrbanSong *s) {
    unsigned g=s->genre%URBAN_GENRES;
    unsigned sub=s->subgenre%4u;
    if(!s->scale_manual) {
        unsigned variation=s->harmony_variant%4u;
        s->scale=variation<2u?urban_genre_mode_options[g][sub]:
            urban_style_scale_alternatives[g][sub][variation-2u];
    }
    for(unsigned bar=0;bar<4u;bar++) {
        unsigned deg=urban_chord_degrees[g][sub][bar];
        /* Four coherent cadence variants within this same subgenre:
         * optionally resolve the phrase, or hold the tonic for tension. */
        if((s->harmony_variant&1u) && bar==3u)
            deg=urban_chord_degrees[g][sub][1u];
        if((s->harmony_variant&2u) && bar==2u)
            deg=urban_chord_degrees[g][sub][0u];
        /* Project 7-degree genre cadences onto the chosen scale's actual
         * cardinality, including pentatonic, hexatonic and octatonic modes. */
        unsigned root=urban_scale_root_degree(s->scale,deg);
        s->chord_degrees[bar]=(uint8_t)root;
        s->progression[bar]=(uint8_t)urban_scale_pc(s->scale,root);
        unsigned color=urban_substyle_chord_colors[g][sub][bar];
        /* Revoicing one phrase-end chord varies new songs without
         * destroying the selected style's harmonic identity. */
        if(bar==3u && s->melodic_variant%3u==2u) {
            if(color==URBAN_CH_TRIAD)color=URBAN_CH_SEVENTH;
            else if(color==URBAN_CH_SEVENTH)color=URBAN_CH_NINTH;
        }
        s->chord_colors[bar]=(uint8_t)color;
    }
}
/* 56 bars, 14 complete 4-bar phrases: intro, verse, hook, second verse, hook reprise,
 * bridge, drop, breakdown, outro. All 8 A-H sections appear.
 * Repeated C is the SAME hook motif by design, not another random song.
 */
static void urban_song_arrange(UrbanSong *s) {
    static const uint8_t form[][2] = {
        {URBAN_INTRO,4}, {URBAN_VERSE_A,8}, {URBAN_HOOK,8},
        {URBAN_VERSE_B,8}, {URBAN_HOOK,8}, {URBAN_BRIDGE,4},
        {URBAN_DROP,8}, {URBAN_BREAK,4}, {URBAN_OUTRO,4}
    };
    unsigned n=0;
    for(unsigned k=0;k<sizeof(form)/sizeof(form[0]);k++)
        for(unsigned i=0;i<form[k][1] && n<URBAN_ARRANGEMENT;i++)
            s->arrangement[n++]=form[k][0];
    s->arrangement_len=(uint8_t)n;
    s->arrangement_pos=0;
    s->section_bar=0;
    if(s->song_mode) {
        s->current=s->arrangement[0];
        s->queued=s->arrangement[n>1u?1u:0u];
    }
}
static void urban_init(UrbanSong *s, uint32_t seed) {
    memset(s, 0, sizeof(*s));
    s->seed=seed;
    s->tempo=urban_genre_bpm[0];
    s->complexity=2;
    urban_song_harmony(s);
    urban_song_arrange(s);
}
static void urban_song_set_genre(UrbanSong *s,uint8_t genre) {
    uint8_t next=(uint8_t)(genre%URBAN_GENRES);
    if(s->genre!=next)s->scale_manual=0u;
    s->genre=next;
    urban_song_harmony(s);
}
static void urban_song_set_mode(UrbanSong *s,int song_mode) {
    s->song_mode=(uint8_t)!!song_mode;
    if(s->song_mode) {
        s->arrangement_pos=0;
        s->section_bar=0;
        s->current=s->arrangement[0];
        s->queued=s->arrangement[1];
    } else s->queued=s->current;
}
static void urban_generate_track(UrbanSong *s, unsigned track, uint32_t seed) {
    if (track >= URBAN_TRACKS || s->tracks[track].locked) return;
    UrbanTrack *t = &s->tracks[track];
    uint32_t state = seed ^ ((uint32_t)(track+1u)*0x9e3779b9u);
    for (unsigned seq=0;seq<URBAN_SEQUENCES;seq++) {
        UrbanPattern *p=&t->seq[seq];
        p->seed=urban_rng(&state);
        uint32_t r=p->seed;
        unsigned threshold=30u+((seq*7u+track*5u)%35u);
        for(unsigned step=0;step<URBAN_STEPS;step++)
            p->steps[step]=(uint8_t)((urban_rng(&r)%100u)<threshold);
    }
}
/* New song != a re-roll of a few notes against one static chord chart.
 * Compose an entire modal palette, a 4-bar diatonic progression, chord
 * colours, rhythmic fingerprint and melodic contour from the NEW seed.
 * The genre, subgenre, user key, tempo and locked tracks are preserved.
 */
static void urban_generate_song(UrbanSong *s,uint32_t seed) {
    uint8_t g=s->genre%URBAN_GENRES;
    uint8_t former_harmony=s->harmony_variant;
    uint8_t former_melody=s->melodic_variant;
    uint32_t former_seed=s->seed;
    uint32_t r=urban_mix_seed(seed ^ ((uint32_t)g*0x9e3779b9u) ^
                              ((uint32_t)s->subgenre*0x85ebca6bu));
    s->seed=seed;
    s->harmony_variant=(uint8_t)(urban_rng(&r)%4u);
    /* A fresh Generate MUST NOT silently return the last harmony or
     * melodic contour again. Random reseeding alone cannot ensure this;
     * force a meaningful musical change even on an unlucky random draw.
     * The selected genre, subgenre, key, BPM and mode remain intact. */
    if(seed!=former_seed && s->harmony_variant==former_harmony)
        s->harmony_variant=(uint8_t)((former_harmony+1u+
                                 (urban_rng(&r)&1u))%4u);
    /* The chosen subgenre defines its tonal identity. Generating a
     * NEW song varies harmony, voicings and contour without randomly
     * turning a romantic major Reggaeton tune into a Phrygian one. */
    s->melodic_variant=(uint8_t)(urban_rng(&r)%8u);
    if(seed!=former_seed && s->melodic_variant==former_melody)
        s->melodic_variant=(uint8_t)((former_melody+1u+
                                   (urban_rng(&r)%3u))%8u);
    s->drum_variant=(uint8_t)(urban_rng(&r)%4u);
    urban_song_harmony(s);
    urban_song_arrange(s);
    for(unsigned t=0;t<URBAN_TRACKS;t++)
        urban_generate_track(s,t,seed ^ ((uint32_t)t*0x85ebca6bu));
}
static void urban_queue_next(UrbanSong *s,int delta) {
    if(s->song_mode) return; /* automatic arrangement owns the queue */
    int n=(int)s->queued+delta;
    n%=URBAN_SEQUENCES;
    if(n<0)n+=URBAN_SEQUENCES;
    s->queued=(uint8_t)n;
    if(!s->playing)s->current=s->queued;
}
/* Exactly once after 16 steps (one bar); the genre/song arrangement
 * clock advances without altering a track's locked patterns.
 */
/* Each A-H phrase has four real bars with four harmonic steps.
 * Manual LOOP repeats that phrase forever, and the knob queues a change
 * at the next *bar*, never mid-bar or after a forced four-bar wait. */
static unsigned urban_phrase_bar(const UrbanSong *s) {
    return (unsigned)s->section_bar % URBAN_BARS_PER_PHRASE;
}
static void urban_sequence_boundary(UrbanSong *s) {
    if(!s->playing)return;
    if(s->song_mode) {
        s->arrangement_pos=(uint8_t)((s->arrangement_pos+1u)%s->arrangement_len);
        s->current=s->arrangement[s->arrangement_pos];
        s->queued=s->arrangement[(s->arrangement_pos+1u)%s->arrangement_len];
        if(s->arrangement_pos && s->arrangement[s->arrangement_pos-1u]==s->current)
            s->section_bar++;
        else s->section_bar=0;
    } else {
        if(s->current!=s->queued)s->section_bar=0;
        else s->section_bar++;
        s->current=s->queued;
    }
}
#endif
