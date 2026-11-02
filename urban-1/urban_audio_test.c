#include "urban_audio.h"
#include <assert.h>
#include <stdio.h>
typedef struct {unsigned on,off;uint8_t last_track,last_note,last_glide;} Probe;
static void on(void *p,uint8_t t,uint8_t n,uint8_t v,uint8_t g){
 Probe *x=(Probe*)p;assert(v>0);x->on++;x->last_track=t;x->last_note=n;x->last_glide=g;
}
static void off(void *p,uint8_t t,uint8_t n){
 Probe *x=(Probe*)p;x->off++;x->last_track=t;x->last_note=n;
}
int main(void){
 UrbanRuntime rt;UrbanAudio audio;Probe p={0};
 urban_runtime_init(&rt,1234u);
 urban_audio_init(&audio,&rt,(UrbanAudioDriver){&p,on,off});
 rt.ui.song.playing=1;
 /* The new four-bar score owns the event and articulation; do NOT
  * assume the original single-bar array is sent unchanged to audio. */
 UrbanBassStep expected=urban_bass_bar_score(&rt.ui.song,&rt.bass,
     rt.ui.song.current,urban_phrase_bar(&rt.ui.song),0u);
 assert(expected.velocity>0u);
 uint8_t sounding=urban_harmonize_note(&rt.ui.song,expected.note,0u);
 for(unsigned i=0;i<URBAN_TRACKS;i++)rt.ui.song.tracks[i].muted=(uint8_t)(i!=2u);
 urban_audio_tick(&audio);
 assert(p.on==1u && p.last_track==2u &&
        p.last_note==sounding && p.last_glide==expected.glide);
 for(unsigned i=0;i<URBAN_TICKS_PER_STEP;i++)urban_audio_tick(&audio);
 assert(p.off>=1u);
 rt.ui.song.playing=0;
 urban_audio_tick(&audio);
 for(unsigned i=0;i<URBAN_TRACKS;i++)assert(!audio.active[i]);
 puts("URBAN-1 audio adapter tests passed");
 return 0;
}
