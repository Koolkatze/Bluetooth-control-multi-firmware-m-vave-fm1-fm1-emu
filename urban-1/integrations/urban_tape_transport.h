/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN play/stop tape transport for HYP DSP.
 * STOP: reuse HYP's existing FX_STOP pitch-bent ring read for exactly
 * one beat before en_play(0), never hard stop.
 * START: resample the ALREADY ALLOCATED HYP master tape ring from
 * low playback speed to full speed over ~0.42 s, crossfade to live.
 * No new large ring, no allocation, UI posts an atomic 32-bit request.
 * Audio callback owns all phase and DMA sample-rate calculations.
 */
#ifndef URBAN_TAPE_TRANSPORT_H
#define URBAN_TAPE_TRANSPORT_H
static volatile uint32_t urban_tape_start_request;
static uint32_t urban_tape_start_applied;
static float urban_tape_start_read;
static uint32_t urban_tape_start_remaining;
volatile uint32_t urban_tape_start_frames;
volatile uint32_t urban_tape_start_completed;

void urban_tape_start_trigger(void) {
    ++urban_tape_start_request;
}
static inline void urban_tape_start_mix(float *l,float *r) {
    uint32_t req=urban_tape_start_request;
    const uint32_t total=(uint32_t)(EN_SR*0.42f);
    if(req!=urban_tape_start_applied) {
        urban_tape_start_applied=req;
        urban_tape_start_read=(float)(rb_w*2u);
        urban_tape_start_remaining=total;
    }
    if(!urban_tape_start_remaining)return;
    uint32_t done=total-urban_tape_start_remaining;
    float progress=(float)done/(float)total;
    /* Initial read head is slower than the write head, creating an
     * audible upward pitch sweep on the song's NEW audio, not stale
     * audio from a previous recording. Within the existing 0.95s ring. */
    float speed=0.20f+0.80f*progress*progress;
    float index=urban_tape_start_read*0.5f;
    unsigned k=(unsigned)index;
    float f=index-(float)k;
    unsigned a=k%RB_N,b=(a+1u)%RB_N;
    float tl=((float)rb[2u*a]+
             ((float)rb[2u*b]-(float)rb[2u*a])*f)*(1.0f/12000.0f);
    float tr=((float)rb[2u*a+1u]+
             ((float)rb[2u*b+1u]-(float)rb[2u*a+1u])*f)*(1.0f/12000.0f);
    urban_tape_start_read+=speed;
    /* By the last 20% smoothly blend from pitched tape back to
     * synchronous live. An amplitude ramp masks the initial stale
     * ring bytes before the first freshly written frames arrive. */
    float live=progress>0.80f?(progress-0.80f)*5.0f:0.0f;
    float fade=progress<0.12f?progress/0.12f:1.0f;
    *l=fade*(tl*(1.0f-live)+*l*live);
    *r=fade*(tr*(1.0f-live)+*r*live);
    urban_tape_start_remaining--;
    urban_tape_start_frames++;
    if(!urban_tape_start_remaining)urban_tape_start_completed++;
}
#endif
