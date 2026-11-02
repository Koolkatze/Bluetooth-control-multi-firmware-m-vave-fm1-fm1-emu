/* SPDX-License-Identifier: GPL-3.0-only
 * Audio-sample-spaced chord attacks for real pianist-like articulation.
 * Include AFTER note_cmd() in HYP's DSP. No malloc, no external samples;
 * ambient styles never arm this and keep unrolled pad envelopes.
 */
#ifndef URBAN_PIANO_ROLL_ENGINE_H
#define URBAN_PIANO_ROLL_ENGINE_H
typedef struct {
    uint8_t pitches[4],count,gap_ms;
} UrbanPianoRoll;
static UrbanPianoRoll urban_roll_buffers[2];
static volatile uint8_t urban_roll_front;
static volatile uint32_t urban_roll_revision;
static uint32_t urban_roll_seen;
static uint8_t urban_roll_position,urban_roll_count;
static uint32_t urban_roll_until;
static UrbanPianoRoll urban_roll_playing;
volatile uint32_t urban_piano_note_onsets;
volatile uint32_t urban_piano_rolled_chords;

void urban_piano_roll_arm(const uint8_t *notes,unsigned count,unsigned gap_ms) {
    unsigned back=(unsigned)urban_roll_front^1u;
    UrbanPianoRoll *buf=&urban_roll_buffers[back];
    if(count>4u)count=4u;
    if(gap_ms>40u)gap_ms=40u;
    for(unsigned i=0;i<4u;i++)
        buf->pitches[i]=i<count?notes[i]:0u;
    buf->count=(uint8_t)count;
    buf->gap_ms=(uint8_t)gap_ms;
    BARRIER();
    urban_roll_front=(uint8_t)back;
    BARRIER();
    urban_roll_revision++;
}
void urban_piano_roll_stop(void) {
    urban_piano_roll_arm(0,0u,0u);
}
static void urban_roll_receive(void) {
    uint32_t rev=urban_roll_revision;
    if(rev==urban_roll_seen)return;
    urban_roll_seen=rev;
    urban_roll_playing=urban_roll_buffers[urban_roll_front];
    urban_roll_count=urban_roll_playing.count;
    urban_roll_position=0u;urban_roll_until=0u;
    if(!urban_roll_count)return;
    note_cmd(EN_V_PAD,urban_roll_playing.pitches[0],1);
    urban_piano_note_onsets++;
    urban_roll_position=1u;
    if(urban_roll_count>1u) {
        uint32_t delay=(EN_SR*(uint32_t)urban_roll_playing.gap_ms)/1000u;
        urban_roll_until=delay?delay:1u;
        urban_piano_rolled_chords++;
    }
}
/* The native FM-1 render loop splits its sample chunk at this pending
 * piano press, as it already does for scheduled drum wait events. */
static uint32_t urban_roll_wait(void) {
    return urban_roll_position<urban_roll_count?urban_roll_until:0u;
}
static void urban_roll_advance(uint32_t n) {
    if(urban_roll_position>=urban_roll_count)return;
    if(n<urban_roll_until) {
        urban_roll_until-=n;
        return;
    }
    note_cmd(EN_V_PAD,urban_roll_playing.pitches[urban_roll_position++],1);
    urban_piano_note_onsets++;
    if(urban_roll_position<urban_roll_count) {
        uint32_t delay=(EN_SR*(uint32_t)urban_roll_playing.gap_ms)/1000u;
        urban_roll_until=delay?delay:1u;
    } else urban_roll_until=0u;
}
#endif
