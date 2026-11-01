#include "urban_ui.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    UrbanUI ui;
    urban_ui_init(&ui,123u);
    urban_ui_page(&ui,URBAN_PAGE_SONG);
    urban_ui_generate(&ui);
    UrbanSong snapshot=ui.song;
    urban_ui_page(&ui,URBAN_PAGE_TRACKS);
    urban_ui_select_track(&ui,3);
    urban_ui_set_genre(&ui,6,2);
    urban_ui_generate(&ui);
    for(unsigned i=0;i<URBAN_TRACKS;i++) if(i!=3)
        assert(memcmp(&ui.song.tracks[i],&snapshot.tracks[i],sizeof(UrbanTrack))==0);
    assert(memcmp(ui.song.arrangement,snapshot.arrangement,sizeof(ui.song.arrangement))==0);
    for(unsigned s=0;s<URBAN_SEQUENCES;s++)
        assert(ui.song.tracks[3].seq[s].seed!=snapshot.tracks[3].seq[s].seed);
    ui.song.playing=1;
    ui.song.current=0;
    ui.song.queued=0;
    urban_ui_page(&ui,URBAN_PAGE_SONG);
    urban_ui_next_knob(&ui,1);
    assert(ui.song.current==0 && ui.song.queued==1);
    urban_ui_on_sequence_end(&ui);
    assert(ui.song.current==1);
    puts("URBAN-1 UI bridge tests passed");
    return 0;
}
