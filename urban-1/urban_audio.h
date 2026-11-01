/* URBAN-1 event-to-synth adapter.
 * Hardware-neutral callback interface: wire callbacks to HYP-1 voice APIs.
 * One monophonic voice per logical track. No audio generated in this module.
 */
#ifndef URBAN_AUDIO_H
#define URBAN_AUDIO_H
#include "urban_runtime.h"
typedef struct {
    void *context;
    void (*note_on)(void *,uint8_t,uint8_t,uint8_t,uint8_t);
    void (*note_off)(void *,uint8_t,uint8_t);
} UrbanAudioDriver;
typedef struct {
    UrbanRuntime *runtime;
    UrbanAudioDriver driver;
    uint8_t active[URBAN_TRACKS];
    uint8_t note[URBAN_TRACKS];
    uint16_t remaining[URBAN_TRACKS];
} UrbanAudio;
static void urban_audio_init(UrbanAudio *a,UrbanRuntime *rt,UrbanAudioDriver driver){
    memset(a,0,sizeof(*a));
    a->runtime=rt;
    a->driver=driver;
}
static void urban_audio_all_off(UrbanAudio *a){
    for(unsigned i=0;i<URBAN_TRACKS;i++){
        if(a->active[i] && a->driver.note_off)
            a->driver.note_off(a->driver.context,(uint8_t)i,a->note[i]);
        a->active[i]=0u;
        a->remaining[i]=0u;
    }
}
/* Call once per 96-PPQN tick. gate counts 16th-note steps. */
static void urban_audio_tick(UrbanAudio *a){
    UrbanRuntime *rt=a->runtime;
    if(!rt->ui.song.playing){urban_audio_all_off(a);return;}
    /* Expire notes before triggering new notes, preventing stuck voices. */
    for(unsigned i=0;i<URBAN_TRACKS;i++){
        if(a->active[i] && a->remaining[i]>0u){
            --a->remaining[i];
            if(a->remaining[i]==0u){
                if(a->driver.note_off)
                    a->driver.note_off(a->driver.context,(uint8_t)i,a->note[i]);
                a->active[i]=0u;
            }
        }
    }
    UrbanMidiFrame frame;
    urban_runtime_tick(rt,&frame);
    for(unsigned i=0;i<URBAN_TRACKS;i++){
        UrbanNote n=frame.event[i];
        if(!n.velocity) continue;
        if(a->active[i] && a->driver.note_off)
            a->driver.note_off(a->driver.context,(uint8_t)i,a->note[i]);
        a->active[i]=1u;
        a->note[i]=n.note;
        a->remaining[i]=(uint16_t)(n.gate?n.gate:1u)*URBAN_TICKS_PER_STEP;
        if(a->driver.note_on)
            a->driver.note_on(a->driver.context,(uint8_t)i,n.note,n.velocity,frame.glide[i]);
    }
}
#endif
