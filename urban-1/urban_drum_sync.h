/* SPDX-License-Identifier: GPL-3.0-only
 * A 16th has already sounded when HYP publishes en_bars.
 * Stage the following drum bar early, never in the final three
 * sixteenths where an Android/UI stall could race the DSP bank swap.
 */
#ifndef URBAN_DRUM_SYNC_H
#define URBAN_DRUM_SYNC_H
#include <stdint.h>
#define URBAN_DRUM_STAGE_LAST_STEP 12u
static uint32_t urban_drum_bar_id(uint32_t played_sixteenths) {
    return played_sixteenths ? (played_sixteenths-1u)/16u : 0u;
}
static uint8_t urban_drum_can_stage(uint32_t played_sixteenths) {
    return (uint8_t)(played_sixteenths &&
        ((played_sixteenths-1u)%16u)<=URBAN_DRUM_STAGE_LAST_STEP);
}
static uint8_t urban_drum_should_stage(uint32_t played_sixteenths,
                                      uint32_t staged_for_bar) {
    return (uint8_t)(urban_drum_can_stage(played_sixteenths) &&
        urban_drum_bar_id(played_sixteenths)!=staged_for_bar);
}
#endif
