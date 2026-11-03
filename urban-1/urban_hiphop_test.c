/* SPDX-License-Identifier: GPL-3.0-only
 * Hip-hop realism guardrails. Musical constraints, not a perceptual
 * listening test: sustain and warm registered patches, grounded bass,
 * stable snare anchors, no surprise claps/kicks or high-pitched chip leads.
 */
#include "urban_fx.h"
#include "urban_chord_score.h"
#include "urban_runtime.h"
#include <assert.h>
#include <stdio.h>

static void hiphop(unsigned genre,unsigned sub,unsigned seed) {
    UrbanSong s;
    UrbanBassSong bass;
    urban_init(&s,seed);
    s.genre=(uint8_t)genre;
    s.subgenre=(uint8_t)sub;
    s.complexity=3u;
    for(unsigned t=0;t<URBAN_TRACKS;t++) {
        s.tracks[t].genre=(uint8_t)genre;
        s.tracks[t].subgenre=(uint8_t)sub;
    }
    urban_generate_song(&s,seed+1u);
    UrbanGroove g=urban_groove((uint8_t)genre,(uint8_t)sub);
    assert(g.snare==0x1010u);
    assert(!(g.clap&0x0010u));
    assert(g.kit==4u || (genre==3u && sub==2u && g.kit==7u));
    /* Kit 4 is not merely similar: it is the same Bristol Trip-hop kick.
     * G-Funk keeps the 808 *snare*, not the harsh 808 kick. */
    assert(urban_kick_kit(genre,sub)==urban_style_patch(4u,0u).drum_kit);
    if(genre==3u && sub==0u) {
        assert(g.clap==0u);
        assert(urban_style_patch(genre,sub).drum_kit==4u);
    }
    UrbanInstrumentProgram ins=urban_instrument_program(&s,2u,URBAN_HOOK);
    UrbanInstrumentProgram leads=urban_instrument_program(&s,3u,URBAN_HOOK);
    UrbanInstrumentProgram pad=urban_instrument_program(&s,4u,URBAN_HOOK);
    assert(ins.patch.bass!=6u && ins.patch.bass!=12u
           && ins.patch.bass!=13u && ins.patch.bass!=14u);
    assert(leads.patch.lead==15u || leads.patch.lead==12u
           || leads.patch.lead==5u);
    assert(pad.patch.pad==0u || pad.patch.pad==4u || pad.patch.pad==5u);
    UrbanBassGrammar bg=urban_bass_style(&s.tracks[2]);
    assert(bg.octave==0u && bg.gate>=6u);
    urban_generate_bass(&s,s.tracks[2].seq[0].seed,&bass);
    for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
      for(unsigned st=0;st<URBAN_STEPS;st++) {
        UrbanBassStep n=bass.seq[sec].steps[st];
        if(n.velocity) {
            assert(n.note>=36u && n.note<=58u);
            assert(n.gate>=6u && n.gate<=16u);
        }
      }
    for(unsigned bar=0;bar<4u;bar++) {
        uint16_t mask[6],again[6];
        urban_drum_lanes(&s,URBAN_HOOK,bar,mask);
        urban_drum_lanes(&s,URBAN_HOOK,bar,again);
        for(unsigned lane=0;lane<6u;lane++)assert(mask[lane]==again[lane]);
        assert((mask[1]&0x1010u)==0x1010u);
        assert((mask[2]&0x0010u)==0u);
        if(genre==3u && sub==0u) {
            for(unsigned section=0u;section<URBAN_SEQUENCES;section++)
              for(unsigned complexity=0u;complexity<=3u;complexity++) {
                s.complexity=(uint8_t)complexity;
                urban_drum_lanes(&s,section,bar,mask);
                assert(mask[2]==0u); /* no clap on any section/fill */
                assert((mask[1]&0x1010u)==0x1010u ||
                       section==URBAN_INTRO ||
                       section==URBAN_BRIDGE ||
                       section==URBAN_BREAK ||
                       section==URBAN_OUTRO);
              }
            s.complexity=3u;
        }
        /* HYP's random break-kit chop has been disabled in the port. */
        UrbanChordScore cs=urban_chord_score(&s,URBAN_HOOK,bar);
        assert(cs.gate>=6u && cs.onsets);
        assert((cs.onsets&0x0001u)!=0u);
        UrbanMixPlan mx=urban_mix_plan(&s,URBAN_HOOK,bar);
        assert(mx.snare>mx.clap);
        assert(mx.kick==(genre==3u && sub==3u?69u:70u));
        assert(mx.drive==(genre==3u && sub==3u?22u:8u));
        if(genre==3u && sub==0u)assert(mx.clap==0u);
        /* The drum-track override must work when the song and the
         * other tracks use a completely different global genre. */
        UrbanSong mixed=s;
        mixed.genre=10u;
        mixed.subgenre=1u;
        UrbanMixPlan override=urban_mix_plan(&mixed,URBAN_HOOK,bar);
        assert(override.kick==mx.kick);
        assert(override.drive==mx.drive);
        if(genre==3u && sub==0u)assert(override.clap==0u);
        assert(mx.hat<=35u);
        assert(mx.lead<=53u);
        assert(urban_mix_energy(&mx)<=15500u);
    }
    UrbanFxPlan fx=urban_fx_plan(&s);
    assert(fx.pad_lfo_depth<=17u);
    assert(fx.bass_lfo_depth<=12u);
    assert(fx.lead_resonance<=20u);
    UrbanSong reference=s;
    reference.genre=4u; /* Bristol Trip-hop */
    reference.subgenre=0u;
    reference.tracks[0].genre=4u;
    reference.tracks[0].subgenre=0u;
    UrbanFxPlan trip_fx=urban_fx_plan(&reference);
    UrbanMixPlan trip_mix=urban_mix_plan(&reference,URBAN_HOOK,0u);
    if(genre==3u && sub==3u) {
        assert(fx.kick_decay>trip_fx.kick_decay);
        assert(fx.drive>trip_fx.drive);
        assert(urban_mix_plan(&s,URBAN_HOOK,0u).kick<trip_mix.kick);
    } else {
        assert(fx.kick_decay==trip_fx.kick_decay);
        assert(fx.drive<trip_fx.drive);
        assert(urban_mix_plan(&s,URBAN_HOOK,0u).kick==trip_mix.kick);
    }
    if(genre==3u && sub==2u)assert(fx.lead_slide>=50u);
    for(unsigned sec=0;sec<URBAN_SEQUENCES;sec++)
      for(unsigned bar=0;bar<4u;bar++)
        for(unsigned st=0;st<16u;st++) {
            UrbanNote input={.note=60u,.velocity=0u,.gate=4u};
            UrbanNote out=urban_lead_score(&s,sec,bar,st,input);
            if(out.velocity)assert(out.note>=48u && out.note<=76u);
        }
}
int main(void) {
    for(unsigned g=3u;g<=5u;g+=2u)
      for(unsigned sub=0u;sub<4u;sub++)
        for(unsigned take=0u;take<16u;take++)
            hiphop(g,sub,0x1001u+g*1000u+sub*71u+take*31u);
    puts("URBAN-1: 128 takes, genuine Trip-hop kicks, isolated softer lo-fi kick/drive and Boom Bap zero-clap in 8 sections PASS");
    return 0;
}
