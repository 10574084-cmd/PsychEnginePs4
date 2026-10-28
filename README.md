# PsychEnginePs4

Experimental project to investigate a native PlayStation 4 homebrew port of **Friday Night Funkin': Psych Engine**.

> **Status: prototype / not playable yet.** This repository currently does not contain the Psych Engine source code or a PS4-native port. The GitHub Actions workflow is a repository check and source snapshot only; it does **not** generate a working `.pkg`.

## Goal

- Explore a controller-friendly Psych Engine experience on PS4.
- Investigate a native port using a PS4-compatible rendering, audio, input, and filesystem layer.
- Use GitHub Actions to validate changes and publish reproducible build artifacts once the required port code exists.

## Current repository status

The initial repository contains only this README and the experimental Actions workflow. The uploaded `PsychEngine-Windows32.zip` is a Windows build, not the engine's portable source tree, so it cannot by itself be converted into a native PS4 application.

Psych Engine is built around Haxe/OpenFL/Lime and related libraries. A genuine PS4 port requires more than changing the target platform: platform-dependent graphics, audio, input, file access, and packaging must be made compatible with the PS4 toolchain. No playable PS4 build is claimed at this stage.

## GitHub Actions

Open **Actions → PsychEngine PS4 (experimental) → Run workflow** to run the repository check. It uploads a source snapshot and a status report as an artifact. This is not an installable game package.

Workflow file: `.github/workflows/ps4-build.yml`

## Next development milestones

1. Add the actual Psych Engine source tree and review its license and dependencies.
2. Establish a minimal native PS4 application build using a supported homebrew toolchain.
3. Port and test rendering, audio, controller input, and asset loading in small steps.
4. Integrate the engine's gameplay code with the PS4 platform layer.
5. Add a package-generation step only after a real executable builds successfully.
6. Test on the intended hardware and publish a `.pkg` only when the build produces one.

## Disclaimer

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers. A successful Actions run does not mean the game is playable on PS4.
