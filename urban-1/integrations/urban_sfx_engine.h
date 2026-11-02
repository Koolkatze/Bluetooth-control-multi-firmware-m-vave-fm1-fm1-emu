/* SPDX-License-Identifier: GPL-3.0-only
 * Real mono/stereo procedural SFX voice mixed into HYP's audio buffer.
 * Included in dsp/amb.c AFTER mixl/mixr, sine() and step_len exist.
 * Only 1 voice, no malloc, no files, no decoder, no DSP on the UI thread.
 * User-triggered commands are 32-bit atomic on JieLi and host targets.
 */
#ifndef URBAN_SFX_ENGINE_H
#define URBAN_SFX_ENGINE_H
typedef struct {
    uint32_t left,total,noise;
    uint8_t type;
    float amp,phase,lp,hp;
} UrbanSfxVoice;
static UrbanSfxVoice urban_sfx_voice;
static volatile uint32_t urban_sfx_command;
/* Called from UI/transport thread. New transient replaces older transient
 * rather than queuing events and potentially blocking the audio interrupt.
 * Clearing the command on STOP prevents stray sounds on the next song. */
void urban_sfx_trigger(unsigned type,unsigned level,unsigned length) {
    if(level==0u || type>=10u)return;
    if(level>100u)level=100u;
    if(length<1u)length=1u;
    if(length>16u)length=16u;
    urban_sfx_command=1u+(uint32_t)type+
        ((uint32_t)level<<8)+((uint32_t)length<<16);
}
void urban_sfx_silence(void) {
    urban_sfx_command=0u;
    urban_sfx_voice.left=0u;
}
static void urban_sfx_render(uint32_t n) {
    UrbanSfxVoice *v=&urban_sfx_voice;
    if(!en_playing) {
        v->left=0u;
        urban_sfx_command=0u;
        return;
    }
    uint32_t cmd=urban_sfx_command;
    if(cmd!=0u) {
        urban_sfx_command=0u;
        v->type=(uint8_t)((cmd&0xffu)-1u);
        v->amp=(float)((cmd>>8)&0xffu)*0.01f;
        float len=(float)((cmd>>16)&0xffu)*step_len;
        if(len<EN_SR/30.0f)len=EN_SR/30.0f;
        if(len>EN_SR*3.0f)len=EN_SR*3.0f;
        v->left=v->total=(uint32_t)len;
        v->phase=0.0f;
        v->lp=v->hp=0.0f;
        v->noise^=0x9e3779b9u+(uint32_t)en_bars*0x85ebca6bu;
        if(!v->noise)v->noise=0x456789abu;
    }
    if(!v->left)return;
    for(uint32_t i=0u;i<n && v->left;i++) {
        float p=1.0f-(float)v->left/(float)v->total;
        float tail=1.0f-p;
        v->noise=v->noise*1664525u+1013904223u;
        float white=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
        float cutoff=0.035f,frequency=190.0f,shape=0.0f,envelope=tail;
        /* Each sound has its OWN frequency, dynamics and noise model.
         * These are synthesis imitations, not imported recordings. */
        switch(v->type) {
        case 0: /* record scratching: reversing pitch/fluctuating bright noise */
            frequency=340.0f+210.0f*sine(p*3.0f);
            cutoff=0.06f+0.12f*(1.0f-p);
            envelope=tail*tail;
            break;
        case 1: /* tape-stop: bent-down harmonic tone with noise grit */
            frequency=440.0f*(tail*tail)+35.0f;
            cutoff=0.05f;
            envelope=tail;
            break;
        case 2: /* reversed whoosh: noise slowly opening into a transition */
            frequency=80.0f+360.0f*p;
            cutoff=0.015f+0.19f*p;
            envelope=p*p*(0.9f-0.5f*p);
            break;
        case 3: /* sub impact: short descending sine and muted broadband hit */
            frequency=130.0f*tail+42.0f;
            cutoff=0.05f;
            envelope=tail*tail*tail;
            break;
        case 4: /* dub siren: descending and rising self-modulated tone */
            frequency=270.0f+280.0f*sine(p*1.7f);
            cutoff=0.065f;
            envelope=tail*(0.6f+0.4f*p);
            break;
        case 5: /* glitch zap: hard transient but no global bitcrushing */
            frequency=1100.0f*tail*tail+90.0f;
            cutoff=0.11f;
            envelope=tail*tail;
            break;
        case 6: /* riser: brightening filtered noise with high harmonic */
            frequency=110.0f+830.0f*p;
            cutoff=0.018f+0.21f*p;
            envelope=p*p;
            break;
        case 7: /* downlifter */
            frequency=850.0f*tail+60.0f;
            cutoff=0.015f+0.2f*tail;
            envelope=tail*tail;
            break;
        case 8: /* dusty record pop: very short, damped noise click */
            frequency=78.0f;
            cutoff=0.16f;
            envelope=tail*tail*tail*tail;
            break;
        default: /* air whoosh: soft band-passed wind */
            frequency=130.0f+260.0f*p;
            cutoff=0.02f+0.06f*p;
            envelope=(p<0.5f?2.0f*p:2.0f*tail);
            break;
        }
        v->lp+=cutoff*(white-v->lp);
        v->hp+=0.015f*(v->lp-v->hp);
        float band=v->lp-v->hp;
        v->phase+=frequency*(1.0f/EN_SR);
        if(v->phase>=1.0f)v->phase-=1.0f;
        float osc=sine(v->phase);
        switch(v->type) {
        case 0: shape=band*2.1f+osc*0.18f;break;
        case 1: shape=osc*0.78f+band*0.30f;break;
        case 2: shape=band*2.0f+osc*0.12f;break;
        case 3: shape=osc*0.85f+band*0.38f;break;
        case 4: shape=osc*0.88f+band*0.06f;break;
        case 5: shape=(osc>0.0f?0.6f:-0.6f)+band*0.9f;break;
        case 6: shape=band*2.4f+osc*0.21f;break;
        case 7: shape=band*1.9f+osc*0.12f;break;
        case 8: shape=band*2.2f;break;
        default:shape=band*2.0f;break;
        }
        float sample=shape*envelope*v->amp*0.19f;
        mixl[i]+=sample*0.92f;
        mixr[i]+=sample*1.08f;
        v->left--;
    }
}
#endif
