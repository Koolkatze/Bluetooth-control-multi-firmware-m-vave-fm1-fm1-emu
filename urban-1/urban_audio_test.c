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
 /* Force first-step bass event, so callback behavior is deterministic. */
 rt.bass.seq[0].steps[0]=(UrbanBassStep){36u,110u,1u,1u};
 for(unsigned i=0;i<URBAN_TRACKS;i++)rt.ui.song.tracks[i].muted=(uint8_t)(i!=2u);
 urban_audio_tick(&audio);
 assert(p.on==1u && p.last_track==2u && p.last_note==36u && p.last_glide==1u);
 for(unsigned i=0;i<URBAN_TICKS_PER_STEP;i++)urban_audio_tick(&audio);
 assert(p.off>=1u);
 rt.ui.song.playing=0;
 urban_audio_tick(&audio);
 for(unsigned i=0;i<URBAN_TRACKS;i++)assert(!audio.active[i]);
 puts("URBAN-1 audio adapter tests passed");
 return 0;
}
