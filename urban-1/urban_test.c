#include "urban_song.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    UrbanSong s, old;
    urban_init(&s, 12345u);
    assert(s.tempo==140 && s.arrangement_len==8);
    urban_generate_song(&s, 0x1234u);
    old=s;
    urban_generate_track(&s, 2, 0x5678u);
    for (unsigned t=0;t<URBAN_TRACKS;t++)
        if(t!=2) assert(memcmp(&s.tracks[t],&old.tracks[t],sizeof(UrbanTrack))==0);
    assert(memcmp(s.arrangement,old.arrangement,sizeof(s.arrangement))==0);
    assert(s.key==old.key && s.tempo==old.tempo);
    assert(memcmp(&s.tracks[2],&old.tracks[2],sizeof(UrbanTrack))!=0);
    old=s;
    s.tracks[2].locked=1;
    UrbanTrack locked=s.tracks[2];
    urban_generate_track(&s,2,0x9876u);
    assert(memcmp(&s.tracks[2],&locked,sizeof(UrbanTrack))==0);
    s.playing=1; s.current=0; s.queued=0;
    urban_queue_next(&s,1);
    assert(s.current==0 && s.queued==1);
    urban_sequence_boundary(&s);
    assert(s.current==1);
    urban_queue_next(&s,-2);
    assert(s.queued==7 && s.current==1);
    s.playing=0;
    urban_queue_next(&s,1);
    assert(s.queued==0 && s.current==0);
    UrbanSong reproducible;
    urban_init(&reproducible,12345u);
    urban_generate_song(&reproducible,0x1234u);
    assert(memcmp(&reproducible.tracks[0],&old.tracks[0],sizeof(UrbanTrack))==0);
    puts("URBAN-1 core tests passed");
    return 0;
}
