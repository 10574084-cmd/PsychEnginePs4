# PsychEnginePs4

Experimental work toward a native PlayStation 4 port of **Friday Night Funkin': Psych Engine**.

> **Current status: native preview only.** The package is not yet the original game running on PS4. The real Haxe/OpenFL/Lime source is now tracked in `vendor/PsychEngine`, but its runtime has not been ported or linked into the PS4 executable.

## Real source

- `vendor/PsychEngine` is a Git submodule pointing to the upstream `ShadowMario/FNF-PsychEngine` repository, pinned to the upstream 0.6.3 commit `eb0ade39e8d1c5437db2166a9ddb6eb3c05956e1`.
- Both GitHub Actions workflows check out that submodule.
- The native-preview workflow reads the menu art, stock song files, charts and license from this checked-out source tree instead of fetching individual raw asset URLs.
- Upstream `Project.xml`, Haxe classes, Flixel/OpenFL/Lime dependencies and other source files are present through the submodule. Merely checking them out does not mean the PS4 app compiles or executes them.

## What the current native preview does

- `src/main.cpp` draws a native preview of the Psych Engine 0.6.3 menu using original art and the OpenOrbis sample renderer.
- D-pad Up/Down navigates items; Cross opens a page and Circle returns.
- Options has native toggles for version labels, controller hints and background visibility.
- Freeplay lists stock tracks plus custom folders under `/data/PSYH00001/songs`.
- The experimental playtest parses parts of Psych Engine chart JSON and displays four lanes, hit/miss feedback, score and results.
- OGG decode/mixing and PS4 audio output are implemented as prototype paths, but must still be validated on a real console.

These are native-preview features, **not proof that the real Psych Engine gameplay runtime is working**.

## Build artifact

Open **Actions → PsychEnginePs4 native bootstrap** and download `PsychEnginePs4-native-preview-v0.2` from the latest successful run. The artifact contains the native preview PKG, ELF and package metadata.

A green Actions run only confirms the current native preview compiled and packaged successfully. It does not prove the game runs on hardware.

## What's still needed for the real game

The PS4 executable currently does **not** run the source under `vendor/PsychEngine`. The source uses Haxe, HaxeFlixel, OpenFL and Lime, and the upstream `Project.xml` does not define a PS4 target. The significant work is to port/build the hxcpp and Lime/OpenFL runtime for OpenOrbis, implement the PS4 graphics/input/audio/filesystem backends, then connect the original Psych Engine game loop and assets. Only after that can the port replace the preview with the actual game.

Until that runtime work is done, the following are not integrated into the original gameplay: Story Mode, characters and stages, full chart/sustain handling and exact audio sync, Lua/mod support, and the real asset-loading pipeline. The native preview should not be described as a playable Psych Engine port yet.

## Next milestones

1. Make the original Haxe source build against a viable PS4 runtime target.
2. Implement and validate Lime/OpenFL rendering, controller input, audio and filesystem access through OpenOrbis.
3. Link the real Psych Engine game loop, chart parser, asset loader and states; remove the native menu/playtest replacement.
4. Test a full song on compatible PS4 homebrew hardware and fix timing/audio issues.

This is an independent community project and is not affiliated with Sony or the original Psych Engine developers. Upstream code, assets and song data retain their respective licenses; the upstream license is included in the preview package.
