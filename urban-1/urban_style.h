/* SPDX-License-Identifier: GPL-3.0-only
 * Genre palette maps to HYP-1's REAL voice names:
 * DRONE: Sub=9, Reese=4, Rumble=8, Pulse=5, Hoover=11.
 * ACID: Pluck=4, Driven=2, Trance=8, Soft=15, Bell=11.
 * PAD: Warm=5, Dark=7, Rhodes=0, Glass=3, Trance=9.
 * Each of the 14 genres has four musically named variants.
 * These are compositional defaults, not claims of perfect genre synthesis.
 */
#ifndef URBAN_STYLE_H
#define URBAN_STYLE_H
#include "urban_song.h"
typedef struct {
    uint8_t bass_sound,lead_sound,pad_sound;
    uint8_t bass_cut,lead_cut,pad_bright;
    uint8_t drive,reverb,echo;
    uint8_t sub_octave;
} UrbanGenreTimbre;
static const UrbanGenreTimbre urban_timbre[URBAN_GENRES]={
 /*           Bass Lead Pad B-C L-C P-B Drv Rev Echo Sub */
 /* Trap     */ { 9, 4, 5, 23,40,37, 36,27,20,1},
 /* Goth trap*/ { 8,15, 7, 18,26,15, 49,68,37,1},
 /* Sad trap */ { 1,11, 0, 20,34,21, 22,63,44,1},
 /* Hip-hop  */ { 1, 4, 0, 48,51,46, 28,29,23,0},
 /* Trip-hop */ { 2, 9, 3, 32,34,24, 18,64,49,0},
 /* Undergr. */ { 1,12, 0, 43,46,34, 33,32,36,0},
 /* Horror   */ { 8, 2, 7, 25,35,18, 50,49,26,1},
 /* Reggaet. */ { 9, 4, 6, 55,52,51, 28,24,20,1},
 /* DnB      */ { 4, 3, 6, 61,62,54, 48,23,20,0},
 /* Dubstep  */ { 4, 2, 7, 40,47,39, 62,42,34,1},
 /* Techno   */ { 5, 0, 6, 64,77,50, 44,23,27,0},
 /* Hardcore */ {11, 2,14, 75,83,76, 78,17,14,0},
 /* Trance   */ { 5, 8, 9, 63,73,69, 31,69,47,0},
 /* Liquid   */ { 1,15, 1, 54,60,48, 24,58,50,0}
};
/* Explicit 56 subgenre instrument/effect assignments using *existing*
 * HYP-1 DSP sounds. Each row is four named substyles from urban_substyle.
 * Sounds: bass HYP DRONE 0..15; lead HYP ACID 0..15;
 * chord HYP PAD 0..15; kits HYP 0..7.
 * Effects are HYP parameters 0..100; no invented sample instruments.
 */
typedef struct {
    uint8_t bass,lead,pad,drum_kit,hat_kit;
    uint8_t drive,reverb,echo,bass_cut,lead_cut;
} UrbanStylePatch;
static const UrbanStylePatch urban_style_patches[URBAN_GENRES][4]={
 /* Trap: 808 Sub / Drill Grit / airy cloud / rolled chip hats */
 { {9,4,5,7,7,35,25,22,24,40},
   {9,2,7,7,6,57,32,17,23,48},
   {9,15,3,7,5,18,72,54,19,28},
   {9,6,12,7,6,39,21,24,29,64} },
 /* Goth trap: Witch House, Industrial, Darkwave, Cloud */
 { {8,15,7,7,5,48,73,52,20,27},
   {3,2,7,6,6,72,56,31,29,49},
   {8,9,11,7,5,36,83,65,19,24},
   {10,15,3,7,5,22,89,63,22,30} },
 /* Sad trap: emo, piano, lo-fi, guitarish pluck */
 { {9,11,0,7,5,22,72,53,24,32},
   {9,4,0,7,5,17,64,40,19,47},
   {1,15,15,4,4,13,45,28,32,28},
   {9,4,2,7,5,16,67,39,23,52} },
 /* Hip-hop: boom bap, jazzy Rhodes, G-Funk, lo-fi */
 { {1,4,0,4,4,26,24,15,52,49},
   {7,12,4,4,4,22,34,30,49,52},
   {5,5,6,4,4,34,33,25,60,57},
   {1,15,0,4,5,18,49,39,45,39} },
 /* Trip-hop: Bristol, downtempo, abstract, dub */
 { {8,15,7,4,4,29,61,54,28,31},
   {1,4,5,4,5,19,68,45,36,35},
   {6,9,11,5,4,40,58,58,30,46},
   {8,15,8,4,5,21,78,75,29,28} },
 /* Underground: dusty, jazz, experimental, break */
 { {2,4,0,4,4,29,29,15,39,45},
   {7,11,0,4,4,22,48,33,50,59},
   {6,9,3,5,4,50,57,54,36,68},
   {2,6,6,4,4,38,23,21,49,61} },
 /* Horrorcore: Memphis, cinematic, industrial, dark */
 { {8,2,7,7,4,51,47,29,25,35},
   {8,13,11,7,5,41,81,65,19,39},
   {3,2,7,6,6,73,55,35,33,52},
   {9,9,7,7,6,62,73,68,20,31} },
 /* Reggaeton: dembow, perreo, romantic, futuro */
 { {9,4,6,3,3,34,19,16,56,52},
   {9,4,6,3,6,45,18,16,61,56},
   {1,11,0,3,3,18,57,33,49,42},
   {6,9,10,3,6,46,47,47,60,62} },
 /* DnB: neurofunk, jump-up, darkstep, techstep */
 { {4,2,6,4,6,76,19,14,61,64},
   {4,6,6,4,4,54,22,20,65,76},
   {8,3,7,4,6,57,37,28,43,51},
   {15,2,6,4,6,67,24,15,66,72} },
 /* Dubstep: UK 140, deep, riddim, brostep */
 { {4,2,7,7,5,62,34,37,46,52},
   {9,7,8,7,5,31,73,64,24,29},
   {11,2,6,6,6,77,29,20,63,68},
   {4,14,14,6,6,88,21,14,72,74} },
 /* Techno: Detroit, hypnotic, dub techno, minimal */
 { {5,0,6,2,2,42,23,26,62,72},
   {0,9,7,0,0,37,42,39,58,64},
   {1,15,8,5,5,27,79,72,48,36},
   {5,1,13,5,5,35,27,21,55,62} },
 /* Hardcore: gabber, industrial, frenchcore, uptempo */
 { {11,2,14,6,6,86,13,12,78,85},
   {13,14,7,6,6,93,23,14,80,89},
   {11,2,14,6,6,95,11,11,86,92},
   {11,13,14,6,6,100,9,9,90,91} },
 /* Trance: progressive, uplifting, psy, tech */
 { {5,8,9,1,1,34,56,51,64,72},
   {12,8,9,1,1,42,83,59,72,79},
   {6,9,10,3,3,49,48,45,76,83},
   {14,10,14,2,2,60,50,37,77,86} },
 /* Liquid: soulful, atmospheric, jazzy, deep */
 { {1,15,0,4,4,23,62,47,55,60},
   {1,15,1,4,5,20,82,75,49,46},
   {7,11,0,4,4,21,65,62,58,53},
   {9,15,5,4,5,19,76,74,39,35} }
};
static UrbanStylePatch urban_style_patch(unsigned genre,unsigned subgenre){
    return urban_style_patches[genre%URBAN_GENRES][subgenre%4u];
}

static const char *const urban_substyle[URBAN_GENRES][4]={
    {"Classic 808","Dark/Drill","Ambient Trap","Rolling Hats"},
    {"Witch House","Industrial","Darkwave","Cloud Trap"},
    {"Emo Trap","Piano Melanch.","Lo-fi Sad","Guitar Ballad"},
    {"Boom Bap","Jazz Rap","G-Funk","Lo-fi Hip-hop"},
    {"Bristol","Downtempo","Abstract","Dub Trip-hop"},
    {"Raw/Dusty","Jazz Underground","Experimental","Breakbeat"},
    {"Memphis","Cinematic","Industrial","Dark Horror"},
    {"Classic Dembow","Perreo","Romantico","Futurista"},
    {"Neurofunk","Jump-up","Darkstep","Techstep"},
    {"UK 140","Deep Dubstep","Riddim","Brostep"},
    {"Detroit","Hypnotic","Dub Techno","Minimal"},
    {"Gabber","Industrial","Frenchcore","Uptempo"},
    {"Progressive","Uplifting","Psytrance","Tech Trance"},
    {"Soulful","Atmospheric","Jazz Liquid","Deep Liquid"}
};
static const char *urban_style_name(unsigned genre,unsigned subgenre){
    return urban_substyle[genre%URBAN_GENRES][subgenre%4u];
}
#endif
