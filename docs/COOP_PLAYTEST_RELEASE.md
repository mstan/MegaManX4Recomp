# Co-op playtest 2

Version: `v0.1.1-coop-playtest.2`. The owner-approved Windows executable is
`60ec07fe7597cec9a1a3ad0d12ecb019c3c7ae72b4f6e1197248419151b134f1`.
It is packaged unchanged. Source `e2952be` contains the tested death-particle,
audio, HUD-layout and quit-menu changes; the release pin retains those gameplay
sources while aligning the Linux execution stamp to the approved Windows build.

The owner confirmed P2 damage after pause, safe P1 during P2 death, correct
particle origin/color, voices in both rosters, HUD alignment, Side by side,
and no P2 gameplay drawing on Quit Game. Both platforms pass 14 title checks.
Native SPU regressions cover concurrent private sample banks, DMA replacement,
looping, KEYOFF, slot reuse and snapshot replay. Linux gets a fresh native build;
Windows uses the owner's tested executable. Package checks cover catalog,
fallback toolchain, release defaults, execution/co-op fingerprints and hashes.

Unified and Stacked remain defaults; Split is experimental. Remaining coverage
is the Split refill/section-transfer/door matrix, vehicles, chained scenes,
performance and retry/menu disconnects. This release does not claim those are
exhaustively validated. The unrelated music report was explicitly withdrawn.

## Previous co-op playtest 1

Version: `v0.1.1-coop-playtest.1`. Windows production binary SHA256:
`347d614a428227ab50d2eeedecfd04915906e46e0fc91d18236f3324b8368d4b`.
Framework pin: `8fff59f82cd2b712c0dd6f8b781722572f26f60b`.
Unified is the default; Split is labelled experimental. The co-op feature
remains opt-in outside its trusted netplay profile.

## Fixes and evidence

- At native 4:3 the wide background anchor returns unavailable. Split local
  passes now draw authored map tiles rather than the canonical player's shared
  tile ring. The new background regression tests the actual draw callback with
  conflicting ring/map tiles. The prior `8872de09` native two-peer distance
  probe passed through matching tick 4618; Zero stayed at the start with HP32,
  and inspected before/after presented images retained the correct background.
  The owner confirmed that build looked much better.
- Health collection `800C00BC` dispatches kinds 0, 1 and 3 through the native
  health routine, which calls `800C03BC` to change `PLAY+10..17` freeze flags.
  Split now restores those flags around collection and each original actor
  update, preserving preexisting scene locks. Both seats retain collector
  ownership for delayed refills. Unified retains the original global pause.
  The lifecycle model checks both seats/modes, per-callback HP counting, scene
  locks and death during refill. It fails against `a327413` and passes now.
  Owner testing of the earlier camera-bound fix confirmed the body stayed put;
  the new asynchronous refill still needs native confirmation.
- A native outgoing beam ends with body state 3 and positive HP. The section
  carry code previously treated that as death. The new lifecycle regression
  reproduces this for both leading seats; the fix distinguishes an in-transit
  or voluntarily withdrawn living body from a corpse. True HP-zero deaths,
  team retries and Game Over cases remain covered. Sparse health/stage-init
  logging records state, location and warp ownership for future reports.
  No historical HP trace exists for the owner's area-2 death, so its exact
  cause is unresolved. Native transition confirmation remains pending.

## Exact production checks

All 13 configured CTests pass, including plugin/catalog audit, ownership,
camera, background, save fixtures and release defaults. Native room offers
publish distinct declared Unified/Split settings and fingerprints without
altering offline settings. Production compilation disables debug tools and
uses a statically linked ENHANCED runtime; only Windows system DLLs are imported.
Shared framework gates stage the mod catalog, toolchain and execution manifest.
The package contains no disc, retail BIOS, save data or private captures.

Private evidence is under `build-coop-playtest/`: `qa-production-offers`,
`qa-production` and the package reports. The input-neutral timed native probe
has failed reports because it did not reach attract gameplay. Its matching
boot core/mod samples are admission evidence only, not gameplay qualification.
Earlier owner boss-door success and separate original-disc input/death/menu
checks are documented in `COOP_SPIKE.md`; they are not a full current-build
door/campaign/survivor matrix.

## Remaining playtesting

Prioritize Split health pickups while the partner moves, Web Spider area-2
transfers led by either seat, then boss-door/dialogue handoffs. Unified's refill
pause should remain native. Note stage/area, leader, partner survival and camera
mode with reports. Split can run below full speed even on LAN. Retry/menu
disconnects, vehicles and broader scene coverage remain open.

An explicit synchronized netplay debug fixture mode is tracked separately and
deferred from this first release so it does not delay owner playtesting.

## Linux AppImage addition

The native x86_64 AppImage requires glibc 2.38 or newer. It retains the Windows
gameplay sources, `mmx4-coop-delay-v3:d7655e8c166ef597a32468fc393664343b53208b`
compatibility ID and the same ENHANCED execution identity. Linux packaging
enforces netplay support; a fresh title CMake configuration now selects it
before including the shared runtime. The Windows release already had it on.

All 13 Linux CTests pass. The actual packaged image passes extraction/seeding,
catalog/toolchain layout, player-file preservation, native room-offer checks
for default Unified and selected Split, launcher startup and an OpenGL/OpenBIOS
boot advancing 501 frames. Room fingerprints match Windows. These are startup
checks under Ubuntu 24.04 WSL/Xvfb with software GL, not Linux online gameplay,
cross-platform gameplay or a performance benchmark.

AppImage SHA256: `b7d0856c6c87f4a75aaf06604a1a58408838f7bb62a537c71db9a2b6deba643d`.
Native build ELF SHA256: `aea53989f622e5b61aa2158f389e93a0bdaa8af20e914de6923943f29567f54c`.
Deployment adjusts RPATH and binds the final ELF with a separate execution
receipt. Private build/evidence: `/home/matthew/mmx4-coop-appimage-20261009`;
passing packaged check: `qa-packaged-net-3/report.json`.

The first, unshipped package had netplay disabled; its room-query timeout is
retained. Later smoke attempts incorrectly expected Windows heartbeat output
on Linux. The final probe uses production frame reporting instead. Failed
reports remain separate from the passing probe. The October 9 original-disc
inventory and current generic AOT discovery still find no additional game
overlay producers; see `DISC_INVENTORY.md`.

## Packaging

After the production build, run with Python 3.11 or newer:

```powershell
python tools/package_coop_playtest.py --build-dir build-coop `
  --binary-dir build-coop-playtest --output-dir build-coop-playtest/release-new
```

The output directory must be new. The helper validates version/build settings,
uses the framework's shared staging gates, and emits a Windows ZIP and checksum.
