# Development

For playing, use the [release download and setup instructions](../README.md).
This guide is for building or contributing to the native port.

## Source and tools

Clone with recursive submodules; the checked-in pins select PSXRecomp and recomp-ui:

```sh
git clone --recurse-submodules https://github.com/mstan/MegaManX4Recomp.git
cd MegaManX4Recomp
```

Builds use CMake, a C/C++ toolchain, Python, SDL2 and OpenGL. Windows release
builds use MSYS2/MinGW; Linux builds use GCC and Ninja. Follow the pinned
[framework's build instructions](../psxrecomp-v4/README.md) for prerequisites
and BIOS generation. OpenBIOS is bundled and can be generated without a retail
BIOS dump. Do not rely on generated output from a different framework revision.

Supply a Mega Man X4 USA disc matching [DISC.md](../DISC.md). Keep your `.cue`
and `.bin` files under the ignored `mmx4/` directory and extract its
`SLUS_005.61` executable with the framework's `tools/extract_psx_exe.py`.
Do not convert the source disc to a cooked 2048-byte ISO: X4 uses XA sectors.

Build the framework's `psxrecomp-game` and `psxrecomp-bios` emitters first.
[tools/regen.ps1](../tools/regen.ps1) invokes the game emitter on Windows;
on Linux run `psxrecomp-v4/recompiler/build/psxrecomp-game --config game.toml`.
Generate OpenBIOS with the framework's `tools/regen_bios.sh --config bios/OpenBIOS.toml`.
Generated C stays in ignored output directories and is never hand-edited.

Configure the game after generation. Example release configuration:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPSX_EXECUTION_PROFILE=ENHANCED -DPSX_SDL_BACKEND=SDL2 \
  -DPSX_NETPLAY=ON -DPSX_RECOMP_UI=ON \
  -DRNET_ENABLE_ICE=ON -DRNET_ICE_BUNDLE_STATIC=ON \
  -DPSX_DEBUG_TOOLS=OFF -DMMX4_COOP_NETPLAY_DEBUG=OFF -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run from the project directory with an explicit disc path. Windows executables
have an `.exe` suffix. Public Windows packaging also requires
`PSX_STATIC_RUNTIME=ON`.

## Release and investigation tools

| Tool / file | Purpose |
| --- | --- |
| [game.toml](../game.toml) | Game generation, hooks and development defaults |
| [packaging/release](../packaging/release) | Release version and portable defaults |
| [package_coop_playtest.py](../tools/package_coop_playtest.py) | Package an existing Windows production build |
| [package_native_appimage.sh](../tools/package_native_appimage.sh) | Package an existing native Linux build |
| [test_appimage_layout.sh](../tools/test_appimage_layout.sh) | Verify AppImage contents and writable-data behavior |
| [stage_linux_sources.py](../tools/stage_linux_sources.py) | Snapshot committed source and generated shards for a private Linux build |
| [test_mmx4_coop_regressions.py](../tools/test_mmx4_coop_regressions.py) | Hidden original-disc scenarios with bounded stage/teleport fixtures |
| [test_mmx4_coop_netplay_debug.py](../tools/test_mmx4_coop_netplay_debug.py) | Synchronized two-peer development scenarios |
| [replay_mmx4_diagnostics.py](../tools/replay_mmx4_diagnostics.py) | Offline replay and hash verification of local recordings |

Development fixtures require `PSX_DEBUG_TOOLS=ON` and
`MMX4_COOP_NETPLAY_DEBUG=ON`. Release packages disable both. Always use isolated
settings, cards and hidden windows when running automated scenarios.

[Disc inventory](DISC_INVENTORY.md) documents the resident executable and asset archives; a runtime capture alone is not evidence of a separate disc overlay.

Framework changes belong upstream in PSXRecomp. Keep game images, generated
code, memory cards, native caches, raw overlay captures and Ghidra databases
out of commits. Export human Ghidra annotations instead. Game screenshot
imagery has its own [notice](../THIRD_PARTY_NOTICES.md).
