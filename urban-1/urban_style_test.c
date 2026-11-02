#include "urban_style.h"
#include "urban_rhythm.h"
#include "urban_bass.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
 for(unsigned g=0;g<URBAN_GENRES;g++) {
  const UrbanGenreTimbre t=urban_timbre[g];
  assert(urban_genre_bpm[g]>=70u && urban_genre_bpm[g]<=190u);
  assert(t.bass_sound<16u && t.lead_sound<16u && t.pad_sound<16u);
  assert(t.bass_cut<=100u && t.lead_cut<=100u && t.drive<=100u);
  for(unsigned s=0;s<4;s++) {
   const char *name=urban_style_name(g,s);
   assert(name && strlen(name)>=3u);
   UrbanStylePatch patch=urban_style_patch(g,s);
   assert(patch.bass<16u && patch.lead<16u && patch.pad<16u);
   assert(patch.drum_kit<8u && patch.hat_kit<8u);
   assert(patch.drive<=100u && patch.reverb<=100u && patch.echo<=100u);
   assert(patch.bass_cut<=100u && patch.lead_cut<=100u);
   if(s) {
     UrbanStylePatch base=urban_style_patch(g,0);
     assert(memcmp(&base,&patch,sizeof(base))!=0);
   }
  }
 }
 /* Exhaust the existing HYP-1 instrument bank in playable
  * genre/subgenre assignments: all 16 DRONE, 16 ACID, 16 PAD sounds,
  * and all 8 drum kits. These are genuine HYP sounds, not false names. */
 unsigned heard[5][16]={{0}};
 for(unsigned g=0;g<URBAN_GENRES;g++)
  for(unsigned st=0;st<4u;st++) {
   UrbanStylePatch p=urban_style_patch(g,st);
   heard[0][p.bass]=1;
   heard[1][p.lead]=1;
   heard[2][p.pad]=1;
   heard[3][p.drum_kit]=1;
   heard[4][p.hat_kit]=1;
  }
 for(unsigned i=0;i<16u;i++)
  assert(heard[0][i] && heard[1][i] && heard[2][i]);
 for(unsigned i=0;i<8u;i++)
  assert(heard[3][i] || heard[4][i]);
 assert(urban_groove(0,1).hat!=urban_groove(0,0).hat);
 assert(urban_groove(0,2).hat!=urban_groove(0,0).hat);
 assert(urban_groove(7,1).kick!=urban_groove(7,0).kick);
 assert(urban_groove(8,1).kick!=urban_groove(8,0).kick);
 assert(urban_groove(10,3).hat!=urban_groove(10,0).hat);
 UrbanSong s;UrbanBassSong a,b;
 urban_init(&s,12u);
 s.tracks[2].genre=7u;
 s.tracks[2].subgenre=0;
 urban_generate_bass(&s,122u,&a);
 s.tracks[2].subgenre=1;
 urban_generate_bass(&s,122u,&b);
 assert(memcmp(&a,&b,sizeof(a))!=0);
 puts("URBAN-1 56 named substyles and real HYP DSP palette tests passed");
 return 0;
}
