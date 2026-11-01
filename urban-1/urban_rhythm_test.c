#include "urban_rhythm.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 UrbanGroove trap=urban_groove(0,0), dembow=urban_groove(7,0);
 UrbanGroove dnb=urban_groove(8,0), techno=urban_groove(10,0);
 assert(trap.kick!=dembow.kick);
 assert(dnb.hat!=trap.hat);
 assert(techno.kick==0x1111u);
 assert(urban_drum_note(0,0,8)==38u);
 assert(urban_drum_note(0,1,8)==42u);
 uint32_t r=42u;
 assert(urban_drum_hit(10,0,0,0,1,&r)==1u);
 puts("URBAN-1 rhythm tests passed");
 return 0;
}
