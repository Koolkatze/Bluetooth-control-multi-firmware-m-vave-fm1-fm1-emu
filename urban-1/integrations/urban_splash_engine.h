/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 Kitsune/rider Neo-Tokyo splash, native square LCD composition.
 * Use EXACT user-approved 1254x1254 artwork mapped to 240x240, with no
 * crop, blank bands, altered graphic or runtime PNG decoder. 64-colour
 * six-bit indexed art stays in read-only firmware; no extra framebuffer.
 * Glitches operate only in the UI thread and leave sound/MIDI untouched.
 */
#ifndef URBAN_SPLASH_ENGINE_H
#define URBAN_SPLASH_ENGINE_H
#include "urban_splash_art.h"
#define URBAN_SPLASH_IN_MS 180u
#define URBAN_SPLASH_HOLD_MS 1200u
#define URBAN_SPLASH_OUT_MS 180u

/* A 6-bit colour index is stored in one ASCII character. No row expansion
 * on the stack and no 57.6 kB decompression buffer in SRAM. */
static unsigned urban_splash_index(char c) {
    if(c>='A' && c<='Z')return (unsigned)(c-'A');
    if(c>='a' && c<='z')return (unsigned)(c-'a'+26);
    if(c>='0' && c<='9')return (unsigned)(c-'0'+52);
    if(c=='+')return 62u;
    if(c=='/')return 63u;
    return 0u;
}

/* Three display states: 1 glitch-in, 2 perfectly stable original image,
 * 3 glitch-out. Digital tearing shifts only a few selected scanlines;
 * there are no overlays in the steady state. */
static void urban_splash_render_band(unsigned y0,unsigned h,
                                     unsigned phase,unsigned step) {
    unsigned severity=phase==1u?(step>=3u?0u:3u-step):
                      phase==3u?(step>=3u?4u:step+1u):0u;
    cv_begin(240u,h,RGB(8,13,31));
    for(unsigned y=0u;y<h;y++) {
        unsigned py=y0+y;
        const char *row=urban_splash_image_b64+py*URBAN_SPLASH_ART_ROW_B64;
        int shift=0;
        if(severity && (((py>>2u)+step*7u)&15u)<severity*2u)
            shift=(int)((py*13u+step*11u)%15u)-7;
        uint16_t *dst=&cv_px[y*240u];
        for(unsigned x=0u;x<240u;x++) {
            int sx=(int)x+shift;
            if(sx<0)sx=0;
            if(sx>239)sx=239;
            dst[x]=swap16(urban_splash_palette[urban_splash_index(row[sx])]);
        }
    }
    if(severity)for(unsigned i=0u;i<severity;i++) {
        unsigned line=(i*37u+step*29u+phase*13u)%240u;
        if(line>=y0 && line<y0+h)
            cv_rect(0,(int)(line-y0),240,1,
                    i&1u?RGB(66,202,244):RGB(254,74,123));
    }
    cv_blit(0u,y0);
}
static void urban_splash_render(unsigned phase,unsigned step) {
    urban_splash_render_band(0u,144u,phase,step);
    urban_splash_render_band(144u,96u,phase,step);
    lcd_sync();
}
#endif