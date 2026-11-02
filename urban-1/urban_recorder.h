/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 bounded MIDI-note/parameter loop recorder.
 * Playable on FM-1's 27-key keyboard; 1/2/4/8 bars; 32nd-note
 * position grid, quantization OFF/1-4/1-8/1-16/1-32.
 * Overdub begins automatically after the first loop boundary, without
 * clearing the previous pass. Note and knob automation share a timeline.
 * No allocation; MIDI events are distinct from the generated song score.
 */
#ifndef URBAN_RECORDER_H
#define URBAN_RECORDER_H
#include <stdint.h>
#define UR_REC_MAX_EVENTS 512u
#define UR_REC_TICKS_BAR 32u
enum {UR_REC_OFF,UR_REC_ARM,UR_REC_RECORD,UR_REC_EDIT};
enum {UR_REC_NOTE_OFF,UR_REC_NOTE_ON,UR_REC_PARAM};
typedef struct {
    uint16_t pos,pass;
    uint8_t kind,note,value,parameter;
} UrbanRecEvent;
typedef struct {
    UrbanRecEvent event[UR_REC_MAX_EVENTS];
    uint16_t count,position,pass,play_cursor;
    uint8_t state,track,bars,quant,overdub,mod_page;
    uint8_t held[27];
    uint8_t edit_note,edit_velocity;
} UrbanRecorder;
static void urban_rec_init(UrbanRecorder *r) {
    for(unsigned i=0;i<sizeof(*r);i++)((uint8_t *)r)[i]=0u;
    r->state=UR_REC_OFF;
    r->track=4u; /* poly chord / keyboard */
    r->bars=4u;r->quant=3u; /* quant=3 is 1/16 */
    r->edit_note=60u;r->edit_velocity=110u;
}
static unsigned urban_rec_length(const UrbanRecorder *r) {
    return r->bars*UR_REC_TICKS_BAR;
}
static unsigned urban_rec_grid(unsigned quant) {
    static const uint8_t grid[5]={1u,8u,4u,2u,1u};
    return grid[quant%5u];
}
static uint16_t urban_rec_quantize(const UrbanRecorder *r,unsigned pos) {
    unsigned grid=urban_rec_grid(r->quant);
    unsigned len=urban_rec_length(r);
    if(!len)return 0u;
    return (uint16_t)(((pos+grid/2u)/grid*grid)%len);
}
static void urban_rec_arm(UrbanRecorder *r) {
    r->state=UR_REC_ARM;
    r->position=0u;r->pass=0u;r->overdub=0u;
    r->mod_page=0u;
}
static void urban_rec_start(UrbanRecorder *r) {
    r->state=UR_REC_RECORD;
    r->position=0u;r->pass=0u;r->overdub=0u;
    r->count=0u;r->play_cursor=0u;
    for(unsigned i=0u;i<27u;i++)r->held[i]=0u;
}
static void urban_rec_overdub(UrbanRecorder *r) {
    r->state=UR_REC_RECORD;
    r->pass++;
    r->overdub=1u;
}
static void urban_rec_stop(UrbanRecorder *r) {
    r->state=UR_REC_EDIT;
    for(unsigned i=0u;i<27u;i++)r->held[i]=0u;
}
static void urban_rec_set_bars(UrbanRecorder *r,int delta) {
    static const uint8_t lengths[4]={1u,2u,4u,8u};
    unsigned n=2u;
    for(unsigned i=0u;i<4u;i++)if(r->bars==lengths[i])n=i;
    n=(unsigned)(((int)n+delta%4+4)%4);
    r->bars=lengths[n];
    r->position=(uint16_t)(r->position%urban_rec_length(r));
}
static int urban_rec_add(UrbanRecorder *r,unsigned kind,unsigned note,
                         unsigned val,unsigned parameter,unsigned pos) {
    if(r->count>=UR_REC_MAX_EVENTS)return 0;
    UrbanRecEvent *e=&r->event[r->count++];
    e->pos=urban_rec_quantize(r,pos);
    e->pass=r->pass;
    e->kind=(uint8_t)kind;e->note=(uint8_t)note;
    e->value=(uint8_t)val;e->parameter=(uint8_t)parameter;
    return 1;
}
static int urban_rec_capture(UrbanRecorder *r,unsigned kind,unsigned note,
                             unsigned value,unsigned parameter,unsigned pos) {
    if(r->state!=UR_REC_RECORD)return 0;
    return urban_rec_add(r,kind,note,value,parameter,pos);
}
/* A 16th-note transport clock advances two 32nd subticks. Events
 * placed between two 16ths are dispatched on the next observed UI
 * poll. Quantization OFF preserves their subgrid positions in the MIDI
 * editor; precise 32nd playback requires audio-thread scheduling.
 */
static void urban_rec_advance(UrbanRecorder *r,unsigned ticks) {
    unsigned len=urban_rec_length(r);
    if(!len)return;
    unsigned advance=r->position+ticks;
    if(advance>=len) {
        r->pass=(uint16_t)(r->pass+advance/len);
        if(r->state==UR_REC_RECORD)r->overdub=1u;
    }
    r->position=(uint16_t)(advance%len);
}
static unsigned urban_rec_events_at(const UrbanRecorder *r,
                                    unsigned pos,uint16_t *indices,
                                    unsigned capacity) {
    unsigned count=0u,len=urban_rec_length(r);
    if(!len)return 0u;
    pos%=len;
    for(unsigned i=0u;i<r->count;i++) {
        const UrbanRecEvent *e=&r->event[i];
        /* Don't replay what the player just pressed in this pass. */
        if(r->state==UR_REC_RECORD&&e->pass==r->pass)continue;
        if(e->pos==pos || ((pos&1u)==0u && e->pos==(pos+1u)%len)) {
            if(count<capacity)indices[count]=(uint16_t)i;
            count++;
        }
    }
    return count;
}
/* Step-grid editing: toggle note event without damaging other passes
 * or the automation lane, and keep recorder note events bounded. */
static void urban_rec_toggle_note(UrbanRecorder *r,unsigned pos,unsigned note) {
    unsigned q=urban_rec_quantize(r,pos);
    for(unsigned i=0u;i<r->count;i++) {
        if(r->event[i].kind==UR_REC_NOTE_ON &&
           r->event[i].pos==q&&r->event[i].note==note) {
            for(unsigned k=i+1u;k<r->count;k++)
                r->event[k-1u]=r->event[k];
            r->count--;
            return;
        }
    }
    (void)urban_rec_add(r,UR_REC_NOTE_ON,note,r->edit_velocity,0u,q);
    (void)urban_rec_add(r,UR_REC_NOTE_OFF,note,0u,0u,q+2u);
}
#endif
