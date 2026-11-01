/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 / HYP-1 port. Included after HYP-1 app/ui.c in its unity build.
 * Uses the actual HYP-1 synth API: en_note, en_set, en_pats, en_play, en_bars.
 * Initially audible: drum tracks 0/1, bass 2, lead 3, pad 4.
 * Tracks 5..7 remain song-state only until new DSP voices are integrated.
 * HYP's en_note has no velocity arg; glide on bass uses legato ordering.
 */
#ifndef URBAN_HYP_PORT_H
#define URBAN_HYP_PORT_H
#include "urban/urban_runtime.h"

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
static void urban_port_pattern(uint8_t sequence,uint8_t bank) {
    UrbanHypPort *p=&urban_port;
    uint16_t lanes[EN_NLANE]={0};
    uint8_t any_solo=0u;
    unsigned i,k;
    for(i=0;i<URBAN_TRACKS;i++) any_solo|=p->rt.ui.song.tracks[i].solo;
    for(k=0;k<URBAN_STEPS;k++) {
        const UrbanNote *dr=&p->rt.voices[0].sequence[sequence].steps[k];
        const UrbanNote *hat=&p->rt.voices[1].sequence[sequence].steps[k];
        if(dr->velocity && !p->rt.ui.song.tracks[0].muted &&
           (!any_solo || p->rt.ui.song.tracks[0].solo))
            lanes[dr->note==38u?1u:0u]|=(uint16_t)(1u<<k);
        if(hat->velocity && !p->rt.ui.song.tracks[1].muted &&
           (!any_solo || p->rt.ui.song.tracks[1].solo))
            lanes[3]|=(uint16_t)(1u<<k);
    }
    for(i=0;i<EN_NLANE;i++) en_pats[bank][i]=lanes[i];
}
static void urban_port_stage_pattern(void) {
    UrbanHypPort *p=&urban_port;
    uint8_t bank;
    if(!p->rt.ui.song.playing) {
        bank=0u;
    } else {
        bank=(uint8_t)(en_pat_now==0u?1u:0u);
    }
    urban_port_pattern(p->rt.ui.song.queued,bank);
    p->bank=bank;
    en_set(P_PATTERN,bank);
}
static void urban_port_generate(void) {
    urban_runtime_generate(&urban_port.rt);
    urban_port_stage_pattern();
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
    urban_runtime_tick(&p->rt,&frame);
    /* URBAN's 96 PPQN clock is advanced 24 ticks per HYP sixteenth;
     * capture the initial event before its empty intermediate ticks. */
    for(t=1;t<URBAN_TICKS_PER_STEP;t++) {
        UrbanMidiFrame unused;
        urban_runtime_tick(&p->rt,&unused);
    }
    for(i=2;i<=4;i++)
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
        en_play(1);
    }
}
static void urban_port_queue(int n) {
    urban_ui_next_knob(&urban_port.rt.ui,n);
    urban_port_stage_pattern();
}
static void urban_port_init(uint32_t seed) {
    unsigned i;
    UrbanHypPort *p=&urban_port;
    memset(p,0,sizeof(*p));
    urban_runtime_init(&p->rt,seed);
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
    urban_port_stage_pattern();
    p->last_sixteenth=en_bars;
}
static void urban_port_input(void) {
    UrbanHypPort *p=&urban_port;
    uint32_t b=plat_buttons(),down=b&~p->held_buttons;
    int32_t d;
    p->held_buttons=b;
    if(down&(1u<<B_HOME)) urban_ui_page(&p->rt.ui,URBAN_PAGE_SONG);
    if(down&(1u<<B_SEQ)) urban_ui_page(&p->rt.ui,URBAN_PAGE_TRACKS);
    if(down&(1u<<B_PLAY)) urban_port_play();
    if(down&(1u<<B_SAVE)) urban_port_generate();
    d=plat_enc(EN_K1);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            int v=(int)p->rt.ui.song.tempo+(int)d;
            if(v<70)v=70;
            if(v>190)v=190;
            p->rt.ui.song.tempo=(uint8_t)v;
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
                   p->rt.ui.song.tracks[p->rt.ui.selected_track].genre)+(int)d;
        v=(v%URBAN_GENRES+URBAN_GENRES)%URBAN_GENRES;
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            unsigned i;
            p->rt.ui.genre=(uint8_t)v;
            for(i=0;i<URBAN_TRACKS;i++) if(!p->overridden[i] && !p->rt.ui.song.tracks[i].locked)
                p->rt.ui.song.tracks[i].genre=(uint8_t)v;
        } else {
            urban_ui_set_genre(&p->rt.ui,(uint8_t)v,
                              p->rt.ui.song.tracks[p->rt.ui.selected_track].subgenre);
            p->overridden[p->rt.ui.selected_track]=1u;
        }
    }
    d=plat_enc(EN_K3);
    if(d) {
        if(p->rt.ui.page==URBAN_PAGE_SONG) {
            int v=(int)p->rt.ui.song.key+(int)d;
            v=(v%12+12)%12;
            p->rt.ui.song.key=(uint8_t)v;
            en_set(P_ROOT,v);
        } else {
            UrbanTrack *t=&p->rt.ui.song.tracks[p->rt.ui.selected_track];
            int v=(int)t->subgenre+(int)d;
            t->subgenre=(uint8_t)((v%16+16)%16);
            p->overridden[p->rt.ui.selected_track]=1u;
        }
    }
    d=plat_enc(EN_K4);
    if(d && p->rt.ui.page==URBAN_PAGE_SONG)
        urban_port_queue((int)d);
    urban_port_poll();
}
static void urban_port_frame(void) {
    UrbanHypPort *p=&urban_port;
    char seq[20]="NOW A   NEXT A";
    char info[25]="TRACK 1";
    uint8_t curr=p->rt.ui.song.current,queued=p->rt.ui.song.queued;
    unsigned tr=p->rt.ui.selected_track;
    urban_port_input();
    curr=p->rt.ui.song.current;
    queued=p->rt.ui.song.queued;
    seq[4]=(char)('A'+curr);
    seq[13]=(char)('A'+queued);
    info[6]=(char)('1'+tr);
    lcd_fill(0,0,240,240,RGB(14,17,25));
    draw_text_box(0,12,240,&FONT_B,"URBAN-1",RGB(255,165,110),1);
    draw_text_box(0,52,240,&FONT_S,
                  p->rt.ui.page==URBAN_PAGE_SONG?"SONG PAGE":"TRACKS PAGE",C_WHITE,1);
    draw_text_box(0,77,240,&FONT_S,seq,RGB(255,165,110),1);
    draw_text_box(0,104,240,&FONT_S,
                  p->rt.ui.page==URBAN_PAGE_SONG?
                  urban_genre_name[p->rt.ui.genre]:
                  urban_track_name[tr],C_WHITE,1);
    draw_text_box(0,127,240,&FONT_S,
                  p->rt.ui.page==URBAN_PAGE_SONG?
                  (p->rt.ui.song.playing?"PLAYING":"STOPPED"):
                  urban_genre_name[p->rt.ui.song.tracks[tr].genre],C_WHITE,1);
    if(p->rt.ui.page==URBAN_PAGE_TRACKS)
        draw_text_box(0,150,240,&FONT_S,info,RGB(255,165,110),1);
    draw_text_box(0,183,240,&FONT_S,"SAVE: GENERATE",C_WHITE,1);
    draw_text_box(0,205,240,&FONT_S,"HOME: SONG  SEQ: TRACKS",C_WHITE,1);
    plat_leds((p->rt.ui.song.playing?1u<<B_PLAY:0u)|
              (p->rt.ui.page==URBAN_PAGE_SONG?1u<<B_HOME:1u<<B_SEQ),0u);
}
#endif
