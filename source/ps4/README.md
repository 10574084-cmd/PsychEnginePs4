# PS4 platform layer scaffold

This folder defines the planned platform boundary for a native PS4 port. The interfaces are not yet connected to OpenOrbis APIs or to the Psych Engine game loop.

## Current components

- `psychengine_ps4_platform.hpp`: backend contracts for controller input, PCM audio, rendering and file access.
- `src/main.c`: a separate native bootstrap smoke test used only to validate the OpenOrbis build/package route. It is not the game's entry point and does not render gameplay.

## Required port work

1. Choose a viable strategy for the Haxe/OpenFL/Lime runtime used by Psych Engine.
2. Implement and test PS4 input and filesystem access.
3. Implement a working graphics and audio backend.
4. Connect the real gameplay loop, charts, assets, songs and mods.
5. Test on compatible hardware.

## Important distinction

The native bootstrap workflow may generate a valid test package if its toolchain steps succeed, but that does **not** mean Psych Engine itself is playable on PS4. Do not label the bootstrap package as a game release.
