#include "urban_bass.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
 UrbanSong s; UrbanBassSong a,b,c;
 urban_init(&s,123u);s.key=2;s.tracks[2].genre=0;
 urban_generate_bass(&s,111u,&a);
 urban_generate_bass(&s,111u,&b);
 assert(memcmp(&a,&b,sizeof(a))==0);
 s.tracks[2].genre=8;
 urban_generate_bass(&s,111u,&c);
 assert(memcmp(&a,&c,sizeof(a))!=0);
 for(unsigned q=0;q<URBAN_SEQUENCES;q++)
  for(unsigned k=0;k<URBAN_STEPS;k++){
   UrbanBassStep n=c.seq[q].steps[k];
   assert(n.note>=36u && n.note<=71u);
   assert(n.glide==0u);
  }
 puts("URBAN-1 bass tests passed");
 return 0;
}
