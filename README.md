# PsychEnginePs4

Experimental project investigating a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: interactive native menu preview — not the full game yet.** The repository does not yet contain a playable port of Psych Engine. The Haxe/OpenFL/Lime gameplay runtime and its audio, chart, mod and asset systems are not integrated into the native PS4 app.

## Current progress

- `source/ps4/psychengine_ps4_platform.hpp` defines the planned input, audio, renderer and file backend interfaces.
- `.github/workflows/ps4-build.yml` audits the official upstream engine and reports porting blockers.
- `src/main.cpp` uses OpenOrbis video output to render a native menu with **STORY MODE**, **FREEPLAY**, and **OPTIONS**.
- D-pad Up/Down moves the selection; Cross opens the selected page; Circle returns to the main menu. The Options page lets the user switch the accent color with Left/Right.
- Story Mode and Freeplay are honest placeholder pages explaining that songs and the gameplay runtime are not integrated yet.
- The bootstrap Makefile links the OpenOrbis video and controller/user-service libraries.
- `.github/workflows/ps4-bootstrap.yml` validates the generated 512×512 RGB icon, stages runtime modules, and builds the experimental PKG using OpenOrbis packaging tools.

## Artifact

Open **Actions → PsychEnginePs4 native bootstrap** and download `PsychEnginePs4-menu-preview-v0.2` from the latest successful run. It contains the PKG, ELF, generated icon, SFO metadata, and packaged runtime modules.

The audit workflow is separate: [PsychEngine PS4 (experimental)](.github/workflows/ps4-build.yml).

## Current limitations

The menu is a native proof of concept. **Songs, charts, gameplay, audio playback and mod support are not implemented.** A green CI build confirms the compile and packaging steps succeeded, but does not prove that the PKG launches on every physical PS4; device testing is still required.

## Milestones

1. Validate the interactive menu and controller navigation on compatible PS4 homebrew hardware.
2. Improve the menu and add proper settings persistence.
3. Decide how to adapt or replace the Haxe/OpenFL/Lime runtime.
4. Integrate charts, songs, characters, stages, Lua/mod support and asset loading.
5. Validate the gameplay port before calling it playable.

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers.
