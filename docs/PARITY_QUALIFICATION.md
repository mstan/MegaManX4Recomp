# X4 parity qualification queue

Source branch: `campaign/mmx4-parity-hle-20261006`, based on `51f5f44`.
Owner `master` remains at `74aa7e8`; the candidate retains the six newer commits
already present in `origin/master`. Framework pin:
`8f95b599997399a476062e99634ddbb67e5c8731` (CODEGEN 18, emitter `18d1867bb4`).
Central tracking: `beads-eio.9.2`, beneath X4/System PlayStation.

This is a source milestone. No compiler, game, disc hash, download, AOT generation
or package compression ran during preparation. Two portable configuration tests,
PowerShell parsing, Bash syntax and whitespace checks passed. Coordinator grants
each subsequent resource phase; at most two compiler jobs and one game process
may run globally. Owner playtesting is the final gate.

## Applied source and actual limits

| Requirement | Candidate source/evidence | Remaining qualification |
| --- | --- | --- |
| Enhanced display | OpenGL and explicit 1080p target in development/player config; one portable player config feeds both packagers. | Fresh settings, correct 5x/area-resolved target, existing settings preserved, movement/UI seams at 1080p. |
| Native scene interpolation | Existing feature blends completed images at display refresh/fixed rates. Shared motion/replay services are available after repin. | Actual X4 world draw boundary, actors/camera positions and identities, current late HUD/fade packets, native phase images. Blending is insufficient. |
| Adaptive 4:3 through 32:9 | Historical 4:3/16:9 backgrounds, draw/lifetime classifier funnels, source-filtered HUD and Jungle ring refill. | Expanded background/packet/residency producer, safe placement activation, transitions/revisits, 21:9/32:9 images and live resize. Current control remains 16:9. |
| PGXP/perspective/filtering | Main world consists of authored 2D sprites/tiles; nearest filtering retained for this pixel art. | Inventory actual 3D/GTE usage before claiming applicability; sharp UI and moving world inspection. A stable 3D filter setting alone would not classify 2D world/UI. |
| Distance/fog/subdivision | No relevant 3D span established in this bounded source inventory. | Inspect any exceptional polygon/effect paths; do not call this universally inapplicable without broader coverage. |
| Loads/HLE | Verified ARC namespace and resident EXE; original loader callbacks, graphics/sound queues, texture decoder identified below. Existing Fast Loading controls host pacing/CD speed. | Cold startup, initial stage, transitions/deaths/revisits, caller contract and useful host performance; retained working reference. No title HLE family is registered yet. |
| Multiplayer | Existing framework integration retained. | Validate any currently supported mode within one profile; this campaign adds no new multiplayer mode. |
| Artifacts | ENHANCED/REFERENCE packaging selection, isolated default build directories, two-job defaults, correct FrameworkRoot include, final-byte execution binding. | Fresh generated game/BIOS, canonical current-ABI AOT audit, native Windows/Linux builds, ZIP/AppImage receipts, owner playtest. |

The profile is build-time selection, not a player implementation toggle.
It does not promote unqualified HLE or certify existing BIOS HLE. Historical
`bios_hle` preferences remain unchanged as shared migration is separate work.
REFERENCE must be built/run, including the separate faithful BIOS boot route
when comparing BIOS acceleration. Normal memory cards remain game data; do not
exchange profile-specific savestates.

## Concrete native interpolation and wide draw candidates

Evidence: [widescreen annotations](../annotations/widescreen_bg2d_sites.md),
plus read-only inspection of existing owner-generated `SLUS_005.61_full_06.c`.
Generated code is evidence; original disc instructions must validate any new
hooks before emission. Never copy X6 addresses.

- Background driver `0x80026648` resets scratchpad tile count `0x1F80011C`,
  iterates three `0x54`-byte layer records at `0x801419B0`, and calls layer update
  paths before selecting the background packet arena. It is not established as
  a pure whole-world draw wrapper. Replaying this driver unchanged risks running
  stage/streaming updates again.
- Leaf `0x80026AA0` consumes the layer index in `a0`, builds tiles into the live
  scratchpad packet pointer `0x1F800108`, and reads layer scroll. It draws
  backgrounds only. Sprite/effect producers and final OT submission still need
  separate capture boundaries. The shared presenter cannot turn a background-only
  replay into whole-scene interpolation.
- Tile streamer `0x8002728C`, ring base `0x801441C8`, map width `0x80172224`,
  layer stride `0x8013BD48`, and three layers are the verified X4 contracts.
  X4 does not compose the X5/X6 parent selected at `+0x52`.
- The stock 21-column by 16-row leaf is widened to 29 columns at 16:9; its ring
  contains only 32 columns. At 32:9 a 240-line view needs about 854 horizontal
  pixels, roughly 55 tile columns. Merely setting a wider aspect exceeds the
  ring. The shared packet cap is 1000 in a 1024-slot, `0x4000`-byte bank at
  `0x8015D9D0`; the historical dense 16:9 sample used 652. A host producer or
  audited larger owned arena must preserve layer order, transparency and residency.
- Actor funnels `0x8002B160`, `0x8002B1E8`, `0x8002B288`, `0x8002B318`,
  `0x8002B3C0` and vertex draw cull `0x800D46F4` are existing hook evidence.
  Wider draw/lifetime windows do not prove placement activation or event safety.
- Keep HUD range `[0x80139800,0x8013A000)` source-filtered. Capture current late
  fade/dialogue/menu packets and stage-select framing; do not move every 2D
  primitive as though it were world geometry.

Next native step: establish the smallest complete world draw span after logic,
identify sprite/camera 16.16 inputs and reusable-object identity, isolate packet
bank/OT ownership and the real display/VSync generation. Use shared motion/replay
transactions; include intermediate phase screenshots and side-effect verification.
Do not use the broad update driver simply because it draws something.

## Concrete load/HLE candidates and caller gates

Evidence: [original-disc inventory](DISC_INVENTORY.md), its guarded
`aot/disc_inventory.json`, and read-only existing generated bodies. The primary
disc is USA SLUS-00561, SHA-1 `26ffe24a79384b24af7571674251ee575e889b38`.
No separate game executable overlay was found; preserve that newer inventory.

| Boundary | Observed interface/effects | Next caller evidence |
| --- | --- | --- |
| `0x80013614` archive table lookup | Fixed 12-byte LBA/size/first-word records at `0x800F0E18`; all 138 records matched ISO. | Actual argument/output register use, archive index limits and observers of loader globals. Lookup alone is unlikely to dominate load time. |
| `0x800136B0` sector delivery | Original RAM callback forwards here; guest CD/device completion remains involved. | Submission versus completion ownership, async callbacks, status ordering, cancellation, XA/FMV exclusion and timing dependencies. Do not replace with false success. |
| `0x80013E68` archive callback / `0x80014008` dispatch | 2048-byte header then typed members; static callback table copied from `0x80010014`. | Header progress, descriptor cursor, per-member callback/result and buffer lifetime; resident pack keyed to the mounted effective disc. |
| `0x80014140` RAM member install | Verbatim data copy; published pointers through `0x800F15BC`. | Required destination/length/overlap, pointer publication before dependent readers, reset/revisit behavior. |
| `0x800142BC` / `0x800148EC` graphics | Queue then image upload consumer. | Queue occupancy/order, palette/VRAM rectangles, producer buffer reuse and GPU draining. |
| `0x80014514` / `0x80014968` sound | Queue then sound upload consumer. | SPU transfer address/wrap, callback flags, sequences and live audio during transition. |
| `0x80016FF4`, caller `0x80015F54` | Small halfword texture decoder: source `a0`, destination `a1`, MSB-first groups of 16 flag bits. Literal words, zero runs and backward copies; zero length/distance terminates. Output goes to `0x8016DEA8` or `0x8016EEA8 + slot*0x1000`; caller builds image rectangles, upload wrapper `0x80015E54`. | Caller reads of advanced `a0/a1`, `v0` and scratch registers, source bounds/destination ownership, overlapping backward copies, malformed/truncated input, interior entry coverage, actual load cost. Source/destination pointer changes may be caller ABI, so a nominal `void decode(src,dst)` contract is premature. |

Decoder tokens use upper five bits for short length and low eleven bits for
backward distance in halfwords. A zero short length reads an extended length;
zero distance with nonzero length emits zero halfwords. Copy is forward and may
overlap its history. Do not substitute `memcpy` for this loop. Confirm termination
and every actual observer before selecting a native implementation. Replacing
the archive completion boundary may buy much more than a small decoder alone.

## Next build phase (coordinator grant required)

The isolated worktree has no private assets or generated C. Use the existing
owner assets read-only. Create an ignored local emission config at the title
root from `game.toml`, changing `game.exe` to
`F:/Projects/psxrecomp/MegaManX4Recomp/mmx4/SLUS_005.61`, `game.disc` to the sibling
`Mega Man X4.cue`, and `recompiler.bios_config` to an existing verified canonical
SCPH1001 profile with its owner dump. Keep all seeds/hooks and `out_dir=generated`.
Use the existing CODEGEN-18 emitter only after its provenance is checked:

```powershell
$title='F:/Projects/psxrecomp/_wt-parity-mmx4-hle-20261006'
$fw='F:/Projects/psxrecomp/_wt-parity-hle-20261006'
Set-Location $title
& 'F:/Projects/psxrecomp/_build-fwtests-7b253941-20261006-sol-recompiler/psxrecomp-game.exe' --config _campaign-emission.toml
& 'C:/Program Files/CMake/bin/cmake.exe' -S $title -B "$title/build-campaign-enhanced" -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe `
  -DCMAKE_BUILD_TYPE=Release -DPSX_EXECUTION_PROFILE=ENHANCED `
  "-DPSXRECOMP_V4_ROOT=$fw" "-DPSXRECOMP_ROOT=$fw" `
  -DRECOMP_UI_ROOT=F:/Projects/psxrecomp/MegaManX4Recomp/recomp-ui `
  "-DPSX_THIRD_PARTY_DIR=$fw/third_party" -DPSX_RECOMP_UI=ON `
  -DPSX_DEBUG_TOOLS=ON -DPSX_PGXP_VARIANT=OFF -DPSX_NETPLAY=OFF `
  -DPSXRECOMP_REQUIRE_GAME_C=ON -DPSXRECOMP_BIOS_STALE_FATAL=ON
& 'C:/Program Files/CMake/bin/cmake.exe' --build "$title/build-campaign-enhanced" --target psx-runtime mmx4_preloaded_mods_test --parallel 2
& 'C:/Program Files/CMake/bin/ctest.exe' --test-dir "$title/build-campaign-enhanced" -R 'mmx4_(preloaded_mods|release_config)' --output-on-failure
```

Repeat in `build-campaign-reference` with `PSX_EXECUTION_PROFILE=REFERENCE`.
Verify the actual profile/manifest and runnable reference rather than assuming
the setting establishes title parity. BIOS regeneration/stale-backend resolution
and fresh AOT are separately granted work. Existing shards must not silently run
interpreted after the ABI/codegen repin.

## Next gameplay and artifact phases (separate grants)

Use isolated saves/mod state and ports (e.g. 4594); launch the approved executable
with `--game <title>/game.toml --disc <owner-cue> --memcard-dir <private-dir>
--debug-port 4594 --renderer opengl`. `Start-Process` helpers use `-WindowStyle
Hidden`. Do not overwrite personal settings, cards or checkpoints. Repeat actual
input routes from fresh starts for both profiles, rather than transferring states.
The existing Jungle route depends on a particular local memory card; retain that
dependency explicitly before using `tools/replay_input_route.py`.

After native interpolation exists, observe a known playable window:

```powershell
py -3 "$fw/tools/qualify_render_passes.py" --port 4594 --case 'X4 Jungle movement native phases' --seconds 10 --purpose correctness --executable "$title/build-campaign-enhanced/MegaManX4Recomp.exe" --output "$title/build-campaign-enhanced/native-correctness.json"
py -3 "$fw/tools/load_probe.py" --port 4594 run --secs 45 --out "$title/build-campaign-enhanced/stage-load.json"
```

Correctness requires `PSX_RENDER_PASS_VERIFY=1` at launch; performance requires a
separate relaunch without verification and `--purpose performance`. The current
blended feature should fail native-pass qualification. Capture 4:3/16:9 first;
after implementing expanded backgrounds, resize to 21:9/32:9 and test sky/doors,
Jungle/ice/parallax, bosses, X/Zero, HUD, pause/menus/FMV, fade/darkness, deaths,
revisits and newly revealed actors. Record useful host startup/load/frame costs.

Windows packaging, after canonical cache audit and build approval:

```powershell
./tools/package_release.ps1 -Version v0.0.5-parity-candidate -BuildDir build-campaign-enhanced -CacheBuildDir <fresh-current-ABI-cache-build> -FrameworkRoot $fw -ExecutionProfile ENHANCED -Jobs 2 -SkipRegen
```

This command still builds the emitter/runtime and may generate BIOS/download the
toolchain; `SkipRegen` is not a source-only/stage-only promise. It requires its own
grant. Existing historical private-capture AOT route remains pending canonical
audit. No unverified cache-removal/migration is part of source preparation.

Native Linux uses the same emission hooks, a fresh Linux ELF and matching `.so`
cache, then:

```sh
bash tools/package_appimage.sh --framework /path/to/qualified/psxrecomp --profile ENHANCED --build-dir build-campaign-linux-enhanced --jobs 2 --skip-build --version v0.0.5-parity-candidate
bash tools/test_appimage_layout.sh release-linux/MegaManX4Recomp-v0.0.5-parity-candidate-linux-x86_64.AppImage
```

This legacy wrapper still hashes/downloads/stages/compresses; grant first. Its
147-shard assertion must be reconciled with fresh emission, not guessed away.
When migrating the legacy route, shared `tools/package_game_release.sh
--stage-only` stages/binds the executable; shared `tools/package_appimage.sh`
accepts a validated native Linux payload and binds the ELF after linuxdeploy.
Both final candidates need execution identity, native coverage/catalog checks,
package hashes, no retail BIOS/disc/player-save payload, and owner playtest.
No public push, merge or release is authorized by this source milestone.
