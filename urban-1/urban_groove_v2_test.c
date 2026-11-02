/* SPDX-License-Identifier: GPL-3.0-only
 * New groove architecture regression. This test intentionally checks
 * the REAL four-bar bass score rather than just its one-bar template.
 * It verifies 14x4 groove independence, deterministic variation,
 * expressive jazz-roll policy and unchanged ambient block chords.
 */
#include "urban_bass.h"
#include "urban_chord_score.h"
#include "urban_rhythm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t mix(uint32_t h,unsigned x){return (h^x)*16777619u;}
static uint32_t signature_bass(const UrbanSong *s,const UrbanBassSong *score) {
    uint32_t hash=2166136261u;
    for(unsigned bar=0;bar<4u;bar++)
      for(unsigned step=0;step<16u;step++) {
        UrbanBassStep n=urban_bass_bar_score(s,score,URBAN_HOOK,bar,step);
        hash=mix(hash,n.velocity?((unsigned)n.note<<8)|n.velocity:0u);
        hash=mix(hash,n.velocity?n.gate:0u);
      }
    return hash;
}
static unsigned pop(uint16_t s) {
    unsigned n=0u;while(s){n+=s&1u;s>>=1u;}return n;
}
int main(void) {
    unsigned differences=0u,bar_variation=0u,jazz=0u,ambient=0u;
    for(unsigned genre=0u;genre<URBAN_GENRES;genre++) {
        uint32_t signatures[4]={0};
        for(unsigned sub=0u;sub<4u;sub++)
        for(unsigned take=0u;take<16u;take++) {
            UrbanSong s;
            UrbanBassSong bass;
            urban_init(&s,0x52a1u+(uint32_t)take*59u);
            s.genre=(uint8_t)genre;s.subgenre=(uint8_t)sub;
            s.complexity=3u;
            for(unsigned i=0u;i<URBAN_TRACKS;i++){
                s.tracks[i].genre=(uint8_t)genre;
                s.tracks[i].subgenre=(uint8_t)sub;
            }
            urban_generate_song(&s,0x4ab0u+take*109u+genre*457u+sub*71u);
            urban_generate_bass(&s,s.tracks[2].seq[0].seed,&bass);
            UrbanBassSong again;
            urban_generate_bass(&s,s.tracks[2].seq[0].seed,&again);
            assert(memcmp(&bass,&again,sizeof bass)==0);
            uint32_t sig=signature_bass(&s,&bass);
            if(take==0u)signatures[sub]=sig;
            uint16_t authored[4];
            for(unsigned bar=0;bar<4u;bar++) {
                authored[bar]=urban_bass_bar_mask(&s.tracks[2],URBAN_HOOK,bar);
                unsigned found=0u;
                for(unsigned step=0;step<16u;step++) {
                    UrbanBassStep n=urban_bass_bar_score(&s,&bass,URBAN_HOOK,bar,step);
                    UrbanBassStep repeat=urban_bass_bar_score(&s,&bass,URBAN_HOOK,bar,step);
                    assert(memcmp(&n,&repeat,sizeof n)==0);
                    assert((n.velocity!=0u)==((authored[bar]>>step)&1u));
                    if(n.velocity) {
                        assert(n.note>=36u && n.note<=71u);
                        assert(n.gate>=1u && n.gate<=16u);
                        assert(n.glide<=1u);
                        found++;
                    }
                }
                assert(found==pop(authored[bar]));
            }
            if(authored[0]!=authored[1] || authored[1]!=authored[2] ||
               authored[2]!=authored[3])bar_variation++;
            UrbanChordExpression e=urban_chord_expression(&s,URBAN_HOOK,0u);
            assert(e.spacing_ms<=40u);
            if((genre==3u&&sub==1u)||(genre==5u&&sub==1u)||
               (genre==13u&&sub==2u)||(genre==2u&&sub==1u)) {
                assert(e.touch==URBAN_CH_TOUCH_UP || e.touch==URBAN_CH_TOUCH_DOWN);
                assert(e.spacing_ms>=14u && e.spacing_ms<=24u);
                UrbanChordScore chord=urban_chord_score(&s,URBAN_HOOK,0u);
                assert(chord.onsets==1u && chord.gate>=14u);
                jazz++;
            }
            if((genre==2u&&sub==2u)||(genre==13u&&sub==1u)||
               (genre==1u&&sub==0u)) {
                assert(e.touch==URBAN_CH_TOUCH_BLOCK && e.spacing_ms==0u);
                ambient++;
            }
            UrbanGroove g=urban_groove((uint8_t)genre,(uint8_t)sub);
            assert(g.kick || g.snare);
            for(unsigned bar=0u;bar<4u;bar++){
                uint16_t a[6],repeat[6];
                urban_drum_lanes(&s,URBAN_HOOK,bar,a);
                urban_drum_lanes(&s,URBAN_HOOK,bar,repeat);
                assert(memcmp(a,repeat,sizeof a)==0);
                if(genre==3u && sub==0u) {
                    assert(a[2]==0u);
                    assert((a[1]&0x1010u)==0x1010u);
                }
                /* Never force universally-doubled claps in other styles. */
                if(genre==3u || genre==5u)
                    assert((a[2]&0x0010u)==0u);
            }
        }
        for(unsigned i=0;i<4u;i++)for(unsigned j=i+1u;j<4u;j++){
            assert(signatures[i]!=signatures[j]);
            differences++;
        }
    }
    assert(differences==84u && bar_variation>400u && jazz>=64u && ambient>=48u);
    printf("URBAN: 896 score takes, 56 four-bar bass lines, 84 subgenre contrasts, expressive chords and drum pockets PASS\n");
    return 0;
}
