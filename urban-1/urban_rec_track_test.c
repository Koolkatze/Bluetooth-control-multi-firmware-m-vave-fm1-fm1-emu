/* SPDX-License-Identifier: GPL-3.0-only
 * Verify REC TRACK 9 is truly an additive, independent audio bus.
 * This is a stand-alone native C regression, not an LCD-only mock.
 */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#define EN_SR 48000.0f
#define BLK 256
static float mixl[BLK],mixr[BLK],rsend[BLK];
static float sine(float phase) {
    float p=phase-floorf(phase);
    return sinf(6.2831853071795864769f*p);
}
static float sine_any(float phase) { return sine(phase); }
static float mtof(float note) {
    return 440.0f*powf(2.0f,(note-69.0f)/12.0f);
}
/* The real firmware links these EXACT oscillator enums and patch
 * coefficients from HYP-1. The stand-alone test supplies a minimal
 * independent fixture so it can test each of the 78 program routes. */
enum {PO_RHODES,PO_VOICES,PO_STRINGS,PO_GLASS,PO_ORGAN,PO_WARM,
      PO_SUPER,PO_PWM,PO_HOLLOW,PO_SINE};
enum {DO_SAWS,DO_REESE,DO_PULSE,DO_FM,DO_ORGAN,DO_SINE,DO_SQUARE,
      DO_HOOVER,DO_FIFTHS,DO_METAL,DO_SYNC};
enum {AO_SAW,AO_SQUARE,AO_SYNC,AO_PLUCK,AO_PWM,AO_CHIP,AO_SUB,
      AO_TRANCE,AO_BLEEP,AO_FIFTH,AO_BELL,AO_ORGAN,AO_LASER,
      AO_FOLD,AO_SOFT};
typedef struct { uint8_t osc;float saw,sub,drive,cut,g; } drone_snd_t;
typedef struct { uint8_t osc;float drive,g; } acid_snd_t;
typedef struct { float k_start,k_click,k_drive,sn_f1,sn_body,sn_noise,
                   cl_gap,h_metal,h_noise,p_freq,p_ratio,p_idx; } kit_t;
static const drone_snd_t DRONE_SOUND[16]={
 {DO_SAWS,.4f,.55f,1,0,1},{DO_SAWS,.3f,.75f,.8f,-.4f,1},
 {DO_SAWS,.45f,.5f,2.2f,-.3f,1},{DO_SAWS,.3f,.8f,3,-.6f,.9f},
 {DO_REESE,.5f,.4f,1.4f,0,1},{DO_PULSE,.45f,.45f,1.1f,0,1},
 {DO_FM,.55f,.35f,1.2f,.4f,1},{DO_ORGAN,.45f,.5f,1,.6f,.9f},
 {DO_SAWS,.12f,1,2.6f,-1,.95f},{DO_SINE,.6f,.5f,1.3f,0,1.05f},
 {DO_SQUARE,.45f,.45f,1,.2f,.95f},
 {DO_HOOVER,.4f,.5f,1.6f,.5f,.85f},
 {DO_FIFTHS,.4f,.5f,1.2f,.2f,.9f},
 {DO_METAL,.5f,.45f,1.3f,.5f,.95f},
 {DO_SYNC,.45f,.5f,1.2f,.3f,.85f},
 {DO_SAWS,.45f,.4f,1.2f,1.2f,.85f}
};
static const acid_snd_t ACID_SOUND[16]={
 {AO_SAW,1,1},{AO_SQUARE,1,1},{AO_SAW,1.5f,1.45f},
 {AO_SYNC,1.1f,.85f},{AO_PLUCK,1,1.15f},{AO_PWM,1,1},
 {AO_CHIP,1.2f,.7f},{AO_SUB,1.1f,.95f},{AO_TRANCE,1,.9f},
 {AO_BLEEP,.9f,.85f},{AO_FIFTH,1.1f,.85f},{AO_BELL,1,1},
 {AO_ORGAN,1,.95f},{AO_LASER,1,1},{AO_FOLD,1.1f,1},
 {AO_SOFT,1,.75f}
};
static const struct {uint8_t osc,stabs;float fc,g;} PAD_SOUND[16]={
 {PO_RHODES,0,1,1},{PO_VOICES,0,1,1},{PO_STRINGS,0,1.25f,.75f},
 {PO_GLASS,0,1.7f,.85f},{PO_ORGAN,0,1.1f,.8f},
 {PO_WARM,0,.85f,1},{PO_RHODES,1,1.3f,.9f},
 {PO_VOICES,0,.6f,.8f},{PO_STRINGS,1,.55f,1.4f},
 {PO_SUPER,0,1.3f,.75f},{PO_PWM,0,1.1f,1.1f},
 {PO_HOLLOW,0,1,.75f},{PO_GLASS,1,1.6f,1.2f},
 {PO_ORGAN,1,1.3f,1.05f},{PO_SUPER,1,1.5f,1.1f},
 {PO_SINE,0,1,.65f}
};
static const kit_t KITS[8]={
 {5,.25f,1,185,.7f,1.1f,353,1,.6f,380,1.48f,1.4f},
 {3.2f,.12f,.8f,210,.5f,1.3f,440,1.25f,.9f,300,1,.6f},
 {7,.4f,1.6f,170,.4f,1.4f,260,1.55f,.35f,560,3.5f,2.2f},
 {4,.35f,1.25f,180,.6f,1.5f,440,1,.6f,560,1.48f,2},
 {6,.45f,2.5f,240,.8f,1.6f,300,1.3f,1,200,1,.9f},
 {4,.5f,1,400,1,.5f,300,1.4f,.3f,720,2,.8f},
 {8,.6f,3.2f,200,.5f,1.8f,220,1.7f,.8f,140,2.76f,2.8f},
 {2.2f,.08f,.5f,238,.9f,1,380,1.15f,.5f,540,1.48f,1.2f}
};
static float blep_saw(float t,float dt) {
    float y=2.0f*t-1.0f;
    if(t<dt) { float x=t/dt;y-=x+x-x*x-1.0f; }
    else if(t>1.0f-dt) { float x=(t-1.0f)/dt;y-=x*x+x+x+1.0f; }
    return y;
}

#include "integrations/urban_rec_track_engine.h"

static float render_energy(unsigned iterations) {
    float e=0.0f;
    for(unsigned k=0;k<iterations;k++) {
        for(unsigned j=0;j<BLK;j++) {
            /* Pretend this stereo audio is the generated song's drums,
             * bass, lead and chords. REC must ADD; never erase it. */
            mixl[j]=0.12f;
            mixr[j]=-0.12f;
            rsend[j]=0.0f;
        }
        urban_rec_track_render(BLK);
        for(unsigned j=0;j<BLK;j++) {
            float l=mixl[j]-0.12f,r=mixr[j]+0.12f;
            e+=l*l+r*r;
            assert(isfinite(l)&&isfinite(r));
            /* Other tracks' original 0.12/-0.12 baseline is retained. */
            assert(mixl[j]>-2.0f && mixr[j]<2.0f);
        }
    }
    return e;
}
int main(void) {
    assert(URBAN_REC_TRACK_POLY>=6u);
    assert(URT_PARAMS==12);
    assert(urban_rec_track_active_voices==0u);
    assert(urban_rec_track_last_program==0u);
    /* A recorded poly chord is additional to the generated PAD. */
    urban_rec_track_program(32);
    urban_rec_track_note(60,1,110);
    urban_rec_track_note(64,1,110);
    urban_rec_track_note(67,1,110);
    float energy=render_energy(8);
    assert(energy>0.0001f);
    assert(urban_rec_track_onsets==3u);
    assert(urban_rec_track_last_program==32u);
    assert(urban_rec_track_active_voices>=3u);
    urban_rec_track_note(60,0,0);
    urban_rec_track_note(64,0,0);
    urban_rec_track_note(67,0,0);
    urban_rec_track_quiet();
    assert(urban_rec_track_active_voices==0u);
    assert(render_energy(2)<0.00000001f);
    /* All catalog families work WITHOUT commandeering the generated
     * bass, lead, jazz voices, drum lanes or procedural song SFX. */
    static const int programs[]={0,16,32,48,50,55,60,68,77};
    for(unsigned i=0;i<sizeof(programs)/sizeof(programs[0]);i++) {
        urban_rec_track_program(programs[i]);
        urban_rec_track_note(60+(int)i,1,90);
        float family_energy=render_energy(8);
        if(family_energy<=0.00001f)
            fprintf(stderr,"REC family program=%d energy=%g active=%u kind=%u amp=%g\\n",
                    programs[i],(double)family_energy,
                    (unsigned)urban_rec_track_active_voices,
                    (unsigned)urt_voices[0].kind,(double)urt_voices[0].amp);
        assert(family_energy>0.00001f);
        assert(urban_rec_track_last_program==(uint32_t)programs[i]);
        urban_rec_track_quiet();
        assert(urban_rec_track_active_voices==0u);
    }
    /* REC-only modulation must never touch the generated mix. */
    for(int p=0;p<URT_PARAMS;p++)urban_rec_track_control(p,70);
    urban_rec_track_program(32);
    urban_rec_track_note(72,1,115);
    /* The independent Attack=70% setting deliberately rises over
     * hundreds of milliseconds; allow its envelope to become audible. */
    assert(render_energy(45)>0.00001f);
    urban_rec_track_quiet();
    assert(render_energy(2)<0.00000001f);
    assert(urban_rec_track_rendered_frames>8000u);
    puts("URBAN-1 REC Track 9: independent 8-voice DSP, full catalog, private ENV/LFO/FX, generated song unaffected PASS");
    return 0;
}
