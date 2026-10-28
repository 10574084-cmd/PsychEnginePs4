# PsychEnginePs4

Experimental project to investigate a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: platform-layer scaffold / not playable yet.** The repository still does not contain the actual Psych Engine source, a concrete PS4 runtime implementation, or a playable build. The Actions workflow creates a source snapshot and status report; it does **not** generate a working `.pkg`.

## Work added so far

- Added `source/ps4/psychengine_ps4_platform.hpp`, an initial backend contract for controller input, PCM audio, rendering, and file access.
- Added `source/ps4/README.md` describing the integration order and current blockers.
- Kept the experimental Actions workflow as a repository/source-snapshot check so its green status is not confused with a successful PS4 build.

The platform interfaces are only declarations. They are not connected to OpenOrbis APIs or to the game loop yet.

## Why the Windows build is not enough

The uploaded `PsychEngine-Windows32.zip` is a Windows build, not the engine's portable source tree, so it cannot by itself be converted into a native PS4 application.

Psych Engine is built around Haxe/OpenFL/Lime and related libraries. A genuine port requires the exact source version plus compatible graphics, audio, controller input, filesystem, and packaging layers.

## GitHub Actions

Open **Actions → PsychEngine PS4 (experimental)** to run the repository check. It uploads a source snapshot and status report. This is not an installable game package.

Workflow: [`.github/workflows/ps4-build.yml`](.github/workflows/ps4-build.yml)

## Next milestones

1. Add the actual Psych Engine source tree and review its license and dependencies.
2. Establish and validate a minimal native PS4 application build with a supported homebrew toolchain.
3. Implement and test controller input and filesystem access.
4. Choose a viable graphics/audio route and connect it to the engine.
5. Integrate the gameplay loop, charts, assets, and mods.
6. Add PKG generation only after a real executable and its runtime dependencies build successfully.
7. Test on target hardware before claiming the game works.

## Disclaimer

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers. A successful Actions run does not mean the game is playable on PS4.
