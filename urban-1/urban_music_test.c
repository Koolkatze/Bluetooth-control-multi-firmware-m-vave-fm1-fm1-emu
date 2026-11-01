#include "urban_music.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
 UrbanSong song; UrbanVoice a,b,c;
 urban_init(&song,123u); song.key=5;
 song.tracks[3].genre=0;
 urban_generate_voice(&song,3,999u,&a);
 urban_generate_voice(&song,3,999u,&b);
 assert(memcmp(&a,&b,sizeof(a))==0);
 urban_generate_voice(&song,3,1000u,&c);
 assert(memcmp(&a,&c,sizeof(a))!=0);
 unsigned sounding=0;
 for(unsigned s=0;s<URBAN_SEQUENCES;s++)
  for(unsigned k=0;k<URBAN_STEPS;k++){
   UrbanNote n=a.sequence[s].steps[k];
   assert(n.note>=48 && n.note<=95);
   if(n.velocity) sounding++;
  }
 assert(sounding>0);
 puts("URBAN-1 music generator tests passed");
 return 0;
}
