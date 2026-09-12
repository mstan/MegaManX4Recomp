# MegaManX4Recomp

> _This recompilation is a **byproduct of developing
> [psxrecomp](https://github.com/mstan/psxrecomp)** — the games are the proving ground, the framework is the goal.
> **These are in-development previews, not finished ports — expect rough
> edges**, and depth will keep landing over months, not days. My time for any
> one title is limited, so I ask for your patience. Contributions are welcome —
> testing, issues, and PRs to the game or framework all help and will
> accelerate this game's polish. More on the why at:
> [Recomp + AI: 5 Months Later »](https://1379.tech/recomp-ai-5-months-later/)_

Mega Man X4 (USA, SLUS-00561) statically recompiled to a native PC executable
with [PSXRecomp](https://github.com/mstan/psxrecomp) — the same framework behind
[TombaRecomp](https://github.com/mstan/TombaRecomp),
[MegaManX5Recomp](https://github.com/mstan/MegaManX5Recomp) and
[MegaManX6Recomp](https://github.com/mstan/MegaManX6Recomp).

## What This Is

This repository contains the game-specific configuration, seeds, tools, and
build glue for running Mega Man X4 on the PSXRecomp framework. The game's MIPS
code is machine-translated ("recompiled") ahead of time into native C, then
compiled into a native Windows or Linux program that runs the game's own logic on a
faithful simulation of the PS1 hardware (GPU, SPU, GTE, memory cards) plus the
real, recompiled PS1 BIOS — no high-level emulation shims.

It does **not** contain the Mega Man X4 disc image, the PS1 BIOS, generated game
code, or any decompiled game C. Those are produced locally from your own legally
obtained assets.

Important files:

- `game.toml`: runtime / recompiler / video / controller config.
- `seeds/`: Ghidra-derived function starts and game-specific seed data.
- `tools/regen.ps1`: regenerates the recompiled C output.
- `tools/package_release.ps1`: builds the redistributable release zip.
- `tools/build_overlay_shards.sh`: rebuilds native Linux overlay shards.
- `tools/package_appimage.sh`: builds the reproducible Linux AppImage.
- `tools/test_appimage_layout.sh`: verifies the AppImage payload and seeding.
- `psxrecomp-v4.pin`: framework commit this project is known-good against.
- `ISSUES.md`: game-specific issue log.
- `DISC.md`: source-disc identity and verification hashes.

## Status

**Early playable preview — `v0.0.5-alpha`.** Mega Man X4 **boots and plays**: the intro
cinematics (X vs. Zero) decode and play, the title screen and menus respond,
the attract demos run, and you can start a game — with working controller
input and no known crashes on the covered path. It has **not** been verified
deep into stages or to the end; uncovered code paths halt loudly rather than
misbehave silently (see ISSUES.md #1).

| Area | State |
|---|---|
| PS1 BIOS boot | Works (real recompiled BIOS core, HLE-accelerated boot) |
| Disc-detect / boot | Works (resident executable and streamed assets) |
| Intro cinematics / FMV | Plays (X vs. Zero opening decodes) |
| Controller | Works; digital pad (X4 predates the DualShock — see below) |
| Title / menus / attract | Works |
| Stage gameplay | Starts; not yet verified broadly (see ISSUES.md #1) |
| Memory-card save / load | Card probing works; save/load not yet verified end-to-end |
| Renderers | OpenGL (default) **and** Software, selectable in the launcher |
| Built-in mods | Widescreen, interpolated frames, Fast Loading, and an integer Damage Multiplier |
| Widescreen 16:9 | Experimental default-off mod; true wider 2D field of view |
| Interpolated frames | Default-off mod; presentation only, game timing remains stock |

See `ISSUES.md` for notes and the remaining follow-ups.

## Features

These are the framework features that are already working in this build:

- **Two renderers.** A GPU-authoritative OpenGL backend (this release's default)
  and a CPU software rasterizer, both selectable in the launcher.
- **Game-owned Mods catalog.** The Mods page owns MMX4's experimental
  widescreen and temporal frame-blending enhancements, unified loading modes,
  and an integer Damage Multiplier for testing and save-state repair.
- **Opt-in true widescreen.** The experimental 16:9 mod widens X4's background
  tile window plus actor activation, despawn, and draw-cull bounds; authentic
  4:3 remains the default. Player health/weapon HUD pieces anchor to the true
  wide left edge, while enemy/boss health pieces anchor to the wide right edge.
  The generated dispatcher uses optimizer-independent binary lookup, avoiding
  the severe `-O0` slowdown caused by X4's nearly 60,000 dispatch entries.
  Release builds additionally compile out developer tracing and telemetry.
- **Interpolated-frames mod.** Select display-refresh or a fixed presentation
  rate from the Mods panel. It blends completed frames without changing guest
  VBlank, game logic, timers, or audio speed.
- **Fast Loading mod.** Disabled by default. One mutually exclusive selector
  offers host-only 2x/4x/8x/16x/uncapped pacing or experimental 2x/4x/instant
  guest-visible CD timing. Host pacing is the recommended timing-safe path.
- **Digital controller, as the game expects.** X4 shipped **before** the
  DualShock existed and rejects analog pads outright (with one presented, the
  title screen ignores Start entirely). The runtime therefore presents a plain
  digital pad and the launcher offers no pad-mode selector — there is exactly
  one mode the game supports. Keyboard and SDL gamepads both work.
- **Supersampling + anti-aliasing.** Internal-resolution SSAA (1×–4×) with
  optional linear present filtering for clean edges.
- **Self-contained Windows fallback toolchain.** The runtime can convert
  eligible dirty-RAM code using its bundled toolchain, and packaged native
  caches remain supported. A bounded original-disc inspection found no separate
  X4 game overlays in the ARC assets; see [the inventory](docs/DISC_INVENTORY.md).
  This is not a claim that every runtime path has native coverage.
- **Graphical launcher.** Pick your BIOS, disc, and memory cards; verify the
  disc; configure renderer / supersampling / controller, with live settings
  persistence — then press Launch.

## Setup

### Release Package (recommended)

On Linux, download `MegaManX4Recomp-v*-linux-x86_64.AppImage`, make it
executable, and run it:

```sh
chmod +x MegaManX4Recomp-v*-linux-x86_64.AppImage
./MegaManX4Recomp-v*-linux-x86_64.AppImage
```

The AppImage stores writable data under
`~/.local/share/MegaManX4Recomp` (override with `MMX4_RECOMP_DATA_DIR`).

On Windows:

1. Download `MegaManX4Recomp-v*-windows-x64.zip` from Releases and extract it.
2. Run `MegaManX4Recomp.exe`. A **launcher window** opens.
3. Set your PlayStation **BIOS**: select your legally obtained `SCPH1001.BIN`
   (a 512 KB file dumped from your own console).
4. Set the game **disc**: select your legally obtained Mega Man X4 (USA,
   SLUS-00561) disc image. The launcher verifies the ISO9660 header, region, and
   serial.
5. Optionally adjust renderer, supersampling, screen look, and controller
   settings, then press **Launch**. Your choices are remembered.

Accepted disc formats: `.cue` + `.bin` (preferred — pick the `.cue`) and `.bin`.
**Do not convert the disc to a 2048-byte "cooked" `.iso`** — that discards the
Mode-2 Form-2 XA sectors X4 streams its FMV/audio from. If the header or game
ID does not match `SLUS-00561`, the launcher warns and tries to run it anyway.

Selected paths persist next to the executable on Windows and under the
AppImage's writable data directory on Linux. Delete `settings.toml` there to
reset launcher choices.

### Building From Source

Windows builds use MSYS2/MinGW. Native Linux and WSL builds use GCC or Clang,
CMake, Ninja, SDL development files, OpenGL development files, ImageMagick,
and curl.

Requirements:

- A C/C++ toolchain (MSYS2 `mingw-w64-x86_64`) and CMake 3.20+.
- Mega Man X4 (USA, SLUS-00561) disc image (`.cue` + `.bin` or `.bin`). Not
  included. Verify it against `DISC.md` before reporting regressions.
- Sony SCPH1001 BIOS ROM (`SCPH1001.BIN`). Not included.
- The `psxrecomp` framework available at the sibling path `../psxrecomp` (linked
  in as the `psxrecomp-v4` junction at the `psxrecomp-v4.pin` SHA), plus a
  recompiled BIOS in `psxrecomp/generated/` (see the framework README).

The recompiler needs the game's PS-X EXE extracted from the disc. A helper is
included:

```sh
python3 ../psxrecomp/tools/extract_psx_exe.py "mmx4/Mega Man X4.bin" SLUS_005.61 mmx4/SLUS_005.61
```

Generate the recompiled C, then build and run:

```sh
# Regenerate generated/SLUS_005.61_{full_*,dispatch}.c from the disc/EXE.
#   Windows: pwsh tools/regen.ps1
#   (or invoke the recompiler directly:
#    ../psxrecomp/recompiler/build/psxrecomp-game.exe --config game.toml)

cmake -S . -B build -G "Unix Makefiles"
cmake --build build -j16
./build/MegaManX4Recomp.exe
```

The current recompiler emits X4 as parallel-compilable `full_*.c` shards.

To build the redistributable Windows release (regens, builds with the launcher,
bundles assets + toolchain, and zips it): `pwsh tools/package_release.ps1`.

To cut the Linux release from WSL or native Linux:

```sh
# Private capture input -> tagged gcc/linux-x64 .so shards.
bash tools/build_overlay_shards.sh --captures build-master/overlay_captures.json

# Builds all 141 static units, requires matching Linux shards, and packages.
bash tools/package_appimage.sh

# Read-only payload, writable-state, shard ABI, and path-portability checks.
bash tools/test_appimage_layout.sh \
  release-linux/MegaManX4Recomp-v0.0.5-alpha-linux-x86_64.AppImage
```

The packager accepts Windows output paths under WSL, stages its AppDir on the
native Linux filesystem, verifies pinned tooling by SHA-256, normalizes staged
timestamps, and refuses to ship a Windows `.dll` cache or a mismatched shard
tag. Set `SOURCE_DATE_EPOCH` explicitly to reproduce a historical cut.

## Configuration

Most options are exposed in the launcher and persist to `settings.toml`. The
underlying defaults live in `game.toml`:

- `[video]` — `renderer` (`software` / `opengl`), `supersampling` (1–4),
  `antialiasing`, `texture_filtering`, and `auto_skip_fmv`.
- `[controller]` — `default_mode` (`digital`, locked — X4 supports exactly one
  pad type), `deadzone`.
- `[runtime]` — authentic `disc_speed`/`turbo_loads` baselines, `bios_hle`,
  `overlay_cache`.

Widescreen and temporal frame blending are game-owned features on the Mods
page rather than duplicate generic Settings controls. The same page contains
Fast Loading and Damage Multiplier. Loading and display mods default off;
damage enforcement defaults to integer value 1 so save states cannot preserve
stale cheat code.

## Controls

| PSX button | Keyboard |
|---|---|
| D-Pad Up / Down / Left / Right | Arrow keys |
| Cross | X |
| Square | Z |
| Circle | S |
| Triangle | A |
| L1 / R1 | Q / W |
| L2 / R2 | E / R |
| Start | Enter |
| Select | Right Shift |
| Turbo | Tab (hold) |
| Fullscreen | Alt+Enter |

A game controller (Xbox, PlayStation, or any SDL-recognized pad) is supported via
SDL when connected. X4 is a digital-pad game; sticks map onto the D-pad.

| PSX button | Xbox controller |
|---|---|
| D-Pad Up / Down / Left / Right | D-pad or left stick |
| Cross | A |
| Circle | B |
| Square | X |
| Triangle | Y |
| L1 / R1 | LB / RB |
| L2 / R2 | LT / RT |
| Start | Menu |
| Select | View / Back |

Release builds include `input.ini` in their writable data directory (beside
the executable on Windows; under `~/.local/share/MegaManX4Recomp` for the
AppImage). Edit it to change controller device index, deadzone, or button
mapping. Keyboard bindings are configurable in `keybinds.ini` and
live-rebindable in the launcher's Controls page.

## Memory Cards

The runtime uses standard PS1 memory-card images (`.mcd` / `.mcr`) compatible
with DuckStation, PCSX-Redux, Mednafen, ePSXe, and similar emulators. Cards are
stored in the `saves` directory and managed in the launcher's memory-card UI.
X4's in-game save/load has not yet been verified end-to-end in this build (see
ISSUES.md #3). Runtime memory-card files are local artifacts and must not be
committed.

## Code coverage and private diagnostics

X4's resident executable is recompiled ahead of time. The original-disc and
loader inspection found no separate game overlay in its ARC files; those
inspected contain assets. See [the verified inventory and its limits](docs/DISC_INVENTORY.md).
Earlier claims that visiting every area was required to obtain X4's stage code
were unsupported.

The generic native cache, runtime compilation and interpreter fallback remain
available for eligible runtime code. A cache entry or dirty-RAM capture does not
by itself identify a distinct on-disc overlay or guarantee native coverage.

Keep `overlay_captures.json` private: it contains verbatim memory bytes from
your game and BIOS. Useful reports can describe the area, behavior and enabled
mods without publishing those bytes.

## Replaying the Jungle widescreen regression route

Developers can return to the Jungle location used to verify the widescreen
background-ring fix without manually navigating the menus and stage. The
checked-in route uses the current local memory-card save, selects the Jungle
stage, walks X to the captured location, and then releases all controls:

```powershell
python tools/replay_input_route.py tools/routes/mmx4_jungle_widescreen.json `
  --exe build-mods/MegaManX4Recomp.exe `
  --disc "F:\path\to\Mega Man X4.cue"
```

The replay is consumed once per guest VBlank, so the default turbo-loading
setting may remain enabled. To record a replacement route, launch with a debug
port, note its starting and ending frame numbers, then run
`tools/capture_input_route.py --start START --end END --output ROUTE.json`.

## Development Rules

- Use the real recompiled BIOS and real hardware simulation in PSXRecomp.
- No HLE BIOS shims, no stubs, no fake events, no hand-edited generated files.
- Framework changes go in `mstan/psxrecomp`, not here.
- Game binaries, generated code, memory cards, Ghidra databases, and build
  outputs stay local.
- See `CLAUDE.md` for project-specific rules.

## License

PolyForm Noncommercial 1.0.0. See `LICENSE`.

Mega Man X4 is copyright Capcom. This repository contains none of the game's
original binaries or assets. Release packages contain no game assets, no disc
data, and no BIOS image — those are always read from files you supply. The
release executable and the bundled `cache` folder do contain statically
recompiled (machine-translated) builds of the game's code, the same distribution
model used by other static recompilation projects such as N64: Recompiled.

---

<p align="center">
  <sub><b>R.A.I.D. — Retro AI Development</b> · a Discord for AI-assisted retro reverse-engineering, decomp &amp; recomp</sub>
</p>

<p align="center">
  <a href="https://discord.gg/Ad9BwSzctP"><img src=".github/raid-discord.png" alt="Join the Retro AI Development (R.A.I.D.) Discord" width="200"></a>
</p>
