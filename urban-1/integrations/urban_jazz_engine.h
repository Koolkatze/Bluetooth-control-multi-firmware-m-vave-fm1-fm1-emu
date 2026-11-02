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
    float phase,freq,strength,decay,hammer,lp,lfo,tone_lp;
    uint32_t noise,age,order;
    uint8_t key,held,active,program;
} UrbanJazzVoice;
static UrbanJazzVoice urban_jazz_chords[EN_NPAD],urban_jazz_melody;
static volatile uint32_t urban_jazz_requested;
static uint32_t urban_jazz_current,urban_jazz_age;
enum { URBAN_JAZZ_BASS_OFF=0,URBAN_JAZZ_BASS_UPRIGHT=1,
       URBAN_JAZZ_BASS_FINGER=2 };
typedef struct {
    float phase,phase2,freq,env,pluck,brightness,body,body_low,attack,tone_lp;
    uint32_t noise,age;
    uint8_t note,held,active,program;
} UrbanJazzBass;
static UrbanJazzBass urban_jazz_bass;
uint8_t urban_jazz_bass_mode;
volatile uint32_t urban_jazz_bass_onsets,urban_jazz_bass_rendered_frames;
volatile uint32_t urban_jazz_note_onsets,urban_jazz_rendered_frames;
volatile uint32_t urban_jazz_modulated_frames;
uint8_t urban_jazz_chord_mode,urban_jazz_lead_mode;

/* The device and Android host use one identical audio implementation.
 * No engine state may be written from UI/transport except this 32-bit word. */
void urban_jazz_programs(unsigned chord,unsigned lead,unsigned bass) {
    if(chord>URBAN_JAZZ_BELL)chord=0u;
    if(lead>URBAN_JAZZ_BELL)lead=0u;
    if(bass>URBAN_JAZZ_BASS_FINGER)bass=0u;
    urban_jazz_requested=(uint32_t)(chord|(lead<<8u)|(bass<<16u));
}
static void urban_jazz_quiet(void) {
    for(unsigned i=0u;i<EN_NPAD;i++)
        urban_jazz_chords[i].active=urban_jazz_chords[i].held=0u;
    urban_jazz_melody.active=urban_jazz_melody.held=0u;
    urban_jazz_bass.active=urban_jazz_bass.held=0u;
}
static void urban_jazz_apply(void) {
    uint32_t next=urban_jazz_requested;
    if(next==urban_jazz_current)return;
    urban_jazz_quiet(); /* no tails leaked across timbre / style changes */
    urban_jazz_current=next;
    urban_jazz_chord_mode=(uint8_t)(next&255u);
    urban_jazz_lead_mode=(uint8_t)((next>>8u)&255u);
    urban_jazz_bass_mode=(uint8_t)((next>>16u)&255u);
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
    v->tone_lp=0.0f;
    v->lfo=0.0f;
    v->age=0u;
    v->noise=0xa511e9b3u^((uint32_t)midi*0x9e3779b9u)^(pan_seed*0x85ebca6bu);
    v->order=++urban_jazz_age;
    ++urban_jazz_note_onsets; /* this counter runs ONLY inside DSP note_cmd */
}
static void urban_jazz_note_cmd(int voice,int key,int on) {
    urban_jazz_apply();
    if(voice==EN_V_DRONE && urban_jazz_bass_mode) {
        UrbanJazzBass *b=&urban_jazz_bass;
        if(!on) {
            if(b->note==(uint8_t)key)b->held=0u;
        } else {
            int midi=fold_note(key-24+12*(par[P_BASSOCT]+par[P_OCT]),
                               28,66);
            b->freq=mtof((float)midi);
            b->note=(uint8_t)key;
            b->program=urban_jazz_bass_mode;
            b->held=b->active=1u;
            b->env=1.0f;b->attack=0.0f;b->age=0u;
            b->pluck=1.0f;b->brightness=1.0f;
            b->phase=0.12f;b->phase2=0.0f;
            b->body=b->body_low=b->tone_lp=0.0f;
            b->noise=0xa511e9b3u ^ ((uint32_t)midi*0x9e3779b9u) ^
                     ((uint32_t)urban_jazz_bass_onsets*0x85ebca6bu);
            ++urban_jazz_bass_onsets;
        }
        return;
    }
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
    if(v==&urban_jazz_melody) {
        /* The true melodic engine obeys its lead envelope knobs too. */
        if(v->program!=URBAN_JAZZ_FLUTE)
            rate=1.0f-(1.0f-rate)*
                (1.8f-1.4f*(float)par[P_AC_DEC]*0.01f);
    } else {
        /* Native PAD/ENV controls also shape our acoustic keys, not
         * just the original HYP oscillator that is now bypassed. */
        attack*=1.0f-0.87f*(float)par[P_PE_ATK]*0.01f;
    }
    v->strength+=(1.0f-v->strength)*attack;
    v->decay*=rate;
    /* Instrument-specific dampening; resonant mallet tails aren't cut
     * like woodwind, but all voices have a bounded release. */
    if(!v->held) {
        float release=v==&urban_jazz_melody?
            (0.9991f+0.00068f*(float)par[P_AC_DEC]*0.01f):
            (0.99935f+0.00063f*(float)par[P_PE_REL]*0.01f);
        v->strength*=release;
    }
    if(v->strength*v->decay<0.00035f && v->age>512u) {
        v->active=0u;return 0.0f;
    }
    ++v->age;
    float freq=v->freq;
    if(v->program==URBAN_JAZZ_FLUTE && v->age>EN_SR/8u)
        freq*=1.0f+0.0022f*sine(v->lfo);
    v->phase+=freq*(1.0f/EN_SR);
    if(v->phase>=1.0f)v->phase-=1.0f;
    /* The chord-LFO rate is now the selected HYP rate knob; flute
     * keeps its natural breath vibrato at roughly 5 Hz. */
    float hz=v==&urban_jazz_melody?5.1f:
             (0.25f+0.45f*(float)par[P_PL_RATE]);
    v->lfo+=hz*(1.0f/EN_SR);
    if(v->lfo>=1.0f)v->lfo-=1.0f;
    v->hammer*=0.990f;
    return x*v->strength*v->decay;
}

/* A plucked upright bass is NOT HYP DRONE preset "Warm".
 * Band-limited partial oscillator with quickly damped upper harmonics,
 * finger scrape, soft initial pitch transient and a low wooden-body
 * resonance. No saw/PWM, drone LFO, FM-metal, clipping or bitcrusher.
 * Option 2 is a slightly tighter/fingered bass for Jazz Liquid.
 *
 * All attack/decay lives in the HYP audio thread, note-on inside its
 * sample-accurate note_cmd queue. Note-off damps rather than hard-cuts.
 */
static void urban_jazz_bass_render(uint32_t n) {
    UrbanJazzBass *b=&urban_jazz_bass;
    if(!urban_jazz_bass_mode || !b->active)return;
    const float g=bass_vol*(b->program==URBAN_JAZZ_BASS_FINGER?
                             0.34f:0.42f);
    for(uint32_t j=0u;j<n;j++) {
        float pp=b->phase,bright=b->brightness;
        /* Acoustic strings: multiple non-perfect harmonics, with upper
         * modes vanishing before fundamental, plus fretboard/finger noise.
         * The body suppresses brittle, metallic transients. */
        float raw=0.81f*sine(pp)+
             (b->program==URBAN_JAZZ_BASS_FINGER?0.28f:0.24f)*
                       bright*sine(pp*2.006f)+
             0.12f*bright*bright*sine(pp*3.015f)+
             0.04f*bright*bright*sine(pp*4.027f);
        b->noise=b->noise*1664525u+1013904223u;
        float white=(float)(int32_t)b->noise*(1.0f/2147483648.0f);
        b->body_low+=0.058f*(white-b->body_low);
        b->body+=0.11f*(raw-b->body);
        float scrape=b->body_low*b->pluck*0.045f;
        /* Low wood resonance adds pluck character without another synth
         * oscillator taking over the perceived pitch. */
        float x=(0.79f*raw+0.21f*b->body+scrape);
        /* Real knob-driven filter: native DRONE controls remain
         * functional when the electronic oscillator is replaced
         * by this acoustic string. LFO changes timbre, not pitch. */
        float depth=(float)par[P_DL_DEPTH]*0.01f;
        float cf=0.045f+0.33f*(float)par[P_DR_CUT]*0.01f+
                 0.22f*depth*sine(b->phase2);
        cf=fm_clampf(cf,0.018f,0.86f);
        b->tone_lp+=cf*(x-b->tone_lp);
        x=b->tone_lp;
        b->phase2+=(0.25f+0.45f*(float)par[P_DL_RATE])*(1.0f/EN_SR);
        if(b->phase2>=1.0f)b->phase2-=1.0f;
        b->attack+=(1.0f-b->attack)*
                   (0.029f-0.024f*(float)par[P_DE_ATK]*0.01f);
        float envelope=b->env*b->attack;
        x*=envelope;
        b->brightness*=b->program==URBAN_JAZZ_BASS_FINGER?
                        0.99991f:0.99987f;
        b->pluck*=0.9974f; /* scrape gone in tens of milliseconds */
        float dec=0.99994f+0.00005f*(float)par[P_DE_DEC]*0.01f;
        b->env*=b->program==URBAN_JAZZ_BASS_FINGER?dec*0.999997f:dec;
        if(!b->held)b->env*=0.99981f;
        ++b->age;
        if(b->env<0.00038f && b->age>256u) {
            b->active=0u;break;
        }
        /* Within 0.3% initial string stretch, settling in 25 ms; no
         * electronic portamento even on repeated walking notes. */
        float settle=1.0f+0.0024f*b->pluck;
        b->phase+=b->freq*settle*(1.0f/EN_SR);
        if(b->phase>=1.0f)b->phase-=1.0f;
        float l=x*g*0.94f,r=x*g;
        mixl[j]+=l;
        mixr[j]+=r;
        rsend[j]+=(l+r)*0.095f;
        esend[j]+=(l+r)*0.025f;
        ++urban_jazz_bass_rendered_frames;
        ++urban_jazz_modulated_frames;
    }
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
                UrbanJazzVoice *v=&urban_jazz_chords[i];
                float x=urban_jazz_sample(v);
                /* Brightness and the actual PAD LFO now filter piano,
                 * Rhodes and mallet voices directly in this engine. */
                float cf=0.14f+
                       0.60f*(float)par[P_BRIGHT]*0.01f+
                       0.25f*(float)par[P_PL_DEPTH]*0.01f*sine(v->lfo);
                cf=fm_clampf(cf,0.012f,0.94f);
                v->tone_lp+=cf*(x-v->tone_lp);
                x=v->tone_lp*cg;
                l+=x*(i&1u?0.79f:1.0f);
                r+=x*(i&1u?1.0f:0.79f);
                if(v->active)++urban_jazz_modulated_frames;
            }
        if(urban_jazz_lead_mode) {
            UrbanJazzVoice *v=&urban_jazz_melody;
            float x=urban_jazz_sample(v);
            /* New independent LEAD LFO affects cutoff on flute,
             * vibraphone and bell too; P_AC_ENV adds pluck brightness. */
            float cf=0.035f+
                     0.70f*(float)par[P_AC_CUT]*0.01f+
                     0.065f*(float)par[P_AC_ENV]*0.01f*v->decay+
                     0.095f*urban_lead_lfo_oct;
            cf=fm_clampf(cf,0.009f,0.95f);
            v->tone_lp+=cf*(x-v->tone_lp);
            x=v->tone_lp*lg;
            l+=x*0.96f;r+=x*1.04f;
            if(v->active)++urban_jazz_modulated_frames;
        }
        mixl[j]+=l;mixr[j]+=r;
        rsend[j]+=(l+r)*0.16f;
        esend[j]+=(l+r)*0.075f;
        if(l>0.00001f||l< -0.00001f||r>0.00001f||r< -0.00001f)
            ++urban_jazz_rendered_frames;
    }
}
#endif
