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
