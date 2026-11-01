/* URBAN-1: section-aware bassline sketches, including 808 glide flags.
 * Glide is metadata for the future synth adapter, not rendered audio.
 */
#ifndef URBAN_BASS_H
#define URBAN_BASS_H
#include "urban_song.h"
typedef struct { uint8_t note,velocity,gate,glide; } UrbanBassStep;
typedef struct { UrbanBassStep steps[URBAN_STEPS]; } UrbanBassPhrase;
typedef struct { UrbanBassPhrase seq[URBAN_SEQUENCES]; } UrbanBassSong;
static void urban_generate_bass(const UrbanSong *song,uint32_t seed,UrbanBassSong *out) {
    static const uint8_t roots[4]={0u,8u,3u,10u}; /* i-VI-iv-VII, natural minor */
    uint8_t genre=song->tracks[2].genre;
    uint8_t sub=song->tracks[2].subgenre;
    uint32_t r=seed ^ 0x5f356495u;
    for(unsigned section=0;section<URBAN_SEQUENCES;section++){
        for(unsigned step=0;step<URBAN_STEPS;step++){
            UrbanBassStep *n=&out->seq[section].steps[step];
            uint32_t v=urban_rng(&r);
            unsigned chord=(section/2u+step/8u)%4u;
            uint8_t root=(uint8_t)(36u+(song->key%12u)+roots[chord]);
            uint8_t hit;
            switch(genre){
                case 0: case 1: case 2: case 6: /* trap */
                    hit=(uint8_t)(step==0u||step==6u||step==10u||(step==14u&&(v%3u==0u))); break;
                case 7: /* reggaeton */
                    hit=(uint8_t)(step==0u||step==3u||step==8u||step==11u); break;
                case 8: case 13: /* dnb/jungle */
                    hit=(uint8_t)(step==0u||step==4u||step==7u||step==10u||step==14u); break;
                case 10: case 11: case 12: /* four-on-floor */
                    hit=(uint8_t)(step%4u==0u); break;
                default:
                    hit=(uint8_t)(step==0u||step==7u||step==10u); break;
            }
            if((section==0u||section==7u)&&step>8u) hit=0u;
            n->note=(uint8_t)(root+((v%11u==0u)?12u:0u));
            n->velocity=hit?(uint8_t)(83u+(v%35u)):0u;
            n->gate=(uint8_t)(genre<=2u?5u+(v%5u):2u+(v%4u));
            n->glide=(uint8_t)(hit&&genre<=2u&&sub%2u==0u&&step>=10u&&(v%4u==0u));
        }
    }
}
#endif
