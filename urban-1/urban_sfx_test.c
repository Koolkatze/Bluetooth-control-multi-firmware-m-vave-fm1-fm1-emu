/* SPDX-License-Identifier: GPL-3.0-only
 * 56 styles x 12 seeds: every Sound FX must be a scored musical event,
 * not an inert label or random per-frame sample.
 */
#include "urban_sfx.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    unsigned used[URBAN_SFX_COUNT]={0};
    for(unsigned genre=0u;genre<URBAN_GENRES;genre++)
    for(unsigned sub=0u;sub<4u;sub++) {
        UrbanSfxStyle palette=urban_sfx_styles[genre][sub];
        assert(palette.primary<URBAN_SFX_COUNT);
        assert(palette.secondary<URBAN_SFX_COUNT);
        assert(palette.level>=20u && palette.level<=60u);
        used[palette.primary]++; used[palette.secondary]++;
        assert(urban_sfx_names[palette.primary] &&
               strlen(urban_sfx_names[palette.primary])>4u);
        for(unsigned take=0u;take<12u;take++) {
            UrbanSong s;
            urban_init(&s,0x2300u+genre*401u+sub*73u+take);
            s.genre=(uint8_t)genre;
            s.subgenre=(uint8_t)sub;
            s.complexity=3u;
            s.tracks[6].genre=(uint8_t)genre;
            s.tracks[6].subgenre=(uint8_t)sub;
            urban_generate_song(&s,0x99fa101u+genre*997u+sub*23u+take);
            unsigned count=0u;
            for(unsigned section=0u;section<URBAN_SEQUENCES;section++)
            for(unsigned bar=0u;bar<4u;bar++)
            for(unsigned step=0u;step<16u;step++) {
                UrbanSfxEvent a=urban_sfx_event(&s,section,bar,step);
                UrbanSfxEvent b=urban_sfx_event(&s,section,bar,step);
                assert(memcmp(&a,&b,sizeof(a))==0);
                if(a.level) {
                    count++;
                    assert(a.type==palette.primary || a.type==palette.secondary);
                    assert(a.length>=1u && a.length<=8u);
                    assert(a.level>=palette.level && a.level<=palette.level+6u);
                    assert((section==URBAN_INTRO && bar==3u && step==12u)
                        || section!=URBAN_INTRO);
                }
            }
            assert(count>=7u && count<=12u); /* sparse, not toy-like 16th repeats */
            s.complexity=0u;
            for(unsigned step=0u;step<16u;step++)
                assert(urban_sfx_event(&s,URBAN_VERSE_A,1u,step).level==0u);
        }
    }
    for(unsigned i=0;i<URBAN_SFX_COUNT;i++)assert(used[i]);
    puts("URBAN-1 SFX: 56 subgenres, 10 real synth types, sparse deterministic 4-bar cues PASS");
    return 0;
}
