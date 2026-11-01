/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 deterministic song composition and 8-section transport.
 * Each arrangement entry is one 4/4 bar. Song mode plays the full form;
 * Sequence mode changes A-H only at bar boundaries. No dynamic allocation.
 */
#ifndef URBAN_SONG_H
#define URBAN_SONG_H
#include <stdint.h>
#if !defined(URBAN_EMBEDDED)
#include <string.h>
#endif
#define URBAN_TRACKS 8
#define URBAN_SEQUENCES 8
#define URBAN_STEPS 16
#define URBAN_ARRANGEMENT 64
#define URBAN_GENRES 14u

enum { URBAN_INTRO, URBAN_VERSE_A, URBAN_HOOK, URBAN_VERSE_B,
       URBAN_BRIDGE, URBAN_DROP, URBAN_BREAK, URBAN_OUTRO };
enum { URBAN_NATURAL_MINOR, URBAN_PHRYGIAN, URBAN_DORIAN, URBAN_HARMONIC_MINOR };

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
    uint8_t genre, scale, progression[4];
    uint8_t song_mode, arrangement_pos, section_bar;
} UrbanSong;

static const uint8_t urban_genre_bpm[URBAN_GENRES] = {
    142, 135, 128, 94, 82, 90, 92, 96, 174, 140, 132, 180, 138, 170
};
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
static uint32_t urban_rng(uint32_t *x) {
    uint32_t a = *x ? *x : 0x6d2b79f5u;
    a ^= a << 13; a ^= a >> 17; a ^= a << 5;
    return *x = a;
}
static void urban_song_harmony(UrbanSong *s) {
    unsigned g=s->genre%URBAN_GENRES;
    s->scale=urban_genre_scale[g];
    for(unsigned i=0;i<4u;i++) s->progression[i]=urban_progressions[g][i];
}
/* 56 bars: intro, verse, hook, second verse, hook reprise,
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
    urban_song_harmony(s);
    urban_song_arrange(s);
}
static void urban_song_set_genre(UrbanSong *s,uint8_t genre) {
    s->genre=(uint8_t)(genre%URBAN_GENRES);
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
static void urban_generate_song(UrbanSong *s,uint32_t seed) {
    s->seed=seed;
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
