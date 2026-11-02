/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 phase-locked Trance lead grid, calculated by the UI before
 * playback and consumed on HYP's EXACT drum-sample tick in DSP.
 * This makes onscreen frame cadence irrelevant to lead note timing.
 */
#ifndef URBAN_TRANCE_CLOCK_H
#define URBAN_TRANCE_CLOCK_H
#include <stdint.h>
#define URBAN_TRANCE_SECTIONS 8u
#define URBAN_TRANCE_PHRASE 4u
#define URBAN_TRANCE_STEPS 16u
#define URBAN_TRANCE_ARRANGEMENT 64u
typedef struct {
    uint8_t note,gate;
} UrbanTranceNote;
typedef struct {
    UrbanTranceNote notes[URBAN_TRANCE_SECTIONS][URBAN_TRANCE_PHRASE][URBAN_TRANCE_STEPS];
    uint8_t sequence[URBAN_TRANCE_ARRANGEMENT];
    uint8_t bar[URBAN_TRANCE_ARRANGEMENT];
    uint8_t length,song_mode,loop_queued,enabled;
} UrbanTranceGrid;
void urban_trance_publish(const UrbanTranceGrid *grid);
extern volatile uint32_t urban_trance_audio_onsets;
extern volatile uint32_t urban_trance_audio_last_step;
#endif
