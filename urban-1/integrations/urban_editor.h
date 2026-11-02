/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 browsable hardware editor.
 * FX/EDIT each have MASTER + 8 track pages selected by ALGORITHM.
 * PRESET or SELECT scrolls banks; K1..K4 edit four real parameters.
 * This is compiled inside the actual HYP-1 DSP, which provides P_*
 * definitions and en_param_info(). HYP-1 echo and reverb are SHARED.
 */
#ifndef URBAN_EDITOR_H
#define URBAN_EDITOR_H
#define URBAN_EDITOR_FX 1u
#define URBAN_EDITOR_EDIT 2u
#define URBAN_EDITOR_PAGE_COUNT (URBAN_TRACKS + 1u)
#define URBAN_EDITOR_NO_PARAM (-1)
enum {
    UC_TEMPO=P_NPARAMS, UC_KEY, UC_SCALE, UC_COMPLEX,
    UC_GATE16, UC_GATE8, UC_CRUSH, UC_THROW, UC_LOWPASS,
    UC_HIGHPASS, UC_REVERSE, UC_FILL, UC_STOP,
    UC_REPEAT4, UC_REPEAT8, UC_SFX_SOUND, UC_SFX_LEVEL,
    UC_SFX_LENGTH, UC_SFX_DENSITY
};
#define URBAN_EDITOR_CUSTOM_LAST UC_SFX_DENSITY
typedef struct {
    const char *title;
    int16_t param[4];
} UrbanEditorBank;
typedef struct {
    uint8_t view,target,bank;
    uint8_t changed[P_NPARAMS], valid[P_NPARAMS];
    int16_t override[P_NPARAMS],live[P_NPARAMS];
    uint32_t performance_mask;
} UrbanEditor;
/* Master FX; the only place real shared wet bus controls are found.
 * The performance FX flags are global; never label them as per-voice
 * send controls. */
static const UrbanEditorBank urban_fx_master[] = {
    {"ECHO / REVERB", {P_ECHO,P_ETIME,P_REVERB,P_SIZE}},
    {"DRIVE / GATE", {P_DRIVE,UC_GATE16,UC_GATE8,UC_CRUSH}},
    {"FILTER / FX", {UC_LOWPASS,UC_HIGHPASS,UC_THROW,UC_FILL}},
    {"TAPE / REPEAT", {UC_REVERSE,UC_STOP,UC_REPEAT4,UC_REPEAT8}}
};
static const UrbanEditorBank urban_fx_drums[] = {
    {"KICK EQ/DECAY", {P_LANE_LVL(0),P_LANE_TUNE(0),P_LANE_DEC(0),P_KIT_KICK}},
    {"SNARE SHAPING", {P_LANE_LVL(1),P_LANE_TUNE(1),P_LANE_DEC(1),P_KIT_SNARE}},
    {"CLAP SHAPING", {P_LANE_LVL(2),P_LANE_TUNE(2),P_LANE_DEC(2),P_KIT_SNARE}}
};
static const UrbanEditorBank urban_fx_hats[] = {
    {"CLOSED HAT", {P_LANE_LVL(3),P_LANE_TUNE(3),P_LANE_DEC(3),P_KIT_HAT}},
    {"OPEN HAT", {P_LANE_LVL(4),P_LANE_TUNE(4),P_LANE_DEC(4),P_KIT_HAT}},
    {"PERCUSSION", {P_LANE_LVL(5),P_LANE_TUNE(5),P_LANE_DEC(5),P_KIT_PERC}}
};
static const UrbanEditorBank urban_fx_bass[] = {
    {"DRONE FILTER", {P_DR_CUT,P_DR_RES,P_DE_AMT,P_BASS}},
    {"DRONE MOD", {P_DL_RATE,P_DL_DEPTH,P_DL_SHAPE,P_DL_DEST}}
};
static const UrbanEditorBank urban_fx_lead[] = {
    {"ACID FILTER", {P_AC_CUT,P_AC_RES,P_AC_ENV,P_AC_DEC}},
    {"ACCENT/SLIDE", {P_AC_ACC,P_AC_SLIDE,P_ACID,P_AC_WAVE}}
};
static const UrbanEditorBank urban_fx_pad[] = {
    {"PAD MOVEMENT", {P_BRIGHT,P_DETUNE,P_PL_RATE,P_PL_DEPTH}},
    {"PAD SPACE", {P_PL_SHAPE,P_PL_DEST,P_PE_REL,P_PAD}}
};
static const UrbanEditorBank urban_fx_noise[] = {
    {"NOISE SHAPING", {P_NZ_TONE,P_NZ_PUMP,P_NZ_COLOR,P_NOISE}}
};
static const UrbanEditorBank urban_fx_sfx[] = {
    {"SFX PERFORMANCE", {UC_SFX_SOUND,UC_SFX_LEVEL,UC_SFX_LENGTH,UC_SFX_DENSITY}}
};
/* Master Edit: the real composition state is kept in URBAN's score.
 * Tempo / key / mode / complexity update the notes as well as HYP params.
 * Song-wide octaves and tuning are directly exposed by HYP-1. */
static const UrbanEditorBank urban_edit_master[] = {
    {"SONG THEORY", {UC_TEMPO,UC_KEY,UC_SCALE,UC_COMPLEX}},
    {"SONG OCTAVES", {P_OCT,P_PAD_OCT,P_BASSOCT,P_AC_OCT}},
    {"GLOBAL MOTION", {P_TUNE,P_MOTION,P_DRIFT,P_EVOLVE}}
};
static const UrbanEditorBank urban_edit_drums[] = {
    {"KICK / SNARE", {P_KIT_KICK,P_KIT_SNARE,P_LANE_TUNE(0),P_LANE_TUNE(1)}},
    {"DRUM DECAY", {P_LANE_DEC(0),P_LANE_DEC(1),P_LANE_DEC(2),P_LANE_LVL(0)}},
    {"DRUM LEVELS", {P_LANE_LVL(1),P_LANE_LVL(2),P_LANE_TUNE(2),P_PULSE}}
};
static const UrbanEditorBank urban_edit_hats[] = {
    {"HAT / PERC", {P_KIT_HAT,P_KIT_PERC,P_LANE_TUNE(3),P_LANE_TUNE(4)}},
    {"HAT ENVELOPES", {P_LANE_DEC(3),P_LANE_DEC(4),P_LANE_DEC(5),P_LANE_TUNE(5)}},
    {"HAT LEVELS", {P_LANE_LVL(3),P_LANE_LVL(4),P_LANE_LVL(5),P_PULSE}}
};
static const UrbanEditorBank urban_edit_bass[] = {
    {"DRONE SOUND", {P_DR_INST,P_DR_CUT,P_DR_RES,P_BASSOCT}},
    {"DRONE ENVELOPE", {P_DE_TRIG,P_DE_ATK,P_DE_DEC,P_DE_AMT}},
    {"DRONE LFO", {P_DL_RATE,P_DL_DEPTH,P_DL_SHAPE,P_DL_DEST}},
    {"DRONE LEVEL", {P_BASS,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM}}
};
static const UrbanEditorBank urban_edit_lead[] = {
    {"ACID SOUND", {P_AC_WAVE,P_AC_CUT,P_AC_RES,P_AC_ENV}},
    {"ACID ENVELOPE", {P_AC_DEC,P_AC_ACC,P_AC_SLIDE,P_AC_OCT}},
    {"ACID LEVEL", {P_ACID,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM}}
};
static const UrbanEditorBank urban_edit_pad[] = {
    {"PAD SOUND", {P_PADTONE,P_BRIGHT,P_DETUNE,P_PAD_OCT}},
    {"PAD ENVELOPE", {P_PE_ATK,P_PE_HOLD,P_PE_REL,P_PE_REST}},
    {"PAD LFO", {P_PL_RATE,P_PL_DEPTH,P_PL_SHAPE,P_PL_DEST}},
    {"PAD LEVEL", {P_PAD,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM,URBAN_EDITOR_NO_PARAM}}
};
static const UrbanEditorBank urban_edit_noise[] = {
    {"NOISE ENGINE", {P_NZ_COLOR,P_NZ_TONE,P_NZ_PUMP,P_NOISE}}
};
static const UrbanEditorBank urban_edit_sfx[] = {
    {"SFX GENERATOR", {UC_SFX_SOUND,UC_SFX_LEVEL,UC_SFX_LENGTH,UC_SFX_DENSITY}}
};
#define UE_SIZE(a) ((unsigned)(sizeof(a)/sizeof((a)[0])))
static const UrbanEditorBank *urban_editor_banks(
    unsigned view,unsigned target,unsigned *count) {
    const UrbanEditorBank *out=0;
    *count=0u;
#define UE_CASE(t,array) case t: out=array; *count=UE_SIZE(array); break
    if(view==URBAN_EDITOR_FX) {
        switch(target) {
            UE_CASE(0,urban_fx_master);
            UE_CASE(1,urban_fx_drums);
            UE_CASE(2,urban_fx_hats);
            UE_CASE(3,urban_fx_bass);
            UE_CASE(4,urban_fx_lead);
            UE_CASE(5,urban_fx_pad);
            UE_CASE(6,urban_fx_noise);
            UE_CASE(7,urban_fx_sfx);
            default: break; /* Track 8 remains reserved */
        }
    } else if(view==URBAN_EDITOR_EDIT) {
        switch(target) {
            UE_CASE(0,urban_edit_master);
            UE_CASE(1,urban_edit_drums);
            UE_CASE(2,urban_edit_hats);
            UE_CASE(3,urban_edit_bass);
            UE_CASE(4,urban_edit_lead);
            UE_CASE(5,urban_edit_pad);
            UE_CASE(6,urban_edit_noise);
            UE_CASE(7,urban_edit_sfx);
            default: break;
        }
    }
#undef UE_CASE
    return out;
}
static const UrbanEditorBank *urban_editor_bank(const UrbanEditor *ed,
                                                unsigned *count) {
    const UrbanEditorBank *p=urban_editor_banks(ed->view,ed->target,count);
    if(!p || !*count)return 0;
    return &p[ed->bank%*count];
}
static int urban_editor_special_bit(int id) {
    switch(id) {
      case UC_GATE16: return FX_GATE16;
      case UC_GATE8: return FX_GATE8;
      case UC_CRUSH: return FX_CRUSH;
      case UC_THROW: return FX_THROW;
      case UC_LOWPASS: return FX_LP;
      case UC_HIGHPASS: return FX_HP;
      case UC_REVERSE: return FX_REV;
      case UC_FILL: return FX_FILL;
      case UC_STOP: return FX_STOP;
      case UC_REPEAT4: return FX_RPT4;
      case UC_REPEAT8: return FX_RPT8;
      default: return -1;
    }
}
static const char *urban_editor_label(int id) {
    switch(id) {
      case UC_TEMPO:return "TEMPO";
      case UC_KEY:return "KEY";
      case UC_SCALE:return "SCALE";
      case UC_COMPLEX:return "COMPLEX";
      case UC_GATE16:return "GATE16";
      case UC_GATE8:return "GATE8";
      case UC_CRUSH:return "CRUSH";
      case UC_THROW:return "THROW";
      case UC_LOWPASS:return "LOWPASS";
      case UC_HIGHPASS:return "HIGHPASS";
      case UC_REVERSE:return "REVERSE";
      case UC_FILL:return "FILL";
      case UC_STOP:return "STOP";
      case UC_REPEAT4:return "RPT 1/4";
      case UC_REPEAT8:return "RPT 1/8";
      case UC_SFX_SOUND:return "SFX SOUND";
      case UC_SFX_LEVEL:return "SFX LEVEL";
      case UC_SFX_LENGTH:return "SFX LENGTH";
      case UC_SFX_DENSITY:return "SFX DENSITY";
      default:return en_param_info(id)->name;
    }
}
static unsigned urban_editor_page_next(unsigned current,int delta,unsigned count) {
    if(!count)return 0u;
    int n=(int)current+(int)delta;
    n%=(int)count;
    if(n<0)n+=(int)count;
    return (unsigned)n;
}
#endif
