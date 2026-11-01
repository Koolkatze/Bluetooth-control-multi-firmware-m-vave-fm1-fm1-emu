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
  }
 }
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
