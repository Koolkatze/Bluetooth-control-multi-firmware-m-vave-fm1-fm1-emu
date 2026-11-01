/* URBAN-1 genre-specific 16-step drum grammars.
 * Bit 0 is step 0. Accent masks define core groove, optional hits add variation.
 * Song subgenre changes the probability and the deterministic variation seed.
 */
#ifndef URBAN_RHYTHM_H
#define URBAN_RHYTHM_H
#include "urban_song.h"
typedef struct { uint16_t kick, snare, hat; uint8_t extras; } UrbanGroove;
static UrbanGroove urban_groove(uint8_t genre,uint8_t subgenre) {
    UrbanGroove g;
    switch(genre) {
    case 0: case 1: case 2: case 6: /* trap family / horrorcore */
        g=(UrbanGroove){0x0111u,0x0100u,0x5555u,26u}; break;
    case 3: case 5: /* hip-hop / underground */
        g=(UrbanGroove){0x1011u,0x0101u,0x5555u,13u}; break;
    case 4: /* trip-hop */
        g=(UrbanGroove){0x0409u,0x0101u,0x4444u,12u}; break;
    case 7: /* reggaeton / dembow */
        g=(UrbanGroove){0x1111u,0x0444u,0x5555u,12u}; break;
    case 8: case 13: /* DnB / liquid jungle */
        g=(UrbanGroove){0x1041u,0x0104u,0xFFFFu,30u}; break;
    case 9: /* dubstep halftime */
        g=(UrbanGroove){0x0021u,0x0100u,0x5555u,20u}; break;
    case 10: case 11: case 12: /* techno / hardcore / trance */
        g=(UrbanGroove){0x1111u,0x0101u,0x5555u,genre==11?28u:14u}; break;
    default:
        g=(UrbanGroove){0x1111u,0x0101u,0x5555u,14u}; break;
    }
    /* More intense subgenres add fills; never remove the core genre groove. */
    g.extras=(uint8_t)(g.extras+(subgenre%5u)*3u);
    return g;
}
static uint8_t urban_drum_hit(uint8_t genre,uint8_t subgenre,
                               unsigned track,unsigned step,unsigned section,
                               uint32_t *rng) {
    UrbanGroove g=urban_groove(genre,subgenre);
    uint16_t mask=track==0?g.kick|g.snare:g.hat;
    uint8_t hit=(uint8_t)((mask>>step)&1u);
    uint32_t v=urban_rng(rng)%100u;
    if(!hit && (section==2u || section==5u) && v<g.extras) hit=1u;
    if((section==0u || section==7u) && !((mask>>step)&1u))
        hit=0u;
    return hit;
}
static uint8_t urban_drum_note(uint8_t genre,unsigned track,unsigned step) {
    if(track==1) return 42u;
    UrbanGroove g=urban_groove(genre,0u);
    return (uint8_t)((g.snare>>step)&1u?38u:36u);
}
#endif
