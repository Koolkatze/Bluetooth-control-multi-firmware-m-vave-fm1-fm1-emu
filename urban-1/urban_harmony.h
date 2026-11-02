/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 diatonic harmonic engine. All parts follow the same chord-root
 * degrees while each genre chooses compatible scale, harmonic rhythm
 * and chord color. No extra DSP voices needed for the planning stage.
 */
#ifndef URBAN_HARMONY_H
#define URBAN_HARMONY_H
#include "urban_song.h"
static int urban_theory_abs(int x){return x<0?-x:x;}
static int urban_scale_note(const UrbanSong *s,int degree,int octave) {
    int oct=0,idx=degree;
    while(idx<0){idx+=7;oct--;}
    while(idx>=7){idx-=7;oct++;}
    return 12*octave + (int)(s->key%12u) +
        (int)urban_scale_intervals[s->scale%URBAN_SCALE_COUNT][idx]+12*oct;
}
static int urban_nearest_degree(const UrbanSong *s,int note) {
    int best=0,best_error=10000;
    for(int degree=-7;degree<=13;degree++) {
        int cand=urban_scale_note(s,degree,5);
        int err=urban_theory_abs(cand-note);
        if(err<best_error){best_error=err;best=degree;}
    }
    return best;
}
/* Map a melody or bass note in the chosen scale onto the *current chord*
 * diatonically, rather than chromatically shifting by the root interval.
 * That avoids notes outside the mode on borrowed III/IV/VII chords.
 */
static uint8_t urban_harmonize_note(const UrbanSong *s,uint8_t note,unsigned bar) {
    int degree=urban_nearest_degree(s,note);
    int target=urban_scale_note(s,degree+(int)s->chord_degrees[bar%4u],5);
    if(target<24)target=24;
    if(target>111)target=111;
    return (uint8_t)target;
}
static unsigned urban_build_chord(const UrbanSong *s,unsigned bar,
                                  const uint8_t previous[4],uint8_t out[4]) {
    unsigned color=s->chord_colors[bar%4u];
    static const int degrees[6][4]={
        {0,2,4,0}, /* triad */
        {0,2,4,6}, /* 7th */
        {0,2,6,8}, /* 9th shell / root 3rd 7th 9th */
        {0,3,4,0}, /* sus4 */
        {0,4,0,0}, /* root and fifth */
        {0,3,6,9}  /* quartal, extended scale fourths */
    };
    static const uint8_t counts[6]={3,4,4,3,2,4};
    if(color>=6u)color=0u;
    unsigned n=counts[color];
    unsigned base_degree=s->chord_degrees[bar%4u];
    /* Use octave 4 for chords, keep each voice within its register and
     * minimize total voice motion from the previous bar. */
    for(unsigned i=0;i<n;i++) {
        int note=urban_scale_note(s,(int)base_degree+degrees[color][i],4);
        int target=previous?previous[i]:0;
        int best=note,dist=10000;
        for(int shift=-24;shift<=24;shift+=12) {
            int x=note+shift;
            if(x<46+(int)i*2 || x>85)continue;
            int d=previous?urban_theory_abs(x-target):urban_theory_abs(x-(55+(int)i*5));
            if(d<dist){dist=d;best=x;}
        }
        out[i]=(uint8_t)best;
    }
    for(unsigned i=n;i<4u;i++)out[i]=0u;
    return n;
}
#endif
