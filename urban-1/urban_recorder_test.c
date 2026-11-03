/* SPDX-License-Identifier: GPL-3.0-only */
#include "urban_recorder.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    UrbanRecorder r;
    urban_rec_init(&r);
    assert(r.state==UR_REC_OFF&&r.track==4u&&r.bars==4u);
    urban_rec_arm(&r);
    assert(r.state==UR_REC_ARM);
    urban_rec_set_bars(&r,-2);
    assert(r.bars==1u&&urban_rec_length(&r)==32u);
    urban_rec_start(&r);
    assert(r.state==UR_REC_RECORD);
    /* Standard 1/16: two 32nd ticks per slot. */
    assert(urban_rec_capture(&r,UR_REC_NOTE_ON,60u,105u,0u,3u));
    assert(r.event[0].pos==4u);
    assert(urban_rec_capture(&r,UR_REC_NOTE_OFF,60u,0u,0u,15u));
    assert(r.event[1].pos==16u);
    for(unsigned i=0u;i<16u;i++)urban_rec_advance(&r,2u);
    assert(r.position==0u && r.pass==1u && r.overdub);
    uint16_t found[4]={0};
    assert(urban_rec_events_at(&r,4u,found,4u)==1u);
    assert(found[0]==0u);
    /* Record knobs on the SAME loop and preserve old notes on overdub. */
    assert(urban_rec_capture(&r,UR_REC_PARAM,0u,88u,42u,5u));
    assert(r.event[2].pos==6u&&r.event[2].value==88u);
    for(unsigned i=0;i<16u;i++)urban_rec_advance(&r,2u);
    assert(r.pass==2u&&r.count==3u);
    assert(urban_rec_events_at(&r,6u,found,4u)==1u);
    urban_rec_stop(&r);
    assert(r.state==UR_REC_EDIT&&r.count==3u);
    urban_rec_toggle_note(&r,8u,64u);
    assert(r.count==5u);
    urban_rec_toggle_note(&r,8u,64u);
    assert(r.count==3u); /* paired note-off removed too */
    urban_rec_overdub(&r);
    assert(r.state==UR_REC_RECORD&&r.count==3u&&r.overdub);
    r.quant=0u; /* no quantization, preserve 32nd position */
    assert(urban_rec_capture(&r,UR_REC_NOTE_ON,67u,100u,0u,9u));
    assert(r.event[r.count-1u].pos==9u);
    assert(urban_rec_capture(&r,UR_REC_PARAM,0u,(unsigned)-7,44u,11u));
    assert((int)(int8_t)r.event[r.count-1u].value==-7);
    r.quant=1u; /* quarter notes */
    assert(urban_rec_quantize(&r,13u)==16u);
    r.quant=2u; /* eighths */
    assert(urban_rec_quantize(&r,13u)==12u);
    r.quant=4u; /* 32nds */
    assert(urban_rec_quantize(&r,13u)==13u);
    for(unsigned i=0u;i<600u;i++)
        (void)urban_rec_add(&r,UR_REC_PARAM,0u,1u,1u,i);
    assert(r.count==UR_REC_MAX_EVENTS);
    assert(!urban_rec_add(&r,UR_REC_PARAM,0u,1u,1u,0u));
    puts("URBAN-1 recorder: looping 32nd grid, quant OFF/4/8/16/32, poly events, overdub, automation and bounded events PASS");
    return 0;
}
