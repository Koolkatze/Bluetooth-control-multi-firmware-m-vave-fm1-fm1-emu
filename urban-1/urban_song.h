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
#define URBAN_BARS_PER_PHRASE 4u
#define URBAN_ARRANGEMENT 64
#define URBAN_GENRES 14u
#define URBAN_SCALE_COUNT 10u

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
/* Scales are diatonic collections, in semitones from the tonic. Each
 * genre can choose musically related modes on each Generate press.
 * Any track can have its own groove/timbre while sharing the song's
 * selected scale and chord progression.
 */
static const uint8_t urban_scale_intervals[URBAN_SCALE_COUNT][7]={
  {0,2,3,5,7,8,10},   /* Aeolian */
  {0,1,3,5,7,8,10},   /* Phrygian */
  {0,2,3,5,7,9,10},   /* Dorian */
  {0,2,3,5,7,8,11},   /* Harmonic minor */
  {0,2,4,5,7,9,11},   /* Ionian */
  {0,2,4,5,7,9,10},   /* Mixolydian */
  {0,1,4,5,7,8,10},   /* Phrygian dominant */
  {0,2,4,6,7,9,11},   /* Lydian */
  {0,2,3,5,7,9,11},   /* Melodic minor */
  {0,1,3,5,6,8,10}    /* Locrian */
};
static const uint8_t urban_genre_mode_options[URBAN_GENRES][4]={
  {0,0,3,1}, {1,1,9,6}, {0,0,2,3}, {2,0,5,4},
  {0,2,1,8}, {2,5,4,0}, {1,3,9,6}, {0,2,4,3},
  {0,2,5,8}, {1,0,6,3}, {0,2,1,5}, {1,0,6,3},
  {2,0,8,7}, {2,4,5,0}
};
/* Scale-DEGREE (not arbitrary chromatic) chord root movements.
 * 14 genres * 4 alternative 4-chord progressions. Every root belongs
 * to the selected mode, even on subgenre / tonal shifts.
 */
static const uint8_t urban_chord_degrees[URBAN_GENRES][4][4]={
  {{0,5,6,3},{0,5,2,6},{0,3,5,6},{0,6,5,3}}, /* trap */
  {{0,1,5,6},{0,1,0,6},{0,4,1,5},{0,5,1,6}}, /* goth */
  {{0,5,2,6},{0,3,5,2},{0,5,3,4},{0,2,6,5}}, /* sad */
  {{0,3,2,6},{0,4,3,0},{0,5,3,4},{0,2,3,6}}, /* hiphop */
  {{0,6,3,5},{0,4,3,6},{0,5,1,6},{0,3,2,4}}, /* triphop */
  {{0,2,3,6},{0,3,5,2},{0,4,2,3},{0,5,6,2}}, /* underground */
  {{0,1,3,6},{0,6,1,0},{0,4,1,3},{0,5,1,6}}, /* horror */
  {{0,6,5,6},{0,3,6,5},{0,5,3,6},{0,4,3,6}}, /* reggaeton */
  {{0,5,2,6},{0,3,6,4},{0,4,5,6},{0,6,3,5}}, /* dnb */
  {{0,1,6,5},{0,4,1,6},{0,5,1,3},{0,6,1,4}}, /* dubstep */
  {{0,0,6,5},{0,4,0,6},{0,1,0,4},{0,6,3,0}}, /* techno */
  {{0,1,0,6},{0,4,1,0},{0,6,1,4},{0,1,5,6}}, /* hardcore */
  {{0,5,2,6},{0,3,5,2},{0,4,3,6},{0,6,5,2}}, /* trance */
  {{0,3,2,6},{0,5,2,3},{0,4,3,6},{0,6,3,2}}  /* liquid */
};
/* Companion chord colours: triad, seventh, ninth shell, suspension,
 * fifth-only power, stacked fourths. Transposition remains diatonic.
 */
enum { URBAN_CH_TRIAD, URBAN_CH_SEVENTH, URBAN_CH_NINTH,
       URBAN_CH_SUS, URBAN_CH_POWER, URBAN_CH_QUARTAL };
static const uint8_t urban_chord_palettes[URBAN_GENRES][4]={
  {0,4,1,0},{0,1,4,3},{1,2,0,1},{1,2,0,3},
  {2,1,3,5},{2,1,5,0},{4,0,3,1},{0,1,3,0},
  {1,3,4,2},{4,3,1,0},{4,3,5,0},{4,4,3,0},
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
static void urban_song_harmony(UrbanSong *s) {
    unsigned g=s->genre%URBAN_GENRES;
    s->scale=urban_genre_scale[g];
    for(unsigned i=0;i<4u;i++) {
        s->progression[i]=urban_progressions[g][i];
        s->chord_degrees[i]=0;
        for(unsigned d=0;d<7u;d++) {
            if(urban_scale_intervals[s->scale][d]==s->progression[i]) {
                s->chord_degrees[i]=(uint8_t)d;
                break;
            }
        }
        s->chord_colors[i]=urban_chord_palettes[g][i];
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
/* New song != a re-roll of a few notes against one static chord chart.
 * Compose an entire modal palette, a 4-bar diatonic progression, chord
 * colours, rhythmic fingerprint and melodic contour from the NEW seed.
 * The genre, subgenre, user key, tempo and locked tracks are preserved.
 */
static void urban_generate_song(UrbanSong *s,uint32_t seed) {
    uint8_t g=s->genre%URBAN_GENRES;
    uint32_t r=urban_mix_seed(seed ^ ((uint32_t)g*0x9e3779b9u) ^
                              ((uint32_t)s->subgenre*0x85ebca6bu));
    s->seed=seed;
    s->harmony_variant=(uint8_t)(urban_rng(&r)%4u);
    s->scale=urban_genre_mode_options[g][urban_rng(&r)%4u];
    s->melodic_variant=(uint8_t)(urban_rng(&r)%8u);
    s->drum_variant=(uint8_t)(urban_rng(&r)%4u);
    for(unsigned bar=0;bar<4u;bar++) {
        unsigned deg=urban_chord_degrees[g][s->harmony_variant][bar];
        s->chord_degrees[bar]=(uint8_t)deg;
        s->progression[bar]=urban_scale_intervals[s->scale][deg];
        s->chord_colors[bar]=urban_chord_palettes[g][(bar+s->melodic_variant)%4u];
    }
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
