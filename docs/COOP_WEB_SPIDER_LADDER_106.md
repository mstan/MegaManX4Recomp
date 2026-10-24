# Web Spider survivor ladder follow-up

Tracked by `beads-eio.9.226`. Report: `Wen Spider ladder of death.zip`,
SHA256 `365b0a60ff87609f64921a0088b49483e1885983e22c894c7db5f5283dc17d41`,
and [the supplied video](https://youtu.be/UFCmBLA9LJs).

In local 8:3 Split on public 1.0.6, P2 reaches the last upper ladder in
Web Spider part 1 while P1 remains below. After P1 dies, the camera stays in
the upper enclosure. Descending kills P2 at world Y521, above the real floor.

The relevant session is `session-1792824225821837`. The original Windows
1.0.6 executable verifies all 15,402 complete recorded input/state hashes;
the incomplete final interval is omitted by the normal replay preparer.
The route contains two fatal descents and a final attempt that reverses Up.

## Cause and correction

Native camera-area controller `800B5798` first calls the visibility helper
`8002B160`, then applies its authored room variables. The upper ladder's
kind0/subtype5 and subtype8 actors at (5728,400) are live before P1 dies.
Subtype8 covers X5728..5744, Y432..448 and restores the deeper bottom1280.

The survivor stage pool `8002166C` runs before the ordinary camera-owner
handoff. Its player was already projected to P2, but the installed camera
could still belong to P1's distant room. On that first pass both ladder
controllers were culled. P2 consequently retained bottom256 and the native
death check killed the descending player at Y521.

The existing survivor camera transaction was restricted to Sigma1. Apply it
to the one native survivor stage-pool pass in every stage: install P2's real
camera before culling and retain the resulting room/camera writes for P2.
The shared actors still update once. Unified and living P1 keep their
existing paths; camera transaction eligibility still excludes private draws
and incompatible shared scenes.

## Validation

- Fresh Windows build and 39/39 CTest checks pass. The lifecycle model now
  checks all 14 stage IDs, both sections, both campaign identities, and four
  combinations of Split/Unified and living/dead players. It checks one pool
  call, the installed camera, retained bounds, campaign and health restoration.
  Compiling that test against the original restricted policy fails.
- Native baseline fixture admits the actual shaft and upper room, verifies
  both ladder-area actors are present, then causes a distant P1 void death.
  Down reproduces P2's death at Y521 with the old runtime.
- Corrected P2 survivors pass with both X/P1 + Zero/P2 and Zero/P1 + X/P2:
  descents reach Y597.5/Y602 with 32HP and camera Y256 -> Y453/Y458, then
  climb back to the upper platform. P1 remains dead and lives are unchanged.
- P1-survivor controls pass for both characters, descending to Y606.5/Y605
  with 32HP and the dead P2 preserved. The both-alive P2 control passes at
  Y600.5 with camera Y456 and returns to the upper platform.
- Existing Sigma1 survivor-ladder scenarios pass for both character orders:
  P2 lands at Y1018/Y1019 with 32HP, camera Y826/Y827 and target bottom2048.

These are seven successful native scenarios. New online gameplay, Unified
gameplay and the rest of the campaign were not audited for this follow-up;
Unified's unchanged ownership path is covered by the lifecycle model.

The developer comparison of the supplied input changes only the private
build-identity header. Its corrected behavior first diverges from the old
expected state at input5877, so it is explicitly **not** a successful replay
or cross-build compatibility claim. The original release replay and fresh
before/after gameplay fixtures serve different purposes.

The native scenario `spider_ladder_survivor` in
`tools/test_mmx4_coop_regressions.py` uses the original USA disc, a private
hidden developer process, native stage/checkpoint setup, safe observed
teleports and enemy-damage protection. Input crosses the shaft admission
strip, climbs the actual ladder, descends and returns. Camera/bounds, ladder
controller flags and success flags are never written by the fixture.
`spider_ladder_pair` runs the same route with both players alive.

Private evidence and the retained QA fixture live under
`G:/Temp/Codex/psxrecomp/x4-web-ladder-106/`. Rejected early fixtures that
teleported into terrain or bypassed camera admission are excluded from passes.
This is bounded ladder qualification, not a full-stage or full-campaign audit.
