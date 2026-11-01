/* URBAN-1: transport ticks and sequence-boundary scheduling.
 * 96 ticks/quarter note; caller supplies clock ticks (internal or MIDI).
 * All sections currently use 1 bar (16 steps); section lengths to follow.
 */
#ifndef URBAN_TRANSPORT_H
#define URBAN_TRANSPORT_H
#include "urban_ui.h"
#define URBAN_PPQN 96u
#define URBAN_TICKS_PER_STEP 24u
#define URBAN_STEPS_PER_SEQUENCE 16u
typedef struct {
    UrbanUI *ui;
    uint32_t tick;
    uint8_t step;
} UrbanTransport;
typedef struct {
    uint8_t sequence, step;
    uint8_t notes[URBAN_TRACKS];
} UrbanFrame;
static void urban_transport_init(UrbanTransport *t, UrbanUI *ui) {
    t->ui=ui; t->tick=0; t->step=0;
}
static void urban_transport_tick(UrbanTransport *t, UrbanFrame *out) {
    UrbanSong *s=&t->ui->song;
    memset(out,0,sizeof(*out));
    out->sequence=s->current;
    out->step=t->step;
    if(!s->playing) return;
    if(t->tick % URBAN_TICKS_PER_STEP == 0u) {
        uint8_t any_solo=0;
        for(unsigned i=0;i<URBAN_TRACKS;i++) any_solo |= s->tracks[i].solo;
        for(unsigned i=0;i<URBAN_TRACKS;i++) {
            UrbanTrack *track=&s->tracks[i];
            if(!track->muted && (!any_solo || track->solo))
                out->notes[i]=track->seq[s->current].steps[t->step];
        }
    }
    t->tick++;
    if(t->tick % URBAN_TICKS_PER_STEP==0u) {
        t->step++;
        if(t->step==URBAN_STEPS_PER_SEQUENCE) {
            t->step=0;
            urban_ui_on_sequence_end(t->ui);
        }
    }
}
#endif
