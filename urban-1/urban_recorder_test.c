/* SPDX-License-Identifier: GPL-3.0-only */
#include "urban_recorder.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    UrbanRecorder r;
    urban_rec_init(&r);
    assert(r.state==UR_REC_OFF&&r.track==8u&&r.bars==4u);
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
    /* Exercise the entire real patch catalog and preserve REC routing. */
    UrbanRecorder cat;
    urban_rec_init(&cat);
    assert(UR_REC_PROGRAM_COUNT==78u && cat.program==32u && cat.track==8u);
    urban_rec_set_program(&cat,-32);
    assert(cat.program==0u && cat.track==8u);
    unsigned family_count[5]={0};
    for(unsigned i=0u;i<UR_REC_PROGRAM_COUNT;i++) {
        UrbanRecInstrument p=urban_rec_instrument(cat.program);
        assert(cat.program==i && cat.track==UR_REC_TRACK_INDEX);
        if(i<16u) {assert(p.track==2u&&p.patch==i&&!p.jazz);family_count[0]++;}
        else if(i<32u) {assert(p.track==3u&&p.patch==i-16u&&!p.jazz);family_count[1]++;}
        else if(i<48u) {assert(p.track==4u&&p.patch==i-32u&&!p.jazz);family_count[2]++;}
        else if(i<50u) {assert(p.track==2u&&p.jazz==i-47u);family_count[0]++;}
        else if(i<55u) {assert(p.track==3u&&p.jazz==i-49u);family_count[1]++;}
        else if(i<60u) {assert(p.track==4u&&p.jazz==i-54u);family_count[2]++;}
        else if(i<68u) {assert(p.track==0u&&p.patch==i-60u);family_count[3]++;}
        else {assert(p.track==6u&&p.patch==i-68u);family_count[4]++;}
        urban_rec_set_program(&cat,1);
    }
    assert(cat.program==0u&&cat.track==8u);
    assert(family_count[0]==18u&&family_count[1]==21u&&
           family_count[2]==21u&&family_count[3]==8u&&family_count[4]==10u);
    urban_rec_set_program(&cat,-1);
    assert(cat.program==77u&&cat.track==8u);
    urban_rec_set_program(&cat,78);
    assert(cat.program==77u);
    /* Physical keyboard octave moves in semitones and saturates at +/-3,
     * without transposing any MIDI note event already captured. */
    assert(cat.octave==0&&urban_rec_key_note(&cat,7u)==60u);
    urban_rec_shift_octave(&cat,1);
    assert(cat.octave==1&&urban_rec_key_note(&cat,7u)==72u);
    urban_rec_shift_octave(&cat,8);
    assert(cat.octave==3&&urban_rec_key_note(&cat,7u)==96u);
    urban_rec_shift_octave(&cat,-9);
    assert(cat.octave== -3&&urban_rec_key_note(&cat,7u)==24u);
    urban_rec_shift_octave(&cat,3);
    assert(cat.octave==0&&urban_rec_key_note(&cat,7u)==60u);
    urban_rec_arm(&cat);
    urban_rec_start(&cat);
    assert(urban_rec_capture(&cat,UR_REC_NOTE_ON,60u,110u,0u,4u));
    urban_rec_stop(&cat);
    urban_rec_overdub(&cat);
    assert(cat.count==1u&&cat.track==8u);
    /* PAUSE must be non-destructive: no recorder reset is triggered.
     * SONG/TRACK SAVE or genre/subgenre change explicitly resets the
     * entire saved take, including program, quant, events and cursors. */
    uint16_t saved_count=cat.count;
    unsigned saved_program=cat.program;
    urban_rec_stop(&cat); /* PAUSE finalizes overdub but keeps its notes */
    assert(cat.state==UR_REC_EDIT);
    assert(cat.count==saved_count&&cat.program==saved_program);
    urban_rec_reset(&cat);
    assert(cat.state==UR_REC_OFF&&cat.count==0u&&cat.position==0u);
    assert(cat.pass==0u&&!cat.overdub&&!cat.play_cursor&&!cat.edit_cursor);
    assert(cat.program==32u&&cat.track==8u&&cat.bars==4u&&cat.quant==3u);
    assert(cat.octave==0&&cat.mod_page==0u);
    uint16_t reset_found[4]={0};
    assert(urban_rec_events_at(&cat,0u,reset_found,4u)==0u);
    /* Native REC pitches (bass, lead, chords) are a separate bounded
     * active set. A PAUSE must drain every sounding pitch, even when
     * its corresponding loop NOTE_OFF was never scheduled. */
    UrbanRecVoices voices;
    urban_rec_voices_clear(&voices);
    assert(urban_rec_voices_count(&voices)==0u);
    urban_rec_voice_set(&voices,0u,60u,1);
    urban_rec_voice_set(&voices,0u,60u,1);
    urban_rec_voice_set(&voices,1u,127u,1);
    urban_rec_voice_set(&voices,2u,24u,1);
    assert(urban_rec_voices_count(&voices)==3u);
    assert(urban_rec_voice_active(&voices,0u,60u));
    assert(urban_rec_voice_active(&voices,1u,127u));
    assert(!urban_rec_voice_active(&voices,3u,60u));
    urban_rec_voice_set(&voices,0u,60u,0);
    assert(urban_rec_voices_count(&voices)==2u);
    urban_rec_voices_clear(&voices);
    assert(urban_rec_voices_count(&voices)==0u);
    puts("URBAN-1 recorder: looping 32nd grid, quant OFF/4/8/16/32, poly events, overdub, automation and bounded events PASS");
    return 0;
}
