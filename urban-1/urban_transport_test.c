#include "urban_transport.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 UrbanUI ui; UrbanTransport tr; UrbanFrame frame;
 urban_ui_init(&ui,42u); urban_generate_song(&ui.song,123u);
 urban_transport_init(&tr,&ui);
 ui.song.playing=1;
 ui.song.current=0; ui.song.queued=0;
 urban_ui_next_knob(&ui,1);
 for(unsigned i=0;i<URBAN_PPQN*4u-1u;i++){
   urban_transport_tick(&tr,&frame);
   assert(ui.song.current==0);
 }
 urban_transport_tick(&tr,&frame);
 assert(ui.song.current==1 && tr.step==0);
 urban_transport_tick(&tr,&frame);
 assert(frame.sequence==1 && frame.step==0);
 ui.song.playing=0;
 unsigned before=tr.tick;
 urban_transport_tick(&tr,&frame);
 assert(tr.tick==before);
 puts("URBAN-1 transport tests passed");
 return 0;
}
