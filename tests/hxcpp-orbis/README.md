# Haxe to hxcpp to OpenOrbis smoke test

This test is intentionally smaller than Psych Engine. It verifies the lowest part of the proposed porting path: Haxe 4.2.5 emits C++ and hxcpp 4.2.1 compiles and links that program with the OpenOrbis PS4 toolchain.

The custom hxcpp target is installed into the CI Haxelib checkout at build time. It uses the OpenOrbis sysroot, x86_64-pc-freebsd12-elf, __ORBIS__, and the same linker script and startup object used by the OpenOrbis sample projects.

A passing result proves only that a small Haxe-generated program can be cross-compiled to a PS4 ELF. It does not yet prove Lime, OpenFL, HaxeFlixel, audio, graphics, input, or the full Psych Engine can run on PS4.

Workflow: .github/workflows/hxcpp-orbis-smoke.yml.
