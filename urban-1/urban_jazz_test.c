/* SPDX-License-Identifier: GPL-3.0-only
 * DSP instrument assignment regressions: four jazz/piano styles get real
 * engine programs on lead AND chord tracks, other genres keep native HYP.
 */
#include "urban_instruments.h"
#include <assert.h>
#include <stdio.h>
static void test_style(unsigned genre,unsigned sub) {
    unsigned heard_chords[6]={0},heard_leads[6]={0};
    for(unsigned take=0u;take<128u;take++) {
        UrbanSong s;
        urban_init(&s,0x5af13u+take*7919u);
        s.genre=(uint8_t)genre;s.subgenre=(uint8_t)sub;
        for(unsigned i=0u;i<URBAN_TRACKS;i++) {
            s.tracks[i].genre=(uint8_t)genre;
            s.tracks[i].subgenre=(uint8_t)sub;
        }
        urban_generate_song(&s,0x9e37u+take*2917u);
        UrbanInstrumentProgram c=urban_instrument_program(&s,4u,URBAN_HOOK);
        UrbanInstrumentProgram l=urban_instrument_program(&s,3u,URBAN_HOOK);
        assert(c.jazz_voice==1u||c.jazz_voice==2u||
               (genre==13u&&c.jazz_voice==4u));
        assert(l.jazz_voice>=3u && l.jazz_voice<=5u);
        assert(c.patch.pad<16u && l.patch.lead<16u);
        heard_chords[c.jazz_voice]++;
        heard_leads[l.jazz_voice]++;
        /* A non-jazz/ambient track override MUST NOT borrow these sounds. */
        s.tracks[3].genre=1u;s.tracks[3].subgenre=3u;
        s.tracks[4].genre=1u;s.tracks[4].subgenre=3u;
        assert(!urban_instrument_program(&s,3u,URBAN_HOOK).jazz_voice);
        assert(!urban_instrument_program(&s,4u,URBAN_HOOK).jazz_voice);
    }
    assert(heard_chords[1]+heard_chords[2]+heard_chords[4]==128u);
    assert(heard_leads[3]>0u&&heard_leads[4]>0u&&heard_leads[5]>0u);
}
int main(void) {
    test_style(3u,1u);test_style(5u,1u);
    test_style(13u,2u);test_style(2u,1u);
    puts("URBAN-1: 512 jazz instrument assignments; 5 modelled DSP programs, native ambient isolation PASS");
    return 0;
}
