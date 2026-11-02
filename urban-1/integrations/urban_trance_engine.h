/* SPDX-License-Identifier: GPL-3.0-only
 * Included from HYP dsp/amb.c after note_cmd is defined.
 * One lead voice. Audio callback is the ONLY timing authority.
 * Two precompiled 4-bar-by-section grids. No audio-thread allocations,
 * score recomposition, or UI-frame-triggered note-on events.
 */
#ifndef URBAN_TRANCE_ENGINE_H
#define URBAN_TRANCE_ENGINE_H
#include "../app/urban_trance_clock.h"
static UrbanTranceGrid urban_trance_grids[2];
static volatile uint8_t urban_trance_front;
volatile uint32_t urban_trance_audio_onsets;
volatile uint32_t urban_trance_offgrid_onsets;
volatile uint32_t urban_trance_audio_last_step;
void urban_trance_publish(const UrbanTranceGrid *grid) {
    unsigned back=urban_trance_front^1u;
    /* The DSP reads front only; publish the copied back buffer atomically. */
    const uint8_t *src=(const uint8_t *)grid;
    uint8_t *dst=(uint8_t *)&urban_trance_grids[back];
    for(unsigned i=0;i<sizeof(UrbanTranceGrid);i++)dst[i]=src[i];
    BARRIER();
    urban_trance_front=(uint8_t)back;
}
static void urban_trance_audio_tick(uint32_t step) {
    static int held_key=-1;
    static unsigned left,loopbar;
    static uint8_t loopsection;
    static uint32_t prior=UINT32_MAX;
    const UrbanTranceGrid *grid=&urban_trance_grids[urban_trance_front];
    urban_trance_audio_last_step=step;
    if(step==0u || step<=prior) {
        if(held_key>=0)note_cmd(EN_V_ACID,held_key,0);
        held_key=-1; left=0u; loopbar=0u;
        loopsection=grid->loop_queued%URBAN_TRANCE_SECTIONS;
    }
    prior=step;
    if(!grid->enabled || !grid->length) {
        if(held_key>=0)note_cmd(EN_V_ACID,held_key,0);
        held_key=-1; left=0u;return;
    }
    unsigned st=step&15u,section=0u,bar=0u;
    if(grid->song_mode) {
        unsigned pos=(step/16u)%grid->length;
        section=grid->sequence[pos]%URBAN_TRANCE_SECTIONS;
        bar=grid->bar[pos]%URBAN_TRANCE_PHRASE;
    } else {
        if(st==0u && step!=0u) {
            uint8_t next=grid->loop_queued%URBAN_TRANCE_SECTIONS;
            if(loopsection!=next) {
                loopsection=next;loopbar=0u;
            } else loopbar=(loopbar+1u)%URBAN_TRANCE_PHRASE;
        }
        section=loopsection;
        bar=loopbar;
    }
    /* Gate release and next onset occur on EXACT same sixteenth
     * boundaries as tick()->drum_fire(), never from the 30-fps LCD. */
    if(left && --left==0u && held_key>=0) {
        note_cmd(EN_V_ACID,held_key,0);
        held_key=-1;
    }
    const UrbanTranceNote *n=&grid->notes[section][bar][st];
    if(n->note && n->gate) {
        if(held_key>=0)note_cmd(EN_V_ACID,held_key,0);
        int key=(int)n->note+12;
        note_cmd(EN_V_ACID,key,1);
        held_key=key;
        left=n->gate;
        urban_trance_audio_onsets++;
        if(st&1u)urban_trance_offgrid_onsets++;
    }
}
#endif
