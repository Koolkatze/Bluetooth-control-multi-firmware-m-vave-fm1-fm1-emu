/* URBAN-1 playable MIDI-event runtime (host portable).
 * Uses the existing genre/motif generator; still requires a synth/MIDI output HAL.
 * One cached phrase per track per section. Regeneration updates only that track.
 */
#ifndef URBAN_RUNTIME_H
#define URBAN_RUNTIME_H
#include "urban_music.h"
#include "urban_bass.h"
#include "urban_transport.h"
typedef struct {
    UrbanUI ui;
    UrbanTransport transport;
    UrbanVoice voices[URBAN_TRACKS];
    UrbanBassSong bass;
} UrbanRuntime;
typedef struct {
    uint8_t sequence, step;
    UrbanNote event[URBAN_TRACKS];
    uint8_t glide[URBAN_TRACKS];
} UrbanMidiFrame;
static void urban_runtime_refresh_track(UrbanRuntime *rt, unsigned track) {
    if(track>=URBAN_TRACKS) return;
    urban_generate_voice(&rt->ui.song,track,
        rt->ui.song.tracks[track].seq[0].seed,&rt->voices[track]);
    if(track==2u) urban_generate_bass(&rt->ui.song,
        rt->ui.song.tracks[track].seq[0].seed,&rt->bass);
}
static void urban_runtime_init(UrbanRuntime *rt,uint32_t seed) {
    memset(rt,0,sizeof(*rt));
    urban_ui_init(&rt->ui,seed);
    urban_transport_init(&rt->transport,&rt->ui);
    urban_generate_song(&rt->ui.song,seed);
    for(unsigned i=0;i<URBAN_TRACKS;i++) urban_runtime_refresh_track(rt,i);
}
static void urban_runtime_generate(UrbanRuntime *rt) {
    if(rt->ui.page==URBAN_PAGE_TRACKS) {
        unsigned track=rt->ui.selected_track;
        if(rt->ui.song.tracks[track].locked) return;
        urban_ui_generate(&rt->ui);
        urban_runtime_refresh_track(rt,track);
    } else {
        urban_ui_generate(&rt->ui);
        for(unsigned i=0;i<URBAN_TRACKS;i++)
            if(!rt->ui.song.tracks[i].locked) urban_runtime_refresh_track(rt,i);
    }
}
static void urban_runtime_tick(UrbanRuntime *rt, UrbanMidiFrame *frame) {
    UrbanTransport *t=&rt->transport;
    UrbanSong *s=&rt->ui.song;
    memset(frame,0,sizeof(*frame));
    frame->sequence=s->current;
    frame->step=t->step;
    if(!s->playing) return;
    if(t->tick%URBAN_TICKS_PER_STEP==0u) {
        uint8_t any_solo=0;
        for(unsigned i=0;i<URBAN_TRACKS;i++) any_solo|=s->tracks[i].solo;
        for(unsigned i=0;i<URBAN_TRACKS;i++)
            if(!s->tracks[i].muted && (!any_solo || s->tracks[i].solo))
                frame->event[i]=rt->voices[i].sequence[s->current].steps[t->step];
        if(!s->tracks[2].muted && (!any_solo || s->tracks[2].solo)) {
            UrbanBassStep bass=rt->bass.seq[s->current].steps[t->step];
            frame->event[2].note=bass.note;
            frame->event[2].velocity=bass.velocity;
            frame->event[2].gate=bass.gate;
            frame->glide[2]=bass.glide;
        }
    }
    t->tick++;
    if(t->tick%URBAN_TICKS_PER_STEP==0u) {
        t->step++;
        if(t->step==URBAN_STEPS_PER_SEQUENCE) {
            t->step=0;
            urban_ui_on_sequence_end(&rt->ui);
        }
    }
}
#endif
