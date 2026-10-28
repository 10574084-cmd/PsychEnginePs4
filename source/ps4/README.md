# PS4 platform layer scaffold

This folder starts the platform boundary for a native PS4 port. It is **not a playable build** and does not yet call OpenOrbis APIs.

## Interface contract

`psychengine_ps4_platform.hpp` defines the backend responsibilities the engine will need:

- **Input:** controller connection state, buttons, and analog sticks.
- **Audio:** initialization and PCM submission.
- **Renderer:** frame lifecycle for a future PS4 graphics backend.
- **Files:** read/write access for assets, charts, mods, and saves.

The interfaces avoid OpenOrbis headers for now so engine integration can be designed independently from SDK-specific calls.

## Next implementation order

1. Add the actual, licensed Psych Engine source tree.
2. Identify the exact Haxe/OpenFL/Lime features used by that version and which ones can be replaced or adapted.
3. Implement and test the PS4 input and filesystem backends.
4. Choose a viable rendering/audio path and implement it against the PS4 SDK.
5. Connect the game loop and asset/chart loading, then validate on a PS4 test environment.
6. Add PKG generation only after the native executable and required runtime modules build successfully.

## Current blockers

- The repository does not contain the actual Psych Engine source.
- The interfaces above have no concrete implementations yet.
- No PS4 executable or installable PKG is produced by this scaffold.
