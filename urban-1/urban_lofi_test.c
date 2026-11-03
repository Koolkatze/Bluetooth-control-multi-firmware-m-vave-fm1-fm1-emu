/* SPDX-License-Identifier: GPL-3.0-only
 * Original lo-fi hip-hop style audit: four deterministic study/rainy/jazz/
 * sleep feels. This checks MUSIC and DSP program ROUTING, not subjective
 * sonic equivalence to a particular streaming station or copyrighted track.
 */
#include "urban_fx.h"
#include "urban_chord_score.h"
#include "urban_bass.h"
#include "urban_rhythm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t hash(uint32_t h,unsigned x) { return (h^x)*16777619u; }
static unsigned pop(uint16_t x) {
    unsigned n=0u;while(x){n+=x&1u;x>>=1u;}return n;
}
int main(void) {
    assert(urban_style_tempo(3u,3u)==80u);
    assert(urban_style_tempo(3u,0u)==94u);
    assert(urban_style_tempo(3u,1u)==94u);
    assert(urban_style_tempo(3u,2u)==94u);
    assert(urban_style_tempo(4u,3u)==82u);
    uint32_t signature[4]={0u};
    for(unsigned take=0u;take<16u;take++) {
        UrbanSong s;
        urban_init(&s,0x5000u+take*7u);
        s.genre=3u; s.subgenre=3u;
        s.tempo=urban_style_tempo(3u,3u);
        for(unsigned t=0u;t<URBAN_TRACKS;t++){
            s.tracks[t].genre=3u;s.tracks[t].subgenre=3u;
        }
        urban_generate_song(&s,0x9000u+take*133u);
        UrbanBassSong bass;
        urban_generate_bass(&s,s.tracks[2].seq[0].seed,&bass);
        for(unsigned mood=0u;mood<4u;mood++) {
            s.melodic_variant=(uint8_t)mood;
            s.harmony_variant=0u;
            urban_song_harmony(&s);
            assert(urban_lofi_mood(&s)==mood);
            assert(s.scale==2u); /* Dorian, jazz-friendly 7 notes */
            uint32_t h=2166136261u;
            UrbanInstrumentProgram keys=urban_instrument_program(&s,4u,URBAN_HOOK);
            UrbanInstrumentProgram solo=urban_instrument_program(&s,3u,URBAN_HOOK);
            UrbanInstrumentProgram atmosphere=urban_instrument_program(&s,5u,URBAN_HOOK);
            assert(keys.jazz_voice==((mood==1u)?1u:2u));
            if(mood==2u)assert(solo.jazz_voice>=3u && solo.jazz_voice<=4u);
            else assert(solo.jazz_voice==0u);
            assert(atmosphere.noise_color==(mood==1u?2u:1u));
            UrbanFxPlan fx=urban_fx_plan(&s);
            assert(fx.pad_lfo_depth>=9u && fx.pad_lfo_depth<=12u);
            assert(fx.pad_detune==6u);
            assert(fx.drive==22u && fx.kick_decay==55u);
            assert(fx.snare_decay==29u && fx.hat_decay==20u);
            assert(fx.noise_pump==20u);
            for(unsigned bar=0u;bar<4u;bar++) {
                uint16_t drums[6],drums_again[6];
                urban_drum_lanes(&s,URBAN_HOOK,bar,drums);
                urban_drum_lanes(&s,URBAN_HOOK,bar,drums_again);
                for(unsigned i=0u;i<6u;i++)assert(drums[i]==drums_again[i]);
                assert(drums[1]==0x1010u);
                assert(drums[2]==0u && drums[4]==0u && drums[5]==0u);
                assert(pop(drums[0])>=1u && pop(drums[0])<=4u);
                assert(pop(drums[3])>=4u && pop(drums[3])<=9u);
                UrbanChordScore cs=urban_chord_score(&s,URBAN_HOOK,bar);
                assert(cs.onsets==1u || (mood==2u && bar==3u && cs.onsets==0x0081u));
                assert(cs.gate>=7u && cs.gate<=15u);
                assert(cs.color==URBAN_CH_SEVENTH||cs.color==URBAN_CH_NINTH);
                uint8_t notes[4]={0};
                assert(urban_build_chord_colored(&s,bar,cs.color,0,notes)==4u);
                for(unsigned i=0u;i<4u;i++)assert(notes[i]>=46u && notes[i]<=85u);
                UrbanChordExpression expr=urban_chord_expression(&s,URBAN_HOOK,bar);
                if(mood==3u)assert(expr.touch==URBAN_CH_TOUCH_BLOCK);
                else assert(expr.spacing_ms>=10u && expr.spacing_ms<=19u);
                uint16_t bass_mask=urban_bass_bar_mask(&s.tracks[2],URBAN_HOOK,bar);
                assert(pop(bass_mask)>=1u && pop(bass_mask)<=2u);
                for(unsigned step=0u;step<16u;step++) {
                    UrbanBassStep note=urban_bass_bar_score(&s,&bass,URBAN_HOOK,bar,step);
                    assert((note.velocity>0u)==((bass_mask>>step)&1u));
                    if(note.velocity) {
                        assert(note.note>=36u && note.note<=54u);
                        assert(note.gate>=10u && note.gate<=13u);
                        assert(note.glide==0u);
                    }
                }
                UrbanMixPlan mix=urban_mix_plan(&s,URBAN_HOOK,bar);
                assert(mix.kick==69u && mix.drive==22u);
                assert(mix.clap==0u && mix.open==0u && mix.perc==0u);
                assert(mix.noise>=10u && mix.noise<=19u);
                assert(urban_mix_energy(&mix)<=15500u);
                h=hash(h,drums[0]);h=hash(h,drums[3]);
                h=hash(h,cs.color);h=hash(h,keys.jazz_voice);
                h=hash(h,fx.pad_release);
            }
            if(take==0u)signature[mood]=h;
            if(mood==3u)assert(!urban_lead_presence(&s));
        }
    }
    for(unsigned a=0u;a<4u;a++)
      for(unsigned b=a+1u;b<4u;b++)assert(signature[a]!=signature[b]);
    puts("URBAN-1: 64 original lo-fi variants; real modelled keys, 80 BPM, human pockets and dynamic mix PASS");
    return 0;
}
