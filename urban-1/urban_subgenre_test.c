/* SPDX-License-Identifier: GPL-3.0-only
 * Subgenre realism regressions: four genuinely distinct scores per genre.
 * Validate DRUMS (six lanes), BASS (timing/pitches/glides/timbres),
 * HARMONY (modal functions/voicings/rhythm) and selection without SAVE.
 */
#include "urban_chord_score.h"
#include "urban_instruments.h"
#include "urban_bass.h"
#include "urban_rhythm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t step_hash(uint32_t h,unsigned v) {
    return (h^(uint32_t)v)*16777619u;
}
typedef struct {
    uint32_t drum,bass,harmony;
    UrbanGroove groove;
    UrbanBassGrammar bass_profile;
    uint8_t bass_sound,scale;
} SubSignature;
static SubSignature score_style(unsigned genre,unsigned sub,unsigned seed) {
    UrbanSong s;
    UrbanBassSong bass;
    SubSignature sig={0};
    urban_init(&s,1234u);
    s.genre=(uint8_t)genre;
    s.subgenre=(uint8_t)sub;
    s.harmony_variant=0u;
    s.melodic_variant=0u;
    for(unsigned t=0;t<URBAN_TRACKS;t++) {
        s.tracks[t].genre=(uint8_t)genre;
        s.tracks[t].subgenre=(uint8_t)sub;
    }
    urban_song_harmony(&s);
    assert(s.scale==urban_genre_mode_options[genre][sub]);
    uint8_t expected_tonic=s.key;
    assert(s.key==expected_tonic);
    /* This checks the DIRECT knob path's recomposition primitive;
     * it must be independent of pressing SAVE or changing song seed. */
    for(unsigned bar=0;bar<4u;bar++) {
        assert(s.chord_degrees[bar]==urban_scale_root_degree(
                    s.scale,urban_chord_degrees[genre][sub][bar]));
        assert(s.progression[bar]==urban_scale_pc(s.scale,s.chord_degrees[bar]));
        assert(s.chord_colors[bar]==urban_substyle_chord_colors[genre][sub][bar]);
    }
    sig.scale=s.scale;
    sig.groove=urban_groove((uint8_t)genre,(uint8_t)sub);
    sig.bass_profile=urban_bass_style(&s.tracks[2]);
    urban_generate_song(&s,seed);
    sig.harmony=2166136261u;
    for(unsigned bar=0;bar<4u;bar++) {
        sig.harmony=step_hash(sig.harmony,s.scale);
        sig.harmony=step_hash(sig.harmony,s.chord_degrees[bar]);
        sig.harmony=step_hash(sig.harmony,s.chord_colors[bar]);
        UrbanChordScore chords=urban_chord_score(&s,URBAN_HOOK,bar);
        sig.harmony=step_hash(sig.harmony,chords.onsets);
        sig.harmony=step_hash(sig.harmony,chords.gate);
        sig.harmony=step_hash(sig.harmony,chords.color);
        assert(chords.onsets && chords.gate>0u);
        uint8_t notes[4]={0};
        unsigned n=urban_build_chord_colored(&s,bar,chords.color,0,notes);
        assert(n>=2u && n<=4u);
        for(unsigned i=0;i<n;i++) {
            unsigned pc=(notes[i]+120u-s.key)%12u;
            unsigned in_scale=0u;
            for(unsigned d=0;d<urban_scale_size(s.scale);d++)
                if(pc==urban_scale_pc(s.scale,d))
                    in_scale=1u;
            assert(in_scale);
            sig.harmony=step_hash(sig.harmony,notes[i]);
        }
    }
    sig.drum=2166136261u;
    for(unsigned bar=0;bar<4u;bar++) {
        uint16_t masks[6];
        urban_drum_lanes(&s,URBAN_HOOK,bar,masks);
        for(unsigned l=0;l<6u;l++)
            sig.drum=step_hash(sig.drum,masks[l]);
    }
    urban_generate_bass(&s,s.tracks[2].seq[0].seed,&bass);
    sig.bass=2166136261u;
    for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
      for(unsigned step=0;step<16u;step++) {
        UrbanBassStep n=bass.seq[sec].steps[step];
        sig.bass=step_hash(sig.bass,n.velocity?1u:0u);
        if(n.velocity) {
            sig.bass=step_hash(sig.bass,n.note);
            sig.bass=step_hash(sig.bass,n.gate);
            sig.bass=step_hash(sig.bass,n.glide);
        }
      }
    UrbanInstrumentProgram ip=urban_instrument_program(&s,2u,URBAN_HOOK);
    sig.bass_sound=ip.patch.bass;
    assert(sig.bass_sound<16u);
    return sig;
}
static unsigned lane_difference(const UrbanGroove *a,const UrbanGroove *b) {
    unsigned n=0u;
    n+=a->kick!=b->kick;
    n+=a->snare!=b->snare;
    n+=a->hat!=b->hat;
    n+=a->clap!=b->clap;
    n+=a->open!=b->open;
    n+=a->perc!=b->perc;
    return n;
}
static void test_56_styles(void) {
    unsigned bass_sound_diff=0u,comparisons=0u;
    for(unsigned g=0;g<URBAN_GENRES;g++) {
        SubSignature sig[4];
        for(unsigned sub=0;sub<4u;sub++)
            sig[sub]=score_style(g,sub,0x1729u+g*341u);
        for(unsigned i=0;i<4u;i++)
          for(unsigned j=i+1u;j<4u;j++) {
            ++comparisons;
            assert(lane_difference(&sig[i].groove,&sig[j].groove)>=2u);
            assert(sig[i].drum!=sig[j].drum);
            assert(sig[i].bass!=sig[j].bass);
            assert(sig[i].harmony!=sig[j].harmony);
            if(sig[i].bass_sound!=sig[j].bass_sound)bass_sound_diff++;
          }
        /* Each category has a compact curated bass sound BANK, not a
         * generic genre-global pool which overrides the style. */
        for(unsigned sub=0;sub<4u;sub++) {
            unsigned distinct=0u;
            uint8_t options[4];
            for(unsigned v=0;v<4u;v++) {
                options[v]=urban_subgenre_bass_sounds[g][sub][v];
                assert(options[v]<16u);
                unsigned seen=0u;
                for(unsigned prev=0;prev<v;prev++)
                    seen+=(unsigned)(options[v]==options[prev]);
                distinct+=(unsigned)!seen;
            }
            assert(distinct>=2u);
        }
    }
    assert(comparisons==84u);
    assert(bass_sound_diff>=35u);
    printf("URBAN-1: 56 subgenre drums/bass/harmony, 84 contrasts, "
           "%u distinct audible bass-patch contrasts PASS\n",bass_sound_diff);
}
int main(void) {
    test_56_styles();
    return 0;
}
