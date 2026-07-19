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
compiled into a real Windows program that runs the game's own logic on a
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
- `psxrecomp-v4.pin`: framework commit this project is known-good against.
- `ISSUES.md`: game-specific issue log.
- `DISC.md`: source-disc identity and verification hashes.

## Status

**Early playable preview — `v0.0.2-alpha`.** Mega Man X4 **boots and plays**: the intro
cinematics (X vs. Zero) decode and play, the title screen and menus respond,
the attract demos run, and you can start a game — with working controller
input and no known crashes on the covered path. It has **not** been verified
deep into stages or to the end; uncovered code paths halt loudly rather than
misbehave silently (see ISSUES.md #1).

| Area | State |
|---|---|
| PS1 BIOS boot | Works (real recompiled BIOS core, HLE-accelerated boot) |
| Disc-detect / boot | Works (loads the engine and streamed ARC overlays) |
| Intro cinematics / FMV | Plays (X vs. Zero opening decodes) |
| Controller | Works; digital pad (X4 predates the DualShock — see below) |
| Title / menus / attract | Works |
| Stage gameplay | Starts; not yet verified broadly (see ISSUES.md #1) |
| Memory-card save / load | Card probing works; save/load not yet verified end-to-end |
| Renderers | OpenGL (default) **and** Software, selectable in the launcher |
| Widescreen 16:9 | Experimental opt-in; true wider 2D field of view (4:3 remains default) |

See `ISSUES.md` for notes and the remaining follow-ups.

## Features

These are the framework features that are already working in this build:

- **Two renderers.** A GPU-authoritative OpenGL backend (this release's default)
  and a CPU software rasterizer, both selectable in the launcher.
- **Opt-in true widescreen.** The experimental 16:9 mode widens X4's background
  tile window plus actor activation, despawn, and draw-cull bounds; authentic
  4:3 remains the default. Player health/weapon HUD pieces anchor to the true
  wide left edge, while enemy/boss health pieces anchor to the wide right edge.
  The generated dispatcher uses optimizer-independent binary lookup, avoiding
  the severe `-O0` slowdown caused by X4's nearly 60,000 dispatch entries.
  Release builds additionally compile out developer tracing and telemetry.
- **Fast loading (turbo loads).** While a load is in progress the whole machine
  fast-forwards at your PC's full speed, then drops back to normal the instant
  it finishes — so disc loads complete far faster while all of the game's
  internal timing (and audio) stays correct. Authentic 1× disc timing is kept;
  the speed comes from the load fast-forward, not from speeding up the emulated
  CD (which would break timing). On by default; toggleable in the launcher.
- **Digital controller, as the game expects.** X4 shipped **before** the
  DualShock existed and rejects analog pads outright (with one presented, the
  title screen ignores Start entirely). The runtime therefore presents a plain
  digital pad and the launcher offers no pad-mode selector — there is exactly
  one mode the game supports. Keyboard and SDL gamepads both work.
- **Supersampling + anti-aliasing.** Internal-resolution SSAA (1×–4×) with
  optional linear present filtering for clean edges.
- **Self-contained overlay toolchain.** As you explore new areas the runtime
  converts the game's streamed ARC overlay code to native code in the
  background. That needs no developer tools installed — the release bundles a
  fully self-contained toolchain (embedded Python + TinyCC), so newly visited
  areas are accelerated on any machine.
- **Graphical launcher.** Pick your BIOS, disc, and memory cards; verify the
  disc; configure renderer / supersampling / controller, with live settings
  persistence — then press Launch.

## Setup

### Release Package (recommended)

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

Selected paths persist next to the executable (`bios.cfg` / `disc.cfg` and
`settings.toml`). Delete those to pick different files or reset settings.

### Building From Source

Builds on **Windows (MSYS2/MinGW)**.

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
# Regenerate generated/SLUS_005.61_{full,dispatch}.c from the disc/EXE.
#   Windows: pwsh tools/regen.ps1
#   (or invoke the recompiler directly:
#    ../psxrecomp/recompiler/build/psxrecomp-game.exe --config game.toml)

cmake -S . -B build -G "Unix Makefiles"
cmake --build build -j16
./build/MegaManX4Recomp.exe
```

Note: X4's generated `full.c` is currently very large (~225 MB, a known
recompiler follow-up), so the single-TU compile step takes ~40 minutes.

To build the redistributable Windows release (regens, builds with the launcher,
bundles assets + toolchain, and zips it): `pwsh tools/package_release.ps1`.

## Configuration

Most options are exposed in the launcher and persist to `settings.toml`. The
underlying defaults live in `game.toml`:

- `[video]` — `renderer` (`software` / `opengl`), `supersampling` (1–4),
  `antialiasing`, `texture_filtering`, `aspect_ratio` (`4:3` / experimental `16:9`),
  `auto_skip_fmv`.
- `[controller]` — `default_mode` (`digital`, locked — X4 supports exactly one
  pad type), `deadzone`.
- `[runtime]` — `disc_speed` (kept at `1x`), `turbo_loads`, `bios_hle`,
  `overlay_cache`.

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

Release builds include `input.ini` next to `MegaManX4Recomp.exe`. Edit it to
change controller device index, deadzone, or button mapping. Keyboard bindings
are configurable in `keybinds.ini` and live-rebindable in the launcher's
Controls page.

## Memory Cards

The runtime uses standard PS1 memory-card images (`.mcd` / `.mcr`) compatible
with DuckStation, PCSX-Redux, Mednafen, ePSXe, and similar emulators. Cards are
stored in the `saves` directory and managed in the launcher's memory-card UI.
X4's in-game save/load has not yet been verified end-to-end in this build (see
ISSUES.md #3). Runtime memory-card files are local artifacts and must not be
committed.

## Help make your game faster — just by playing

**Why isn't the game already at full speed everywhere?** Most of X4's code is
converted ("recompiled") into a fast native program ahead of time. But
PlayStation games don't keep all of their code in memory at once — they stream
extra chunks of code off the disc as you reach new areas (these chunks are
called *overlays*; X4 streams stage and engine code from its `ARC/*.ARC`
archives). We can't convert a chunk we've never seen, and the only way to see
it is for someone to actually visit that area. Until then, that area's code
runs in a slower compatibility mode.

**Your cache grows as you play.** While you play, the runtime records newly
visited areas into `overlay_captures.json` and converts them with the bundled
toolchain; your own `cache` folder grows automatically and those areas run at
full speed from then on.

**Please do not post `overlay_captures.json` publicly.** It contains verbatim
snapshots of the game's code read from your disc, which is copyrighted material —
keep it on your own machine, alongside your disc image.

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
