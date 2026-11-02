#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check_song_form(void) {
 UrbanSong s;
 urban_init(&s,0x1234u);
 urban_song_set_mode(&s,1);
 static const uint8_t sections[9]={0,1,2,3,2,4,5,6,7};
 static const uint8_t lengths[9]={4,8,8,8,8,4,8,4,4};
 unsigned p=0;
 for(unsigned i=0;i<9;i++)
  for(unsigned j=0;j<lengths[i];j++)
   assert(s.arrangement[p++]==sections[i]);
 assert(p==56u && s.arrangement_len==56u);
 assert(s.current==URBAN_INTRO && s.queued==URBAN_INTRO);
 s.playing=1;
 for(unsigned i=0;i<112;i++){
  uint8_t expected=s.arrangement[(i+1u)%56u];
  uint8_t queued=s.queued;
  urban_queue_next(&s,1); /* never override auto arrangement */
  assert(s.queued==queued);
  urban_sequence_boundary(&s);
  assert(s.current==expected);
  assert(s.arrangement_pos==(i+1u)%56u);
  assert(s.queued==s.arrangement[(i+2u)%56u]);
 }
 urban_song_set_mode(&s,0);
 assert(s.song_mode==0);
 uint8_t current=s.current;
 urban_queue_next(&s,2);
 assert(s.queued==(current+2u)%8u);
 urban_sequence_boundary(&s);
 assert(s.current==(current+2u)%8u);
}
static void check_genre_identity(void) {
 UrbanSong s;
 urban_init(&s,43u);
 for(unsigned a=0;a<URBAN_GENRES;a++){
  unsigned identical=0;
  UrbanGroove ga=urban_groove(a,0);
  for(unsigned b=0;b<URBAN_GENRES;b++){
   UrbanGroove gb=urban_groove(b,0);
   if(ga.kick==gb.kick && ga.snare==gb.snare &&
      ga.hat==gb.hat && ga.perc==gb.perc)
     identical++;
  }
  assert(identical==1u); /* 14 distinguishable drum signatures */
  urban_song_set_genre(&s,(uint8_t)a);
  assert(s.scale==urban_genre_scale[a]);
  assert(memcmp(s.progression,urban_progressions[a],4)==0);
  uint16_t lanes[6],repeat[6];
  urban_generate_song(&s,123u);
  for(unsigned i=0;i<2;i++)s.tracks[i].genre=(uint8_t)a;
  urban_drum_lanes(&s,URBAN_HOOK,3,lanes);
  urban_drum_lanes(&s,URBAN_HOOK,3,repeat);
  assert(memcmp(lanes,repeat,sizeof lanes)==0);
  assert(lanes[0] || lanes[1]);
  s.tracks[0].muted=1;
  urban_drum_lanes(&s,URBAN_HOOK,3,repeat);
  assert(!repeat[0]&&!repeat[1]&&!repeat[2]);
  s.tracks[0].muted=0;
 }
}
static void check_harmonic_progression(void){
 UrbanRuntime rt;
 UrbanMidiFrame f;
 urban_runtime_init(&rt,2222u);
 urban_song_set_mode(&rt.ui.song,1);
 rt.ui.song.playing=1;
 uint8_t tonic=rt.bass.seq[0].steps[0].note;
 for(unsigned bar=0;bar<4u;bar++){
  urban_runtime_tick(&rt,&f);
  assert(f.sequence==URBAN_INTRO && f.step==0);
  assert(f.event[2].velocity!=0);
  assert(f.event[2].note==tonic+rt.ui.song.progression[bar]);
  for(unsigned i=1;i<384;i++)urban_runtime_tick(&rt,&f);
 }
 assert(rt.ui.song.current==URBAN_VERSE_A);
}
/* The A-H phrase is four bars, NOT a one-bar phrase multiplied by
 * playback speed. The harmony and melodic ornaments progress over bars
 * 1..4, and a repeat of verse B starts the same recognizable motif. */
static void check_four_bar_phrase(void) {
    UrbanRuntime rt;
    UrbanMidiFrame first[4],second[4];
    urban_runtime_init(&rt,0x12345678u);
    urban_song_set_mode(&rt.ui.song,1);
    rt.ui.song.playing=1;
    for(unsigned bar=0;bar<12u;bar++) {
        UrbanMidiFrame start;
        urban_runtime_tick(&rt,&start);
        if(bar>=4u && bar<8u)first[bar-4u]=start;
        if(bar>=8u && bar<12u)second[bar-8u]=start;
        for(unsigned t=1;t<384;t++)
            urban_runtime_tick(&rt,&start);
    }
    for(unsigned i=0;i<4u;i++) {
        assert(first[i].sequence==URBAN_VERSE_A);
        assert(second[i].sequence==URBAN_VERSE_A);
        assert(first[i].event[2].note==second[i].event[2].note);
        assert(first[i].event[3].note==second[i].event[3].note);
        assert(first[i].event[4].note==second[i].event[4].note);
        assert(first[i].event[2].velocity==second[i].event[2].velocity);
    }
    assert(first[0].event[2].note!=first[1].event[2].note ||
           first[1].event[2].note!=first[2].event[2].note);
}
static void check_manual_loop(void) {
    UrbanSong s;
    urban_init(&s,616u);
    urban_song_set_mode(&s,0);
    s.playing=1;
    s.current=URBAN_HOOK;
    s.queued=URBAN_HOOK;
    for(unsigned bar=0;bar<20u;bar++) {
        assert(urban_phrase_bar(&s)==bar%4u);
        urban_sequence_boundary(&s);
        assert(s.current==URBAN_HOOK && s.queued==URBAN_HOOK);
    }
    urban_queue_next(&s,3);
    assert(s.current==URBAN_HOOK && s.queued==URBAN_DROP);
    urban_sequence_boundary(&s); /* queued ONLY at end of this bar */
    assert(s.current==URBAN_DROP);
    assert(urban_phrase_bar(&s)==0u);
    for(unsigned i=0;i<12;i++) {
        urban_sequence_boundary(&s);
        assert(s.current==URBAN_DROP);
    }
}

int main(void) {
 check_song_form();
 check_genre_identity();
 check_harmonic_progression();
 check_four_bar_phrase();
 check_manual_loop();
 puts("URBAN-1 4-bar phrases, manual bar-quantized loop, 56-bar song and 14 genres passed");
 return 0;
}
