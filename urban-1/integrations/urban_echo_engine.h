/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 shared DSP echo bus. HYP's original P_ECHO only multiplies
 * very weak per-voice esend; bass/drums have sends as low as 0.04,
 * so Echo 45 is practically inaudible while FX_THROW injects the
 * whole dry mix at 0.5*(L+R). Route NORMAL Echo through the SAME
 * master pre-delay mix as Throw, with continuously adjustable level.
 *
 * This is a send fix, not an extra delay line. Keep HYP's stereo tape,
 * Echoplex, clock-synced timing, feedback, reverb and master intact.
 */
#ifndef URBAN_ECHO_ENGINE_H
#define URBAN_ECHO_ENGINE_H

volatile uint32_t urban_echo_normal_input_frames;
volatile uint32_t urban_echo_normal_tap_frames;
volatile uint32_t urban_echo_throw_input_frames;

/* eg = HYP's echo_send (P_ECHO^2 / 10000).
 * A typical knob value of 50 gives an audible 22% of the Throw input
 * while 100 is close to full Throw. Throw at 100% continues to use
 * 0.5*(L+R), as in upstream HYP. Saturate the SEND amount, not the
 * delay waveform, when Echo and Throw are both active, avoiding
 * unexpectedly overdriving the tape's fixed-point input.
 */
static inline float urban_echo_bus_input(float l,float r,float colored,
                                         float eg,float throw_amount) {
    float dry=0.5f*(l+r);
    float normal=eg*0.88f;
    float total=fm_minf(normal+throw_amount,1.0f);
    float input=dry*total+colored*eg*0.09f;
    if(eg>0.00001f && normal>0.0f &&
       (input>0.00001f || input< -0.00001f))
        ++urban_echo_normal_input_frames;
    if(throw_amount>0.001f &&
       (dry>0.00001f || dry< -0.00001f))
        ++urban_echo_throw_input_frames;
    return input;
}

static inline void urban_echo_tap_meter(float el,float er,
                                        float eg,float throw_amount) {
    /* Not a UI parameter change: real delayed tape HEAD output.
     * Only count the normal echo without the Throw effect held. */
    if(eg>0.00001f && throw_amount<0.001f &&
       (el>0.0001f || el< -0.0001f ||
        er>0.0001f || er< -0.0001f))
        ++urban_echo_normal_tap_frames;
}
#endif
