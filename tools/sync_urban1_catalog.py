#!/usr/bin/env python3
"""Synchronize the Android *existing firmware* update catalog on URBAN-1 releases.

This edits only web/firmwares-own.json and web/firmwares.json on master.
It deliberately does not modify the Android APK, its bundled catalog, or
installed state. Existing APKs use the original id "urban1" and consult
master/web/firmwares.json when Update All is pressed.
"""
from __future__ import annotations
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
REPO = "Koolkatze/Bluetooth-control-multi-firmware-m-vave-fm1-fm1-emu"
TAG_PATTERN = re.compile(r"urban1-firmware-v(\d+\.\d+\.\d+(?:[-+][a-zA-Z0-9.-]+)?)\Z")

def sync(release: dict) -> tuple[str, str]:
    tag = str(release.get("tag_name", ""))
    match = TAG_PATTERN.fullmatch(tag)
    if not match:
        raise ValueError("Not a URBAN-1 firmware release tag")
    version = match.group(1)
    assets = [a for a in release.get("assets", [])
              if isinstance(a, dict) and a.get("name", "").endswith(".fwsc")]
    if len(assets) != 1:
        raise ValueError("Require exactly one .fwsc asset in the release")
    asset = assets[0]
    url = str(asset.get("browser_download_url", ""))
    parsed = urlparse(url)
    if (parsed.scheme != "https" or parsed.netloc != "github.com"
            or not parsed.path.startswith(f"/{REPO}/releases/download/{tag}/")
            or not (1024 <= int(asset.get("size", 0)) <= 15 * 1024 * 1024)):
        raise ValueError("Firmware asset URL/size is not acceptable")

    own_path = ROOT / "web/firmwares-own.json"
    catalog_path = ROOT / "web/firmwares.json"
    own = json.loads(own_path.read_text(encoding="utf-8"))
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    own_entries = [f for f in own["firmwares"] if f.get("id") == "urban1"]
    cat_entries = [f for f in catalog["firmwares"] if f.get("id") == "urban1"]
    if len(own_entries) != 1 or len(cat_entries) != 1:
        raise ValueError("URBAN-1 must already be installed in both catalogs")
    own_item, cat_item = own_entries[0], cat_entries[0]
    if own_item["version"] == version and own_item["package"] == url and (
            cat_item["version"] == version and cat_item["package"] == url):
        print(f"URBAN-1 already at {version}: no catalog changes")
        return version, url

    for item in (own_item, cat_item):
        item["version"] = version
        item["package"] = url

    # Preserve the generated catalog's compact, line-oriented format:
    # the Hub catalog generator reads web/firmwares-own.json and will retain
    # this custom entry on future Hub refreshes.
    src = catalog_path.read_text(encoding="utf-8")
    lines = src.splitlines(keepends=True)
    hits = [i for i, line in enumerate(lines)
            if re.match(r'^    \{ "id": "urban1",', line)]
    if len(hits) != 1:
        raise ValueError("Generated URBAN-1 entry cannot be located")
    i = hits[0]
    ends = [k for k in range(i, min(i + 10, len(lines)))
            if lines[k].rstrip().endswith(" },") or lines[k].rstrip().endswith(" }")]
    if not ends:
        raise ValueError("Generated URBAN-1 entry has unexpected shape")
    j = ends[0]
    def value(field: str) -> str:
        return json.dumps(cat_item[field], ensure_ascii=False)
    entry = (
        f'    {{ "id": {value("id")}, "name": {value("name")}, '
        f'"version": {value("version")}, "author": {value("author")}, '
        f'"license": {value("license")},\n'
        f'      "about": {value("about")},\n'
        f'      "repo": {value("repo")},\n'
        f'      "package": {value("package")} }}'
        + ("," if lines[j].rstrip().endswith(",") else "") + "\n"
    )
    new_cat = "".join(lines[:i] + [entry] + lines[j + 1:])
    result = json.loads(new_cat)
    matches = [f for f in result["firmwares"] if f.get("id") == "urban1"]
    if len(matches) != 1 or matches[0]["package"] != url or matches[0]["version"] != version:
        raise ValueError("Generated catalog failed consistency checks")

    own_path.write_text(json.dumps(own, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    catalog_path.write_text(new_cat, encoding="utf-8")
    print(f"Updated both catalogs: URBAN-1 {version} -> {url}")
    return version, url

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: sync_urban1_catalog.py release.json")
    release = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    sync(release)
