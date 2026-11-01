/* URBAN-1 genre-aware note events, deterministic and host-portable.
 * Musical sketch only: no FM-1 audio rendering or licensed sample content.
 */
#ifndef URBAN_MUSIC_H
#define URBAN_MUSIC_H
#include "urban_song.h"
#define URBAN_GENRES 14u
typedef struct { uint8_t note, velocity, gate; } UrbanNote;
typedef struct { UrbanNote steps[URBAN_STEPS]; uint32_t seed; } UrbanPhrase;
typedef struct { UrbanPhrase sequence[URBAN_SEQUENCES]; } UrbanVoice;
static const uint8_t urban_minor_scale[7]={0,2,3,5,7,8,10};
static uint8_t urban_density(uint8_t genre, unsigned track) {
    static const uint8_t density[URBAN_GENRES]={40,31,25,35,28,33,36,43,70,48,53,78,51,65};
    unsigned d=density[genre%URBAN_GENRES];
    if(track==2) d=d/2u; /* bass */
    if(track==4 || track==5) d=d/3u; /* chords / atmosphere */
    return (uint8_t)d;
}
/* Preserve an anchor motif between song sections, vary its rhythm and register.
 * Key is pitch class 0..11. Notes 36..95. 0 velocity means silence.
 */
static void urban_generate_voice(const UrbanSong *song, unsigned track, uint32_t seed, UrbanVoice *out) {
    if(track>=URBAN_TRACKS) return;
    const UrbanTrack *cfg=&song->tracks[track];
    uint32_t root=seed ^ (track+1u)*0x9e3779b9u;
    uint8_t motif[4];
    for(unsigned i=0;i<4;i++) motif[i]=urban_minor_scale[urban_rng(&root)%7u];
    for(unsigned s=0;s<URBAN_SEQUENCES;s++){
        UrbanPhrase *phrase=&out->sequence[s];
        uint32_t r=root ^ ((s+1u)*0x85ebca6bu);
        phrase->seed=r;
        unsigned density=urban_density(cfg->genre,track);
        /* Intro/outro sparse; hook/drop denser. */
        if(s==0 || s==7) density=density*2u/3u;
        if(s==2 || s==5) density=(density*5u/4u>95u)?95u:density*5u/4u;
        for(unsigned step=0;step<URBAN_STEPS;step++){
            UrbanNote *n=&phrase->steps[step];
            unsigned hit=urban_rng(&r)%100u;
            unsigned motif_index=(step/4u)%4u;
            unsigned degree=motif[motif_index];
            /* Non-drum voices remain in a common natural-minor pitch collection. */
            n->note=(uint8_t)(48u+song->key%12u+degree+(track==2?0u:12u)+(s==2?12u:0u));
            n->velocity=(uint8_t)(hit<density?72u+urban_rng(&r)%40u:0u);
            n->gate=(uint8_t)(track==0 || track==1?1u:2u+urban_rng(&r)%6u);
            /* Drum tracks use conventional MIDI percussion note numbers. */
            if(track==0) n->note=(uint8_t)((step%4u==0u)?36u:38u);
            if(track==1) n->note=42u;
        }
    }
}
#endif
