#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 UrbanRuntime rt; UrbanMidiFrame f;
 urban_runtime_init(&rt,0x9876u);
 UrbanVoice before[URBAN_TRACKS];
 memcpy(before,rt.voices,sizeof(before));
 urban_ui_page(&rt.ui,URBAN_PAGE_TRACKS);
 urban_ui_select_track(&rt.ui,3);
 urban_runtime_generate(&rt);
 for(unsigned i=0;i<URBAN_TRACKS;i++)
  if(i!=3) assert(memcmp(&before[i],&rt.voices[i],sizeof(UrbanVoice))==0);
 assert(memcmp(&before[3],&rt.voices[3],sizeof(UrbanVoice))!=0);
 rt.ui.song.playing=1;
 rt.ui.song.current=0; rt.ui.song.queued=0;
 urban_ui_page(&rt.ui,URBAN_PAGE_SONG);
 urban_ui_next_knob(&rt.ui,2);
 for(unsigned i=0;i<384u;i++) {
  urban_runtime_tick(&rt,&f);
  assert(f.sequence==0);
 }
 assert(rt.ui.song.current==2);
 urban_runtime_tick(&rt,&f);
 assert(f.sequence==2);
 puts("URBAN-1 runtime integration tests passed");
 return 0;
}
