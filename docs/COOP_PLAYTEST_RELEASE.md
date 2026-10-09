# Co-op playtest 1

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

## Packaging

After the production build, run with Python 3.11 or newer:

```powershell
python tools/package_coop_playtest.py --build-dir build-coop `
  --binary-dir build-coop-playtest --output-dir build-coop-playtest/release-new
```

The output directory must be new. The helper validates version/build settings,
uses the framework's shared staging gates, and emits a Windows ZIP and checksum.
