/* SPDX-License-Identifier: GPL-3.0-only
 * Automated musical and DSP-parameter audit: all 56 named subgenres
 * have a fixed intentional modal palette, genuine playable phrases
 * (including optional leadless songs) and nonzero native HYP-1 effects.
 */
#include "urban_runtime.h"
#include "urban_fx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned lead_hits(const UrbanSong *s,unsigned sec,unsigned bar) {
    unsigned n=0;
    for(unsigned step=0;step<URBAN_STEPS;step++) {
        UrbanNote v={.note=72,.velocity=70,.gate=5};
        UrbanNote a=urban_lead_score(s,sec,bar,step,v);
        UrbanNote b=urban_lead_score(s,sec,bar,step,v);
        assert(memcmp(&a,&b,sizeof(a))==0); /* never RNG per tick */
        if(!a.velocity)continue;
        assert((urban_lead_bar_mask(s,sec,bar)&(1u<<step))!=0u);
        assert(a.gate>=1u && a.gate<=12u);
        assert(a.note>=48u && a.note<=96u);
        unsigned pc=(a.note+120u-(unsigned)s->key)%12u;
        unsigned ok=0u;
        for(unsigned d=0;d<7u;d++)
            ok|=pc==urban_scale_intervals[s->scale][d];
        assert(ok);  /* real diatonic mode, chord tone or permitted passing note */
        n++;
    }
    return n;
}
static void test_all(void) {
    unsigned leadless=0u,present=0u, different_modes=0u;
    for(unsigned genre=0u;genre<URBAN_GENRES;genre++)
      for(unsigned sub=0u;sub<4u;sub++) {
        UrbanSong s;
        urban_init(&s,0x6918u);
        s.genre=(uint8_t)genre;
        s.subgenre=(uint8_t)sub;
        for(unsigned t=0;t<URBAN_TRACKS;t++) {
            s.tracks[t].genre=(uint8_t)genre;
            s.tracks[t].subgenre=(uint8_t)sub;
        }
        UrbanFxPlan fx[4];
        for(unsigned take=0u;take<4u;take++) {
            urban_generate_song(&s,0xa5617u+genre*73u+sub*113u+take*19u);
            assert(s.scale==urban_genre_mode_options[genre][sub]);
            assert(s.scale<URBAN_SCALE_COUNT);
            unsigned lead_sum=0u;
            for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
              for(unsigned bar=0;bar<4u;bar++) {
                lead_sum+=lead_hits(&s,sec,bar);
                UrbanMixPlan mx=urban_mix_plan(&s,sec,bar);
                assert(urban_mix_energy(&mx)<=15500u);
                if(!urban_lead_active(&s,sec,bar))assert(mx.lead==0u);
              }
            if(urban_lead_presence(&s)) {
                assert(lead_sum>0u);
                present++;
            } else {
                assert(lead_sum==0u);
                leadless++;
            }
            fx[take]=urban_fx_plan(&s);
            assert(fx[take].echo>=8u && fx[take].reverb>=8u);
            assert(fx[take].drive>=8u);
            assert(fx[take].pad_lfo_depth>=6u && fx[take].pad_detune>=3u);
            assert(fx[take].bass_lfo_depth>=6u);
            assert(fx[take].lead_env>=12u && fx[take].lead_resonance>=12u);
            assert(fx[take].kick_decay>=30u && fx[take].hat_decay>=14u);
            assert(fx[take].snare_decay>=20u && fx[take].perc_decay>=18u);
            assert(fx[take].echo_time<=5u && fx[take].reverb_size<=90u);
        }
        UrbanFxPlan a=fx[0];
        for(unsigned other=0;other<4u;other++) {
            if(other==sub)continue;
            UrbanSong q=s;
            q.subgenre=(uint8_t)other;
            UrbanFxPlan f=urban_fx_plan(&q);
            assert(memcmp(&a,&f,sizeof(a))!=0);
        }
        if(sub)different_modes+=(unsigned)
             (urban_genre_mode_options[genre][sub] !=
              urban_genre_mode_options[genre][0]);
      }
    assert(leadless>0u && present>0u);
    assert(different_modes>=21u);
    printf("URBAN-1: 56 substyle modes, >21 modal distinctions, all-wet FX and "
           "%u vocal/bass-first + %u lead-forward takes verified\n",leadless,present);
}
int main(void){test_all();return 0;}
