/* SPDX-License-Identifier: GPL-3.0-only
 * Audible-score regression: Generate must write new music, not merely update
 * its serial/seed. Verify harmonic rhythm, per-genre chord scores, 48 HYP
 * synth sound indexes and independence of track regeneration.
 */
#include "urban_runtime.h"
#include "urban_chord_score.h"
#include "urban_instruments.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t hash(uint32_t h,unsigned v){return (h^(uint32_t)v)*16777619u;}
static uint32_t audible_score(const UrbanRuntime *r) {
    const UrbanSong *s=&r->ui.song;
    uint32_t h=2166136261u;
    h=hash(h,s->scale);
    for(unsigned bar=0;bar<4;bar++) {
        uint8_t notes[4]={0};
        unsigned count=urban_build_chord(s,bar,0,notes);
        h=hash(h,count);
        for(unsigned v=0;v<count;v++)h=hash(h,notes[v]);
        for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++) {
            UrbanChordScore cs=urban_chord_score(s,sec,bar);
            h=hash(h,cs.onsets);h=hash(h,cs.gate);
        }
    }
    for(unsigned t=2;t<5;t++) {
        for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++) {
            for(unsigned st=0;st<URBAN_STEPS;st++) {
                if(t==2) {
                    UrbanBassStep note=r->bass.seq[sec].steps[st];
                    h=hash(h,note.velocity?note.note:0u);
                    h=hash(h,note.velocity?note.gate:0u);
                }else{
                    UrbanNote note=r->voices[t].sequence[sec].steps[st];
                    h=hash(h,note.velocity?note.note:0u);
                    h=hash(h,note.velocity?note.gate:0u);
                }
            }
        }
    }
    for(unsigned t=2;t<5;t++) {
        UrbanInstrumentProgram ip=urban_instrument_program(s,t,URBAN_HOOK);
        h=hash(h,t==2?ip.patch.bass:t==3?ip.patch.lead:ip.patch.pad);
    }
    return h;
}
static void check_14_genres_and_instruments(void) {
    uint16_t bass_used=0,lead_used=0,pad_used=0;
    for(unsigned g=0;g<URBAN_GENRES;g++) {
        for(unsigned sub=0;sub<4u;sub++) {
            UrbanRuntime rt;
            urban_runtime_init(&rt,41u);
            rt.ui.genre=(uint8_t)g;
            rt.ui.song.subgenre=(uint8_t)sub;
            for(unsigned t=0;t<URBAN_TRACKS;t++) {
                rt.ui.song.tracks[t].genre=(uint8_t)g;
                rt.ui.song.tracks[t].subgenre=(uint8_t)sub;
            }
            urban_runtime_generate(&rt);
            for(unsigned bar=0;bar<4;bar++) {
                UrbanChordScore cs=urban_chord_score(&rt.ui.song,
                                      URBAN_HOOK,bar);
                assert(cs.onsets && cs.gate>=1u && cs.gate<=16u);
                assert(cs.color<=URBAN_CH_QUARTAL);
                uint8_t tones[4]={0};
                unsigned n=urban_build_chord_colored(&rt.ui.song,bar,
                                    cs.color,0,tones);
                assert(n>=2u && n<=4u);
                for(unsigned k=0;k<n;k++) {
                    unsigned pc=(tones[k]+120u-rt.ui.song.key)%12u;
                    unsigned in_scale=0;
                    for(unsigned d=0;d<7;d++)
                        if(pc==urban_scale_intervals[rt.ui.song.scale][d])
                            in_scale=1;
                    assert(in_scale);
                }
            }
            UrbanStylePatch patch=urban_style_patch(g,sub);
            bass_used|=(uint16_t)(1u<<patch.bass);
            lead_used|=(uint16_t)(1u<<patch.lead);
            pad_used|=(uint16_t)(1u<<patch.pad);
            UrbanInstrumentProgram b=urban_instrument_program(&rt.ui.song,2,URBAN_HOOK);
            UrbanInstrumentProgram l=urban_instrument_program(&rt.ui.song,3,URBAN_HOOK);
            UrbanInstrumentProgram p=urban_instrument_program(&rt.ui.song,4,URBAN_HOOK);
            assert(b.patch.bass<16u && l.patch.lead<16u && p.patch.pad<16u);
            assert(b.noise_color<8u && b.noise_amount<=100u);
        }
    }
    /* Every native sound family is reachable through the 56 genre patches,
     * not just 10 presets distributed across every style. */
    assert(bass_used==0xffffu && lead_used==0xffffu && pad_used==0xffffu);
}
static void check_fresh_compositions(void) {
    for(unsigned g=0;g<URBAN_GENRES;g++) {
        UrbanRuntime rt;
        urban_runtime_init(&rt,123456u);
        rt.ui.genre=(uint8_t)g;
        rt.ui.song.subgenre=(uint8_t)(g%4u);
        rt.ui.song.tempo=105;
        rt.ui.song.key=8u;
        for(unsigned t=0;t<URBAN_TRACKS;t++) {
            rt.ui.song.tracks[t].genre=(uint8_t)g;
            rt.ui.song.tracks[t].subgenre=rt.ui.song.subgenre;
        }
        uint32_t seen[40];
        unsigned distinct=0u;
        for(unsigned take=0;take<40u;take++) {
            uint8_t previous_chords=rt.ui.song.harmony_variant;
            uint8_t previous_contour=rt.ui.song.melodic_variant;
            urban_runtime_generate_entropy(&rt,(take%3u)*77u);
            const UrbanSong *s=&rt.ui.song;
            assert(s->harmony_variant!=previous_chords);
            assert(s->melodic_variant!=previous_contour);
            assert(s->genre==g && s->subgenre==g%4u);
            assert(s->key==8u && s->tempo==105u);
            uint32_t audible=audible_score(&rt);
            int dup=0;
            for(unsigned j=0;j<take;j++)if(seen[j]==audible)dup=1;
            seen[take]=audible;
            distinct+=(unsigned)!dup;
        }
        assert(distinct==40u); /* every consecutive Generate must compose anew */
    }
}
static void check_track_regeneration(void) {
    UrbanRuntime r;
    urban_runtime_init(&r,888u);
    r.ui.genre=0u;
    urban_runtime_generate(&r);
    r.ui.page=URBAN_PAGE_TRACKS;
    r.ui.selected_track=3u;
    UrbanVoice before=r.voices[2];
    UrbanVoice chords=r.voices[4];
    UrbanSong song=r.ui.song;
    urban_runtime_generate_entropy(&r,0xdeadbeefu);
    assert(memcmp(&before,&r.voices[2],sizeof(before))==0);
    assert(memcmp(&chords,&r.voices[4],sizeof(chords))==0);
    assert(memcmp(song.progression,r.ui.song.progression,4u)==0);
    assert(song.scale==r.ui.song.scale);
    assert(song.tempo==r.ui.song.tempo);
}
int main(void){
    check_14_genres_and_instruments();
    check_fresh_compositions();
    check_track_regeneration();
    puts("URBAN-1: 560 newly generated scores, 14 chord grammars, 48 native HYP instruments passed");
    return 0;
}
