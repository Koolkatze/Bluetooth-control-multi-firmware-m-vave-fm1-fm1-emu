#!/usr/bin/env python3
"""Apply URBAN-1 to a local checkout of HYP-1, without editing the original repo.

HYP-1 revision validated for integration layout:
57c456db61826331d356458c9c1a136517da7cbf

Usage:
  git clone https://github.com/RustyPawz/fm1-HYP1.git hyp1
  python3 urban-1/tools/apply_hyp1.py --hyp-root hyp1
  cd hyp1 && sh host/build_host.sh
Requires GPL-3.0 attribution when redistributing a derived firmware.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil

HERE = Path(__file__).resolve().parents[1]

def replace_once(path: Path, old: str, new: str) -> bool:
    content = path.read_text()
    if new in content:
        return False
    count = content.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one integration anchor, got {count}: {old!r}")
    path.write_text(content.replace(old, new))
    return True

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hyp-root", type=Path, required=True)
    args = parser.parse_args()
    root = args.hyp_root.resolve()
    expected = [
        root / "firmware/src/main.c",
        root / "firmware/src/app/main_fm1.c",
        root / "firmware/src/dsp/amb.h",
        root / "host/amb_host.c",
    ]
    if not all(p.is_file() for p in expected):
        raise SystemExit("Expected an unpacked RustyPawz/fm1-HYP1 checkout")
    dst = root / "firmware/src/app/urban"
    dst.mkdir(parents=True, exist_ok=True)
    for source in HERE.glob("urban_*.h"):
        shutil.copyfile(source, dst / source.name)
    shutil.copyfile(HERE / "integrations/urban_editor.h",
                    root / "firmware/src/app/urban_editor.h")
    shutil.copyfile(HERE / "integrations/urban_hyp_port.h",
                    root / "firmware/src/app/urban_hyp_port.h")
    shutil.copyfile(HERE / "integrations/urban_sfx_engine.h",
                    root / "firmware/src/app/urban_sfx_engine.h")
    shutil.copyfile(HERE / "integrations/urban_trance_clock.h",
                    root / "firmware/src/app/urban_trance_clock.h")
    shutil.copyfile(HERE / "integrations/urban_trance_engine.h",
                    root / "firmware/src/app/urban_trance_engine.h")
    shutil.copyfile(HERE / "integrations/urban_piano_roll_engine.h",
                    root / "firmware/src/app/urban_piano_roll_engine.h")
    shutil.copyfile(HERE / "integrations/urban_jazz_engine.h",
                    root / "firmware/src/app/urban_jazz_engine.h")
    shutil.copyfile(HERE / "integrations/urban_echo_engine.h",
                    root / "firmware/src/app/urban_echo_engine.h")
    shutil.copyfile(HERE / "integrations/urban_tape_transport.h",
                    root / "firmware/src/app/urban_tape_transport.h")
    # HYP-1's Break kit applies random ghost kicks/snares and 32nd-roll
    # retriggers *after* reading the six authored pattern lanes. These
    # unscored hits sound like glitches in URBAN-1, where all fills and
    # ratchets must come from the subgenre's explicit drum score.
    # HYP Break has a permanently crushed (0.6) and overdriven (2.5)
    # kick even when P_DRIVE=0. URBAN's ordinary hip-hop explicitly
    # chooses clean Break kick tone and envelope. Lo-fi alone may
    # select the mild distortion path with P_DRIVE > 35.
    dsp = root / "firmware/src/dsp/amb.c"
    replace_once(dsp,
        'const float chop = KK->chop * (0.4f + 0.6f * evolve);',
        'const float chop = 0.0f; /* URBAN: no stochastic kit retriggers */')
    replace_once(dsp,
        'const float dk = drive_k * KK->k_drive, drop = KK->k_drop, cka = KK->k_click, rum = KK->k_rumble;',
        'const float dk = drive_k * (KK == &KITS[4] ? (par[P_DRIVE] <= 35 ? 0.65f : 0.90f) : KK->k_drive), drop = KK->k_drop, cka = (KK == &KITS[4] && par[P_DRIVE] <= 35 ? 0.16f : KK->k_click), rum = KK->k_rumble;')
    replace_once(dsp,
        '    if (KK->crush > 0.0f) {                      /* breakcore: the drums crushed (what they add, held and stepped) */',
        '    const float urban_crush = (KK == &KITS[4] && par[P_DRIVE] <= 35) ? 0.0f : KK->crush;\n    if (urban_crush > 0.0f) { /* deliberate lo-fi/breakcore crusher */')
    replace_once(dsp,
        '(uint32_t)(KK->crush * 4.0f)',
        '(uint32_t)(urban_crush * 4.0f)')
    replace_once(dsp,
        'const float c = KK->crush, q = 24.0f;',
        'const float c = urban_crush, q = 24.0f;')
    # Real Jazz instrument voices in the same HYP audio callback.
    # HYP's note_cmd and scheduled chord rolls trigger these at sample precision.
    replace_once(dsp,
        'static void note_cmd(int voice, int key, int on)',
        'static void urban_jazz_note_cmd(int voice,int key,int on);\n'
        'static void urban_jazz_quiet(void);\n'
        'static void note_cmd(int voice, int key, int on)')
    replace_once(dsp,
        '    if (voice == EN_V_PAD) {\n        pad_play(key, on);',
        '    urban_jazz_note_cmd(voice,key,on);\n'
        '    if (voice == EN_V_PAD) {\n        pad_play(key, on);')
    replace_once(dsp,
        '    acid.amp = acid.amp_t = 0.0f;\n    notes_off();\n}',
        '    acid.amp = acid.amp_t = 0.0f;\n    urban_jazz_quiet();\n    notes_off();\n}')
    replace_once(dsp,
        'static float mixl[BLK], mixr[BLK], rsend[BLK], esend[BLK];',
        'static float mixl[BLK], mixr[BLK], rsend[BLK], esend[BLK];\n'
        '#include "../app/urban_jazz_engine.h"\n'
        '#include "../app/urban_echo_engine.h"')
    replace_once(dsp,
        'static void render_pad(uint32_t n)\n{',
        'static void render_pad(uint32_t n)\n{\n'
        '    if(urban_jazz_chord_mode)return; /* use modelled keys instead */')
    replace_once(dsp,
        'static void render_acid(uint32_t n)\n{',
        'static void render_acid(uint32_t n)\n{\n'
        '    if(urban_jazz_lead_mode)return; /* use modelled flute/mallets */')
    replace_once(dsp,
        'static void render_bass(uint32_t n)\n{',
        'static void render_bass(uint32_t n)\n{\n'
        '    if(urban_jazz_bass_mode)return; /* no electronic DRONE in jazz */')
    replace_once(dsp,
        '    render_pad(n);\n    render_bass(n);',
        '    urban_jazz_apply();\n    render_pad(n);\n    render_bass(n);\n'
        '    urban_jazz_bass_render(n);')
    replace_once(dsp,
        '    render_acid(n);\n    render_noise(n);',
        '    render_acid(n);\n    urban_jazz_render(n);\n    render_noise(n);')
    # Shared Echo and Throw use the same actual DRY master mix at distinct
    # gains. Upstream Echo only feeds extremely faint per-voice esend,
    # which made the knob ineffective in actual songs despite the P_ECHO
    # register responding. Preserve tape/Echoplex delay and wet mix.
    replace_once(dsp,
        '        echoes(esend[j] * eg + throw_amt * 0.5f * (mixl[j] + mixr[j]), &el, &er, &xl, &xr);',
        '        echoes(urban_echo_bus_input(mixl[j],mixr[j],esend[j],eg,throw_amt),'
        ' &el, &er, &xl, &xr);\n'
        '        urban_echo_tap_meter(el,er,eg,throw_amt);')
    # New monophonic procedural SFX audio voice: 10 modelled transients.
    # The engine lives inside HYP's DSP mix; no stolen drum/lead lanes.
    # Include after the mixer buffers and sine oscillator have been defined.
    replace_once(dsp,
        'static void render_drums(uint32_t n)',
        '#include "../app/urban_sfx_engine.h"\nstatic void render_drums(uint32_t n)')
    replace_once(dsp,
        '    duck_env = fm_flush(de);',
        '    urban_sfx_render(n); /* dedicated SFX voice, mixed before master FX */\n    duck_env = fm_flush(de);')
    # Tape Start uses the SAME existing master tape ring; no new large
    # allocation. Run after HYP's FX master, before 24-bit PCM conversion.
    replace_once(dsp,
        'static void drain(void)',
        '#include "../app/urban_tape_transport.h"\nstatic void drain(void)')
    replace_once(dsp,
        '        fx_master(&l, &r);',
        '        fx_master(&l, &r);\n'
        '        urban_tape_start_mix(&l,&r);')
    # HYP's drum tick is sample-aligned, while the previous Acid lead
    # note-on ran after WebView/LCD polling (up to a full UI frame late).
    # Trance now consumes a prebuilt score on the very same DSP tick.
    replace_once(dsp,
        'static void tick(void)',
        'static void urban_trance_audio_tick(uint32_t step);\nstatic void tick(void)')
    replace_once(dsp,
        '    sixteenth++;\n    en_bars = sixteenth;',
        '    urban_trance_audio_tick(sixteenth); /* sample-locked with drums */\n    sixteenth++;\n    en_bars = sixteenth;')
    replace_once(dsp,
        'static void quiet(void)',
        '#include "../app/urban_trance_engine.h"\nstatic void quiet(void)')
    # Piano voicings need key-press spacing of tens of milliseconds,
    # not a 16th-note arpeggiator or simultaneous MIDI chord note-ons.
    # Reuse the HYP renderer's existing subchunk slicing mechanism.
    replace_once(dsp,
        'static void quiet(void)',
        '#include "../app/urban_piano_roll_engine.h"\nstatic void quiet(void)')
    replace_once(dsp,
        '    drain();\n    if (hush && hush_g <= 0.0f)',
        '    drain();\n    urban_roll_receive();\n    if (hush && hush_g <= 0.0f)')
    replace_once(dsp,
        '        if (timed)\n            pulse_left -= (float)k;',
        '        { uint32_t wait = urban_roll_wait();\n'
        '          if (wait > 0u && wait < k) k = wait; }\n'
        '        if (timed)\n            pulse_left -= (float)k;')
    replace_once(dsp,
        '        drums_wait_step((int32_t)k);',
        '        drums_wait_step((int32_t)k);\n        urban_roll_advance(k);')
    # One shared port implementation in both the FM-1 and HYP-1 host simulator.
    replace_once(root / "firmware/src/main.c",
        '#include "app/main_fm1.c"',
        '#include "app/urban_hyp_port.h"\n#include "app/main_fm1.c"')
    replace_once(root / "firmware/src/app/main_fm1.c",
        '    ui_init();\n\n    for (;;) {',
        '    ui_init();\n    urban_port_init(plat_seed());\n\n    for (;;) {')
    replace_once(root / "firmware/src/app/main_fm1.c",
        '            ui_frame();',
        '            urban_port_frame();')
    replace_once(root / "firmware/src/app/main_fm1.c",
        '            ui_input_only();',
        '            urban_port_input();')
    replace_once(root / "host/amb_host.c",
        '#include "../firmware/src/app/ui.c"',
        '#include "../firmware/src/app/ui.c"\n#include "../firmware/src/app/urban_hyp_port.h"')
    replace_once(root / "host/amb_host.c",
        '    ui_init();\n}',
        '    ui_init();\n    urban_port_init(plat_seed());\n}')
    replace_once(root / "host/amb_host.c",
        '            ui_frame();',
        '            urban_port_frame();')
    replace_once(root / "host/amb_host.c",
        '            ui_input_only();',
        '            urban_port_input();')
    # Real SFX diagnostics: distinguish an authored cue from a voice
    # actually consumed by HYP's audio interrupt.
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "sixteenths"))',
        '    else if (!strcmp(what, "urbansfxrendered")) got = urban_sfx_rendered_events;\n'
        '    else if (!strcmp(what, "urbansfxlevel")) got = urban_port.sfx_level;\n'
        '    else if (!strcmp(what, "urbansfxlength")) got = urban_port.sfx_length;\n'
        '    else if (!strcmp(what, "urbansfxdensity")) got = urban_port.sfx_density;\n'
        '    else if (!strcmp(what, "sixteenths"))')
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "urbansfxrendered"))',
        '    else if (!strcmp(what, "urbantranceonsets")) got = urban_trance_audio_onsets;\n'
        '    else if (!strcmp(what, "urbantrancelaststep")) got = urban_trance_audio_last_step;\n'
        '    else if (!strcmp(what, "urbantranceoffgrid")) got = urban_trance_offgrid_onsets;\n'
        '    else if (!strcmp(what, "urbansfxrendered"))')
    # Verify that piano note onsets come from HYP audio processing,
    # not a synthetic editor label; ambient modes should never roll.
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "urbantranceonsets"))',
        '    else if (!strcmp(what, "urbanpianorolls")) got = urban_piano_rolled_chords;\n'
        '    else if (!strcmp(what, "urbanpianonotes")) got = urban_piano_note_onsets;\n'
        '    else if (!strcmp(what, "urbantranceonsets"))')
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "urbanpianorolls"))',
        '    else if (!strcmp(what, "urbanjazzbass")) got = urban_jazz_bass_mode;\n'
        '    else if (!strcmp(what, "urbanjazzbassnotes")) got = urban_jazz_bass_onsets;\n'
        '    else if (!strcmp(what, "urbanjazzbassframes")) got = urban_jazz_bass_rendered_frames;\n'
        '    else if (!strcmp(what, "urbanjazzchord")) got = urban_jazz_chord_mode;\n'
        '    else if (!strcmp(what, "urbanjazzlead")) got = urban_jazz_lead_mode;\n'
        '    else if (!strcmp(what, "urbanjazznotes")) got = urban_jazz_note_onsets;\n'
        '    else if (!strcmp(what, "urbanjazzframes")) got = urban_jazz_rendered_frames;\n'
        '    else if (!strcmp(what, "urbanpianorolls"))')
    # Expose the actual URBAN-1 song transport to host regression scripts.
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "sixteenths"))',
        '    else if (!strcmp(what, "urbanview")) got = urban_port.editor.view;\n'
        '    else if (!strcmp(what, "urbantarget")) got = urban_port.editor.target;\n'
        '    else if (!strcmp(what, "urbanbank")) got = urban_port.editor.bank;\n'
        '    else if (!strcmp(what, "urbantapestartframes")) got = urban_tape_start_frames;\n'
        '    else if (!strcmp(what, "urbantapestarts")) got = urban_tape_start_completed;\n'
        '    else if (!strcmp(what, "urbanfxechosend")) got = urban_echo_normal_input_frames;\n'
        '    else if (!strcmp(what, "urbanfxechowet")) got = urban_echo_normal_tap_frames;\n'
        '    else if (!strcmp(what, "urbanfxthrowsend")) got = urban_echo_throw_input_frames;\n'
        '    else if (!strcmp(what, "urbanfxecho")) got = urban_port_editor_value(P_ECHO);\n'
        '    else if (!strcmp(what, "urbanfxreverb")) got = urban_port_editor_value(P_REVERB);\n'
        '    else if (!strcmp(what, "urbanfxoverride")) got = urban_port.editor.changed[P_ECHO];\n'
        '    else if (!strcmp(what, "urbanbassoverride")) got = urban_port.editor.changed[P_DR_CUT];\n'
        '    else if (!strcmp(what, "urbanleaddecayedit")) got = urban_port.editor.changed[P_AC_DEC];\n'
        '    else if (!strcmp(what, "urbanpadattackedit")) got = urban_port.editor.changed[P_PE_ATK];\n'
        '    else if (!strcmp(what, "urbanfxmask")) got = urban_port.editor.performance_mask;\n'
        '    else if (!strcmp(what, "urbaneditkey")) got = urban_port.rt.ui.song.key;\n'
        '    else if (!strcmp(what, "urbanedittempo")) got = urban_port.rt.ui.song.tempo;\n'
        '    else if (!strcmp(what, "urbaneditmode")) got = urban_port.rt.ui.song.scale;\n'
        '    else if (!strcmp(what, "urbanmode")) got = urban_port.rt.ui.song.song_mode;\n'
        '    else if (!strcmp(what, "urbansub")) got = urban_port.rt.ui.song.subgenre;\n'
        '    else if (!strcmp(what, "urbanroot1")) got = urban_port.rt.ui.song.chord_degrees[1];\n'
        '    else if (!strcmp(what, "urbandrumkick")) got = urban_groove(urban_port.rt.ui.song.tracks[0].genre, urban_port.rt.ui.song.tracks[0].subgenre).kick;\n'
        '    else if (!strcmp(what, "urbanbasssub")) got = urban_port.rt.ui.song.tracks[2].subgenre;\n'
        '    else if (!strcmp(what, "urbanbasssound")) got = urban_instrument_program(&urban_port.rt.ui.song,2u,urban_port.rt.ui.song.current).patch.bass;\n'
        '    else if (!strcmp(what, "urbanphrase")) got = urban_phrase_bar(&urban_port.rt.ui.song);\n'
        '    else if (!strcmp(what, "urbanchord")) got = urban_port.chord_count;\n'
        '    else if (!strcmp(what, "urbantake")) got = urban_port.rt.ui.generation_serial;\n'
        '    else if (!strcmp(what, "urbanscale")) got = urban_port.rt.ui.song.scale;\n'
        '    else if (!strcmp(what, "urbanbar")) got = urban_port.rt.ui.song.arrangement_pos;\n'
        '    else if (!strcmp(what, "urbancurrent")) got = urban_port.rt.ui.song.current;\n'
        '    else if (!strcmp(what, "urbannext")) got = urban_port.rt.ui.song.queued;\n'
        '    else if (!strcmp(what, "urbanpage")) got = urban_port.rt.ui.page;\n'
        '    else if (!strcmp(what, "urbantrack")) got = urban_port.rt.ui.selected_track;\n'
        '    else if (!strcmp(what, "urbanlength")) got = urban_port.rt.ui.song.arrangement_len;\n'
        '    else if (!strcmp(what, "sixteenths"))')
    print("URBAN-1 overlay applied to:", root)
    print("Host: sh host/build_host.sh")
    print("Physical firmware: ./build.sh (requires JieLi toolchain + AC79 SDK)")
    print("Original HYP-1 source and licensing must be preserved.")

if __name__ == "__main__":
    main()
