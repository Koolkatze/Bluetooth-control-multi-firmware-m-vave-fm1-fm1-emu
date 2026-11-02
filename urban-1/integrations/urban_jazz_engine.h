/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 jazz instruments: original, oscillator-modelled acoustic piano,
 * Rhodes-like tine, breathy flute, vibraphone and soft bell.
 * Not sampled instruments, soundfonts or an imported ROM.
 *
 * Audio-only voice state, five bounded voices (4 poly chord + mono lead),
 * no dynamic allocation, sine() uses HYP's 1024-point LUT.
 * Program commands are 32-bit single-writer UI -> DSP messages; notes
 * arrive within HYP's audio drain() / note_cmd(), including piano rolls.
 * Include after HYP mix buffers, before render_pad().
 */
#ifndef URBAN_JAZZ_ENGINE_H
#define URBAN_JAZZ_ENGINE_H
enum {
    URBAN_JAZZ_OFF=0,URBAN_JAZZ_PIANO=1,URBAN_JAZZ_RHODES=2,
    URBAN_JAZZ_FLUTE=3,URBAN_JAZZ_VIBES=4,URBAN_JAZZ_BELL=5
};
typedef struct {
    float phase,freq,strength,decay,hammer,lp,lfo;
    uint32_t noise,age,order;
    uint8_t key,held,active,program;
} UrbanJazzVoice;
static UrbanJazzVoice urban_jazz_chords[EN_NPAD],urban_jazz_melody;
static volatile uint32_t urban_jazz_requested;
static uint32_t urban_jazz_current,urban_jazz_age;
volatile uint32_t urban_jazz_note_onsets,urban_jazz_rendered_frames;
static uint8_t urban_jazz_chord_mode,urban_jazz_lead_mode;

/* The device and Android host use one identical audio implementation.
 * No engine state may be written from UI/transport except this 32-bit word. */
void urban_jazz_programs(unsigned chord,unsigned lead) {
    if(chord>URBAN_JAZZ_BELL)chord=0u;
    if(lead>URBAN_JAZZ_BELL)lead=0u;
    urban_jazz_requested=(uint32_t)(chord|(lead<<8u));
}
static void urban_jazz_quiet(void) {
    for(unsigned i=0u;i<EN_NPAD;i++)
        urban_jazz_chords[i].active=urban_jazz_chords[i].held=0u;
    urban_jazz_melody.active=urban_jazz_melody.held=0u;
}
static void urban_jazz_apply(void) {
    uint32_t next=urban_jazz_requested;
    if(next==urban_jazz_current)return;
    urban_jazz_quiet(); /* no tails leaked across timbre / style changes */
    urban_jazz_current=next;
    urban_jazz_chord_mode=(uint8_t)(next&255u);
    urban_jazz_lead_mode=(uint8_t)((next>>8u)&255u);
}
static void urban_jazz_start(UrbanJazzVoice *v,int midi,unsigned program,
                             unsigned pan_seed) {
    v->key=(uint8_t)midi;
    v->freq=mtof((float)midi);
    v->program=(uint8_t)program;
    v->active=v->held=1u;
    v->phase=(float)(pan_seed%5u)*0.113f;
    v->strength=0.0f;
    v->decay=1.0f;
    v->hammer=program==URBAN_JAZZ_PIANO?0.10f:0.028f;
    v->lp=0.0f;
    v->lfo=0.0f;
    v->age=0u;
    v->noise=0xa511e9b3u^((uint32_t)midi*0x9e3779b9u)^(pan_seed*0x85ebca6bu);
    v->order=++urban_jazz_age;
    ++urban_jazz_note_onsets; /* this counter runs ONLY inside DSP note_cmd */
}
static void urban_jazz_note_cmd(int voice,int key,int on) {
    urban_jazz_apply();
    if(voice==EN_V_PAD && urban_jazz_chord_mode) {
        for(unsigned i=0u;i<EN_NPAD;i++)
            if(urban_jazz_chords[i].active &&
               urban_jazz_chords[i].key==(uint8_t)key)
                urban_jazz_chords[i].held=0u;
        if(!on)return;
        unsigned best=0u;
        for(unsigned i=0u;i<EN_NPAD;i++)
            if(!urban_jazz_chords[i].active) {best=i;break;}
            else if(urban_jazz_chords[i].order<urban_jazz_chords[best].order)
                best=i;
        int midi=fold_note(key+12*(par[P_PAD_OCT]+par[P_OCT]),36,96);
        urban_jazz_start(&urban_jazz_chords[best],midi,
                         urban_jazz_chord_mode,best);
        /* Key release uses the untransposed key, like HYP pad_play(). */
        urban_jazz_chords[best].key=(uint8_t)key;
    } else if(voice==EN_V_ACID && urban_jazz_lead_mode) {
        if(!on) {
            if(urban_jazz_melody.key==(uint8_t)key)
                urban_jazz_melody.held=0u;
            return;
        }
        int midi=fold_note(key-12+12*(par[P_AC_OCT]+par[P_OCT]),48,96);
        urban_jazz_start(&urban_jazz_melody,midi,urban_jazz_lead_mode,3u);
        urban_jazz_melody.key=(uint8_t)key;
    }
}
static float urban_jazz_sample(UrbanJazzVoice *v) {
    if(!v->active)return 0.0f;
    const float p=v->phase;
    float x=0.0f;
    v->noise=v->noise*1664525u+1013904223u;
    const float white=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
    float attack=0.035f,rate=0.999988f;
    switch(v->program) {
    case URBAN_JAZZ_PIANO:
        /* Hammer transient + mildly stretched/detuned piano partials. */
        attack=0.035f;rate=0.999972f;
        x=0.70f*sine(p)+0.24f*sine(p*2.006f)+0.085f*sine(p*3.02f)
            +v->hammer*white;
        break;
    case URBAN_JAZZ_RHODES: {
        /* Warm tine carrier with a rapidly fading 2:1 FM bell attack. */
        attack=0.022f;rate=0.999984f;
        float idx=0.02f+0.23f*v->decay*v->decay;
        x=0.81f*sine(p+idx*sine(2.0f*p))+0.13f*sine(2.01f*p);
        break;
    }
    case URBAN_JAZZ_FLUTE:
        /* No harsh saw/square wave: near-sine pipe and filtered breath,
         * with natural delayed ~5Hz vibrato and slow tongue-on attack. */
        attack=0.0010f;rate=1.0f;
        v->lp+=0.022f*(white-v->lp);
        x=0.93f*sine(p)+0.15f*sine(2.0f*p)+0.045f*v->lp;
        break;
    case URBAN_JAZZ_VIBES:
        /* Dampened metal bars: inharmonic upper resonances and tremolo. */
        attack=0.019f;rate=0.999987f;
        x=(0.77f*sine(p)+0.20f*sine(3.99f*p)
              +0.055f*sine(9.14f*p))*
            (0.90f+0.10f*sine(v->lfo));
        break;
    case URBAN_JAZZ_BELL:
        attack=0.027f;rate=0.999971f;
        x=0.65f*sine(p)+0.26f*sine(2.01f*p)+
          0.09f*sine(3.91f*p)+0.018f*white*v->decay;
        break;
    default:v->active=0u;return 0.0f;
    }
    v->strength+=(1.0f-v->strength)*attack;
    v->decay*=rate;
    /* Instrument-specific dampening; resonant mallet tails aren't cut
     * like woodwind, but all voices have a bounded release. */
    if(!v->held) v->strength*=v->program==URBAN_JAZZ_FLUTE?
                             0.9993f:0.99988f;
    if(v->strength*v->decay<0.00035f && v->age>512u) {
        v->active=0u;return 0.0f;
    }
    ++v->age;
    float freq=v->freq;
    if(v->program==URBAN_JAZZ_FLUTE && v->age>EN_SR/8u)
        freq*=1.0f+0.0022f*sine(v->lfo);
    v->phase+=freq*(1.0f/EN_SR);
    if(v->phase>=1.0f)v->phase-=1.0f;
    v->lfo+=(v->program==URBAN_JAZZ_VIBES?5.6f:5.1f)*(1.0f/EN_SR);
    if(v->lfo>=1.0f)v->lfo-=1.0f;
    v->hammer*=0.990f;
    return x*v->strength*v->decay;
}
static void urban_jazz_render(uint32_t n) {
    if(!urban_jazz_chord_mode && !urban_jazz_lead_mode)return;
    float cg=pad_vol*0.19f,lg=acid_vol*0.22f;
    /* Preserve per-track faders/mutes/solo through HYP's real mix levels.
     * Apply a fixed ceiling in this engine, never a post-voice crusher. */
    for(uint32_t j=0u;j<n;j++) {
        float l=0.0f,r=0.0f;
        if(urban_jazz_chord_mode)
            for(unsigned i=0u;i<EN_NPAD;i++) {
                float x=urban_jazz_sample(&urban_jazz_chords[i])*cg;
                l+=x*(i&1u?0.79f:1.0f);
                r+=x*(i&1u?1.0f:0.79f);
            }
        if(urban_jazz_lead_mode) {
            float x=urban_jazz_sample(&urban_jazz_melody)*lg;
            l+=x*0.96f;r+=x*1.04f;
        }
        mixl[j]+=l;mixr[j]+=r;
        rsend[j]+=(l+r)*0.16f;
        esend[j]+=(l+r)*0.075f;
        if(l>0.00001f||l< -0.00001f||r>0.00001f||r< -0.00001f)
            ++urban_jazz_rendered_frames;
    }
}
#endif
