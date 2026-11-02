/* SPDX-License-Identifier: GPL-3.0-only
 * Lead, mix and silence regression across 14 genres x 4 subgenres.
 * Not a subjective quality score; validates control structure and
 * guards against clip-prone layer settings and constantly-on leads.
 */
#include "urban_mix.h"
#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_musical_lead(void) {
    for(unsigned g=0;g<URBAN_GENRES;g++)
      for(unsigned sub=0;sub<4;sub++) {
        UrbanSong s;
        urban_init(&s,42u+(uint32_t)g);
        s.genre=(uint8_t)g;
        s.subgenre=(uint8_t)sub;
        s.tracks[3].genre=(uint8_t)g;
        s.tracks[3].subgenre=(uint8_t)sub;
        urban_generate_song(&s,1008u+g*131u+sub);
        assert(!urban_lead_active(&s,URBAN_INTRO,0u));
        assert(!urban_lead_active(&s,URBAN_BREAK,0u));
        unsigned intro=0u,verse=0u,hook=0u,drop=0u,silent=0u;
        for(unsigned section=0;section<URBAN_SEQUENCES;section++)
          for(unsigned bar=0;bar<4;bar++) {
            unsigned hits=0u;
            for(unsigned step=0;step<16;step++){
                UrbanNote v={.note=72,.velocity=88,.gate=6};
                UrbanNote note=urban_lead_score(&s,section,bar,step,v);
                if(note.velocity) {
                    assert(note.gate>=2u && note.gate<=12u);
                    assert(note.note>=48u && note.note<=100u);
                    hits++;
                }
            }
            if(!hits)silent++;
            if(section==URBAN_INTRO)intro+=hits;
            if(section==URBAN_VERSE_A)verse+=hits;
            if(section==URBAN_HOOK)hook+=hits;
            if(section==URBAN_DROP)drop+=hits;
          }
        assert(silent>=8u);
        /* Vocal-first / bass-first songs intentionally have no
         * monophonic synth lead at all. This is valid, not a failed
         * generator. Lead-forward takes still require strong choruses. */
        if(urban_lead_presence(&s)) {
            assert(hook>0u && drop>0u);
            assert(intro<hook);
            assert(verse<hook);
        } else assert(intro==0u && verse==0u && hook==0u && drop==0u);
      }
}
static void check_headroom_and_mutes(void) {
    for(unsigned g=0;g<URBAN_GENRES;g++)
      for(unsigned sub=0;sub<4u;sub++) {
        UrbanSong s; urban_init(&s,201u+g);
        s.genre=(uint8_t)g;s.subgenre=(uint8_t)sub;
        for(unsigned t=0;t<URBAN_TRACKS;t++) {
            s.tracks[t].genre=(uint8_t)g;
            s.tracks[t].subgenre=(uint8_t)sub;
        }
        urban_generate_song(&s,2012u+g*31u+sub);
        for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
          for(unsigned bar=0;bar<4u;bar++) {
            UrbanMixPlan m=urban_mix_plan(&s,sec,bar);
            assert(urban_mix_energy(&m)<=15500u);
            assert(m.drums<=82u && m.bass<=78u && m.lead<=55u);
            assert(m.chords<=55u && m.noise<=13u);
            assert(m.drive<=60u && m.reverb<=62u && m.echo<=53u);
            assert(m.kick<=82u && m.hat<=45u);
            if(!urban_lead_active(&s,sec,bar))assert(m.lead==0u);
          }
        s.tracks[3].muted=1u;
        assert(urban_mix_plan(&s,URBAN_HOOK,2u).lead==0u);
        s.tracks[3].muted=0u;
        s.tracks[2].muted=1u;
        assert(urban_mix_plan(&s,URBAN_HOOK,2u).bass==0u);
        s.tracks[2].muted=0u;
        s.tracks[4].solo=1u;
        UrbanMixPlan solo=urban_mix_plan(&s,URBAN_HOOK,2u);
        assert(solo.bass==0u && solo.lead==0u && solo.chords>0u);
      }
}
static void check_runtime_mute(void) {
    UrbanRuntime r;
    urban_runtime_init(&r,713u);
    /* Use uplifting trance (lead-forward) for the explicit unmute test;
     * trap/hip-hop/techno can intentionally generate a leadless track. */
    r.ui.genre=12u;
    r.ui.song.genre=12u;
    r.ui.song.subgenre=1u;
    r.ui.song.tracks[3].genre=12u;
    r.ui.song.tracks[3].subgenre=1u;
    for(unsigned attempt=0;attempt<64u &&
        !urban_lead_presence(&r.ui.song);attempt++)
        urban_runtime_generate_entropy(&r,attempt+9u);
    assert(urban_lead_presence(&r.ui.song));
    urban_song_set_mode(&r.ui.song,0);
    r.ui.song.current=URBAN_HOOK;
    r.ui.song.queued=URBAN_HOOK;
    r.ui.song.tracks[3].muted=1u;
    r.ui.song.playing=1u;
    for(unsigned step=0;step<16u;step++) {
        UrbanMidiFrame frame;
        urban_runtime_tick(&r,&frame);
        assert(!frame.event[3].velocity);
        for(unsigned k=1;k<24u;k++)urban_runtime_tick(&r,&frame);
    }
    r.ui.song.tracks[3].muted=0u;
    unsigned heard=0u;
    for(unsigned bar=0;bar<4u;bar++)
      for(unsigned step=0;step<16u;step++) {
        UrbanMidiFrame frame;
        urban_runtime_tick(&r,&frame);
        heard+=frame.event[3].velocity!=0u;
        for(unsigned k=1;k<24u;k++)urban_runtime_tick(&r,&frame);
      }
    assert(heard>0u);
}
int main(void){
    check_musical_lead();
    check_headroom_and_mutes();
    check_runtime_mute();
    puts("URBAN-1: 56 substyles x 8 section lead scores, silence, mute/solo, safe mix headroom PASS");
    return 0;
}
