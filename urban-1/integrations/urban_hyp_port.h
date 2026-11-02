/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 / HYP-1 port. Included after HYP-1 app/ui.c in its unity build.
 * Uses the actual HYP-1 synth API: en_note, en_set, en_pats, en_play, en_bars.
 * Initially audible: drum tracks 0/1, bass 2, lead 3, pad 4.
 * Tracks 5..7 remain song-state only until new DSP voices are integrated.
 * HYP's en_note has no velocity arg; glide on bass uses legato ordering.
 */
#ifndef URBAN_HYP_PORT_H
#define URBAN_HYP_PORT_H
#define URBAN_EMBEDDED 1
#include "urban/urban_runtime.h"
#include "urban/urban_style.h"
#include "urban/urban_instruments.h"
#include "urban/urban_chord_score.h"
#include "urban/urban_mix.h"
#include "urban/urban_fx.h"
#include "urban_editor.h"
#include "urban/urban_drum_sync.h"

typedef struct {
    UrbanRuntime rt;
    uint32_t last_sixteenth, held_buttons, staged_for_bar;
    uint8_t held[3], note[3], left[3];
    uint8_t chord_notes[4], chord_count, chord_left;
    uint8_t previous_chord[4], previous_chord_count;
    uint8_t overridden[URBAN_TRACKS];
    UrbanEditor editor;
    uint8_t bank;
    uint8_t active_mix_section;
    uint8_t ui_drawn;
    uint32_t ui_signature;
} UrbanHypPort;
static UrbanHypPort urban_port;

/* Keep a shadow of the REAL synth values and user overrides. A manual
 * parameter change cannot be reverted by the next automatic section
 * profile or 16th-note mixer update. Elide duplicate en_set commands
 * to protect the ISR command queue on Android.
 */
static void urban_port_set(int id,int value) {
    if(id>=0 && id<P_NPARAMS) {
        UrbanEditor *ed=&urban_port.editor;
        const en_param_t *info=en_param_info(id);
        if(value<info->lo)value=info->lo;
        if(value>info->hi)value=info->hi;
        if(ed->changed[id])value=ed->override[id];
        if(ed->valid[id] && ed->live[id]==value)return;
        ed->valid[id]=1u;
        ed->live[id]=(int16_t)value;
    }
    en_set(id,value);
}

static const char *const urban_genre_name[URBAN_GENRES] = {
    "Trap","Goth trap","Sad trap","Hip-hop","Trip-hop","Underground",
    "Horrorcore","Reggaeton","Drum & bass","Dubstep","Techno",
    "Hard techno","Trance","Liquid jungle"
};
static const char *const urban_track_name[URBAN_TRACKS] = {
    "Drums","Hats","Bass","Lead","Chords","Atmos","Extra 1","Extra 2"
};
/* HYP1 treats played drone keys as key-24, acid keys as key-12. */
static int urban_hyp_key(unsigned channel, uint8_t midi) {
    int n=(int)midi+(channel==EN_V_DRONE?24:channel==EN_V_ACID?12:0);
    return n>127?127:n;
}
static int urban_hyp_voice(unsigned track) {
    if(track==2u) return EN_V_DRONE;
    if(track==3u) return EN_V_ACID;
    if(track==4u) return EN_V_PAD;
    return -1;
}
static void urban_port_release(void) {
    unsigned i;
    for(i=0;i<3;i++) if(urban_port.held[i]) {
        en_note((int)i,urban_hyp_key(i,urban_port.note[i]),0);
        urban_port.held[i]=urban_port.left[i]=0u;
    }
    /* PAD is polyphonic in HYP-1: release EVERY held chord tone, not
     * merely the last played note. Otherwise changing song/stop may
     * leave notes ringing indefinitely. */
    for(i=0;i<urban_port.chord_count;i++)
        en_note(EN_V_PAD,urban_port.chord_notes[i],0);
    urban_port.chord_count=0u;
    urban_port.chord_left=0u;
    urban_port.previous_chord_count=0u;
}
/* Never edit the pattern currently read by the audio ISR. The inactive
 * HYP drum slot acts as the next-bar buffer. The two URBAN drum tracks
 * are merged into HYP's six lanes: kick, snare and closed hat.
 */
/* HYP-1 has six drum voices. URBAN-1's genre masks drive all of them,
 * with a stable four-bar fill cycle, not the previous single kick/snare
 * note. The bank is prepared while inactive, prior to the bar boundary.
 */
static void urban_port_pattern(uint8_t sequence,unsigned bar,uint8_t bank) {
    UrbanHypPort *p=&urban_port;
    uint16_t lanes[EN_NLANE]={0};
    urban_drum_lanes(&p->rt.ui.song,sequence,bar,lanes);
    for(unsigned i=0;i<EN_NLANE;i++)en_pats[bank][i]=lanes[i];
}
static void urban_port_stage_pattern(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    uint8_t bank,section;
    unsigned bar;
    /* P_PATTERN is consumed asynchronously by the HYP audio renderer.
     * Preparing the next bank at step 15 risks missing the next downbeat
     * (or writing across a bank switch) whenever the Android UI stalls.
     * Allow edits early in the bar; defer late edits to the next one. */
    if(s->playing && en_playing && !urban_drum_can_stage(en_bars))
        return;
    if(!s->playing) {
        bank=0u; section=s->current; bar=s->section_bar;
    } else {
        bank=(uint8_t)(en_pat_now==0u?1u:0u);
        section=s->queued;
        bar=section==s->current?(unsigned)s->section_bar+1u:0u;
    }
    urban_port_pattern(section,bar,bank);
    p->bank=bank;
    urban_port_set(P_PATTERN,bank);
    if(s->playing && en_playing)
        p->staged_for_bar=urban_drum_bar_id(en_bars);
}
/* Match HYP-1's actual DSP with genre-specific sound palette.
 * The shared song genre selects groove/effects, but each track's genre
 * selects its *own* bass/lead/chord engine timbre.
 */
/* Full per-genre and per-subgenre instrumentation uses real HYP-1
 * sound indexes. Global style owns the FX bus; individual track style
 * owns its synth/kit timbre. These patches are not audio samples.
 */
static void urban_port_sound_profile(void) {
    UrbanSong *s=&urban_port.rt.ui.song;
    UrbanStylePatch overall=urban_style_patch(s->genre,s->subgenre);
    UrbanStylePatch d=urban_style_patch(s->tracks[0].genre,
                                        s->tracks[0].subgenre);
    UrbanStylePatch h=urban_style_patch(s->tracks[1].genre,
                                        s->tracks[1].subgenre);
    UrbanInstrumentProgram bp=urban_instrument_program(s,2u,s->current);
    UrbanInstrumentProgram lp=urban_instrument_program(s,3u,s->current);
    UrbanInstrumentProgram pp=urban_instrument_program(s,4u,s->current);
    UrbanInstrumentProgram atmosphere=urban_instrument_program(s,5u,s->current);
    UrbanStylePatch bass=bp.patch,lead=lp.patch,pad=pp.patch;
    /* Match HYP's mode controls where an equivalent exists. HYP does
     * not expose harmonic/melodic minor, Phrygian dominant or Locrian:
     * those are played correctly by our explicit MIDI chord/note score. */
    static const uint8_t hyp_modes[URBAN_SCALE_COUNT]={
        MODE_AEOLIAN,MODE_PHRYGIAN,MODE_DORIAN,MODE_AEOLIAN,
        MODE_IONIAN,MODE_MIXOLYDIAN,MODE_PHRYGIAN,
        MODE_LYDIAN,MODE_AEOLIAN,MODE_PHRYGIAN
    };
    static const uint8_t hyp_chords[6]={
        EN_CH_TRIAD,EN_CH_7TH,EN_CH_9TH,
        EN_CH_SUS,EN_CH_TRIAD,EN_CH_4THS
    };
    urban_port_set(P_ROOT,s->key%12u);
    urban_port_set(P_MODE,hyp_modes[s->scale%URBAN_SCALE_COUNT]);
    urban_port_set(P_CHORD,hyp_chords[s->chord_colors[0]%6u]);
    urban_port_set(P_VOICING,s->chord_colors[0]==URBAN_CH_QUARTAL?1:0);
    /* A subtle seeded variation of synthesis envelopes distinguishes
     * successive takes without changing the selected subgenre patch. */
    uint32_t human=urban_mix_seed(s->seed ^ 0x592fd9e1u);
    int brightness=urban_timbre[s->tracks[4].genre%URBAN_GENRES].pad_bright;
    brightness+=(int)(human%11u)-5;
    if(brightness<0)brightness=0;
    if(brightness>100)brightness=100;
    urban_port_set(P_KIT_KICK,d.drum_kit);
    urban_port_set(P_KIT_SNARE,d.drum_kit);
    urban_port_set(P_KIT_HAT,h.hat_kit);
    urban_port_set(P_KIT_PERC,h.drum_kit);
    /* Hip-hop's laid-back feel requires timing, not just a mask.
     * HYP swing delays ONLY off-sixteenth drum triggers by up to 0.5
     * steps; keep the 2/4 snare fixed and pad/bass on the harmonic grid.
     * Style.swing is 7..33 = roughly 9..42 ms at 90 BPM, not extra hits. */
    unsigned drum_genre=s->tracks[0].genre%URBAN_GENRES;
    UrbanGroove swing_groove=urban_groove((uint8_t)drum_genre,
                                         s->tracks[0].subgenre);
    urban_port_set(P_SWING,(drum_genre==3u||drum_genre==5u)?swing_groove.swing:0);
    /* 56 named FX palettes: HYP has SHARED wet echo/reverb but
     * independent lead/filter envelopes, bass/pad LFO and 6 drum decays. */
    UrbanFxPlan fx=urban_fx_plan(s);
    urban_port_set(P_DRIVE,fx.drive);
    urban_port_set(P_REVERB,fx.reverb);
    urban_port_set(P_ECHO,fx.echo);
    urban_port_set(P_ETIME,fx.echo_time);
    urban_port_set(P_SIZE,fx.reverb_size);
    urban_port_set(P_PL_RATE,fx.pad_lfo_rate);
    urban_port_set(P_PL_DEPTH,fx.pad_lfo_depth);
    urban_port_set(P_DETUNE,fx.pad_detune);
    urban_port_set(P_DL_RATE,fx.bass_lfo_rate);
    urban_port_set(P_DL_DEPTH,fx.bass_lfo_depth);
    urban_port_set(P_LANE_DEC(0),fx.kick_decay);
    urban_port_set(P_LANE_DEC(1),fx.snare_decay);
    urban_port_set(P_LANE_DEC(2),fx.snare_decay);
    urban_port_set(P_LANE_DEC(3),fx.hat_decay);
    urban_port_set(P_LANE_DEC(4),fx.hat_decay);
    urban_port_set(P_LANE_DEC(5),fx.perc_decay);
    urban_port_set(P_NZ_COLOR,atmosphere.noise_color);
    urban_port_set(P_NZ_TONE,fx.noise_tone);
    urban_port_set(P_NZ_PUMP,fx.noise_pump);
    UrbanMixPlan initial=urban_mix_plan(s,s->current,urban_phrase_bar(s));
    urban_port_set(P_LANE_LVL(0),initial.kick);
    urban_port_set(P_LANE_LVL(1),initial.snare);
    urban_port_set(P_LANE_LVL(2),initial.clap);
    urban_port_set(P_LANE_LVL(3),initial.hat);
    urban_port_set(P_LANE_LVL(4),initial.open);
    urban_port_set(P_LANE_LVL(5),initial.perc);
    urban_port_set(P_DR_INST,bass.bass);
    urban_port_set(P_DR_CUT,bass.bass_cut);
    urban_port_set(P_DR_RES,fx.bass_resonance);
    urban_port_set(P_BASSOCT,bass.bass==9u||bass.bass==8u?-1:0);
    urban_port_set(P_AC_WAVE,lead.lead);
    urban_port_set(P_AC_CUT,lead.lead_cut);
    urban_port_set(P_AC_RES,fx.lead_resonance);
    urban_port_set(P_AC_ENV,fx.lead_env);
    urban_port_set(P_AC_DEC,fx.lead_decay);
    urban_port_set(P_AC_ACC,fx.lead_accent);
    urban_port_set(P_AC_SLIDE,fx.lead_slide);
    urban_port_set(P_PADTONE,pad.pad);
    urban_port_set(P_BRIGHT,brightness);
    urban_port_set(P_PE_ATK,fx.pad_attack);
    urban_port_set(P_PE_REL,fx.pad_release);
    /* Noise fader belongs to the same headroom-controlled mix as the
     * other layers, not a separately unbounded atmospheric constant. */
}
/* Song sections are actually orchestrated: the intro/bridge/break strip
 * layers, a hook raises melodic energy, and the drop restores full drums.
 * Repeated hooks keep identical motifs and balanced synthesis settings.
 */
static void urban_port_mix_levels(void) {
    const UrbanSong *s=&urban_port.rt.ui.song;
    UrbanMixPlan m=urban_mix_plan(s,s->current,urban_phrase_bar(s));
    urban_port_set(P_PULSE,m.drums);
    urban_port_set(P_BASS,m.bass);
    urban_port_set(P_ACID,m.lead);
    urban_port_set(P_PAD,m.chords);
    urban_port_set(P_NOISE,m.noise);
}
static void urban_port_section_mix(void) {
    /* Programme changes at SECTION boundaries; the lead volume may also
     * be muted for intentionally silent BARs, updated in port_step. */
    urban_port_sound_profile();
    urban_port_mix_levels();
}
static void urban_port_generate(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    uint8_t song_generate=(uint8_t)(p->rt.ui.page==URBAN_PAGE_SONG);
    if(song_generate && s->playing)en_play(0);
    /* A different take for every SAVE press, including fast repeated taps
     * in the Android emulator. UI serial is always distinct; plat_ms and
     * current engine clock additionally randomize timing-dependent gestures.
     * No 24-ms hardware ADC RNG read is needed while audio plays. */
    uint32_t entropy=urban_mix_seed((uint32_t)plat_ms() ^
          (en_bars*0x9e3779b9u) ^ p->rt.ui.generation_serial);
    urban_runtime_generate_entropy(&p->rt,entropy);
    if(song_generate) {
        urban_port_release();
        p->rt.transport.step=0;
        p->rt.transport.tick=0;
        p->staged_for_bar=UINT32_MAX;
        p->last_sixteenth=en_bars;
    }
    urban_port_sound_profile();
    urban_port_section_mix();
    urban_port_stage_pattern();
    if(song_generate && s->playing)en_play(1);
}
static void urban_port_note(unsigned track,UrbanNote n,uint8_t glide) {
    int voice=urban_hyp_voice(track);
    UrbanHypPort *p=&urban_port;
    uint8_t old_note;
    if(voice<0 || !n.velocity) return;
    old_note=p->note[voice];
    /* For bass glide, note-on precedes old note-off. HYP1's drone glides
     * when another held key remains; this does not implement slide speed. */
    if(p->held[voice] && glide && old_note!=n.note) {
        en_note(voice,urban_hyp_key((unsigned)voice,n.note),1);
        en_note(voice,urban_hyp_key((unsigned)voice,old_note),0);
    } else {
        if(p->held[voice])
            en_note(voice,urban_hyp_key((unsigned)voice,old_note),0);
        en_note(voice,urban_hyp_key((unsigned)voice,n.note),1);
    }
    p->held[voice]=1u;
    p->note[voice]=n.note;
    p->left[voice]=n.gate?n.gate:1u;
}
/* Actual polyphonic score: HYP-1 exposes four independent PAD voices.
 * Send diatonic triads, sevenths, ninth shells, suspended and quartal
 * voicings on each harmony change. Inversions choose the nearest register
 * to the last chord; the Acid melody remains available independently.
 */
static void urban_port_chord_off(void) {
    UrbanHypPort *p=&urban_port;
    for(unsigned i=0;i<p->chord_count;i++)
        en_note(EN_V_PAD,p->chord_notes[i],0);
    p->chord_count=0u;
    p->chord_left=0u;
}
static void urban_port_chord(unsigned color,unsigned gate) {
    UrbanHypPort *p=&urban_port;
    const UrbanSong *s=&p->rt.ui.song;
    uint8_t next[4]={0};
    /* Retain the LAST voiced chord even when its gate has been released.
     * Voice-leading across rhythmic gaps requires that memory. */
    const uint8_t *prev=p->previous_chord_count?p->previous_chord:0;
    urban_port_chord_off();
    uint8_t solo=0u;
    for(unsigned i=0;i<URBAN_TRACKS;i++)solo|=s->tracks[i].solo;
    if(s->tracks[4].muted || (solo && !s->tracks[4].solo))
        return;
    unsigned count=urban_build_chord_colored(s,urban_phrase_bar(s),
                            color,prev,next);
    if(count>EN_NPAD)count=EN_NPAD;
    for(unsigned i=0;i<count;i++) {
        if(!next[i])continue;
        en_note(EN_V_PAD,next[i],1);
        p->chord_notes[p->chord_count++]=next[i];
        p->previous_chord[i]=next[i];
    }
    p->previous_chord_count=p->chord_count;
    p->chord_left=(uint8_t)(gate?gate:1u);
}
static void urban_port_advance_score(void) {
    UrbanHypPort *p=&urban_port;
    UrbanMidiFrame unused;
    for(unsigned i=0;i<URBAN_TICKS_PER_STEP;i++)
        urban_runtime_tick(&p->rt,&unused);
}
static void urban_port_step(void) {
    UrbanHypPort *p=&urban_port;
    UrbanMidiFrame frame;
    unsigned i,t;
    unsigned cstep=p->rt.transport.step;
    /* HYP's drum tick has already advanced its 16th counter by one.
     * Update the chord/voice mix on THIS bar's first 16th, not on
     * the preceding bar's last 16th (the prior code changed section
     * timbre a beat early and sounded out of tempo). */
    if(cstep==0u) {
        if(p->active_mix_section!=p->rt.ui.song.current) {
            urban_port_section_mix();
            p->active_mix_section=p->rt.ui.song.current;
        } else urban_port_mix_levels();
        urban_port_chord_off();
    }
    if(p->chord_left && --p->chord_left==0u)
        urban_port_chord_off();
    UrbanChordScore cs=urban_chord_score(&p->rt.ui.song,
                           p->rt.ui.song.current,
                           urban_phrase_bar(&p->rt.ui.song));
    if((cs.onsets>>cstep)&1u)
        urban_port_chord(cs.color,cs.gate);
    for(i=0;i<3;i++) if(p->held[i] && p->left[i]) {
        if(--p->left[i]==0u) {
            en_note((int)i,urban_hyp_key(i,p->note[i]),0);
            p->held[i]=0u;
        }
    }
    urban_runtime_tick(&p->rt,&frame);
    for(t=1;t<URBAN_TICKS_PER_STEP;t++) {
        UrbanMidiFrame unused;
        urban_runtime_tick(&p->rt,&unused);
    }
    /* Prepare NEXT bar as early as the first processed 16th, allowing
     * up to 13 steps of latency margin. If UI missed that downbeat,
     * still stage within the safe window instead of replaying old bars. */
    if(urban_drum_should_stage(en_bars,p->staged_for_bar))
        urban_port_stage_pattern();
    for(i=2;i<=3;i++)
        urban_port_note(i,frame.event[i],frame.glide[i]);
}
static void urban_port_poll(void) {
    UrbanHypPort *p=&urban_port;
    uint32_t now=en_bars,steps;
    if(!p->rt.ui.song.playing) {
        urban_port_release();
        p->last_sixteenth=now;
        return;
    }
    /* Audio callback is the clock authority. After a slow UI frame,
     * NEVER fire 2..32 lead notes simultaneously: that causes the
     * out-of-tempo bursts described by the user. Fast-forward the
     * score without emitting old MIDI notes, then play only the latest
     * 16th. This keeps the lead on the same drum sixteenth. */
    if(now<p->last_sixteenth) {
        /* HYP play_start() reset its counter, unlike URBAN's previous
         * snapshot. Wrap/reset is NOT thousands of missing steps. */
        urban_port_release();
        p->last_sixteenth=now>0u?now-1u:now;
        p->staged_for_bar=UINT32_MAX;
        p->active_mix_section=255u;
    }
    steps=now-p->last_sixteenth;
    if(steps>1u) {
        unsigned missed=steps-1u;
        urban_port_release();
        if(missed>512u) {
            /* Bounded recovery: no giant MIDI burst, preserve current
             * song/loop mode; await the following fresh 16th tick. */
            p->last_sixteenth=now-1u;
        } else {
            for(unsigned skip=0;skip<missed;skip++)
                urban_port_advance_score();
            p->last_sixteenth+=missed;
        }
        p->active_mix_section=255u;
    }
    if(p->last_sixteenth!=now) {
        urban_port_step();
        p->last_sixteenth=now;
    }
}
static void urban_port_play(void) {
    UrbanHypPort *p=&urban_port;
    p->rt.ui.song.playing=(uint8_t)!p->rt.ui.song.playing;
    if(!p->rt.ui.song.playing) {
        urban_port_release();
        en_play(0);
    } else {
        /* HYP play_start(1) resets its DSP sixteenth to zero. A stale
         * URBAN transport step causes audible phase shifts on resume. */
        urban_runtime_reset_clock(&p->rt);
        p->staged_for_bar=UINT32_MAX;
        p->last_sixteenth=en_bars;
        urban_port_stage_pattern();
        urban_port_section_mix();
        en_play(1);
    }
}
static void urban_port_queue(int n) {
    urban_ui_next_knob(&urban_port.rt.ui,n);
    urban_port_stage_pattern();
}
static void urban_port_set_mode(int automatic) {
    UrbanHypPort *p=&urban_port;
    uint8_t was=p->rt.ui.song.playing;
    if(was)en_play(0);
    urban_port_release();
    urban_ui_song_mode(&p->rt.ui,automatic);
    p->rt.transport.tick=0;
    p->rt.transport.step=0;
    p->staged_for_bar=UINT32_MAX;
    p->last_sixteenth=en_bars;
    p->active_mix_section=255u;
    urban_port_stage_pattern();
    urban_port_section_mix();
    if(was)en_play(1);
}
static void urban_port_restart(void) {
    UrbanHypPort *p=&urban_port;
    if(!p->rt.ui.song.song_mode)return;
    urban_port_set_mode(1);
}

static void urban_port_init(uint32_t seed) {
    unsigned i;
    UrbanHypPort *p=&urban_port;
    memset(p,0,sizeof(*p));
    p->active_mix_section=255u;
    p->staged_for_bar=UINT32_MAX;
    urban_runtime_init(&p->rt,seed);
    // HYP-1 ui_init() starts its engine by default. Stop it explicitly so
    // URBAN-1's STOPPED display agrees with the actual audio transport.
    en_play(0);
    /* First playable prototype: deliberately suppress HYP's auto voices
     * and drift; URBAN supplies notes and drum patterns instead. */
    urban_port_set(P_PAD_MAN,1);
    urban_port_set(P_DR_MAN,1);
    urban_port_set(P_AC_MAN,1);
    urban_port_set(P_DD_CHANGE,0);
    urban_port_set(P_DD_MUTATE,0);
    urban_port_set(P_MOTION,0);
    urban_port_set(P_AC_MUT,0);
    urban_port_set(P_OCT,0);
    urban_port_set(P_PAD_OCT,0);
    urban_port_set(P_BASSOCT,0);
    urban_port_set(P_AC_OCT,0);
    urban_port_set(P_TEMPO,p->rt.ui.song.tempo);
    urban_port_set(P_ROOT,p->rt.ui.song.key);
    urban_port_set(P_MODE,MODE_AEOLIAN);
    urban_port_set(P_PULSE,80);
    urban_port_set(P_BASS,78);
    urban_port_set(P_ACID,75);
    urban_port_set(P_PAD,70);
    urban_port_set(P_NOISE,0);
    urban_port_set(P_KIT_KICK,7);  /* HYP-1 808 kit */
    urban_port_set(P_KIT_SNARE,7);
    urban_port_set(P_KIT_HAT,7);
    for(i=0;i<URBAN_TRACKS;i++) p->rt.ui.song.tracks[i].genre=0;
    for(i=0;i<URBAN_TRACKS;i++) urban_runtime_refresh_track(&p->rt,i);
    urban_song_set_mode(&p->rt.ui.song,1); /* SONG MODE by default */
    urban_port_sound_profile();
    urban_port_section_mix();
    urban_port_stage_pattern();
    p->last_sixteenth=en_bars;
}
/* The four right-hand panel knobs must be sufficient on Android:
 * KNOB3 selects song subgenre (previously used exclusively for key).
 * The left SELECT encoder changes the tonal key as an optional control.
 * ALGORITHM is kept as a second substyle encoder for physical FM-1.
 */
/* Rebuild all unlocked musical scores when the global genre/subgenre
 * changes. Previously KNOB3 updated only the LCD/timbres, leaving old
 * bass notes and old chord roots in memory until SAVE was pressed.
 * Preserve the selected tempo, key, manual LOOP position, locked
 * tracks and four-bar arrangement instead of generating a new song. */
static void urban_port_restyle_song(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    urban_song_harmony(s);
    for(unsigned i=0;i<URBAN_TRACKS;i++)
        if(!s->tracks[i].locked)
            urban_runtime_refresh_track(&p->rt,i);
    urban_port_release();
    urban_port_sound_profile();
    urban_port_mix_levels();
    urban_port_stage_pattern();
}
static void urban_port_song_subgenre(int delta) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    int v=((int)s->subgenre+delta)%4;
    if(v<0)v+=4;
    s->subgenre=(uint8_t)v;
    p->rt.ui.subgenre=(uint8_t)v;
    for(unsigned i=0;i<URBAN_TRACKS;i++)
        if(!p->overridden[i] && !s->tracks[i].locked)
            s->tracks[i].subgenre=(uint8_t)v;
    urban_port_restyle_song();
}
static void urban_port_song_key(int delta) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    int v=((int)s->key+delta)%12;
    if(v<0)v+=12;
    s->key=(uint8_t)v;
    urban_port_set(P_ROOT,v);
    for(unsigned i=0;i<URBAN_TRACKS;i++)
        if(!s->tracks[i].locked)
            urban_runtime_refresh_track(&p->rt,i);
}
/* View 0 is the existing transport; FX and EDIT are overlays, not
 * extra song modes. ALGORITHM selects MASTER / track, PRESET and SELECT
 * select groups of four controls and K1..K4 change parameter values.
 */
static int urban_port_editor_value(int id) {
    const UrbanSong *s=&urban_port.rt.ui.song;
    const UrbanEditor *ed=&urban_port.editor;
    if(id==UC_TEMPO)return s->tempo;
    if(id==UC_KEY)return s->key;
    if(id==UC_SCALE)return s->scale;
    if(id==UC_COMPLEX)return s->complexity;
    int bit=urban_editor_special_bit(id);
    if(bit>=0)return (int)((ed->performance_mask>>(unsigned)bit)&1u);
    if(id>=0 && id<P_NPARAMS)
        return ed->valid[id]?ed->live[id]:en_param_info(id)->def;
    return 0;
}
static void urban_port_editor_open(unsigned view) {
    UrbanEditor *e=&urban_port.editor;
    if(e->view==view)e->view=0u;
    else {
        e->view=(uint8_t)view;
        e->target=0u;
        e->bank=0u;
    }
    urban_port.ui_drawn=0u;
}
static void urban_port_editor_bank_delta(int delta) {
    UrbanEditor *e=&urban_port.editor;
    unsigned count=0u;
    urban_editor_banks(e->view,e->target,&count);
    e->bank=(uint8_t)urban_editor_page_next(e->bank,delta,count);
}
static void urban_port_editor_target_delta(int delta) {
    UrbanEditor *e=&urban_port.editor;
    e->target=(uint8_t)urban_editor_page_next(e->target,delta,
                                               URBAN_EDITOR_PAGE_COUNT);
    e->bank=0u;
}
static void urban_port_editor_refresh_notes(void) {
    UrbanHypPort *p=&urban_port;
    urban_port_release();
    for(unsigned t=0;t<URBAN_TRACKS;t++)
        if(!p->rt.ui.song.tracks[t].locked)
            urban_runtime_refresh_track(&p->rt,t);
    urban_port_sound_profile();
    urban_port_mix_levels();
    urban_port_stage_pattern();
}
static void urban_port_editor_write(int id,int delta) {
    UrbanEditor *ed=&urban_port.editor;
    UrbanSong *s=&urban_port.rt.ui.song;
    if(id==URBAN_EDITOR_NO_PARAM || !delta)return;
    int old=urban_port_editor_value(id),lo=0,hi=100;
    if(id>=0 && id<P_NPARAMS) {
        const en_param_t *info=en_param_info(id);
        lo=info->lo;hi=info->hi;
        /* The FM-1 is a fixed-gain summing synth. Avoid clipping when
         * the user turns up several faders during a dense drop. */
        if(id==P_PAD || id==P_ACID)hi=65;
        else if(id==P_BASS)hi=80;
        else if(id==P_PULSE)hi=85;
        else if(id==P_NOISE)hi=25;
    } else if(id==UC_TEMPO){lo=70;hi=190;}
    else if(id==UC_KEY){hi=11;}
    else if(id==UC_SCALE){hi=URBAN_SCALE_COUNT-1;}
    else if(id==UC_COMPLEX){hi=3;}
    else if(urban_editor_special_bit(id)>=0){hi=1;}
    int value=old+delta;
    if(value<lo)value=lo;
    if(value>hi)value=hi;
    if(value==old)return;
    if(id>=0 && id<P_NPARAMS) {
        ed->changed[id]=1u;
        ed->override[id]=(int16_t)value;
        urban_port_set(id,value);
    } else if(id==UC_TEMPO) {
        s->tempo=(uint8_t)value;
        urban_port_set(P_TEMPO,value);
    } else if(id==UC_KEY) {
        s->key=(uint8_t)value;
        urban_port_editor_refresh_notes();
    } else if(id==UC_SCALE) {
        s->scale=(uint8_t)value;
        for(unsigned bar=0;bar<4u;bar++)
            s->progression[bar]=urban_scale_intervals[s->scale]
                                                    [s->chord_degrees[bar]];
        urban_port_editor_refresh_notes();
    } else if(id==UC_COMPLEX) {
        s->complexity=(uint8_t)value;
        urban_port_editor_refresh_notes();
    } else {
        int bit=urban_editor_special_bit(id);
        if(bit>=0) {
            uint32_t b=1u<<(unsigned)bit;
            if(value)ed->performance_mask|=b;
            else ed->performance_mask&=~b;
            en_fx(ed->performance_mask);
        }
    }
    urban_port.ui_drawn=0u;
}
static void urban_port_editor_input(void) {
    UrbanEditor *ed=&urban_port.editor;
    int d=plat_enc(EN_ALGO);
    if(d)urban_port_editor_target_delta(d);
    d=plat_enc(EN_PRESET);
    if(d)urban_port_editor_bank_delta(d);
    d=plat_enc(EN_SELECT);
    if(d)urban_port_editor_bank_delta(d);
    unsigned count=0u;
    const UrbanEditorBank *bank=urban_editor_bank(ed,&count);
    const int enc[4]={EN_K1,EN_K2,EN_K3,EN_K4};
    for(unsigned i=0;i<4u;i++) {
        d=plat_enc(enc[i]);
        if(d && bank)urban_port_editor_write(bank->param[i],d);
    }
    urban_port_poll();
}
static void urban_port_input(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    uint32_t b=plat_buttons(),down=b&~p->held_buttons;
    int32_t d;
    p->held_buttons=b;
    if(down&(1u<<B_HOME)) {
        p->editor.view=0u;
        urban_ui_page(&p->rt.ui,URBAN_PAGE_SONG);
    }
    if(down&(1u<<B_SEQ)) {
        p->editor.view=0u;
        urban_ui_page(&p->rt.ui,URBAN_PAGE_TRACKS);
    }
    if(down&(1u<<B_FX))urban_port_editor_open(URBAN_EDITOR_FX);
    if(down&(1u<<B_EDIT))urban_port_editor_open(URBAN_EDITOR_EDIT);
    if(down&(1u<<B_PLAY))urban_port_play();
    if(p->editor.view) {
        /* SAVE never silently generates a new song while browsing FX. */
        urban_port_editor_input();
        return;
    }
    if(down&(1u<<B_SAVE))urban_port_generate();
    if(p->rt.ui.page==URBAN_PAGE_SONG) {
        if(down&(1u<<B_ARP))urban_port_set_mode(!s->song_mode);
        if(down&(1u<<B_REC))urban_port_restart();
    } else {
        UrbanTrack *t=&s->tracks[p->rt.ui.selected_track];
        /* FX / EDIT now ALWAYS open their dedicated editor pages.
         * Existing track Lock and Mute move to ENV and LFO;
         * GLO remains solo, so no sequencing capability is lost. */
        if(down&(1u<<B_ENV))t->locked=(uint8_t)!t->locked;
        if(down&(1u<<B_LFO))t->muted=(uint8_t)!t->muted;
        if(down&(1u<<B_GLO))t->solo=(uint8_t)!t->solo;
        if(down&((1u<<B_LFO)|(1u<<B_GLO))) {
            urban_port_stage_pattern();
            urban_port_mix_levels();
        }
    }
    d=plat_enc(EN_K1);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            int v=(int)s->tempo+(int)d;
            if(v<70)v=70;
            if(v>190)v=190;
            s->tempo=(uint8_t)v;
            urban_port_set(P_TEMPO,v);
        } else {
            int v=(int)p->rt.ui.selected_track+(int)d;
            v=(v%URBAN_TRACKS+URBAN_TRACKS)%URBAN_TRACKS;
            urban_ui_select_track(&p->rt.ui,(unsigned)v);
        }
    }
    d=plat_enc(EN_K2);
    if(d) {
        int v=(int)(p->rt.ui.page==URBAN_PAGE_SONG?p->rt.ui.genre:
                   s->tracks[p->rt.ui.selected_track].genre)+(int)d;
        v=(v%URBAN_GENRES+URBAN_GENRES)%URBAN_GENRES;
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            p->rt.ui.genre=(uint8_t)v;
            urban_song_set_genre(s,(uint8_t)v);
            s->tempo=urban_genre_bpm[v];
            urban_port_set(P_TEMPO,s->tempo);
            for(unsigned i=0;i<URBAN_TRACKS;i++)
                if(!p->overridden[i] && !s->tracks[i].locked)
                    s->tracks[i].genre=(uint8_t)v;
            /* Selecting a different genre now changes the ACTUAL
             * chord degrees, scale, bass/drum notes and synth patches,
             * while SAVE still creates a completely new take. */
            urban_port_restyle_song();
        } else {
            urban_ui_set_genre(&p->rt.ui,(uint8_t)v,
                              s->tracks[p->rt.ui.selected_track].subgenre);
            p->overridden[p->rt.ui.selected_track]=1u;
            if(!s->tracks[p->rt.ui.selected_track].locked)
                urban_runtime_refresh_track(&p->rt,p->rt.ui.selected_track);
            urban_port_release();
            urban_port_sound_profile();
            urban_port_mix_levels();
            urban_port_stage_pattern();
        }
    }
    /* Both KNOB3 and ALGORITHM can choose subgenre. The Android
     * instrument displays KNOB3 prominently, so this path is essential. */
    d=plat_enc(EN_ALGO);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG)
        urban_port_song_subgenre((int)d);
    d=plat_enc(EN_SELECT);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG)
        urban_port_song_key((int)d);
    d=plat_enc(EN_PRESET);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG) {
        int v=(int)s->complexity+(int)d;
        if(v<0)v=0;
        if(v>3)v=3;
        s->complexity=(uint8_t)v;
        urban_port_stage_pattern();
    }
    d=plat_enc(EN_K3);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG)
            urban_port_song_subgenre((int)d);
        else {
            UrbanTrack *t=&s->tracks[p->rt.ui.selected_track];
            int v=((int)t->subgenre+(int)d)%4;
            if(v<0)v+=4;
            t->subgenre=(uint8_t)v;
            p->overridden[p->rt.ui.selected_track]=1u;
            if(!t->locked)
                urban_runtime_refresh_track(&p->rt,p->rt.ui.selected_track);
            urban_port_release();
            urban_port_sound_profile();
            urban_port_mix_levels();
            urban_port_stage_pattern();
        }
    }
    d=plat_enc(EN_K4);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG && !s->song_mode)
        urban_port_queue((int)d);
    urban_port_poll();
}
static const char *const urban_section_name[URBAN_SEQUENCES] = {
    "INTRO","VERSE 1","HOOK","VERSE 2","BRIDGE","DROP","BREAK","OUTRO"
};
/* Never erase the real LCD between frames. draw_text_box() uses BLACK
 * opaque backing and lcd_sync() on EVERY label; previously URBAN-1 also
 * did a full 240x240 lcd_fill every frame. On the Android simulator this
 * exposed partially cleared regions as ugly black flicker. Instead build
 * two off-screen 240px-wide canvas bands (144 + 96; <= CV_MAX) and blit
 * each only when the information on screen actually changes.
 */
static uint32_t urban_port_screen_signature(const UrbanHypPort *p) {
    const UrbanSong *s=&p->rt.ui.song;
    const UrbanTrack *t=&s->tracks[p->rt.ui.selected_track];
    uint32_t v=0x811c9dc5u;
#define UI_ACC(x) v=(v ^ ((uint32_t)(x) + 0x9e3779b9u))*16777619u
    UI_ACC(p->rt.ui.page);
    UI_ACC(s->playing);UI_ACC(s->song_mode);
    UI_ACC(s->current);UI_ACC(s->queued);
    UI_ACC(s->arrangement_pos);UI_ACC(s->section_bar);
    UI_ACC(p->rt.ui.selected_track);
    UI_ACC(p->rt.ui.genre);UI_ACC(s->genre);UI_ACC(s->subgenre);
    UI_ACC(s->key);UI_ACC(s->scale);UI_ACC(s->complexity);
    UI_ACC(p->rt.ui.generation_serial);
    UI_ACC(t->genre);UI_ACC(t->subgenre);
    UI_ACC(t->locked);UI_ACC(t->muted);UI_ACC(t->solo);
    UI_ACC(p->editor.view);UI_ACC(p->editor.target);UI_ACC(p->editor.bank);
    if(p->editor.view) {
        unsigned nb=0u;
        const UrbanEditorBank *eb=urban_editor_bank(&p->editor,&nb);
        if(eb)for(unsigned k=0;k<4u;k++)
            if(eb->param[k]!=URBAN_EDITOR_NO_PARAM)
                UI_ACC(urban_port_editor_value(eb->param[k]));
    }
#undef UI_ACC
    return v;
}
static void urban_port_screen_text(int32_t y,const felucca_font_t *font,
                                    const char *str,uint16_t colour) {
    int32_t x=(240-text_w(font,str))/2;
    cv_text(x<0?0:x,y,font,str,colour);
}
static const char *urban_port_instrument_name(const UrbanSong *s,
                                                 unsigned track) {
    UrbanInstrumentProgram ip=urban_instrument_program(s,track,s->current);
    if(track==2u)return urban_bass_names[ip.patch.bass];
    if(track==3u)return urban_lead_names[ip.patch.lead];
    if(track==4u)return urban_pad_names[ip.patch.pad];
    if(track==0u||track==1u)return urban_kit_names[ip.patch.drum_kit];
    return urban_noise_names[ip.noise_color];
}
/* JieLi's freestanding linker has no libc strcpy. The fixed-size
 * explicit loop also ensures no text write exceeds its 24-byte buffer. */
static void urban_port_copy_text(char dst[24],const char *src) {
    for(unsigned i=0;i<23u;i++) {
        char ch=src[i];
        dst[i]=ch;
        if(!ch)return;
    }
    dst[23]=0;
}
static void urban_port_editor_value_text(int id,char buf[24]) {
    int v=urban_port_editor_value(id);
    static const char *const scale_name[URBAN_SCALE_COUNT]={
        "MINOR","PHRYG","DORIAN","HARM MIN","MAJOR",
        "MIXOLYD","PHRYG DOM","LYDIAN","MEL MIN","LOCRIAN"
    };
    if(id==UC_KEY)urban_port_copy_text(buf,EN_NOTE_NAME[(unsigned)v%12u]);
    else if(id==UC_SCALE)urban_port_copy_text(buf,scale_name[(unsigned)v%URBAN_SCALE_COUNT]);
    else if(id==UC_COMPLEX) {
        buf[0]=(char)('0'+v);buf[1]=0;
    } else if(id>=UC_GATE16 && id<=UC_REPEAT8)
        urban_port_copy_text(buf,v?"ON":"OFF");
    else if(id==UC_TEMPO)en_param_text(P_TEMPO,v,buf);
    else if(id>=0 && id<P_NPARAMS)en_param_text(id,v,buf);
    else urban_port_copy_text(buf,"--");
}
static void urban_port_editor_upper(uint16_t orange,uint16_t dim) {
    const UrbanEditor *e=&urban_port.editor;
    unsigned nb=0u;
    const UrbanEditorBank *bank=urban_editor_bank(e,&nb);
    urban_port_screen_text(9,&FONT_B,e->view==URBAN_EDITOR_FX?
                             "URBAN-1 FX":"URBAN-1 EDIT",orange);
    urban_port_screen_text(36,&FONT_S,e->target==0u?
                             "MASTER - ALL TRACKS":
                             urban_track_name[e->target-1u],C_WHITE);
    if(!bank) {
        urban_port_screen_text(72,&FONT_S,"RESERVED TRACK",orange);
        urban_port_screen_text(98,&FONT_S,"NO HYP AUDIO ENGINE",dim);
        urban_port_screen_text(127,&FONT_S,"NO ACTIVE DSP PARAMS",dim);
        return;
    }
    urban_port_screen_text(56,&FONT_S,bank->title,dim);
    for(unsigned i=0;i<4u;i++) {
        int id=bank->param[i];
        if(id==URBAN_EDITOR_NO_PARAM)continue;
        char value[24];
        char line[4]="K1";
        line[1]=(char)('1'+i);
        int yy=76+(int)i*17;
        cv_text(10,yy,&FONT_S,line,orange);
        cv_text(36,yy,&FONT_S,urban_editor_label(id),C_WHITE);
        urban_port_editor_value_text(id,value);
        int right=235-text_w(&FONT_S,value);
        if(right<154)right=154;
        cv_text(right,yy,&FONT_S,value,orange);
    }
}
static void urban_port_editor_lower(uint16_t orange,uint16_t dim) {
    const UrbanEditor *e=&urban_port.editor;
    unsigned nb=0u;
    urban_editor_banks(e->view,e->target,&nb);
    char pos[12]="PAGE 0/9";
    char groups[12]="BANK 0/0";
    pos[5]=(char)('0'+(e->target+1u));
    groups[5]=(char)('0'+(nb?e->bank%nb+1u:0u));
    groups[7]=(char)('0'+nb);
    urban_port_screen_text(7,&FONT_S,pos,orange);
    urban_port_screen_text(23,&FONT_S,groups,orange);
    urban_port_screen_text(42,&FONT_S,"ALGORITHM: MASTER/TRACK",C_WHITE);
    urban_port_screen_text(59,&FONT_S,"PRESET/SELECT: NEXT BANK",C_WHITE);
    urban_port_screen_text(77,&FONT_S,"K1-K4 EDIT  HOME/SEQ EXIT",dim);
}
static void urban_port_frame(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    urban_port_input();
    uint32_t sig=urban_port_screen_signature(p);
    if(!p->ui_drawn || sig!=p->ui_signature) {
        static const uint16_t background=RGB(14,17,25);
        static const uint16_t orange=RGB(255,165,110);
        static const uint16_t dim=RGB(175,185,195);
        static const char *const section_name[URBAN_SEQUENCES]={
            "INTRO","VERSE 1","HOOK","VERSE 2",
            "BRIDGE","DROP","BREAK","OUTRO"
        };
        uint32_t pos=(uint32_t)s->arrangement_pos+1u;
        char seq[20]="NOW A   NEXT A";
        char bar[12]="BAR 01/56";
        char phrase[20]="BAR 1/4 (LOOP)";
        char track[14]="TRACK 1/8";
        char take[20]="STOPPED   TAKE 00";
        seq[4]=(char)('A'+s->current);
        seq[13]=(char)('A'+s->queued);
        phrase[4]=(char)('1'+urban_phrase_bar(s));
        bar[4]=(char)('0'+(pos/10u)%10u);
        bar[5]=(char)('0'+pos%10u);
        track[6]=(char)('1'+p->rt.ui.selected_track);
        if(s->playing) {
            take[0]='P';take[1]='L';take[2]='A';take[3]='Y';
            take[4]='I';take[5]='N';take[6]='G';
        }
        take[15]=(char)('0'+(p->rt.ui.generation_serial/10u)%10u);
        take[16]=(char)('0'+p->rt.ui.generation_serial%10u);

        /* Upper off-screen band: one DMA, no exposed intermediate black. */
        cv_begin(240,144,background);
        if(p->editor.view) {
            urban_port_editor_upper(orange,dim);
        } else {
        urban_port_screen_text(9,&FONT_B,"URBAN-1",orange);
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            urban_port_screen_text(42,&FONT_S,
                s->song_mode?"SONG MODE: 4-BAR PHRASES":"LOOP MODE: REPEAT A-H",dim);
            urban_port_screen_text(66,&FONT_S,
                s->song_mode?bar:seq,orange);
            urban_port_screen_text(89,&FONT_S,section_name[s->current],C_WHITE);
            urban_port_screen_text(107,&FONT_S,
                urban_genre_name[p->rt.ui.genre],C_WHITE);
            urban_port_screen_text(128,&FONT_S,
                urban_style_name(s->genre,s->subgenre),dim);
        } else {
            unsigned tr=p->rt.ui.selected_track;
            const UrbanTrack *t=&s->tracks[tr];
            urban_port_screen_text(41,&FONT_S,track,orange);
            urban_port_screen_text(65,&FONT_S,urban_track_name[tr],C_WHITE);
            urban_port_screen_text(88,&FONT_S,
                urban_genre_name[t->genre],C_WHITE);
            urban_port_screen_text(108,&FONT_S,
                urban_style_name(t->genre,t->subgenre),dim);
            urban_port_screen_text(126,&FONT_S,
                t->locked?"LOCKED - ENV":
                t->muted?"MUTED - LFO":
                t->solo?"SOLO - GLO":"READY",orange);
        }
        }
        cv_blit(0,0);

        /* Lower band uses another off-screen canvas. Never call lcd_fill
         * after sending the upper band: the previous content stays valid
         * until this second DMA is ready. */
        cv_begin(240,96,background);
        if(p->editor.view) {
            urban_port_editor_lower(orange,dim);
        } else if(p->rt.ui.page==URBAN_PAGE_SONG) {
            urban_port_screen_text(4,&FONT_S,take,orange);
            if(s->song_mode) {
                cv_rect(15,24,210,5,RGB(49,55,70));
                cv_rect(15,24,(int)(210u*pos/s->arrangement_len),5,orange);
            } else {
                urban_port_screen_text(25,&FONT_S,phrase,orange);
            }
            urban_port_screen_text(42,&FONT_S,
                s->song_mode?"K2 GENRE  K3 SUBGENRE":"K4 NEXT A-H  K3 SUB",C_WHITE);
            urban_port_screen_text(60,&FONT_S,
                s->song_mode?"SAVE GENERATE  ARP LOOP":"ARP AUTO  SAVE GENERATE",C_WHITE);
            urban_port_screen_text(79,&FONT_S,"SEQ TRACKS   SELECT KEY",dim);
        } else {
            urban_port_screen_text(14,&FONT_S,
                urban_port_instrument_name(s,p->rt.ui.selected_track),orange);
            urban_port_screen_text(36,&FONT_S,"SAVE: REGENERATE TRACK",dim);
            urban_port_screen_text(57,&FONT_S,"K1 TRACK  K2 GENRE",C_WHITE);
            urban_port_screen_text(79,&FONT_S,"FX/EDIT PAGES ENV LOCK LFO MUTE",dim);
        }
        cv_blit(0,144);
        lcd_sync();
        p->ui_signature=sig;
        p->ui_drawn=1u;
    }
    plat_leds((s->playing?1u<<B_PLAY:0u)|
              (p->editor.view==URBAN_EDITOR_FX?1u<<B_FX:
               p->editor.view==URBAN_EDITOR_EDIT?1u<<B_EDIT:
               p->rt.ui.page==URBAN_PAGE_SONG?
               (1u<<B_HOME)|(s->song_mode?1u<<B_ARP:0u):
               (1u<<B_SEQ)),0u);
}
#endif
