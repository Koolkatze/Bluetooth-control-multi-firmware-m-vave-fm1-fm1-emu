/* SPDX-License-Identifier: GPL-3.0-only
 * Regression test every 12-TET scale and every of 56 style palettes.
 * Crucially, pentatonic/hexatonic/octatonic/chromatic intervals and
 * chords must never read the padded zeroes of a 7-note-only array.
 */
#include "urban_harmony.h"
#include "urban_music.h"
#include "urban_bass.h"
#include "urban_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned in_scale(unsigned id,unsigned pc) {
    for(unsigned n=0;n<urban_scale_size(id);n++)
        if(urban_scale_pc(id,n)==pc)return 1u;
    return 0u;
}
int main(void) {
    unsigned count5=0u,count6=0u,count7=0u,count8=0u,count12=0u;
    for(unsigned id=0;id<URBAN_SCALE_COUNT;id++) {
        unsigned size=urban_scale_size(id);
        assert(size>=5u && size<=12u);
        assert(urban_scale_pc(id,0u)==0u);
        for(unsigned degree=0;degree<size;degree++) {
            unsigned note=urban_scale_pc(id,degree);
            assert(note<12u);
            if(degree)assert(note>urban_scale_pc(id,degree-1u));
            assert(urban_scale_offset(id,(int)degree)==(int)note);
            assert(urban_scale_offset(id,(int)degree+(int)size)==(int)note+12);
            assert(urban_scale_offset(id,(int)degree-(int)size)==(int)note-12);
        }
        assert(urban_scale_offset(id,-1)==
               (int)urban_scale_pc(id,size-1u)-12);
        assert(urban_scale_names[id] && strlen(urban_scale_names[id])>2u);
        count5+=(size==5u);count6+=(size==6u);
        count7+=(size==7u);count8+=(size==8u);count12+=(size==12u);
        /* A manual scale must survive an entire generated take and
         * create genuine score notes throughout the entire 4-bar form. */
        UrbanSong song;
        urban_init(&song,7231u);
        song.genre=3u; song.subgenre=0u;
        song.scale=(uint8_t)id;
        song.scale_manual=1u;
        urban_song_harmony(&song);
        urban_generate_song(&song,9031u+id*47u);
        assert(song.scale==id);
        for(unsigned bar=0;bar<4u;bar++) {
            assert(song.chord_degrees[bar]<size);
            assert(song.progression[bar]==urban_scale_pc(id,song.chord_degrees[bar]));
            uint8_t chord[4]={0};
            unsigned n=urban_build_chord(&song,bar,0,chord);
            assert(n>=2u && n<=4u);
            for(unsigned i=0;i<n;i++)
                assert(in_scale(id,(chord[i]+120u-song.key)%12u));
        }
        UrbanBassSong bass;
        urban_generate_bass(&song,song.tracks[2].seq[0].seed,&bass);
        for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
            for(unsigned step=0;step<URBAN_STEPS;step++) {
                UrbanBassStep n=bass.seq[sec].steps[step];
                if(n.velocity)
                    assert(in_scale(id,(n.note+120u-song.key)%12u));
            }
        UrbanVoice notes;
        urban_generate_voice(&song,3u,song.tracks[3].seq[0].seed,&notes);
        for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
            for(unsigned step=0;step<URBAN_STEPS;step++) {
                UrbanNote n=notes.sequence[sec].steps[step];
                if(n.velocity)
                    assert(in_scale(id,(n.note+120u-song.key)%12u));
            }
    }
    assert(count5>8u && count6>5u && count7>60u &&
           count8>=6u && count12==1u);
    unsigned distinct=0u;
    uint8_t used[URBAN_SCALE_COUNT]={0};
    for(unsigned genre=0;genre<URBAN_GENRES;genre++)
      for(unsigned sub=0;sub<4u;sub++) {
        unsigned options[3]={
            urban_genre_mode_options[genre][sub],
            urban_style_scale_alternatives[genre][sub][0],
            urban_style_scale_alternatives[genre][sub][1]
        };
        for(unsigned i=0;i<3u;i++) {
            assert(options[i]<URBAN_SCALE_COUNT);
            if(!used[options[i]]){distinct++;used[options[i]]=1u;}
            for(unsigned j=0;j<i;j++)assert(options[i]!=options[j]);
            UrbanSong s;
            urban_init(&s,5519u);
            s.genre=(uint8_t)genre;s.subgenre=(uint8_t)sub;
            s.harmony_variant=(uint8_t)(i==0u?0u:i+1u);
            urban_song_harmony(&s);
            assert(s.scale==options[i]);
        }
      }
    assert(distinct>=30u);
    UrbanUI ui;
    urban_ui_init(&ui,123u);
    urban_ui_set_genre(&ui,3u,1u);
    ui.song.scale=(uint8_t)(URBAN_SCALE_COUNT-1u);
    ui.song.scale_manual=1u;
    urban_song_harmony(&ui.song);
    urban_ui_generate(&ui);
    assert(ui.song.scale==URBAN_SCALE_COUNT-1u &&
           ui.song.scale_manual==1u);
    urban_ui_set_genre(&ui,12u,0u);
    assert(ui.song.scale_manual==0u);
    assert(ui.song.scale==urban_genre_mode_options[12][0]);
    printf("URBAN: %u scales (5:%u,6:%u,7:%u,8:%u,12:%u), 56 palettes, %u assigned scale colours PASS\n",
      URBAN_SCALE_COUNT,count5,count6,count7,count8,count12,distinct);
    return 0;
}
