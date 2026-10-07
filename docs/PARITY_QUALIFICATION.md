# MMX4 Windows native interpolation review

Updated 2026-10-07. Tracking: `beads-eio.9.2`. Owner acceptance remains open.

`Play-MMX4.ps1` in `F:/Projects/psxrecomp/parity-review-20261006` now
launches `build-native-review`, checks the binary receipt and uses private
memory cards. The ENHANCED build uses shared framework `b8cf0aa8` and recomp-ui
`03d58aa0`. Earlier `build-campaign-enhanced` blending candidates are superseded.

## Implemented and checked

- OpenGL, 1080p internal target, sharp nearest-filtered pixel art.
- Native scene interpolation using shared `PSXSpriteSceneReplay`: interpolate
  camera and actor positions, regenerate background/sprite packets, preserve
  late HUD/fade packets, and restore the machine after each intermediate draw.
  Gameplay, animation selection, input and audio retain their original cadence.
- Native Scene Interpolation / Display refresh is selected in the local review
  settings; the source package remains opt-in. Image blending is not used.
- Existing game-specific 16:9 view and HUD hooks are retained.
- Fresh generated main game code plus one newly compiled historical BIOS shard, 51 functions. This is not a whole-game overlay inventory.
  Cache namespace: `cg18_d1867bb4_gcc321a81a_f0`.
- A five-second moving attract-scene check recorded 300 native passes,
  779 intermediate presents and 300 successful state verifications,
  with zero mismatch, VRAM leaks, watchdogs, aborted passes or blended presents.
  Evidence: `receipts/MMX4.native-attract.json` in the review directory.

This is bounded correctness evidence, not a performance benchmark or acceptance
of every stage. The Windows executable/import closure and execution binding
passed. Broad test suites were deferred at the owner's request.

## Human review

Enable Native Scene Interpolation, choose Display refresh, and keep OpenGL /
1080p. Play the intro as X and Zero. Check smooth movement and camera scrolling,
background layers, crisp HUD/dialogue, pause/resume, a boss door and death/retry.
Enable the existing widescreen feature to inspect 16:9 edges.

## Still outside this first-pass candidate

Full 21:9/32:9 map and packet expansion, a title-specific loading HLE adapter,
Linux packages, broad stage/performance qualification and final owner acceptance.
No title HLE family or loading speedup is claimed merely from ENHANCED metadata.
Original generated LLE remains the reference; future HLE must preserve the real
loader callers and completion behavior while reducing work.

