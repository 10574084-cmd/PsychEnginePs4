#!/usr/bin/env python3
"""Patch hxcpp's Orbis time helpers for the OpenOrbis FreeBSD/POSIX sysroot."""

from __future__ import annotations

import argparse
from pathlib import Path


def patch_date_cpp(hxcpp_root: Path) -> None:
    source_path = hxcpp_root / "src" / "hx" / "Date.cpp"
    if not source_path.is_file():
        raise SystemExit(f"hxcpp Date.cpp not found: {source_path}")

    source = source_path.read_text(encoding="utf-8")
    marker = "#define localtime_r localtime_s"
    if marker not in source:
        if "localtime_s" not in source and "gmtime_s" not in source:
            print("hxcpp Date.cpp already uses POSIX time helpers; no patch needed.")
            return
        raise SystemExit(
            "Found the Orbis secure-time aliases, but could not safely locate "
            "the expected compatibility block."
        )

    start = source.rfind("#if defined(__ORBIS__)", 0, source.index(marker))
    if start < 0:
        raise SystemExit("Could not locate the __ORBIS__ localtime_s compatibility block.")

    end = source.find("#endif", source.index(marker))
    if end < 0:
        raise SystemExit("Could not find the end of the __ORBIS__ compatibility block.")

    end += len("#endif")
    block = source[start:end]
    if "localtime_r localtime_s" not in block or "gmtime_r gmtime_s" not in block:
        raise SystemExit("Unexpected hxcpp Orbis time compatibility block; refusing to patch.")

    patched = source[:start] + source[end:]
    source_path.write_text(patched, encoding="utf-8")
    print(f"Patched {source_path}: OpenOrbis uses localtime_r/gmtime_r, not *_s.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("hxcpp_root", type=Path)
    args = parser.parse_args()
    patch_date_cpp(args.hxcpp_root.resolve())


if __name__ == "__main__":
    main()
