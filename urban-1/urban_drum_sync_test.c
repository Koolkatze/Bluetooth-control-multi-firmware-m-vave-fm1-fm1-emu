#include "urban_drum_sync.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    assert(urban_drum_bar_id(0u)==0u);
    assert(urban_drum_bar_id(1u)==0u);
    assert(urban_drum_bar_id(16u)==0u);
    assert(urban_drum_bar_id(17u)==1u);
    assert(urban_drum_bar_id(33u)==2u);
    assert(!urban_drum_can_stage(0u));
    for(uint32_t bar=0;bar<4u;bar++)
        for(uint32_t step=0;step<16u;step++) {
            uint32_t played=bar*16u+step+1u;
            assert(urban_drum_bar_id(played)==bar);
            assert(urban_drum_can_stage(played)==(step<=12u));
            assert(urban_drum_should_stage(played,UINT32_MAX)==(step<=12u));
            assert(!urban_drum_should_stage(played,bar));
        }
    /* UI frame missed the downbeat: recover at step 4, not at step 15.
       Never queue/overwrite patterns adjacent to a DSP bank switch. */
    assert(urban_drum_should_stage(21u,0u));
    assert(!urban_drum_should_stage(32u,0u));
    assert(urban_drum_should_stage(33u,0u));
    puts("URBAN-1 drum bank synchronisation tests passed");
    return 0;
}
