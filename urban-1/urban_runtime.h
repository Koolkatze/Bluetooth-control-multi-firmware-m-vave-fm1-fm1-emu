/* URBAN-1 playable MIDI-event runtime (host portable).
 * Uses the existing genre/motif generator; still requires a synth/MIDI output HAL.
 * One cached phrase per track per section. Regeneration updates only that track.
 */
#ifndef URBAN_RUNTIME_H
#define URBAN_RUNTIME_H
#include "urban_music.h"
#include "urban_bass.h"
#include "urban_transport.h"
typedef struct {
    UrbanUI ui;
    UrbanTransport transport;
    UrbanVoice voices[URBAN_TRACKS];
    UrbanBassSong bass;
} UrbanRuntime;
typedef struct {
    uint8_t sequence, step;
    UrbanNote event[URBAN_TRACKS];
    uint8_t glide[URBAN_TRACKS];
} UrbanMidiFrame;
static void urban_runtime_refresh_track(UrbanRuntime *rt, unsigned track) {
    if(track>=URBAN_TRACKS) return;
    urban_generate_voice(&rt->ui.song,track,
        rt->ui.song.tracks[track].seq[0].seed,&rt->voices[track]);
    if(track==2u) urban_generate_bass(&rt->ui.song,
        rt->ui.song.tracks[track].seq[0].seed,&rt->bass);
}
static void urban_runtime_init(UrbanRuntime *rt,uint32_t seed) {
    memset(rt,0,sizeof(*rt));
    urban_ui_init(&rt->ui,seed);
    urban_transport_init(&rt->transport,&rt->ui);
    urban_generate_song(&rt->ui.song,seed);
    for(unsigned i=0;i<URBAN_TRACKS;i++) urban_runtime_refresh_track(rt,i);
}
static void urban_runtime_generate(UrbanRuntime *rt) {
    if(rt->ui.page==URBAN_PAGE_TRACKS) {
        unsigned track=rt->ui.selected_track;
        if(rt->ui.song.tracks[track].locked) return;
        urban_ui_generate(&rt->ui);
        urban_runtime_refresh_track(rt,track);
    } else {
        urban_ui_generate(&rt->ui);
        for(unsigned i=0;i<URBAN_TRACKS;i++)
            if(!rt->ui.song.tracks[i].locked) urban_runtime_refresh_track(rt,i);
    }
}
/* Keep the first bar as the recognizable theme; bars 2-4 answer,
 * develop and turn around that theme. Four bars repeat identically in
 * manual LOOP mode and when a hook reappears in Song Mode.
 * No 4x phrase caches and no allocations on the FM-1.
 */
static void urban_runtime_modal_step(const UrbanSong *s,UrbanNote *n,int offset) {
    if(!n->velocity || offset==0)return;
    unsigned tonic=s->key%12u;
    unsigned pc=(n->note+120u-tonic)%12u;
    unsigned degree=7u;
    for(unsigned d=0;d<7u;d++)
        if(urban_scale_intervals[s->scale%4u][d]==pc){degree=d;break;}
    if(degree>=7u)return;
    int next=(int)degree+offset;
    int oct=0;
    while(next<0){next+=7;oct--;}
    while(next>=7){next-=7;oct++;}
    int note=(int)n->note+
      (int)urban_scale_intervals[s->scale%4u][next]-
      (int)urban_scale_intervals[s->scale%4u][degree]+12*oct;
    if(note>=30&&note<=110)n->note=(uint8_t)note;
}
static void urban_runtime_bar_variation(const UrbanSong *s,
   unsigned track,unsigned section,unsigned bar,unsigned step,
   UrbanNote *n,uint8_t *glide) {
    if(!bar || track<2u)return;
    uint8_t genre=s->tracks[track].genre%URBAN_GENRES;
    uint32_t rng=s->tracks[track].seq[section].seed ^
       ((uint32_t)(bar+1u)*0x9e3779b9u) ^
       ((uint32_t)(step+1u)*0x85ebca6bu);
    uint32_t v=urban_rng(&rng);
    unsigned complexity=s->complexity;
    /* bar 2: answer or question, without losing the recognizable downbeat */
    if(bar==1u && track==3u && step==11u &&
       complexity>=2u && !n->velocity)
        n->velocity=(uint8_t)(62u+(v%18u));
    /* bar 3: greater drive in hook/drop but not in a sparse bridge */
    if(bar==2u && (section==URBAN_HOOK||section==URBAN_DROP)) {
        if(track==3u && step==14u && complexity>=1u)
            n->velocity=n->velocity?(uint8_t)(n->velocity+5u):76u;
        if(track==2u && step==12u && complexity>=2u && !n->velocity)
            n->velocity=(uint8_t)(86u+(v%15u));
    }
    /* bar 4: turn-around, tension and genre-specific pick-up */
    if(bar==3u && complexity>=1u && step>=12u) {
        if(track==3u && step==15u && genre>=10u && !n->velocity)
            n->velocity=78u;
        if(track==2u && step==14u && (genre<=2u||genre==9u)) {
            n->velocity=(uint8_t)(91u+(v%17u));
            *glide=genre<=2u || genre==9u;
        }
    }
    /* Variation is diatonic and does not break the shared song harmony.
     * Only chosen phrase accents move by a scale degree. */
    if(n->velocity && track>=3u && (track!=4u) && step%4u!=0u &&
       (v%100u)<complexity*10u) {
        urban_runtime_modal_step(s,n,bar==3u?-1:1);
    }
    if(n->velocity && bar==2u && (section==URBAN_HOOK||section==URBAN_DROP)) {
        unsigned accent=n->velocity+5u;
        n->velocity=(uint8_t)(accent>119u?119u:accent);
    }
}

static void urban_runtime_tick(UrbanRuntime *rt, UrbanMidiFrame *frame) {
    UrbanTransport *t=&rt->transport;
    UrbanSong *s=&rt->ui.song;
    memset(frame,0,sizeof(*frame));
    frame->sequence=s->current;
    frame->step=t->step;
    if(!s->playing) return;
    if(t->tick%URBAN_TICKS_PER_STEP==0u) {
        uint8_t any_solo=0;
        for(unsigned i=0;i<URBAN_TRACKS;i++) any_solo|=s->tracks[i].solo;
        for(unsigned i=0;i<URBAN_TRACKS;i++)
            if(!s->tracks[i].muted && (!any_solo || s->tracks[i].solo))
                frame->event[i]=rt->voices[i].sequence[s->current].steps[t->step];
        if(!s->tracks[2].muted && (!any_solo || s->tracks[2].solo)) {
            UrbanBassStep bass=rt->bass.seq[s->current].steps[t->step];
            frame->event[2].note=bass.note;
            frame->event[2].velocity=bass.velocity;
            frame->event[2].gate=bass.gate;
            frame->glide[2]=bass.glide;
        }
        /* Four REAL bars per phrase. The motif, harmonic motion and
         * section-appropriate variations span 4x16 steps, not a one-bar
         * loop with an A-H label. Manual mode loops the same 4 bars. */
        unsigned bar=urban_phrase_bar(s);
        uint8_t semitones=s->progression[bar];
        for(unsigned i=2u;i<URBAN_TRACKS;i++) {
            UrbanNote *n=&frame->event[i];
            urban_runtime_bar_variation(s,i,s->current,bar,t->step,
                                         n,&frame->glide[i]);
            if(n->velocity && (unsigned)n->note+semitones<=127u)
                n->note=(uint8_t)(n->note+semitones);
        }
    }
    t->tick++;
    if(t->tick%URBAN_TICKS_PER_STEP==0u) {
        t->step++;
        if(t->step==URBAN_STEPS_PER_SEQUENCE) {
            t->step=0;
            urban_ui_on_sequence_end(&rt->ui);
        }
    }
}
#endif
