/* SPDX-License-Identifier: GPL-3.0-only
 * Jazz upright bass is now HALF-TIME (2 onsets/bar, 8/4-bar phrase).
 * Test the ACTUAL runtime-accessed note score, not legacy seq template.
 */
#include "urban_bass.h"
#include "urban_instruments.h"
#include <assert.h>
#include <stdio.h>
static unsigned pop(uint16_t m) {
    unsigned count=0u;
    while(m) {count+=(unsigned)(m&1u);m>>=1u;}
    return count;
}
static void check_jazz(unsigned g,unsigned sub,unsigned take) {
    UrbanSong s;
    UrbanBassSong score;
    urban_init(&s,0x29c81u+take*7919u);
    s.genre=(uint8_t)g;s.subgenre=(uint8_t)sub;
    s.complexity=(uint8_t)(take%4u);
    for(unsigned i=0u;i<URBAN_TRACKS;i++) {
        s.tracks[i].genre=(uint8_t)g;
        s.tracks[i].subgenre=(uint8_t)sub;
    }
    urban_generate_song(&s,0x812ab7u+take*541u);
    urban_generate_bass(&s,s.tracks[2].seq[0].seed,&score);
    for(unsigned sec=0u;sec<URBAN_SEQUENCES;sec++) {
        unsigned total=0u;
        for(unsigned bar=0u;bar<4u;bar++) {
            unsigned count=0u,last=0u;
            uint16_t mask=urban_bass_bar_mask(&s.tracks[2],sec,bar);
            assert((mask&~0x0101u)==0u);
            if(sec==URBAN_HOOK || sec==URBAN_VERSE_A ||
               sec==URBAN_VERSE_B || sec==URBAN_DROP)
                assert(mask==0x0101u);
            for(unsigned step=0u;step<16u;step++) {
                UrbanBassStep n=urban_bass_bar_score(&s,&score,sec,bar,step);
                assert((n.velocity>0u)==((mask&(1u<<step))!=0u));
                if(n.velocity) {
                    if(count)assert(step-last==8u);
                    assert(n.glide==0u);
                    assert(n.gate>=8u && n.gate<=10u);
                    last=step;
                    ++count;
                }
            }
            assert(count==pop(mask));
            total+=count;
        }
        if(sec==URBAN_HOOK||sec==URBAN_VERSE_A||
           sec==URBAN_VERSE_B||sec==URBAN_DROP)
            assert(total==8u);
        else assert(total<=8u);
    }
    assert(urban_instrument_program(&s,2u,URBAN_HOOK).jazz_bass>=1u);
    /* Bass substyle overrides, not the globally selected song genre,
     * govern the halved pocket. */
    s.genre=10u;s.subgenre=0u;
    for(unsigned bar=0;bar<4u;bar++)
        assert(urban_bass_bar_mask(&s.tracks[2],URBAN_HOOK,bar)==0x0101u);
}
int main(void) {
    for(unsigned take=0u;take<64u;take++) {
        check_jazz(3u,1u,take);
        check_jazz(5u,1u,take);
        check_jazz(13u,2u,take);
    }
    /* All other 53 substyles retain their original four-bar masks. */
    for(unsigned g=0u;g<URBAN_GENRES;g++)
      for(unsigned sub=0u;sub<4u;sub++) {
        if((g==3u&&sub==1u)||(g==5u&&sub==1u)||
           (g==13u&&sub==2u))continue;
        UrbanTrack t={0};
        t.genre=(uint8_t)g;t.subgenre=(uint8_t)sub;
        for(unsigned bar=0u;bar<4u;bar++)
            assert(urban_bass_bar_mask(&t,URBAN_HOOK,bar)==
                   urban_bass_fourbar[g][sub][bar]);
    }
    puts("URBAN-1: 192 jazz 4-bar half-time takes; exact 2 notes/bar, no stray attacks; 53 unaffected substyles PASS");
    return 0;
}
