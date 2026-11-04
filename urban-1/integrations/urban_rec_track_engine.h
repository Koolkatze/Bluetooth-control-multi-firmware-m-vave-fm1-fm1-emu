/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 dedicated REC audio track (virtual TRACK 9).
 *
 * Own 8-voice synthesizer, one-shot drums/SFX, envelope, LFO and mix.
 * Never calls HYP PAD/DRONE/ACID, drum_fire, jazz note_cmd, or the
 * generated SFX voice. All commands are applied on HYP's DSP thread,
 * via the existing bounded post()/drain() queue.
 *
 * Included inside amb.c AFTER the mixl/mixr buffers and oscillator
 * helpers, BEFORE the renderer that calls urban_rec_track_render().
 */
#ifndef URBAN_REC_TRACK_ENGINE_H
#define URBAN_REC_TRACK_ENGINE_H

#define URBAN_REC_TRACK_POLY 8u
enum {
    URT_ATTACK, URT_DECAY, URT_SUSTAIN, URT_RELEASE,
    URT_LFO_RATE, URT_LFO_DEPTH, URT_LFO_SHAPE, URT_LFO_TARGET,
    URT_LEVEL, URT_TONE, URT_WIDTH, URT_DRIVE,
    URT_PARAMS
};
typedef struct {
    uint8_t note,program,kind,pressed;
    float phase,subphase,frequency,velocity,amp,peak;
    float attack,decay,release,age,pan;
    uint32_t noise;
} UrbanRecTrackVoice;
static UrbanRecTrackVoice urt_voices[URBAN_REC_TRACK_POLY];
static uint8_t urt_program=32u;
static uint8_t urt_parameter[URT_PARAMS]={
    12u,45u,72u,35u, 25u,0u,0u,0u,
    80u,55u,50u,10u
};
static float urt_lfo_phase;
volatile uint32_t urban_rec_track_onsets;
volatile uint32_t urban_rec_track_rendered_frames;
volatile uint32_t urban_rec_track_active_voices;
volatile uint32_t urban_rec_track_last_program;

static void urban_rec_track_recount(void)
{
    unsigned n=0;
    for(unsigned i=0;i<URBAN_REC_TRACK_POLY;i++)
        if(urt_voices[i].amp>0.00005f)n++;
    urban_rec_track_active_voices=n;
}
static void urban_rec_track_program(int program)
{
    if(program>=0 && program<78)urt_program=(uint8_t)program;
    urban_rec_track_last_program=urt_program;
}
static void urban_rec_track_control(int id,int value)
{
    if(id<0||id>=URT_PARAMS)return;
    if(value<0)value=0;
    if(value>100)value=100;
    urt_parameter[id]=(uint8_t)value;
}
static void urban_rec_track_quiet(void)
{
    for(unsigned i=0;i<URBAN_REC_TRACK_POLY;i++){
        urt_voices[i].amp=0.0f;
        urt_voices[i].pressed=0u;
        urt_voices[i].peak=0.0f;
    }
    urban_rec_track_active_voices=0u;
}
static void urban_rec_track_note(int note,int on,int velocity)
{
    if(note<0||note>127)return;
    if(!on){
        for(unsigned i=0;i<URBAN_REC_TRACK_POLY;i++)
            if(urt_voices[i].pressed && urt_voices[i].note==(uint8_t)note)
                urt_voices[i].pressed=0u;
        return;
    }
    /* Prefer an idle slot, then an almost-silent tail. Voice stealing
     * affects ONLY REC, never an existing song part. */
    unsigned best=0u;
    float quietest=10.0f;
    for(unsigned i=0;i<URBAN_REC_TRACK_POLY;i++) {
        UrbanRecTrackVoice *v=&urt_voices[i];
        if(v->note==(uint8_t)note && v->pressed){best=i;break;}
        if(!v->pressed && v->amp<0.00005f){best=i;break;}
        float score=v->pressed?v->amp+2.0f:v->amp;
        if(score<quietest){quietest=score;best=i;}
    }
    UrbanRecTrackVoice *v=&urt_voices[best];
    v->note=(uint8_t)note;
    v->program=urt_program;
    /* Preserve the 78 program choices as independent REC timbre
     * families. 60..67 are self-contained percussive one-shots,
     * 68..77 are procedural REC SFX; they cannot steal drum lanes. */
    v->kind=(uint8_t)(urt_program<16u?0u:urt_program<32u?1u:
              urt_program<48u?2u:urt_program<60u?3u:
              urt_program<68u?4u:5u);
    v->pressed=(uint8_t)(v->kind<4u);
    v->phase=0.0f;
    v->subphase=0.0f;
    v->frequency=mtof((float)note);
    if(v->kind==0u)v->frequency*=0.5f; /* bass family */
    if(v->frequency>12500.0f)v->frequency=12500.0f;
    if(v->frequency<20.0f)v->frequency=20.0f;
    v->velocity=(velocity>127?127:velocity<1?1:velocity)*(1.0f/127.0f);
    v->amp=0.00001f;
    v->peak=0.0f;
    v->age=0.0f;
    v->attack=1.0f/(EN_SR*(0.002f+0.005f*(float)urt_parameter[URT_ATTACK]));
    v->decay=1.0f/(EN_SR*(0.030f+0.012f*(float)urt_parameter[URT_DECAY]));
    v->release=1.0f/(EN_SR*(0.020f+0.008f*(float)urt_parameter[URT_RELEASE]));
    /* A wide but subtle REC-only stereo spread. */
    v->pan=((int)(best%5u)-2)*0.16f;
    v->noise=(uint32_t)(note+1)*747796405u+((uint32_t)urt_program+1u)*2891336453u;
    urban_rec_track_onsets++;
    urban_rec_track_recount();
}
static void urban_rec_track_render(uint32_t n)
{
    /* Dedicated stereo path: only ADDS to the master mixer. Nothing
     * here modifies existing HYP pads, bass, leads, drums or jazz/SFX. */
    float gain=0.28f*(float)urt_parameter[URT_LEVEL]*0.01f;
    float tone=(float)urt_parameter[URT_TONE]*0.01f;
    float spread=(float)urt_parameter[URT_WIDTH]*0.01f;
    float drive=(float)urt_parameter[URT_DRIVE]*0.01f;
    float sustain=(float)urt_parameter[URT_SUSTAIN]*0.01f;
    float lfo_depth=(float)urt_parameter[URT_LFO_DEPTH]*0.01f;
    float lfo_rate=0.14f+0.11f*(float)urt_parameter[URT_LFO_RATE];
    unsigned lfo_target=urt_parameter[URT_LFO_TARGET]/25u;
    unsigned shape=urt_parameter[URT_LFO_SHAPE]/25u;
    for(unsigned j=0;j<n;j++){
        urt_lfo_phase+=lfo_rate/EN_SR;
        if(urt_lfo_phase>=1.0f)urt_lfo_phase-=1.0f;
        float lfo=sine(urt_lfo_phase);
        if(shape==1u)lfo=urt_lfo_phase<0.5f?
                         4.0f*urt_lfo_phase-1.0f:3.0f-4.0f*urt_lfo_phase;
        else if(shape==2u)lfo=urt_lfo_phase<0.5f?1.0f:-1.0f;
        else if(shape>=3u)lfo=urt_lfo_phase*2.0f-1.0f;
        float left=0.0f,right=0.0f;
        for(unsigned i=0;i<URBAN_REC_TRACK_POLY;i++){
            UrbanRecTrackVoice *v=&urt_voices[i];
            if(v->amp<0.00005f && !v->pressed)continue;
            v->age+=1.0f/EN_SR;
            /* Attack, decay/sustain and release belong only to REC. */
            if(v->pressed || (v->kind>=4u && v->age<0.008f)) {
                if(v->peak<1.0f) {
                    v->peak+=v->attack;
                    if(v->peak>1.0f)v->peak=1.0f;
                }
                if(v->peak>=1.0f) {
                    v->amp+=(sustain-v->amp)*v->decay;
                } else v->amp+=(1.0f-v->amp)*v->attack;
            } else {
                float release=v->kind>=4u ?
                              (v->kind==4u?0.0020f:0.00038f) : v->release;
                v->amp-=v->amp*release;
                if(v->amp<0.00005f){v->amp=0.0f;continue;}
            }
            /* Always use a band-limited-enough low fundamental; the
             * upper harmonics are attenuated by the REC tone knob. */
            float fm=v->kind==3u?0.013f:0.0f;
            float freq=v->frequency;
            if(lfo_target==1u)freq*=1.0f+0.012f*lfo_depth*lfo;
            v->phase+=freq/EN_SR;
            if(v->phase>=1.0f)v->phase-=1.0f;
            v->subphase+=freq*0.502f/EN_SR;
            if(v->subphase>=1.0f)v->subphase-=1.0f;
            float phase=v->phase;
            float osc=sine(phase+fm*sine(v->subphase));
            float second=sine(phase*2.0f);
            float partial=sine(phase*3.0f);
            unsigned variant=v->program%16u;
            if(v->kind==0u) /* bass: fundamental, slight sub and growl */
                osc=osc*0.78f+sine(v->subphase)*0.34f+
                    second*(0.04f+tone*0.12f);
            else if(v->kind==1u) /* lead: sharper harmonics */
                osc=osc*0.65f+second*(0.18f+tone*0.22f)+
                    partial*(0.03f+(float)(variant%4u)*0.025f);
            else if(v->kind==2u || v->kind==3u) /* keys / jazzy bells */
                osc=osc*0.77f+second*(0.06f+tone*0.14f)+
                    sine(phase*2.79f)*(v->kind==3u?0.16f:0.05f);
            else if(v->kind==4u || v->kind==5u) {
                /* Distinct one-shot percussion/SFX, NOT generated drums. */
                v->noise=v->noise*1664525u+1013904223u;
                float nz=(float)(int32_t)v->noise*(1.0f/2147483648.0f);
                osc=osc*(v->kind==4u?0.65f:0.42f)+
                    nz*(v->kind==4u?0.30f:0.50f);
            }
            if(lfo_target==2u)osc*=1.0f+0.25f*lfo*lfo_depth;
            if(lfo_target==0u)osc*=1.0f+0.35f*lfo*lfo_depth;
            /* Bounded soft drive; no unbounded clipping in overdubs. */
            osc=osc*(1.0f+drive*0.8f);
            osc=osc/(1.0f+0.40f*drive*(osc<0.0f?-osc:osc));
            float level=gain*v->velocity*v->amp*osc;
            float pan=v->pan*spread;
            if(lfo_target>=3u)pan+=0.15f*lfo*lfo_depth;
            left+=level*(0.75f-0.25f*pan);
            right+=level*(0.75f+0.25f*pan);
        }
        mixl[j]+=left;
        mixr[j]+=right;
        rsend[j]+=(left+right)*0.07f;
        urban_rec_track_rendered_frames++;
    }
    urban_rec_track_recount();
}
#endif
