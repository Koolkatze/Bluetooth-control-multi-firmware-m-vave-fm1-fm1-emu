#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 UrbanRuntime rt; UrbanMidiFrame f;
 urban_runtime_init(&rt,0x9876u);
 UrbanBassSong bass_before=rt.bass;
 UrbanVoice before[URBAN_TRACKS];
 memcpy(before,rt.voices,sizeof(before));
 urban_ui_page(&rt.ui,URBAN_PAGE_TRACKS);
 urban_ui_select_track(&rt.ui,3);
 urban_runtime_generate(&rt);
 for(unsigned i=0;i<URBAN_TRACKS;i++)
  if(i!=3) assert(memcmp(&before[i],&rt.voices[i],sizeof(UrbanVoice))==0);
 assert(memcmp(&before[3],&rt.voices[3],sizeof(UrbanVoice))!=0);
 assert(memcmp(&bass_before,&rt.bass,sizeof(bass_before))==0);
 urban_ui_select_track(&rt.ui,2);
 urban_runtime_generate(&rt);
 assert(memcmp(&bass_before,&rt.bass,sizeof(bass_before))!=0);
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
 UrbanBassStep expected=urban_bass_bar_score(&rt.ui.song,&rt.bass,
     rt.ui.song.current,urban_phrase_bar(&rt.ui.song),0u);
 assert(f.event[2].note==urban_harmonize_note(&rt.ui.song,
                                            expected.note,0u));
 /* A transport restart must never inherit the previous off-grid step. */
 rt.transport.tick=URBAN_TICKS_PER_STEP*7u+9u;
 rt.transport.step=7u;
 rt.ui.song.song_mode=1u;
 rt.ui.song.arrangement_pos=19u;
 rt.ui.song.section_bar=7u;
 urban_runtime_reset_clock(&rt);
 assert(rt.transport.tick==0u && rt.transport.step==0u);
 assert(rt.ui.song.arrangement_pos==0u);
 assert(rt.ui.song.section_bar==0u);
 assert(rt.ui.song.current==rt.ui.song.arrangement[0]);
 rt.ui.song.song_mode=0u;
 rt.ui.song.queued=URBAN_DROP;
 rt.ui.song.section_bar=3u;
 rt.transport.tick=99u;rt.transport.step=4u;
 urban_runtime_reset_clock(&rt);
 assert(rt.transport.tick==0u && rt.transport.step==0u);
 assert(rt.ui.song.current==URBAN_DROP);
 assert(rt.ui.song.section_bar==0u);
 puts("URBAN-1 runtime integration tests passed");
 return 0;
}
