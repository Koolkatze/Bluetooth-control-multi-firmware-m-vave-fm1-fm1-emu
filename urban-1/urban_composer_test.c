#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Musical fingerprint deliberately excludes the seed/counter: two songs
 * count as DIFFERENT only when their actual note score, harmony or drum
 * arrangement changes audibly.
 */
static uint32_t mix(uint32_t h,uint32_t value) {
    return (h^value)*16777619u;
}
static uint32_t score_fingerprint(const UrbanRuntime *r) {
    const UrbanSong *s=&r->ui.song;
    uint32_t h=2166136261u;
    h=mix(h,s->scale);
    for(unsigned i=0;i<4;i++) {
        h=mix(h,s->progression[i]);
        h=mix(h,s->chord_colors[i]);
    }
    for(unsigned track=2;track<5;track++) {
        for(unsigned section=0;section<URBAN_SEQUENCES;section++) {
            for(unsigned step=0;step<URBAN_STEPS;step++) {
                UrbanNote n=r->voices[track].sequence[section].steps[step];
                if(track==2) {
                    const UrbanBassStep b=r->bass.seq[section].steps[step];
                    h=mix(h,b.note);h=mix(h,b.velocity?1u:0u);
                    h=mix(h,b.gate);h=mix(h,b.glide);
                }else{
                    h=mix(h,n.note);h=mix(h,n.velocity?1u:0u);
                    h=mix(h,n.gate);
                }
            }
        }
    }
    for(unsigned bar=0;bar<4;bar++) {
        uint16_t drums[6];
        urban_drum_lanes(s,URBAN_HOOK,bar,drums);
        for(unsigned lane=0;lane<6;lane++)h=mix(h,drums[lane]);
    }
    return h;
}
static void check_new_song_every_time(void) {
    for(unsigned genre=0;genre<URBAN_GENRES;genre++) {
        UrbanRuntime r;
        urban_runtime_init(&r,0x12345678u);
        r.ui.genre=(uint8_t)genre;
        r.ui.song.subgenre=(uint8_t)(genre%4u);
        for(unsigned track=0;track<URBAN_TRACKS;track++) {
            r.ui.song.tracks[track].genre=(uint8_t)genre;
            r.ui.song.tracks[track].subgenre=(uint8_t)(genre%4u);
        }
        uint32_t seen[32];
        unsigned distinct=0;
        for(unsigned take=0;take<32;take++) {
            urban_runtime_generate(&r);
            assert(r.ui.song.genre==genre);
            assert(r.ui.song.subgenre==genre%4u);
            uint32_t score=score_fingerprint(&r);
            unsigned exists=0;
            for(unsigned previous=0;previous<take;previous++)
                if(seen[previous]==score)exists=1;
            if(!exists)distinct++;
            seen[take]=score;
            assert(r.ui.song.scale<URBAN_SCALE_COUNT);
            for(unsigned bar=0;bar<4;bar++) {
                assert(r.ui.song.chord_degrees[bar]<7u);
                assert(r.ui.song.progression[bar]==
                  urban_scale_intervals[r.ui.song.scale]
                                       [r.ui.song.chord_degrees[bar]]);
                uint8_t voices[4];
                unsigned count=urban_build_chord(&r.ui.song,bar,0,voices);
                assert(count>=2u && count<=4u);
                for(unsigned k=0;k<count;k++) {
                    assert(voices[k]>=46u && voices[k]<=85u);
                    unsigned pc=(voices[k]+120u-r.ui.song.key)%12u;
                    unsigned diatonic=0;
                    for(unsigned d=0;d<7;d++)
                        diatonic|=(pc==urban_scale_intervals[r.ui.song.scale][d]);
                    assert(diatonic); /* every chord tone is in mode */
                }
            }
        }
        assert(distinct>=30u); /* audibly new parts, NOT unique seed labels */
    }
}
static void check_lock_and_track_isolation(void) {
    UrbanRuntime r;
    urban_runtime_init(&r,777u);
    r.ui.page=URBAN_PAGE_SONG;
    urban_runtime_generate(&r);
    UrbanVoice old=r.voices[3];
    UrbanBassSong oldbass=r.bass;
    r.ui.song.tracks[3].locked=1u;
    urban_runtime_generate(&r);
    assert(memcmp(&old,&r.voices[3],sizeof(old))==0);
    assert(memcmp(&oldbass,&r.bass,sizeof(oldbass))!=0);
    r.ui.page=URBAN_PAGE_TRACKS;
    r.ui.selected_track=2u;
    old=r.voices[3];
    UrbanSong prior=r.ui.song;
    urban_runtime_generate(&r);
    assert(memcmp(&old,&r.voices[3],sizeof(old))==0);
    assert(memcmp(prior.progression,r.ui.song.progression,4)==0);
    assert(prior.scale==r.ui.song.scale);
}
int main(void){
 check_new_song_every_time();
 check_lock_and_track_isolation();
 puts("URBAN-1: 448 musical takes across 14 genres are fresh, modal and internally consistent");
 return 0;
}
