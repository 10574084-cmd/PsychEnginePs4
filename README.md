# PsychEnginePs4

Experimental project investigating a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: native bootstrap experiment — not the game yet.** The repository does not yet contain a playable port of Psych Engine. The Haxe/OpenFL/Lime gameplay runtime and its graphics, audio, input, chart and asset systems are not integrated into the native PS4 app.

## Current progress

- `source/ps4/psychengine_ps4_platform.hpp` defines the planned input, audio, renderer and file backend interfaces.
- `.github/workflows/ps4-build.yml` audits the official upstream engine and reports porting blockers.
- `src/main.c` is a tiny native smoke-test entry point for checking the OpenOrbis build/package toolchain. It prints diagnostic messages and exits; it is **not playable gameplay**.
- `Makefile` and `tools/make_gp4.py` provide an experimental OpenOrbis build and package path.
- `.github/workflows/ps4-bootstrap.yml` attempts to compile and package the bootstrap. Its artifact is explicitly named `PsychEnginePs4-native-bootstrap-not-game`.

## Artifacts

Open **Actions → PsychEnginePs4 native bootstrap** and inspect the latest run. If the build succeeds, download the artifact named `PsychEnginePs4-native-bootstrap-not-game`. It contains a toolchain smoke-test package, not the finished game.

The audit workflow is separate: [PsychEngine PS4 (experimental)](.github/workflows/ps4-build.yml).

## Why this is not a full port yet

Psych Engine is built around Haxe/OpenFL/Lime and related libraries. A real PS4 port needs a viable runtime strategy and working PS4-specific rendering, audio, controller, filesystem, asset/chart loading, and gameplay integration. The current native bootstrap only tests the packaging path and cannot run songs or mods.

## Milestones

1. Verify that the OpenOrbis toolchain can produce a native executable and package in CI.
2. Establish a visible PS4 rendering path and controller input.
3. Decide how to adapt or replace the Haxe/OpenFL/Lime runtime.
4. Integrate charts, songs, characters, stages, Lua/mod support and asset loading.
5. Validate on compatible PS4 homebrew hardware before calling the port playable.

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers. A green workflow only confirms the specific checks that it runs.
