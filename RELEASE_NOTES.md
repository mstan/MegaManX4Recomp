# Mega Man X4 Recompiled — v0.1.1-coop-playtest.3

Two-player co-op regression update for Windows and Linux. Unified remains
the default; online Split cameras remain experimental.

- P2 owns its pause menu and can resume it. Native world drawing, charge
  glow/projectiles, saber hits, dash trails and water effects survive pause.
- Both players' jump/hurt sounds can overlap through independent voices.
- P2 can activate enemies and Web Spider's secret-hole camera.
- Intro, Jet Stingray and Cyber Peacock handoffs retain living players at
  native landing heights. Cyber orbs prevent withdrawal while captured.
  Jet uses one READY actor, and a dead P2 stays dead across its sections.
- Victory has one campaign reward and departure. P1 owns persistent progress
  and upgrades; boss clears agree in Continue, native cards and grey portraits.
  Fresh stage selection resets carry state so re-entry cannot inherit a corpse.
- Playtest Diagnostics defaults on, displaying both accepted input streams
  and recording inputs plus starting memory cards/mod choices under
  `saves/diagnostics`. Offline replay requires the matching build and settings.

The numbered [regression checklist](docs/COOP_PLAYTEST_REGRESSIONS.md) has all
26 outcomes checked. Validation includes all 16 title CTests, hidden original
disc scenarios, a two-boss native save/cold-load loop, and matching two-peer
development netplay state through tick6253. These checks cover the reported
cases; they do not establish complete campaign coverage. Split performance
and an intermittent netplay watchdog stall remain open investigations.

The Windows/Linux launcher lobby, two local controller assignments, stacked
default HUD and Side by side option remain available. Use the same release
on both peers. Co-op requires OpenGL and a fresh boot after changing mods.

## Previous v0.1.1-coop-playtest.2

Windows and Linux two-player co-op playtest. Unified cameras and vertically
stacked lifebars are the defaults; Split cameras remain experimental.

- The launcher exposes Netplay and its two-player lobby on both platforms.
- Local play offers separate Player 1/Player 2 controller assignments,
  including two controllers of the same model.
- Player 2 uses their own character's attack, hurt and death audio. Private
  audio banks prevent the campaign character's samples from replacing them.
- Player 2's death particles start at their own position, with red particles
  for Zero and blue particles for X, while Player 1 stays alive.
- Both lifebars share the left anchor in widescreen. Mods offers **Stacked**
  (default) and **Side by side** HUD layouts.
- Player 2 no longer appears on the Quit Game screen.

The owner approved the exact Windows build after checking P2 damage following
pause/resume, death-particle ownership/color, both character voice orders,
HUD alignment/layout and Quit Game drawing. The candidate passed all 14 title
checks; Linux also passed them and the native private-audio-bank regression.
The Windows download packages that tested executable without rebuilding it.

Enable **X + Zero Co-op** in Mods for local play and assign both players under
Controls. For online play, select **Netplay** and the **X + Zero Co-op** profile.
The host chooses cameras/HUD layout for both peers. Use the same release on
both computers. Offline play uses Unified. Co-op requires OpenGL and a fresh
game launch after changing mods. Controllers use Start/Options to pause;
keyboard pause follows each player's Controls binding.

The Linux x86_64 AppImage requires glibc 2.38 or newer. Make it executable
with `chmod +x`, then run it; `--appimage-extract-and-run` works without FUSE.
Writable data lives in `~/.local/share/MegaManX4Recomp`.

Split can still run below full speed, including on LAN. Split refill/section
handoffs, vehicles, chained scenes and retry/menu disconnects need additional
playtesting. This is a playtest release with limited stage coverage.

## Previous v0.1.1-coop-playtest.1

An early Windows and Linux co-op playtest. Unified is the default camera mode;
Split (Experimental) is available for online co-op. The current main release
remains available separately.

The Linux x86_64 AppImage requires glibc 2.38 or newer. Make it executable
with `chmod +x`, then run it; use `--appimage-extract-and-run` on systems
without FUSE. It keeps writable data in `~/.local/share/MegaManX4Recomp`.
Linux and Windows use the same declared co-op gameplay and execution contracts.

- Simultaneous native X and Zero; vertically stacked lifebars in the shared view.
- Independent online views of one shared world, with local player HUDs.
- Distant Split backgrounds render from the authored map, avoiding shared
  tile-ring corruption when players separate.
- Split health refills stay with their collector, without the shared world
  pause or partner-camera snap. Unified retains the native refill pause.
- Living outgoing teleport passengers survive native section transfers.
  Actual deaths stay dead until a team retry, and scene handoffs retain their
  native teleports.

Enable X + Zero Co-op in Mods for local play. Online rooms use the X + Zero
Co-op profile; the host chooses the camera mode for both peers. Use the same
build and OpenGL on both computers. Offline play always uses Unified. Co-op
is opt-in outside its netplay profile and takes effect on a fresh boot.

Validation includes focused ownership, lifecycle, camera-boundary and authored
background regression checks; original-disc loopback netplay separation; and
owner playtesting in Web Spider. The owner confirmed the boss door worked and
distant background corruption was substantially improved. This does not
establish full stage/campaign/door coverage. The prior run's logs lacked health
history, so the reported area-2 death cannot be conclusively attributed to the
transition regression found in the source.

The exact production binary passes all 13 configured CTests, native room-offer
checks for both camera choices, shared catalog/execution staging, and system
DLL dependency checks. Two input-neutral production peer pairs agree on their
logged core/mod state during boot. The timed attract-gameplay probe did not
reach gameplay; it remains failed and provides no stage coverage. The latest
asynchronous refill and section-transfer changes still need native playtesting.

Known limits: Split may run below full speed even on LAN. Vehicle stages,
chained scenes, surviving-player handoffs, and retry/menu paths need more
coverage. An intermittent post-retry menu disconnect is still under
investigation. This release is intended for playtesting, with isolated co-op
saves; it is not a finished co-op port.

## Previous v0.1.0-alpha

- Adaptive authored-map renderer replaces the retired fixed 16:9 tile-ring path.
- OpenGL and 1080p internal resolution are enabled by default, with pixel-art sampling.
- Title-specific resident ARC loading defaults on; generic CD/host timing options are retired.
- Wider enemy activation, persistent kills at wide edges, distant destructible door hitboxes, and adaptive intro searchlights.
- Gameplay uses the approved 2x guest CPU budget while device clocks retain their timing.
- Bundled OpenBIOS, normal damage by default, and interpolation unavailable.
- Windows x64 ZIP and native Linux x86_64 AppImage use the shared release staging and execution-contract checks.

The owner approved the Windows gameplay candidate. Package checks cover startup,
version, dependencies and defaults; they do not establish complete stage coverage
or a measured correction to the historical sound-delay concern.

Release packaging uses `tools/package_release.sh` for the Windows ZIP and
`tools/package_native_appimage.sh` for the Linux AppImage after a production
build. Both delegate catalog, toolchain and executable-contract gates to the
pinned shared framework; the older standalone packagers are historical.

## Historical v0.0.2-alpha notes

Historical coverage correction (2026-09-11): the ARC-overlay claims below were
assumptions. Original-disc inspection found no separate game overlay; see
[the inventory and its limits](docs/DISC_INVENTORY.md). No new release or version
bump accompanies this correction.

## 🆕 New in v0.0.2

- **Experimental Widescreen (true 16:9).** X4's opt-in wide field of view is now
  available from the launcher's **Widescreen (EXPERIMENTAL)** toggle — it widens
  the three-layer scrolling background and re-anchors the health/weapon HUD to
  the true 16:9 edges, without disturbing world sprites. **4:3 remains the
  default**; flip the toggle to try 16:9.
- **OpenGL is now the default renderer** (full-rate hardware presentation).
  Software is still one click away in the launcher (Settings → Renderer).
- **A large batch of framework engine improvements** since v0.0.1 — full-rate
  OpenGL presentation, audio reserve/transient hardening (the MMX4 attract-demo
  audio fix and beyond), and byte-verified static + overlay dispatch. X4 was
  re-recompiled and re-validated against the updated framework.

---

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
- **Widescreen is experimental and opt-in.** Authentic 4:3 remains the default;
  enabling the launcher toggle widens X4's background, actor activation/despawn,
  and draw-cull bounds for a true 16:9 field of view. Player health/weapon HUD
  pieces move to the 16:9 left boundary and enemy/boss health pieces move to the
  16:9 right boundary without changing their original layout.

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

# Mega Man X4 Recompiled — v0.0.3-alpha

This release replaces the old in-tree launcher with the shared Dear ImGui
`recomp-ui` launcher. BIOS/disc selection, settings, controls, memory cards,
and the Launch button now use the DPI-independent shared layout, addressing
the inaccessible/broken launcher reported in issue #3.

## Launcher and packaging

- Uses the current shared `recomp-ui` PSX profile and game-specific launcher
  art.
- Bundles the matching fonts and assets beside the executable.
- Preserves the existing renderer, widescreen, controller, and runtime choices
  while removing the fixed old launcher layout.

All existing alpha caveats and game-compatibility notes above still apply.

# Mega Man X4 Recompiled — v0.0.4 audio latency test 1

This prerelease reduces X4's host audio cushion from 180 ms to 60 ms. The
smaller cushion should make sound effects respond much sooner. It is an X4-only
choice: the framework retains the safer 180 ms default for games that need it
to cover long audio-production stalls.

Please report whether the delay is improved and whether you hear any new
crackle, especially during stage transitions, cutscenes, or loading.
