/* URBAN-1 initial portable song-state prototype.
 * Not yet integrated with FM-1 audio engine or HYP-1 platform.
 * C99, no dynamic allocation. */
#ifndef URBAN_SONG_H
#define URBAN_SONG_H
#include <stdint.h>
#include <string.h>
#define URBAN_TRACKS 8
#define URBAN_SEQUENCES 8
#define URBAN_STEPS 16
#define URBAN_ARRANGEMENT 64

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
} UrbanSong;
static uint32_t urban_rng(uint32_t *x) {
    uint32_t a = *x ? *x : 0x6d2b79f5u;
    a ^= a << 13; a ^= a >> 17; a ^= a << 5;
    return *x = a;
}
static void urban_init(UrbanSong *s, uint32_t seed) {
    memset(s, 0, sizeof(*s));
    s->seed = seed;
    s->tempo = 140;
    s->arrangement_len = URBAN_SEQUENCES;
    for (unsigned i=0;i<URBAN_SEQUENCES;i++) s->arrangement[i]=(uint8_t)i;
}
static void urban_generate_track(UrbanSong *s, unsigned track, uint32_t seed) {
    if (track >= URBAN_TRACKS || s->tracks[track].locked) return;
    UrbanTrack *t = &s->tracks[track];
    uint32_t state = seed ^ ((uint32_t)(track+1)*0x9e3779b9u);
    for (unsigned seq=0;seq<URBAN_SEQUENCES;seq++) {
        UrbanPattern *p = &t->seq[seq];
        p->seed = urban_rng(&state);
        /* Temporary event pattern: replace with genre-specific grammar.
         * Section and track affect density; generated patterns are deterministic. */
        uint32_t r = p->seed;
        unsigned threshold = 30u + ((seq*7u + track*5u)%35u);
        for (unsigned step=0;step<URBAN_STEPS;step++)
            p->steps[step] = (uint8_t)((urban_rng(&r)%100u)<threshold);
    }
}
static void urban_generate_song(UrbanSong *s, uint32_t seed) {
    s->seed=seed;
    for (unsigned t=0;t<URBAN_TRACKS;t++)
        urban_generate_track(s,t,seed ^ ((uint32_t)t*0x85ebca6bu));
}
static void urban_queue_next(UrbanSong *s, int delta) {
    int n = (int)s->queued + delta;
    n %= URBAN_SEQUENCES;
    if (n < 0) n += URBAN_SEQUENCES;
    s->queued=(uint8_t)n;
    if (!s->playing) s->current=s->queued;
}
/* Call exactly once when the current sequence completes. */
static void urban_sequence_boundary(UrbanSong *s) {
    if (s->playing) s->current=s->queued;
}
#endif
