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

typedef struct {
    UrbanRuntime rt;
    uint32_t last_sixteenth, held_buttons;
    uint8_t held[3], note[3], left[3];
    uint8_t overridden[URBAN_TRACKS];
    uint8_t bank;
} UrbanHypPort;
static UrbanHypPort urban_port;

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
    if(!s->playing) {
        bank=0u; section=s->current; bar=s->section_bar;
    } else {
        bank=(uint8_t)(en_pat_now==0u?1u:0u);
        section=s->queued;
        bar=section==s->current?(unsigned)s->section_bar+1u:0u;
    }
    urban_port_pattern(section,bar,bank);
    p->bank=bank;
    en_set(P_PATTERN,bank);
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
    UrbanStylePatch bass=urban_style_patch(s->tracks[2].genre,
                                           s->tracks[2].subgenre);
    UrbanStylePatch lead=urban_style_patch(s->tracks[3].genre,
                                           s->tracks[3].subgenre);
    UrbanStylePatch pad=urban_style_patch(s->tracks[4].genre,
                                          s->tracks[4].subgenre);
    en_set(P_KIT_KICK,d.drum_kit);
    en_set(P_KIT_SNARE,d.drum_kit);
    en_set(P_KIT_HAT,h.hat_kit);
    en_set(P_KIT_PERC,h.drum_kit);
    en_set(P_SWING,urban_groove(s->genre,s->subgenre).swing);
    en_set(P_DRIVE,overall.drive);
    en_set(P_REVERB,overall.reverb);
    en_set(P_ECHO,overall.echo);
    en_set(P_DR_INST,bass.bass);
    en_set(P_DR_CUT,bass.bass_cut);
    en_set(P_DR_RES,bass.bass==9u?20:51);
    en_set(P_BASSOCT,bass.bass==9u||bass.bass==8u?-1:0);
    en_set(P_AC_WAVE,lead.lead);
    en_set(P_AC_CUT,lead.lead_cut);
    en_set(P_AC_RES,lead.lead==2u?68:43);
    en_set(P_AC_ENV,lead.lead==4u||lead.lead==15u?32:59);
    en_set(P_PADTONE,pad.pad);
    en_set(P_BRIGHT,urban_timbre[s->tracks[4].genre%URBAN_GENRES].pad_bright);
    en_set(P_PE_ATK,pad.pad==7u||pad.pad==3u?36:12);
    en_set(P_PE_REL,pad.pad==7u||pad.pad==3u?72:40);
    en_set(P_NOISE,s->genre==1u||s->genre==4u||s->genre==6u?14:0);
}
/* Song sections are actually orchestrated: the intro/bridge/break strip
 * layers, a hook raises melodic energy, and the drop restores full drums.
 * Repeated hooks keep identical motifs and balanced synthesis settings.
 */
static void urban_port_section_mix(void) {
    const UrbanSong *s=&urban_port.rt.ui.song;
    static const uint8_t levels[URBAN_SEQUENCES][4]={
        /* drums, bass, lead, pad */
        {30,39,25,55}, /* Intro */
        {75,77,53,45}, /* Verse A */
        {86,86,79,66}, /* Hook */
        {77,79,61,48}, /* Verse B */
        {29,48,34,73}, /* Bridge */
        {94,94,86,65}, /* Drop */
        {16,34,19,67}, /* Break */
        {34,42,27,46}  /* Outro */
    };
    unsigned sec=s->current%URBAN_SEQUENCES;
    en_set(P_PULSE,levels[sec][0]);
    en_set(P_BASS,levels[sec][1]);
    en_set(P_ACID,levels[sec][2]);
    en_set(P_PAD,levels[sec][3]);
}
static void urban_port_generate(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    uint8_t song_generate=(uint8_t)(p->rt.ui.page==URBAN_PAGE_SONG);
    if(song_generate && s->playing)en_play(0);
    urban_runtime_generate(&p->rt);
    if(song_generate) {
        urban_port_release();
        p->rt.transport.step=0;
        p->rt.transport.tick=0;
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
static void urban_port_step(void) {
    UrbanHypPort *p=&urban_port;
    UrbanMidiFrame frame;
    unsigned i,t;
    /* A gate lasts N sixteenth notes. */
    for(i=0;i<3;i++) if(p->held[i] && p->left[i]) {
        if(--p->left[i]==0u) {
            en_note((int)i,urban_hyp_key(i,p->note[i]),0);
            p->held[i]=0u;
        }
    }
    /* Stage the NEXT bar on the last sixteenth, before the transport
     * advances the song arrangement and before HYP's pattern boundary. */
    if(p->rt.transport.step==URBAN_STEPS-1u)
        urban_port_stage_pattern();
    uint8_t previous=p->rt.ui.song.current;
    urban_runtime_tick(&p->rt,&frame);
    /* URBAN's 96 PPQN clock is advanced 24 ticks per HYP sixteenth;
     * capture the initial event before its empty intermediate ticks. */
    for(t=1;t<URBAN_TICKS_PER_STEP;t++) {
        UrbanMidiFrame unused;
        urban_runtime_tick(&p->rt,&unused);
    }
    for(i=2;i<=4;i++)
        urban_port_note(i,frame.event[i],frame.glide[i]);
    if(p->rt.ui.song.current!=previous)urban_port_section_mix();
}
static void urban_port_poll(void) {
    UrbanHypPort *p=&urban_port;
    uint32_t now=en_bars,steps;
    if(!p->rt.ui.song.playing) {
        urban_port_release();
        p->last_sixteenth=now;
        return;
    }
    /* After a long stall do not flood HYP's 256-entry command queue. */
    steps=now-p->last_sixteenth;
    if(steps>32u) {
        urban_port_release();
        p->last_sixteenth=now-1u;
        steps=1u;
    }
    while(steps--) {
        urban_port_step();
        p->last_sixteenth++;
    }
}
static void urban_port_play(void) {
    UrbanHypPort *p=&urban_port;
    p->rt.ui.song.playing=(uint8_t)!p->rt.ui.song.playing;
    if(!p->rt.ui.song.playing) {
        urban_port_release();
        en_play(0);
    } else {
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
    p->last_sixteenth=en_bars;
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
    urban_runtime_init(&p->rt,seed);
    // HYP-1 ui_init() starts its engine by default. Stop it explicitly so
    // URBAN-1's STOPPED display agrees with the actual audio transport.
    en_play(0);
    /* First playable prototype: deliberately suppress HYP's auto voices
     * and drift; URBAN supplies notes and drum patterns instead. */
    en_set(P_PAD_MAN,1);
    en_set(P_DR_MAN,1);
    en_set(P_AC_MAN,1);
    en_set(P_DD_CHANGE,0);
    en_set(P_DD_MUTATE,0);
    en_set(P_MOTION,0);
    en_set(P_AC_MUT,0);
    en_set(P_OCT,0);
    en_set(P_PAD_OCT,0);
    en_set(P_BASSOCT,0);
    en_set(P_AC_OCT,0);
    en_set(P_TEMPO,p->rt.ui.song.tempo);
    en_set(P_ROOT,p->rt.ui.song.key);
    en_set(P_MODE,MODE_AEOLIAN);
    en_set(P_PULSE,80);
    en_set(P_BASS,78);
    en_set(P_ACID,75);
    en_set(P_PAD,70);
    en_set(P_NOISE,0);
    en_set(P_KIT_KICK,7);  /* HYP-1 808 kit */
    en_set(P_KIT_SNARE,7);
    en_set(P_KIT_HAT,7);
    for(i=0;i<URBAN_TRACKS;i++) p->rt.ui.song.tracks[i].genre=0;
    for(i=0;i<URBAN_TRACKS;i++) urban_runtime_refresh_track(&p->rt,i);
    urban_song_set_mode(&p->rt.ui.song,1); /* SONG MODE by default */
    urban_port_sound_profile();
    urban_port_section_mix();
    urban_port_stage_pattern();
    p->last_sixteenth=en_bars;
}
static void urban_port_input(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    uint32_t b=plat_buttons(),down=b&~p->held_buttons;
    int32_t d;
    p->held_buttons=b;
    if(down&(1u<<B_HOME))urban_ui_page(&p->rt.ui,URBAN_PAGE_SONG);
    if(down&(1u<<B_SEQ))urban_ui_page(&p->rt.ui,URBAN_PAGE_TRACKS);
    if(down&(1u<<B_PLAY))urban_port_play();
    if(down&(1u<<B_SAVE))urban_port_generate();
    if(p->rt.ui.page==URBAN_PAGE_SONG) {
        if(down&(1u<<B_ARP))urban_port_set_mode(!s->song_mode);
        if(down&(1u<<B_REC))urban_port_restart();
    } else {
        UrbanTrack *t=&s->tracks[p->rt.ui.selected_track];
        if(down&(1u<<B_EDIT))t->locked=(uint8_t)!t->locked;
        if(down&(1u<<B_FX))t->muted=(uint8_t)!t->muted;
        if(down&(1u<<B_GLO))t->solo=(uint8_t)!t->solo;
        if(down&((1u<<B_FX)|(1u<<B_GLO)))urban_port_stage_pattern();
    }
    d=plat_enc(EN_K1);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            int v=(int)s->tempo+(int)d;
            if(v<70)v=70;
            if(v>190)v=190;
            s->tempo=(uint8_t)v;
            en_set(P_TEMPO,v);
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
            en_set(P_TEMPO,s->tempo);
            for(unsigned i=0;i<URBAN_TRACKS;i++)
                if(!p->overridden[i] && !s->tracks[i].locked)
                    s->tracks[i].genre=(uint8_t)v;
            /* Press SAVE to recompose the notes in the new style. */
        } else {
            urban_ui_set_genre(&p->rt.ui,(uint8_t)v,
                              s->tracks[p->rt.ui.selected_track].subgenre);
            p->overridden[p->rt.ui.selected_track]=1u;
        }
    }
    /* The left ALGORITHM encoder selects the genre's four named
     * substyles; PRESETS controls ornament/fill complexity (0..3).
     * KNOB3 remains the musical key in SONG and substyle in TRACKS.
     */
    d=plat_enc(EN_ALGO);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG) {
        int v=(int)s->subgenre+(int)d;
        v=(v%4+4)%4;
        s->subgenre=(uint8_t)v;
        p->rt.ui.subgenre=(uint8_t)v;
        for(unsigned i=0;i<URBAN_TRACKS;i++)
            if(!p->overridden[i] && !s->tracks[i].locked)
                s->tracks[i].subgenre=(uint8_t)v;
    }
    d=plat_enc(EN_PRESET);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG) {
        int v=(int)s->complexity+(int)d;
        if(v<0)v=0;
        if(v>3)v=3;
        s->complexity=(uint8_t)v;
    }
    d=plat_enc(EN_K3);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            int v=(int)s->key+(int)d;
            v=(v%12+12)%12;
            s->key=(uint8_t)v;
            en_set(P_ROOT,v);
            /* Key modulation revoices the existing rhythms, not the song. */
            for(unsigned i=0;i<URBAN_TRACKS;i++)
                if(!s->tracks[i].locked)urban_runtime_refresh_track(&p->rt,i);
        } else {
            UrbanTrack *t=&s->tracks[p->rt.ui.selected_track];
            int v=(int)t->subgenre+(int)d;
            t->subgenre=(uint8_t)((v%4+4)%4);
            p->overridden[p->rt.ui.selected_track]=1u;
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
static void urban_port_frame(void) {
    UrbanHypPort *p=&urban_port;
    UrbanSong *s=&p->rt.ui.song;
    char seq[20]="NOW A   NEXT A";
    char bar[12]="BAR 01/56";
    char trnum[14]="TRACK 1/8";
    unsigned tr;
    urban_port_input();
    tr=p->rt.ui.selected_track;
    seq[4]=(char)('A'+s->current);
    seq[13]=(char)('A'+s->queued);
    unsigned n=(unsigned)s->arrangement_pos+1u;
    bar[4]=(char)('0'+n/10u); bar[5]=(char)('0'+n%10u);
    trnum[6]=(char)('1'+tr);
    lcd_fill(0,0,240,240,RGB(14,17,25));
    draw_text_box(0,9,240,&FONT_B,"URBAN-1",RGB(255,165,110),1);
    if(p->rt.ui.page==URBAN_PAGE_SONG) {
        draw_text_box(0,42,240,&FONT_S,
              s->song_mode?"SONG MODE / AUTO":"SEQUENCE MODE / MANUAL",C_WHITE,1);
        draw_text_box(0,66,240,&FONT_S,
              s->song_mode?bar:seq,RGB(255,165,110),1);
        draw_text_box(0,89,240,&FONT_S,urban_section_name[s->current],C_WHITE,1);
        draw_text_box(0,107,240,&FONT_S,
              urban_genre_name[p->rt.ui.genre],C_WHITE,1);
        draw_text_box(0,128,240,&FONT_S,
              urban_style_name(s->genre,s->subgenre),C_WHITE,1);
        draw_text_box(0,148,240,&FONT_S,
              s->playing?"PLAYING":"STOPPED",RGB(255,165,110),1);
        lcd_fill(15,168,210,5,RGB(49,55,70));
        if(s->song_mode)
            lcd_fill(15,168,(int)(210u*n/s->arrangement_len),5,RGB(255,165,110));
        draw_text_box(0,182,240,&FONT_S,"SAVE: GENERATE SONG",C_WHITE,1);
        draw_text_box(0,202,240,&FONT_S,"ARP: SONG / SEQUENCE",C_WHITE,1);
        draw_text_box(0,221,240,&FONT_S,"SEQ: TRACKS",C_WHITE,1);
    } else {
        draw_text_box(0,41,240,&FONT_S,trnum,RGB(255,165,110),1);
        draw_text_box(0,65,240,&FONT_S,urban_track_name[tr],C_WHITE,1);
        draw_text_box(0,89,240,&FONT_S,
                      urban_genre_name[s->tracks[tr].genre],C_WHITE,1);
        draw_text_box(0,110,240,&FONT_S,
                 urban_style_name(s->tracks[tr].genre,
                                  s->tracks[tr].subgenre),C_WHITE,1);
        draw_text_box(0,136,240,&FONT_S,
                 s->tracks[tr].locked?"LOCKED - EDIT":
                 s->tracks[tr].muted?"MUTED - FX":
                 s->tracks[tr].solo?"SOLO - GLO":"READY",RGB(255,165,110),1);
        draw_text_box(0,158,240,&FONT_S,"A-H INDEPENDENT",C_WHITE,1);
        draw_text_box(0,180,240,&FONT_S,"SAVE: REGENERATE TRACK",C_WHITE,1);
        draw_text_box(0,201,240,&FONT_S,"K1 TRACK  K2 GENRE",C_WHITE,1);
        draw_text_box(0,223,240,&FONT_S,"HOME: SONG   EDIT: LOCK",C_WHITE,1);
    }
    plat_leds((s->playing?1u<<B_PLAY:0u)|
              (p->rt.ui.page==URBAN_PAGE_SONG?
               (1u<<B_HOME)|(s->song_mode?1u<<B_ARP:0u):
               (1u<<B_SEQ)),0u);
}
#endif
