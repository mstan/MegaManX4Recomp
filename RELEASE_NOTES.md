# Mega Man X4 Recompiled — v0.0.1-alpha

The first public cut, made days after first boot. Mega Man X4 boots and
**plays** as a native Windows program — no emulator behind it — on the
[PSXRecomp](https://github.com/mstan/psxrecomp) framework, the same one behind
[TombaRecomp](https://github.com/mstan/TombaRecomp),
[MegaManX5Recomp](https://github.com/mstan/MegaManX5Recomp) and
[MegaManX6Recomp](https://github.com/mstan/MegaManX6Recomp). The game's MIPS
code is machine-translated ahead of time into native C and compiled into a real
Windows executable that runs on a faithful simulation of the PS1 hardware plus
the recompiled PS1 BIOS.

## ✨ Highlights — what works

- **Boots and plays.** BIOS → disc detect → engine load → intro cinematics →
  title → attract demos → into the game, with no known crashes on the covered
  path.
- **The intro cinematics decode and play** (the X vs. Zero opening), riding the
  faithful CD read/seek-timing work that unlocked MMX5's FMV.
- **Correct-by-hardware controller handling.** X4 shipped *before* the
  DualShock existed and its pad driver rejects analog pads outright — exactly
  like the real console, where a DualShock with the analog LED lit is ignored
  by this game. The runtime presents the plain digital pad X4 expects, and the
  launcher offers no pad-mode selector (there is exactly one valid mode).
  Keyboard and SDL gamepads both work.
- **A recompiler milestone made X4 possible:** X4's engine keeps inline data
  tables inside its code, which tripped the old function-extent heuristic into
  classifying ~261 KB of real engine code (7,373 functions) as data. The new
  control-flow-aware extent analysis recovers them — this fix benefits every
  PSXRecomp title.
- **Fast loading (turbo loads).** Loads fast-forward the whole machine at full
  host speed while keeping authentic 1× guest CD timing (and audio) intact.
- **Supersampling + anti-aliasing.** Internal-resolution SSAA (1×–4×) with
  optional linear present filtering.
- **Graphical launcher.** Pick BIOS / disc / memory cards, verify the disc, and
  configure renderer, supersampling, and controller — choices persist between
  launches.
- **Self-contained overlay toolchain.** As you explore new areas the runtime
  converts the game's streamed ARC overlay code to native code in the
  background — no developer tools needed; the release bundles a fully
  self-contained toolchain (embedded Python + TinyCC).

## ⚠️ Known issues

- **Early preview — not verified deep into stages.** The covered path (intro,
  title, menus, attract, starting a game) works with no known crashes, but X4
  has ~2,800 not-yet-recompiled code regions that an unvisited area may hit. By
  design that halts the program loudly instead of silently misbehaving — if it
  happens to you, please report where you were.
- **Memory-card save/load not yet verified end-to-end** in this build (the
  card hardware layer is exercised and healthy).
- **No widescreen this release.** X4 ships **4:3 only**; the launcher's
  Widescreen toggle is hidden for this title until the 2D wide field-of-view is
  ported (same mechanism as MMX6's).

## 📝 Setup

- **Bring your own** PlayStation BIOS (`SCPH1001.BIN`) and Mega Man X4 (USA,
  SLUS-00561) disc image — the launcher asks for each. Verify your disc against
  `DISC.md` before reporting regressions. Use `.cue` + `.bin` (or `.bin`); do
  **not** convert to a 2048-byte "cooked" `.iso` — that discards the XA sectors
  the movies and audio stream from.
- Options live in the launcher's **Settings** and are remembered between
  launches.
- The overlay cache grows as you play; please keep `overlay_captures.json`
  private — it contains game code read from your disc (see README).
