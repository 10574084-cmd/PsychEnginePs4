#!/usr/bin/env python3
"""Prepare the original Psych Engine 0.6.3 menu atlas assets for the PS4 raster renderer."""
from __future__ import annotations

import argparse
import xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image

MENU_ITEMS = (
    "story_mode",
    "freeplay",
    "mods",
    "awards",
    "credits",
    "donate",
    "options",
)
SCREEN_W = 1920
SCREEN_H = 1080
GAME_SCALE = 1.5
RESAMPLE_LANCZOS = getattr(getattr(Image, "Resampling", Image), "LANCZOS")

def crop_animation_frame(atlas_path: Path, xml_path: Path, item: str, state: str, out_path: Path) -> tuple[int, int]:
    root = ET.parse(xml_path).getroot()
    frame_name = f"{item} {state}0002"
    frames = {node.attrib["name"]: node.attrib for node in root.findall("SubTexture")}
    if frame_name not in frames:
        # Some older atlases may contain only the first repeated frame.
        frame_name = f"{item} {state}0000"
    if frame_name not in frames:
        raise RuntimeError(f"Missing atlas frame {item} {state}0002 in {xml_path}")
    meta = frames[frame_name]
    x, y, w, h = (int(meta[key]) for key in ("x", "y", "width", "height"))
    with Image.open(atlas_path) as atlas:
        frame = atlas.convert("RGBA").crop((x, y, x + w, y + h))
    size = (round(w * GAME_SCALE), round(h * GAME_SCALE))
    frame = frame.resize(size, RESAMPLE_LANCZOS)
    frame.save(out_path, format="PNG", optimize=True)
    return size

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True, help="Directory containing the original PNG/XML files")
    parser.add_argument("--out", type=Path, required=True, help="Output directory to package under /app0/assets/psychmenu")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    # Match the source MainMenuState background scale (1.175) at 1280x720,
    # then scale the viewport to 1920x1080 and center-crop like FlxSprite.screenCenter().
    with Image.open(args.source / "menuBG.png") as source:
        bg = source.convert("RGB")
        size = (round(bg.width * 1.175 * GAME_SCALE), round(bg.height * 1.175 * GAME_SCALE))
        bg = bg.resize(size, RESAMPLE_LANCZOS)
        left = (bg.width - SCREEN_W) // 2
        top = (bg.height - SCREEN_H) // 2
        bg = bg.crop((left, top, left + SCREEN_W, top + SCREEN_H))
        if bg.size != (SCREEN_W, SCREEN_H):
            raise RuntimeError(f"Unexpected background size: {bg.size}")
        bg.save(args.out / "menu_background.png", format="PNG", optimize=True)

    for item in MENU_ITEMS:
        atlas = args.source / f"menu_{item}.png"
        xml = args.source / f"menu_{item}.xml"
        for state, suffix in (("basic", "basic"), ("white", "selected")):
            crop_animation_frame(atlas, xml, item, state, args.out / f"{item}_{suffix}.png")

    expected = ["menu_background.png"]
    for item in MENU_ITEMS:
        expected.extend([f"{item}_basic.png", f"{item}_selected.png"])
    missing = [name for name in expected if not (args.out / name).is_file() or (args.out / name).stat().st_size == 0]
    if missing:
        raise RuntimeError("Missing generated menu assets: " + ", ".join(missing))
    print(f"Prepared original Psych Engine menu background and {len(MENU_ITEMS)} button atlas pairs.")
    for name in expected:
        with Image.open(args.out / name) as im:
            print(f"  {name}: {im.width}x{im.height}")

if __name__ == "__main__":
    main()
