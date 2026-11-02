/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 dedicated musical SFX track (track index 6).
 * No samples or fake instruments: ten explicit procedural SFX models,
 * two substyle-specific choices for every one of 56 named subgenres.
 * Sparse 4-bar transitions, deterministic per-track seed and bar.
 * Scores are musical event plans; the HYP DSP renders the audio.
 */
#ifndef URBAN_SFX_H
#define URBAN_SFX_H
#include "urban_song.h"

enum {
    URBAN_SFX_SCRATCH, URBAN_SFX_TAPE_STOP, URBAN_SFX_REVERSE,
    URBAN_SFX_IMPACT, URBAN_SFX_DUB_SIREN, URBAN_SFX_GLITCH,
    URBAN_SFX_RISER, URBAN_SFX_DOWNLIFTER, URBAN_SFX_VINYL_POP,
    URBAN_SFX_WHOOSH, URBAN_SFX_COUNT
};
static const char *const urban_sfx_names[URBAN_SFX_COUNT] = {
    "Vinyl Scratch","Tape Stop","Reverse","Sub Impact","Dub Siren",
    "Glitch Zap","Noise Riser","Downlifter","Vinyl Pop","Air Whoosh"
};
typedef struct {
    uint8_t primary,secondary,level;
} UrbanSfxStyle;
/* Explicit 56 palettes, ordered exactly like urban_substyle.
 * Hip-hop/underground are intentionally dusty and sparse, NOT chipy.
 * Primary is usually the short signature, secondary is a transition.
 */
static const UrbanSfxStyle urban_sfx_styles[URBAN_GENRES][4] = {
    {{URBAN_SFX_IMPACT,URBAN_SFX_RISER,46},{URBAN_SFX_GLITCH,URBAN_SFX_DOWNLIFTER,40},
     {URBAN_SFX_WHOOSH,URBAN_SFX_REVERSE,34},{URBAN_SFX_GLITCH,URBAN_SFX_RISER,43}},
    {{URBAN_SFX_REVERSE,URBAN_SFX_DOWNLIFTER,39},{URBAN_SFX_GLITCH,URBAN_SFX_IMPACT,42},
     {URBAN_SFX_DUB_SIREN,URBAN_SFX_REVERSE,35},{URBAN_SFX_TAPE_STOP,URBAN_SFX_WHOOSH,28}},
    {{URBAN_SFX_TAPE_STOP,URBAN_SFX_REVERSE,29},{URBAN_SFX_REVERSE,URBAN_SFX_WHOOSH,31},
     {URBAN_SFX_VINYL_POP,URBAN_SFX_TAPE_STOP,22},{URBAN_SFX_WHOOSH,URBAN_SFX_REVERSE,27}},
    {{URBAN_SFX_SCRATCH,URBAN_SFX_VINYL_POP,36},{URBAN_SFX_VINYL_POP,URBAN_SFX_SCRATCH,29},
     {URBAN_SFX_DUB_SIREN,URBAN_SFX_TAPE_STOP,38},{URBAN_SFX_VINYL_POP,URBAN_SFX_TAPE_STOP,23}},
    {{URBAN_SFX_VINYL_POP,URBAN_SFX_REVERSE,35},{URBAN_SFX_TAPE_STOP,URBAN_SFX_WHOOSH,31},
     {URBAN_SFX_REVERSE,URBAN_SFX_GLITCH,35},{URBAN_SFX_DUB_SIREN,URBAN_SFX_DOWNLIFTER,36}},
    {{URBAN_SFX_SCRATCH,URBAN_SFX_VINYL_POP,34},{URBAN_SFX_VINYL_POP,URBAN_SFX_SCRATCH,28},
     {URBAN_SFX_REVERSE,URBAN_SFX_GLITCH,30},{URBAN_SFX_SCRATCH,URBAN_SFX_TAPE_STOP,34}},
    {{URBAN_SFX_DUB_SIREN,URBAN_SFX_REVERSE,34},{URBAN_SFX_REVERSE,URBAN_SFX_IMPACT,37},
     {URBAN_SFX_GLITCH,URBAN_SFX_DOWNLIFTER,42},{URBAN_SFX_WHOOSH,URBAN_SFX_DUB_SIREN,32}},
    {{URBAN_SFX_WHOOSH,URBAN_SFX_RISER,37},{URBAN_SFX_IMPACT,URBAN_SFX_WHOOSH,42},
     {URBAN_SFX_RISER,URBAN_SFX_REVERSE,31},{URBAN_SFX_DUB_SIREN,URBAN_SFX_GLITCH,39}},
    {{URBAN_SFX_RISER,URBAN_SFX_GLITCH,43},{URBAN_SFX_GLITCH,URBAN_SFX_IMPACT,46},
     {URBAN_SFX_DOWNLIFTER,URBAN_SFX_REVERSE,41},{URBAN_SFX_WHOOSH,URBAN_SFX_RISER,37}},
    {{URBAN_SFX_IMPACT,URBAN_SFX_DOWNLIFTER,45},{URBAN_SFX_DOWNLIFTER,URBAN_SFX_WHOOSH,37},
     {URBAN_SFX_GLITCH,URBAN_SFX_IMPACT,49},{URBAN_SFX_RISER,URBAN_SFX_GLITCH,46}},
    {{URBAN_SFX_RISER,URBAN_SFX_DOWNLIFTER,38},{URBAN_SFX_DUB_SIREN,URBAN_SFX_GLITCH,36},
     {URBAN_SFX_DOWNLIFTER,URBAN_SFX_REVERSE,34},{URBAN_SFX_GLITCH,URBAN_SFX_WHOOSH,31}},
    {{URBAN_SFX_IMPACT,URBAN_SFX_DOWNLIFTER,52},{URBAN_SFX_GLITCH,URBAN_SFX_IMPACT,54},
     {URBAN_SFX_DOWNLIFTER,URBAN_SFX_RISER,50},{URBAN_SFX_RISER,URBAN_SFX_IMPACT,53}},
    {{URBAN_SFX_RISER,URBAN_SFX_WHOOSH,41},{URBAN_SFX_WHOOSH,URBAN_SFX_REVERSE,37},
     {URBAN_SFX_REVERSE,URBAN_SFX_RISER,38},{URBAN_SFX_GLITCH,URBAN_SFX_DOWNLIFTER,40}},
    {{URBAN_SFX_WHOOSH,URBAN_SFX_REVERSE,32},{URBAN_SFX_REVERSE,URBAN_SFX_WHOOSH,31},
     {URBAN_SFX_VINYL_POP,URBAN_SFX_SCRATCH,29},{URBAN_SFX_DOWNLIFTER,URBAN_SFX_WHOOSH,30}}
};
typedef struct {
    uint8_t type,level,length;
} UrbanSfxEvent;
static UrbanSfxEvent urban_sfx_event(const UrbanSong *s,
                                     unsigned section,unsigned bar,unsigned step) {
    UrbanSfxEvent ev={0u,0u,0u};
    const UrbanTrack *t=&s->tracks[6];
    const unsigned genre=t->genre%URBAN_GENRES,sub=t->subgenre%4u;
    const UrbanSfxStyle st=urban_sfx_styles[genre][sub];
    uint32_t rng=urban_mix_seed(t->seq[section%URBAN_SEQUENCES].seed ^
        ((uint32_t)(bar/4u)*0x9e3779b9u) ^
        ((uint32_t)(sub+1u)*0x85ebca6bu));
    /* One sparse cue per significant arrangement change or four-bar
     * turn. Not an extra snare/hat: no endless 16th-note SFX loops. */
    uint8_t secondary=0u;
    switch(section%URBAN_SEQUENCES) {
      case URBAN_INTRO:   if(bar%4u!=3u || step!=12u)return ev;secondary=1u;break;
      case URBAN_VERSE_A: if(bar%4u!=1u || step!=12u)return ev;break;
      case URBAN_HOOK:
          if(bar%4u==0u && step==0u)break;
          if(bar%4u!=3u || step!=12u)return ev;
          secondary=1u;
          break;
      case URBAN_VERSE_B: if(bar%4u!=3u || step!=14u)return ev;break;
      case URBAN_BRIDGE:  if(bar%4u!=3u || step!=8u)return ev;secondary=1u;break;
      case URBAN_DROP:    if(bar%4u!=0u || step!=0u)return ev;break;
      case URBAN_BREAK:   if(bar%4u!=0u || step!=0u)return ev;break;
      case URBAN_OUTRO:   if(bar%4u!=0u || step!=0u)return ev;secondary=1u;break;
      default: return ev;
    }
    /* Reduce the occasional cue count at low complexity, never add
     * unpredictable fills. A fixed seed gives the same cue every time. */
    if(s->complexity==0u && (section==URBAN_VERSE_A ||
       section==URBAN_VERSE_B)) return ev;
    if(s->complexity<=1u && section==URBAN_HOOK && secondary)return ev;
    ev.type=secondary?st.secondary:st.primary;
    ev.level=(uint8_t)(st.level + (rng%7u)); /* small take variation */
    ev.length=(uint8_t)(secondary?6u:ev.type==URBAN_SFX_IMPACT?
                    2u:ev.type==URBAN_SFX_VINYL_POP?1u:4u);
    return ev;
}
#endif
