/* SPDX-License-Identifier: GPL-3.0-only
 * Shared master LP/HP cutoff knobs, continuously editable 0..100.
 * The original HYP FX_LP and FX_HP keyboard momentaries remain
 * functional: at 100% they override each knob, then return to the
 * exact stored knob level on key release. The DSP exclusively owns
 * the audio smoothing; the UI publishes one atomic 32-bit request.
 */
#ifndef URBAN_FILTER_ENGINE_H
#define URBAN_FILTER_ENGINE_H
static volatile uint32_t urban_filter_requested;
volatile uint32_t urban_filter_applied;
volatile uint32_t urban_filter_processed_frames;
void urban_filter_set(unsigned lp,unsigned hp) {
    if(lp>100u)lp=100u;
    if(hp>100u)hp=100u;
    urban_filter_requested=lp|(hp<<8u);
}
static inline float urban_filter_target(unsigned v,unsigned pressed) {
    return pressed?1.0f:(float)v*0.01f;
}
static inline float urban_filter_slew(float now,float target,float dt) {
    float speed=target>now?8.0f:16.0f;
    return fm_clampf(now+fm_clampf(target-now,-speed*dt,speed*dt),
                     0.0f,1.0f);
}
#endif
