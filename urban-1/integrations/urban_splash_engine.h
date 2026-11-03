/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 Kitsune Neo-Tokyo splash (original widescreen composition).
 * Whole 1672x941 image is letterboxed as 240x136 in the 240x240 LCD:
 * 52px above and below, no horizontal/vertical crop and no stretching.
 * 4 KiB indexed art is stored in firmware flash, with only 60 bytes
 * stack for one row. Reuses existing two LCD canvas bands, not a new
 * full-screen framebuffer. Pure UI; no blocking sleeps or audio work.
 */
#ifndef URBAN_SPLASH_ENGINE_H
#define URBAN_SPLASH_ENGINE_H
#include "urban_splash_art.h"
#define URBAN_SPLASH_IN_MS 180u
#define URBAN_SPLASH_OUT_MS 220u

static unsigned urban_splash_b64_value(char c) {
    if(c>='A' && c<='Z')return (unsigned)(c-'A');
    if(c>='a' && c<='z')return (unsigned)(c-'a'+26);
    if(c>='0' && c<='9')return (unsigned)(c-'0'+52);
    if(c=='+')return 62u;
    if(c=='/')return 63u;
    return 0u;
}
/* Every 80-character row decodes to 60 packed bytes: 120 4bpp pixels.
 * Row-local buffer avoids extra .pool RAM on the constrained FM-1. */
static void urban_splash_decode_row(unsigned row,uint8_t out[60]) {
    const char *source=urban_splash_image_b64+
                       row*URBAN_SPLASH_ART_ROW_B64;
    for(unsigned n=0u;n<20u;n++) {
        unsigned p=n*4u;
        unsigned a=urban_splash_b64_value(source[p]);
        unsigned b=urban_splash_b64_value(source[p+1u]);
        unsigned c=urban_splash_b64_value(source[p+2u]);
        unsigned d=urban_splash_b64_value(source[p+3u]);
        out[n*3u]=(uint8_t)((a<<2u)|(b>>4u));
        out[n*3u+1u]=(uint8_t)((b<<4u)|(c>>2u));
        out[n*3u+2u]=(uint8_t)((c<<6u)|d);
    }
}
/* Intro/outro digital tearing is lightweight. Steady phase 2 is a
 * pixel-exact scaled copy of the ENTIRE original composition, including
 * the left and right borders and original embedded title/prompt.
 * There is no replacement text panel painted over the image. */
static void urban_splash_render_band(unsigned y0,unsigned h,
                                     unsigned phase,unsigned step) {
    unsigned severity=phase==1u?(step>=3u?0u:3u-step):
                      phase==3u?(step>=3u?4u:step+1u):0u;
    cv_begin(240u,h,RGB(8,13,31));
    uint8_t row_pixels[60];
    unsigned decoded_y=URBAN_SPLASH_SOURCE_H;
    for(unsigned y=0u;y<h;y++) {
        unsigned py=y0+y;
        if(py<URBAN_SPLASH_IMAGE_Y ||
           py>=URBAN_SPLASH_IMAGE_Y+URBAN_SPLASH_SOURCE_H*2u)
            continue; /* 52px letterbox on both edges; no crop */
        unsigned art_y=(py-URBAN_SPLASH_IMAGE_Y)>>1u;
        if(art_y!=decoded_y) {
            urban_splash_decode_row(art_y,row_pixels);
            decoded_y=art_y;
        }
        int shift=0;
        if(severity &&
           ((((py-URBAN_SPLASH_IMAGE_Y)>>2u)+step*7u)&15u)<severity*2u)
            shift=(int)((py*13u+step*11u)%15u)-7;
        uint16_t *dst=&cv_px[y*240u];
        for(unsigned x=0u;x<240u;x++) {
            int sx=(int)x+shift;
            if(sx<0)sx=0;
            if(sx>239)sx=239;
            uint8_t packed=row_pixels[(unsigned)sx>>2u];
            unsigned col=((unsigned)sx&2u)?(packed&15u):(packed>>4u);
            dst[x]=swap16(urban_splash_palette[col]);
        }
    }
    if(severity)for(unsigned i=0u;i<severity;i++) {
        unsigned line=URBAN_SPLASH_IMAGE_Y+
                    (i*37u+step*29u+phase*13u)%
                    (URBAN_SPLASH_SOURCE_H*2u);
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
