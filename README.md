# PsychEnginePs4

Experimental project investigating a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: native bootstrap preview — not the game yet.** The repository does not yet contain a playable port of Psych Engine. The Haxe/OpenFL/Lime gameplay runtime and its graphics, audio, input, chart and asset systems are not integrated into the native PS4 app.

## Current progress

- `source/ps4/psychengine_ps4_platform.hpp` defines the planned input, audio, renderer and file backend interfaces.
- `.github/workflows/ps4-build.yml` audits the official upstream engine and reports porting blockers.
- `src/main.cpp` now uses the OpenOrbis graphics sample backend to draw a visible, static PS4 splash screen. It is a video-output test, not gameplay.
- The native bootstrap Makefile links only the needed C/C++ and video-output libraries instead of every available SDK stub.
- `.github/workflows/ps4-bootstrap.yml` validates the generated 512×512 RGB icon, stages the runtime modules, and builds an experimental PKG using OpenOrbis package tools.
- `src/main.c` is an older console-output smoke test and is no longer the entry point used by the bootstrap Makefile.

## Artifacts

Open **Actions → PsychEnginePs4 native bootstrap** and download the artifact named `PsychEnginePs4-bootstrap-preview-v01-01` from the latest successful run. It contains the PKG, ELF, generated icon, SFO metadata, and packaged runtime modules.

The audit workflow is separate: [PsychEngine PS4 (experimental)](.github/workflows/ps4-build.yml).

## Why this is not a full port yet

Psych Engine is built around Haxe/OpenFL/Lime and related libraries. A real PS4 port needs a viable runtime strategy and working PS4-specific rendering, audio, controller, filesystem, asset/chart loading, and gameplay integration. The current native bootstrap displays only a static splash screen and cannot run songs or mods.

A green CI build proves the build and packaging steps succeeded. It does **not** prove that the PKG launches on a physical PS4; device testing is still required.

## Milestones

1. Verify the visible video-output bootstrap on compatible PS4 homebrew hardware.
2. Establish controller input and a stable renderer.
3. Decide how to adapt or replace the Haxe/OpenFL/Lime runtime.
4. Integrate charts, songs, characters, stages, Lua/mod support and asset loading.
5. Validate on compatible PS4 homebrew hardware before calling the port playable.

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers.
