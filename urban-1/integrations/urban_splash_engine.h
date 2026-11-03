/* SPDX-License-Identifier: GPL-3.0-only
 * URBAN-1 / pixel-art Kitsune boot cover.
 * Two 240px-wide canvas transfers avoid a full-frame LCD buffer.
 * Uses a 7200-byte packed 4bpp palette image (120x120, scaled 2x),
 * decoded 60 bytes per row from flash. Only the UI thread touches it;
 * neither the DSP audio interrupt nor the MIDI queue is involved.
 */
#ifndef URBAN_SPLASH_ENGINE_H
#define URBAN_SPLASH_ENGINE_H
#include "urban_splash_art.h"
#define URBAN_SPLASH_W 120u
#define URBAN_SPLASH_H 120u
#define URBAN_SPLASH_PACKED_BYTES 7200u
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
/* Exactly 80 Base64 characters per 120px source row (60 packed bytes).
 * Decode only the row currently being drawn into 60B of UI stack:
 * zero new persistent RAM and no allocations from the constrained .pool.
 * Each source row is decoded once for two scaled LCD raster rows. */
static void urban_splash_decode_row(unsigned row,uint8_t out[60]) {
    const char *source=urban_splash_image_b64+row*80u;
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
/* phase 1: reveal with horizontal digital tearing; phase 2: steady;
 * phase 3: tear out into HOME. 4-5 bounded frames per transition. */
static void urban_splash_render_band(unsigned y0,unsigned h,
                                     unsigned phase,unsigned step) {
    unsigned severity=0u;
    if(phase==1u)severity=step>=3u?0u:3u-step;
    else if(phase==3u)severity=step>3u?3u:step;
    cv_begin(240u,h,RGB(8,13,31));
    uint8_t row_pixels[60];
    unsigned decoded_y=URBAN_SPLASH_H;
    for(unsigned y=0u;y<h;y++) {
        unsigned abs_y=y0+y;
        unsigned art_y=abs_y>>1u;
        if(art_y!=decoded_y) {
            urban_splash_decode_row(art_y,row_pixels);
            decoded_y=art_y;
        }
        const uint8_t *src=row_pixels;
        int shift=0;
        if(severity && (((abs_y>>2u)+step*7u)&15u)<severity*2u)
            shift=(int)(((abs_y>>2u)*13u+step*11u)%23u)-11;
        uint16_t *dst=&cv_px[y*240u];
        for(unsigned x=0u;x<240u;x++) {
            int sx=(int)x+shift;
            if(sx<0)sx=0;
            if(sx>239)sx=239;
            uint8_t packed=src[(unsigned)sx>>2u];
            unsigned index=((unsigned)sx&2u)?packed&15u:packed>>4u;
            dst[x]=swap16(urban_splash_palette[index]);
        }
    }
    /* Repaint the obsolete v0.4.11 bitmap's cropped title panel with
     * current firmware-neutral branding. The fox itself is unmodified. */
    if(y0==0u) {
        cv_rect(0,0,137,29,RGB(8,14,35));
        cv_text(7,4,&FONT_B,"URBAN-1",RGB(255,106,82));
        cv_rect(0,30,137,2,RGB(244,76,105));
    } else {
        cv_rect(39,205-144,163,27,RGB(8,14,35));
        cv_rect(39,205-144,163,1,RGB(252,91,110));
        cv_text(62,212-144,&FONT_S,"PRESS PLAY",RGB(255,174,147));
    }
    if(severity) {
        /* Sparse RGB-split-like interference, not a full-screen strobe. */
        for(unsigned i=0u;i<severity;i++) {
            unsigned line=(i*71u+step*29u+phase*13u)%240u;
            if(line>=y0 && line<y0+h)
                cv_rect(0,(int)(line-y0),240,1u,
                        i&1u?RGB(66,202,244):RGB(254,74,123));
        }
    }
    cv_blit(0u,y0);
}
static void urban_splash_render(unsigned phase,unsigned step) {
    urban_splash_render_band(0u,144u,phase,step);
    urban_splash_render_band(144u,96u,phase,step);
    lcd_sync();
}
#endif
