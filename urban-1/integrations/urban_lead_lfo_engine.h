/* SPDX-License-Identifier: GPL-3.0-only
 * Native 0-100-rate/depth LFO modulates ACID lead filter cutoff.
 * HYP stock has only PAD and DRONE LFOs; this adds a genuinely
 * independent LEAD LFO that works during playback and recording.
 * Song-UI and DSP communicate only via one atomic 32-bit word.
 */
#ifndef URBAN_LEAD_LFO_ENGINE_H
#define URBAN_LEAD_LFO_ENGINE_H
static volatile uint32_t urban_lead_lfo_requested;
static float urban_lead_lfo_phase;
static float urban_lead_lfo_oct;
volatile uint32_t urban_lead_lfo_frames;
void urban_lead_lfo_config(unsigned rate,unsigned depth) {
    if(rate>100u)rate=100u;
    if(depth>100u)depth=100u;
    urban_lead_lfo_requested=rate|(depth<<8u);
}
static void urban_lead_lfo_tick(uint32_t n) {
    uint32_t v=urban_lead_lfo_requested;
    unsigned rate=v&255u,depth=(v>>8u)&255u;
    if(depth==0u) {
        urban_lead_lfo_oct=0.0f;
        return;
    }
    float hz=0.10f+((float)rate*(float)rate)*0.00118f; /* 0.1..11.9 Hz */
    urban_lead_lfo_phase+=hz*(float)n*(1.0f/EN_SR);
    if(urban_lead_lfo_phase>=1.0f)
        urban_lead_lfo_phase-=(float)(int)urban_lead_lfo_phase;
    urban_lead_lfo_oct=2.8f*(float)depth*0.01f*sine(urban_lead_lfo_phase);
    urban_lead_lfo_frames+=n;
}
#endif
