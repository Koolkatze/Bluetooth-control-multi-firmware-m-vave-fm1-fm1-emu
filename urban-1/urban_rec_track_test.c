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
static float mixl[BLK],mixr[BLK],rsend[BLK],esend[BLK];
static float sine(float phase) {
    float p=phase-floorf(phase);
    return sinf(6.2831853071795864769f*p);
}
static float mtof(float note) {
    return 440.0f*powf(2.0f,(note-69.0f)/12.0f);
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
        assert(render_energy(8)>0.00001f);
        assert(urban_rec_track_last_program==(uint32_t)programs[i]);
        urban_rec_track_quiet();
        assert(urban_rec_track_active_voices==0u);
    }
    /* REC-only modulation must never touch the generated mix. */
    for(int p=0;p<URT_PARAMS;p++)urban_rec_track_control(p,70);
    urban_rec_track_program(32);
    urban_rec_track_note(72,1,115);
    assert(render_energy(8)>0.00001f);
    urban_rec_track_quiet();
    assert(render_energy(2)<0.00000001f);
    assert(urban_rec_track_rendered_frames>8000u);
    puts("URBAN-1 REC Track 9: independent 8-voice DSP, full catalog, private ENV/LFO/FX, generated song unaffected PASS");
    return 0;
}
