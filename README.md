# PsychEnginePs4

Experimental project investigating a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: original-menu preview — not the full game yet.** The Haxe/OpenFL/Lime gameplay runtime and its audio, charts, mod support, and asset-loading systems are not integrated into this native PS4 application.

## Current progress

- `source/ps4/psychengine_ps4_platform.hpp` defines the planned input, audio, renderer, and file backend interfaces.
- `.github/workflows/ps4-build.yml` audits the upstream engine and reports porting blockers.
- `src/main.cpp` recreates the original Psych Engine 0.6.3 main-menu layout using the original `menuBG` background and the seven original menu button sprite atlases: Story Mode, Freeplay, Mods, Awards, Credits, Donate, and Options.
- `tools/prepare_psych_menu.py` crops the original atlas frames and scales them for the PS4 renderer.
- `tools/patch_openorbis_graphics.py` adds fast alpha-aware image blitting to the OpenOrbis sample renderer, so transparent button art overlays the background correctly.
- D-pad Up/Down changes the selected item; Cross opens a page and Circle returns to the main menu.
- **Options is now an interactive native PS4 submenu**: toggle version labels, toggle controller hints, toggle the original background, and restore defaults. Preferences are saved to `/data/PSYH00001/menu_settings.cfg` when the console permits writes.
- **Freeplay includes 21 stock FNF songs and custom songs**. CI downloads the 0.6.3 audio and chart files into the PKG; user folders under `/data/PSYH00001/songs` take priority over bundled songs with the same folder name.
- Instrumental and vocal OGG tracks are decoded, mixed, and sent to PS4 audio output; chart playback remains experimental.
- A native OGG instrumental-preview path decodes audio to stereo PCM and submits it to the PS4 audio output API on a worker thread; hardware playback still needs to be confirmed.
- Freeplay now extracts player note rows from chart sections and has an experimental four-lane chart playtest with timing-based hits, misses, a score, and a results screen.
- The main menu labels the build as Psych Engine PS4 BETA and identifies the Psych Engine 0.6.3 asset base.
- The native bootstrap Makefile links the OpenOrbis video, controller, and user-service libraries.
- `.github/workflows/ps4-bootstrap.yml` fetches the matching original 0.6.3 menu assets, validates the icon, bundles runtime modules, and builds the experimental PKG.

## Artifact

Open **Actions → PsychEnginePs4 native bootstrap** and download `PsychEnginePs4-original-menu-v0.2` from the latest successful run. It contains the PKG, ELF, generated icon, SFO metadata, packaged runtime modules, and the original menu assets.

## Current limitations

The main menu's look and button sprites are based on the original Psych Engine 0.6.3 assets, but this is a native recreation of the menu layout—not the original Haxe code running on PS4. Options has real menu-level toggles and Freeplay can browse both bundled stock songs and user folders under `/data/PSYH00001/songs`. Each folder under `songs` should contain `Inst.ogg`; its chart can be beside the audio as `chart.json` or `<folder-name>.json`, or under the separate `data/<folder-name>/` directory using the Psych Engine-style `<folder-name>.json` naming. Easy/normal/hard chart files are counted when present. The details page extracts BPM and note-entry count from `sectionNotes`; left/right switches between discovered Easy/Normal/Hard chart files and updates the displayed metadata. The four-lane playtest is an initial native prototype, not the full Psych Engine gameplay runtime. Animated characters/stages, exact audio-to-chart sync, complete sustain handling, Lua scripts, mod loading, and Haxe/OpenFL/Lime compatibility remain **not implemented**. The audio and chart timing still need testing on the physical console. The audio path still requires testing on the actual PS4. Stock OGGs are bundled at build time from the upstream Psych Engine 0.6.3 repository, and its LICENSE is included in the package. Story Mode, Mods, Awards, Credits, and Donate remain placeholders.

A green CI build confirms the compile and packaging steps succeeded. It does **not** prove that the PKG launches on a physical PS4; the new menu still needs to be tested on the console.

## Milestones

1. Validate original-menu rendering and controller navigation on compatible PS4 homebrew hardware.
2. Expand native Freeplay to resolve difficulties and validate chart schema.
3. Test and stabilize OGG decoding/output and the four-lane chart playtest on the console.
4. Improve sustain-note handling, chart timing, character/stage rendering, and audio synchronization.
5. Decide how to adapt or replace the Haxe/OpenFL/Lime runtime.
6. Integrate song charts, characters, stages, Lua/mod support, and asset loading.
7. Validate gameplay on compatible PS4 homebrew hardware before calling the port playable.

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers. The menu, stock song audio, charts, and license are retrieved from the upstream Psych Engine 0.6.3 source tree during CI.
