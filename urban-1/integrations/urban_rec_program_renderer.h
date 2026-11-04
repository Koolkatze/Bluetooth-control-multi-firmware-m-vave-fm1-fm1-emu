/* SPDX-License-Identifier: GPL-3.0-only
 * REC Track 9 — actual installed HYP-1 program oscillator models.
 * Uses read-only HYP PAD_SOUND, DRONE_SOUND, ACID_SOUND and KITS patch
 * definitions. REC owns its OWN phases, envelopes, filters, noise state,
 * and note/voice allocation. It never invokes note_cmd(), pad_play(),
 * drum_fire(), urban_jazz_note_cmd(), or urban_sfx_trigger().
 *
 * 0..15 HYP bass; 16..31 HYP acid/lead; 32..47 HYP pad;
 * 48..49 modelled acoustic bass; 50..54 modelled melodic jazz;
 * 55..59 polyphonic jazz keys; 60..67 eight HYP drum kits,
 * 68..77 ten procedural urban SFX.
 *
 * Reading immutable tables guarantees REC chooses the SAME oscillator
 * and patch coefficient identity as any generated track using that
 * instrument. A separate synth instance has an independent envelope,
 * modulation and effects mix; it will not phase-lock with the original.
 */
#ifndef URBAN_REC_PROGRAM_RENDERER_H
#define URBAN_REC_PROGRAM_RENDERER_H

static float urban_rec_tri(float p) {
    return p<0.5f?4.0f*p-1.0f:3.0f-4.0f*p;
}
static float urban_rec_soft(float x,float drive) {
    float y=x*drive;
    return y/(1.0f+0.32f*(y<0.0f?-y:y));
}
static float urban_rec_sine(float p) { return sine_any(p); }
static float urban_rec_sample_native_bass(UrbanRecTrackVoice *v,float tone) {
    /* Each REC bass reads the actual installed HYP sound's oscillator,
     * detune/sub mix, drive, cut and output level. No DRONE DSP state
     * or bass note stack is touched. */
    const drone_snd_t *d=&DRONE_SOUND[v->program&15u];
    float p=v->phase,pp=v->subphase,p2=v->phase3,p3=v->phase4;
    float hz=v->frequency;
    float inc=hz*(1.0f/EN_SR);
    float i0=inc*(d->osc==DO_FIFTHS?1.49831f:
              d->osc==DO_REESE||d->osc==DO_HOOVER?0.9862f:0.99481f);
    float i2=inc*(d->osc==DO_REESE||d->osc==DO_HOOVER?1.014f:1.00521f);
    float q=0.0f;
    switch(d->osc){
    case DO_REESE:
        q=(blep_saw(p,i0)+blep_saw(p2,i2))*d->saw*1.35f;break;
    case DO_PULSE: {
        float width=0.5f+0.20f*sine(v->modphase);
        q=(blep_saw(pp,inc)-blep_saw(pp+width>=1.0f?
                pp+width-1.0f:pp+width,inc))*d->saw*1.4f;break;
    }
    case DO_FM:
        q=urban_rec_sine(pp+(0.5f+v->peak*2.2f)*sine(p))*d->saw*2.2f;break;
    case DO_ORGAN:
        q=(sine(pp)+0.5f*sine(2*pp)+0.33f*sine(3*pp)+
            0.20f*sine(4*pp))*d->saw*1.5f;break;
    case DO_SINE:
        q=(sine(pp)+0.15f*sine(2*pp))*d->saw*1.8f;break;
    case DO_SQUARE: {
        float half=pp+0.5f;
        if(half>=1.0f)half-=1.0f;
        q=(blep_saw(pp,inc)-blep_saw(half,inc))*d->saw*1.2f;break;
    }
    case DO_HOOVER: {
        float a=p+0.3f,b=p2+0.3f;
        if(a>=1.0f)a-=1.0f;if(b>=1.0f)b-=1.0f;
        q=(blep_saw(p,i0)-blep_saw(a,i0)+
            blep_saw(p2,i2)-blep_saw(b,i2))*d->saw*0.9f+
            0.5f*d->saw*(2.0f*p3-1.0f);break;
    }
    case DO_FIFTHS:
        q=(blep_saw(pp,inc)+0.75f*blep_saw(p,i0))*d->saw*1.1f;break;
    case DO_METAL:
        q=urban_rec_sine(pp+(0.3f+1.6f*v->peak)*
                  sine(v->modphase))*d->saw*2.0f;break;
    case DO_SYNC:
        q=blep_saw(v->modphase,inc*(1.5f+2.5f*v->peak))*d->saw*1.3f;break;
    default:
        q=(blep_saw(p,i0)+blep_saw(pp,inc)+blep_saw(p2,i2))*d->saw;break;
    }
    q+=d->sub*sine(p3);
    v->phase=p+i0;if(v->phase>=1.0f)v->phase-=1.0f;
    v->subphase=pp+inc;if(v->subphase>=1.0f)v->subphase-=1.0f;
    v->phase3=p2+i2;if(v->phase3>=1.0f)v->phase3-=1.0f;
    v->phase4=p3+inc*0.5f;if(v->phase4>=1.0f)v->phase4-=1.0f;
    v->modphase+=inc*1.4142f;if(v->modphase>=1.0f)
        v->modphase-=(float)(int)v->modphase;
    /* Local low-pass, matching the actual instrument's tone profile
     * but not borrowing the generated DRONE's filter memory. */
    float cutoff=0.007f+(0.09f+0.65f*tone)*fm_exp2f(d->cut*0.35f);
    if(cutoff>0.96f)cutoff=0.96f;
    v->lp+=cutoff*(q-v->lp);
    return urban_rec_soft(v->lp,1.2f*d->drive)*d->g;
}
static float urban_rec_sample_native_lead(UrbanRecTrackVoice *v,float tone) {
    const acid_snd_t *a=&ACID_SOUND[(v->program-16u)&15u];
    float p=v->phase,p2=v->subphase,p3=v->phase3;
    float inc=v->frequency/EN_SR;
    float idx=(0.1f+2.1f*v->peak);
    float x=0.0f;
    switch(a->osc){
    case AO_SQUARE:case AO_PWM: {
        float width=a->osc==AO_PWM?0.5f+0.38f*sine(v->modphase):0.5f;
        float q=p+width;if(q>=1.0f)q-=1.0f;
        x=0.7f*(blep_saw(p,inc)-blep_saw(q,inc));break;
    }
    case AO_SYNC:
        x=blep_saw(p2,inc*(1.6f+2.6f*v->peak));break;
    case AO_PLUCK:
        x=urban_rec_sine(p+idx*sine(2.0f*p))*1.2f;break;
    case AO_CHIP:
        x=p<0.5f?0.75f:-0.75f;break;
    case AO_SUB:
        x=blep_saw(p,inc)+0.7f*(p2<0.5f?1.0f:-1.0f);break;
    case AO_TRANCE:
        x=0.5f*(blep_saw(p,inc)+blep_saw(p2,inc*1.0087f)+
                 blep_saw(p3,inc*0.9913f));break;
    case AO_BLEEP:x=sine(p)*1.3f;break;
    case AO_FIFTH:
        x=0.75f*(blep_saw(p,inc)+0.75f*blep_saw(p2,inc*1.49831f));break;
    case AO_BELL:
        x=urban_rec_sine(p+(0.15f+1.8f*v->peak)*sine(p2))*1.2f;break;
    case AO_ORGAN:
        x=(sine(p)+0.5f*sine(2*p)+0.3f*sine(3*p)+0.2f*sine(4*p))*0.95f;break;
    case AO_LASER:x=sine(p2)*1.25f;break;
    case AO_FOLD:x=urban_rec_sine((0.3f+1.2f*v->peak)*sine(p))*1.1f;break;
    case AO_SOFT:x=urban_rec_tri(p)*1.2f;break;
    default:x=blep_saw(p,inc);break;
    }
    v->phase=p+inc;if(v->phase>=1.0f)v->phase-=1.0f;
    float ratio=a->osc==AO_BELL?3.5f:a->osc==AO_FIFTH?1.49831f:
         a->osc==AO_TRANCE?1.0087f:a->osc==AO_LASER?
         (1.0f+7.0f*v->peak*v->peak):a->osc==AO_SUB?0.5f:
         a->osc==AO_SYNC?1.6f+2.6f*v->peak:1.0f;
    v->subphase=p2+inc*ratio;
    if(v->subphase>=1.0f)v->subphase-=(float)(int)v->subphase;
    v->phase3=p3+inc*0.9913f;
    if(v->phase3>=1.0f)v->phase3-=(float)(int)v->phase3;
    v->modphase+=0.35f/EN_SR;
    if(v->modphase>=1.0f)v->modphase-=1.0f;
    float cutoff=0.06f+0.77f*tone;
    v->lp+=cutoff*(x-v->lp);
    return urban_rec_soft(v->lp,a->drive*(1.2f+tone*0.8f))*a->g;
}
static float urban_rec_sample_native_pad(UrbanRecTrackVoice *v,float tone) {
    unsigned patch=(v->program-32u)&15u;
    unsigned osc=PAD_SOUND[patch].osc;
    float p=v->phase,p2=v->subphase,inc=v->frequency/EN_SR;
    float detune=inc*1.00521f;
    float idx=0.04f+0.13f*v->peak+0.06f*tone;
    float x=0.0f;
    switch(osc){
    case PO_RHODES:
        x=(urban_rec_sine(p+idx*sine(p)+1.0f)+0.12f*sine(2*p))*1.2f;break;
    case PO_STRINGS:
        x=0.62f*(blep_saw(p,inc)+blep_saw(p2,detune));break;
    case PO_GLASS:
        x=(urban_rec_sine(p+(0.25f+4.0f*idx)*sine(p2))+
           0.25f*sine(2*p))*1.1f;break;
    case PO_ORGAN:
        x=(0.62f*sine(p)+0.38f*sine(2*p)+0.26f*sine(3*p)+
           0.16f*sine(4*p)+0.20f*sine(p2))*1.05f;break;
    case PO_WARM:
        x=(urban_rec_tri(p)+urban_rec_tri(p2))*0.7f+0.5f*sine(p);break;
    case PO_SUPER:
        x=0.5f*(blep_saw(p,inc)+blep_saw(p2,detune))+
          0.3f*blep_saw(p*2.0f>=1.0f?p*2.0f-1.0f:p*2.0f,2.0f*inc);break;
    case PO_PWM: {
        float q=p2+0.5f;if(q>=1.0f)q-=1.0f;
        x=0.55f*(blep_saw(p,inc)-blep_saw(q,detune))+0.3f*sine(p);break;
    }
    case PO_HOLLOW:
        x=(sine(p)+0.33f*sine(3*p)+0.20f*sine(5*p)+
           0.14f*sine(7*p))*1.15f;break;
    case PO_SINE:
        x=(sine(p)+0.18f*sine(2*p))*1.3f;break;
    default: /* original HYP Voices formant spectral character */
        x=urban_rec_tri(p)*0.72f+0.28f*blep_saw(p,inc)+
          0.20f*sine(3*p)+0.13f*sine(5*p);break;
    }
    v->phase=p+inc;if(v->phase>=1.0f)v->phase-=1.0f;
    v->subphase=p2+(osc==PO_GLASS?inc*3.5f:detune);
    if(v->subphase>=1.0f)v->subphase-=(float)(int)v->subphase;
    float fc=0.02f+0.45f*tone*PAD_SOUND[patch].fc;
    if(fc>0.97f)fc=0.97f;
    v->lp+=fc*(x-v->lp);
    return v->lp*PAD_SOUND[patch].g;
}
static float urban_rec_sample_jazz(UrbanRecTrackVoice *v,float tone) {
    /* Identical jazz oscillator models, separate REC voice state.
     * 48/49 are upright/finger bass (the same harmonic series as the
     * Jazz Bass module); 50..59 map 1:1 to the five modelled sounds. */
    float p=v->phase,inc=v->frequency/EN_SR;
    float decay=v->brightness;
    float x=0.0f;
    if(v->program==48u||v->program==49u){
        unsigned finger=v->program==49u;
        x=0.81f*sine(p)+(finger?0.28f:0.24f)*decay*sine(2.006f*p)+
          0.12f*decay*decay*sine(3.015f*p)+
          0.04f*decay*decay*sine(4.027f*p);
        v->noise=v->noise*1664525u+1013904223u;
        float white=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
        v->lp2+=0.058f*(white-v->lp2);
        v->lp+=0.11f*(x-v->lp);
        x=0.79f*x+0.21f*v->lp+v->lp2*0.045f*decay;
        v->brightness*=finger?0.99991f:0.99987f;
    }else{
        unsigned model=v->program<55u?v->program-49u:v->program-54u;
        switch(model){
        case 1: /* acoustic piano */
            v->noise=v->noise*1664525u+1013904223u;
            x=0.70f*sine(p)+0.24f*sine(2.006f*p)+
              0.085f*sine(3.02f*p)+
              0.10f*decay*(float)(int32_t)v->noise*(1.0f/2147483648.0f);
            v->brightness*=0.999972f;break;
        case 2: { /* Rhodes-like tine/FM */
            float idx=0.02f+0.23f*decay*decay;
            x=0.81f*urban_rec_sine(p+idx*sine(2*p))+
              0.13f*sine(2.01f*p);
            v->brightness*=0.999984f;break;
        }
        case 3: /* breathy flute */
            v->noise=v->noise*1664525u+1013904223u;
            v->lp2+=0.022f*((float)(int32_t)v->noise*
                       (1.0f/2147483648.0f)-v->lp2);
            x=0.93f*sine(p)+0.15f*sine(2*p)+0.045f*v->lp2;
            break;
        case 4: /* vibraphone */
            x=(0.77f*sine(p)+0.20f*sine(3.99f*p)+
               0.055f*sine(9.14f*p))*
              (0.90f+0.10f*sine(v->modphase));
            v->brightness*=0.999987f;break;
        default: /* soft bell */
            x=0.65f*sine(p)+0.26f*sine(2.01f*p)+
              0.09f*sine(3.91f*p);
            v->brightness*=0.999971f;break;
        }
    }
    v->phase=p+inc;if(v->phase>=1.0f)v->phase-=1.0f;
    v->modphase+=5.1f/EN_SR;
    if(v->modphase>=1.0f)v->modphase-=1.0f;
    if(v->program>=50u && v->program<=59u)
        v->lp+= (0.045f+0.8f*tone)*(x-v->lp);
    return (v->program>=50u&&v->program<=59u)?v->lp:x;
}
static float urban_rec_sample_kit(UrbanRecTrackVoice *v,float tone) {
    const kit_t *k=&KITS[(v->program-60u)&7u];
    /* Same real HYP kit table, with its own local percussive oscillator,
     * hat/noise and snare filter state rather than shared six drum lanes. */
    unsigned lane=((unsigned)v->note+192u-53u)%6u;
    float time=v->age,tail=1.0f-time*5.0f;
    if(tail<0.0f)tail=0.0f;
    v->noise=v->noise*1664525u+1013904223u;
    float nz=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
    float p=v->phase,frequency=0.0f,x=0.0f;
    switch(lane){
    case 0: /* HYP kick sweep/drive/rumble */
        frequency=52.0f*(1.0f+k->k_start*0.11f*tail);
        x=sine(p)*tail*tail+0.10f*k->k_click*nz*tail*tail;
        x=urban_rec_soft(x,k->k_drive);break;
    case 1: /* snare: two pitched resonators plus noise */
        frequency=k->sn_f1;
        x=(sine(p)+0.36f*sine(v->subphase))*0.27f*k->sn_body+
          k->sn_noise*nz*0.45f;
        x*=tail;break;
    case 2: /* clap: burst noise with kit-specific decay */
        frequency=k->cl_gap+190.0f;
        x=nz*(0.58f+0.25f*sine(p*3.0f))*tail;break;
    case 3:case 4: /* closed/open metal hats */
        frequency=k->h_metal;
        v->lp+=0.12f*(nz-v->lp);
        x=(nz-v->lp)*k->h_noise*0.70f;break;
    default: /* kit percussion FM */
        frequency=k->p_freq;
        x=urban_rec_sine(p+k->p_idx*0.13f*sine(v->subphase))*
          0.65f;break;
    }
    if(frequency>12000.0f)frequency=12000.0f;
    v->phase=p+frequency/EN_SR;
    if(v->phase>=1.0f)v->phase-=1.0f;
    v->subphase+=frequency*k->p_ratio/EN_SR;
    if(v->subphase>=1.0f)v->subphase-=(float)(int)v->subphase;
    return x*(0.5f+0.5f*tone);
}
static float urban_rec_sample_sfx(UrbanRecTrackVoice *v) {
    /* Original URBAN SoundFX ten sound identities, computed with
     * per-REC state, without using urban_sfx_voice or its command word. */
    unsigned type=v->program-68u;
    float t=v->age*2.0f;
    if(t>1.0f)t=1.0f;
    float tail=1.0f-t,frequency=190.0f,cutoff=0.035f,envelope=tail;
    v->noise=v->noise*1664525u+1013904223u;
    float white=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
    switch(type){
    case 0:frequency=340.0f+210.0f*sine(t*3.0f);
           cutoff=0.06f+0.12f*tail;envelope=tail*tail;break;
    case 1:frequency=440.0f*tail*tail+35.0f;cutoff=0.05f;break;
    case 2:frequency=80.0f+360.0f*t;cutoff=0.015f+0.19f*t;
           envelope=t*t*(0.9f-0.5f*t);break;
    case 3:frequency=130.0f*tail+42.0f;cutoff=0.05f;
           envelope=tail*tail*tail;break;
    case 4:frequency=270.0f+280.0f*sine(t*1.7f);cutoff=0.065f;
           envelope=tail*(0.6f+0.4f*t);break;
    case 5:frequency=1100.0f*tail*tail+90.0f;cutoff=0.11f;
           envelope=tail*tail;break;
    case 6:frequency=110.0f+830.0f*t;cutoff=0.018f+0.21f*t;
           envelope=t*t;break;
    case 7:frequency=850.0f*tail+60.0f;cutoff=0.015f+0.2f*tail;
           envelope=tail*tail;break;
    case 8:frequency=78.0f;cutoff=0.16f;
           envelope=tail*tail*tail*tail;break;
    default:frequency=130.0f+260.0f*t;cutoff=0.02f+0.06f*t;
           envelope=t<0.5f?2.0f*t:2.0f*tail;break;
    }
    v->lp+=cutoff*(white-v->lp);
    v->lp2+=0.015f*(v->lp-v->lp2);
    float band=v->lp-v->lp2;
    float p=v->phase,x=sine(p);
    v->phase=p+frequency/EN_SR;
    if(v->phase>=1.0f)v->phase-=1.0f;
    switch(type){
    case 0:x=band*2.1f+x*0.18f;break;
    case 1:x=x*0.78f+band*0.30f;break;
    case 2:x=band*2.0f+x*0.12f;break;
    case 3:x=x*0.85f+band*0.38f;break;
    case 4:x=x*0.88f+band*0.06f;break;
    case 5:x=(x>0.0f?0.6f:-0.6f)+band*0.9f;break;
    case 6:x=band*2.4f+x*0.21f;break;
    case 7:x=band*1.9f+x*0.12f;break;
    case 8:x=band*2.2f;break;
    default:x=band*2.0f;break;
    }
    return x*envelope;
}
static float urban_rec_program_sample(UrbanRecTrackVoice *v,float tone) {
    if(v->program<16u)return urban_rec_sample_native_bass(v,tone);
    if(v->program<32u)return urban_rec_sample_native_lead(v,tone);
    if(v->program<48u)return urban_rec_sample_native_pad(v,tone);
    if(v->program<60u)return urban_rec_sample_jazz(v,tone);
    if(v->program<68u)return urban_rec_sample_kit(v,tone);
    if(v->program<78u)return urban_rec_sample_sfx(v);
    return 0.0f;
}
#endif
