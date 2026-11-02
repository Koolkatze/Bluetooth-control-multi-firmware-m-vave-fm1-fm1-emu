/* SPDX-License-Identifier: GPL-3.0-only
 * Trance four-style deterministic 16th grid regressions. Distinct musical
 * designs, no accidental late-sixteenth pickups, note gate and range.
 */
#include "urban_lead.h"
#include <assert.h>
#include <stdio.h>
static unsigned pop(uint16_t x){unsigned n=0u;while(x){n+=x&1u;x>>=1u;}return n;}
int main(void) {
    unsigned totals[4]={0};
    for(unsigned sub=0;sub<4;sub++) {
        uint16_t hook=urban_trance_masks[sub][1];
        for(unsigned other=0;other<sub;other++)
            assert(hook!=urban_trance_masks[other][1]);
        assert(hook);
        /* The old 0x8888 wrongly anticipated every beat by a late 16th.
         * These genre styles must place attacks on even eighths/quarters. */
        assert((hook&0xaaaau)==0u);
        assert((urban_trance_masks[sub][0]&0xaaaau)==0u);
        assert((urban_trance_masks[sub][2]&0xaaaau)==0u);
        if(sub==2u)assert(pop(hook)==8u);
        if(sub==3u)assert(pop(hook)==4u);
        for(unsigned take=0;take<96u;take++) {
            UrbanSong s;
            urban_init(&s,0x1200u+sub*1021u+take);
            s.genre=s.tracks[3].genre=12u;
            s.subgenre=s.tracks[3].subgenre=(uint8_t)sub;
            urban_generate_song(&s,0x8100u+sub*791u+take*17u);
            unsigned active=urban_lead_presence(&s);
            for(unsigned sec=0u;sec<URBAN_SEQUENCES;sec++)
            for(unsigned bar=0u;bar<4u;bar++)
            for(unsigned step=0u;step<16u;step++) {
                UrbanNote input={60u,0u,3u};
                UrbanNote n=urban_lead_score(&s,sec,bar,step,input);
                UrbanNote repeat=urban_lead_score(&s,sec,bar,step,input);
                assert(n.note==repeat.note && n.velocity==repeat.velocity &&
                       n.gate==repeat.gate);
                if(n.velocity) {
                    assert(active);
                    assert((step&1u)==0u);
                    assert(n.note>=60u && n.note<=88u);
                    assert(n.gate==urban_trance_gates[sub]);
                    totals[sub]++;
                }
            }
        }
    }
    for(unsigned i=0;i<4;i++) assert(totals[i]>50u);
    puts("Trance: 4 unique on-grid rhythmic scores, 384 takes, no late 16ths or toy octave leaps PASS");
    return 0;
}
