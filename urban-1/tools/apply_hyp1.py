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
    shutil.copyfile(HERE / "integrations/urban_hyp_port.h",
                    root / "firmware/src/app/urban_hyp_port.h")
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
    # Expose the actual URBAN-1 song transport to host regression scripts.
    replace_once(root / "host/amb_host.c",
        '    else if (!strcmp(what, "sixteenths"))',
        '    else if (!strcmp(what, "urbanmode")) got = urban_port.rt.ui.song.song_mode;\n'
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
