# Sigma entrance with one surviving player

October 21, 2026; `beads-eio.9.210`, checklist item 210.

The 1.0.0 report identifies a specific failure: P1 dies, P2 enters the yellow
Final Weapon teleporter, then walking to Sigma's door never starts the scene.
This is separate from the older, broader death/Continue report 136.

## Cause and change

The original Sigma NPC controller at `800D2A38` reaches `800D2420`, which
tests the singleton player's X against 2321 before issuing scene command `0x14`.
The co-op scene-command hook cannot help when that earlier position test
still sees P1's corpse at 1840. The controller was absent from the emitted
function hooks and the registered scene-owner filters.

Register and emit that complete controller through the existing scene-owner
projection in stage 12/section 0. Admission sees the surviving entrant, and the
camera wait, dialogue and completion keep that owner. The original campaign
identity and native scene code remain in control. Both-living admission still
uses P1. Private render passes, other stages and later Sigma sections do not
use this new projection. This is a default correctness fix.

Both the development and release game configurations include the hook.
Regeneration changes only game shard 63; generated original-game derivatives
remain ignored. The framework and UI pins are unchanged.

## Reproduction and evidence

Private evidence is under
`G:/Temp/Codex/psxrecomp/mmx4-sigma-p2-20261021/`.

- `probe-before-p2`: before the fix, the surviving P2 walks to 2422 with no
  scene admission while P1 remains dead at 1840.
- `probe-before-both-p1`: the unchanged native P1 control starts the scene
  at 2321.5.
- `probe-fixed-p2`: the same survivor approach starts the actual Sigma scene
  at 2321.5. `observation.png` shows Sigma's original X-campaign dialogue with
  P2/Zero present. P1 remains dead throughout that corridor scene.

The bounded native regression is `tools/coop_sigma_entry_scenario.py`, invoked
with `tools/test_mmx4_coop_regressions.py --cases sigma_entry`. It prepares
stage, checkpoint, health and position, and explicitly bypasses the eight
earlier refight prerequisites. It then uses the actual authored yellow pad
and controller input for transport, scene admission, dialogue completion and
forward transfer. It does not set those success flags or actor phases.
Neutral input precedes the idle-control check; both seats must subsequently
move. Campaign and shared lives must remain unchanged. The existing forward
section policy revives a dead partner after the corridor scene, which is an
expected transition rather than a workaround for scene admission.

Zero's original mode10 handler calls movie9 (`80020390` -> `80018000`)
after dialogue; X can briefly use the same transition mode without that movie.
The bounded scene test
uses the original global pad1 Start skip in movie mode, separately from the
living entrant's in-stage dialogue input. This does not qualify watching the
entire movie or skipping it with P2 alone. Two earlier Zero runs timed out
in movie mode (45/120-second bounds); neither is counted as a
completed-route pass. A subsequent run completed the movie/section transfer
but its idle-only wait did not advance Zero's second authored conversation.
The final harness confirms that conversation before checking neutral controls.

The private harness explicitly enables diagnostics because the public default
is now off. Production diagnostic defaults are unchanged.

## Validation

The lifecycle regression calls the registered function dispatcher, so omitting
the new controller registration fails the test. It covers surviving P2 in
both campaigns, original campaign identity, unchanged corpse coordinates and
health, latched scene completion, ordinary P1 admission, other stages, later
Sigma sections and private render passes.

| Native configuration | Cases | Result receipt |
| --- | --- | --- |
| X campaign, local Split 8:3, Both doors | P2 survivor, P1 survivor, both living | `native-yellow-split-both-x-v4/report.json`: passed |
| Zero campaign, local Split 8:3, Both doors | P2 survivor, P1 survivor, both living; movie skipped with global pad1 Start and second conversation confirmed | `native-yellow-split-both-zero-v4/report.json`: passed |
| X campaign, local Unified, Either doors | P2 survivor | `native-yellow-unified-either-x/report.json`: passed |

Those receipts use the earlier `movie_seen` label for observing mode10; that
field does not establish movie playback in X's campaign. The current harness
names it `transition_mode_seen`. The original native mode table at `800F23E8`
selects `80020464` for mode10; `80020390` calls movie9 only for Zero at stage12.

The final production Windows build, with both developer-tool switches off,
passes all38 CTests (`ctest-production.log`), including the registered-dispatcher
lifecycle regression and emitted-hook/release-configuration checks. These
native scenarios used a private development fixture build of the same fix;
the distributable is rebuilt without those tools.

All native case receipts require the actual yellow-pad admission, original
scene and forward transfer, unchanged campaign/lives, neutral controls and
subsequent movement. The X run records the expected 0/32, 32/0 and 32/32 HP
at corridor admission; both campaigns reach section 1 with two shared lives.

## Qualification limits

This qualification concerns the reported yellow-pad corridor scene and native
forward section transfer. It does not establish every older Continue/retry
case, the eight prerequisite refights, a full Sigma victory, online play or a
new Linux gameplay run. Original player memory cards and settings are never
used by the hidden, muted test profiles. Save-state migration is unnecessary.

Failed exploratory setups are retained and excluded: the old harness assumed
default-on diagnostics; a confirm pulse during hub arrival entered a blue
refight pad; sequential two-seat teleports raced pad admission. The corrected
harness uses a settled neutral arrival and one atomic two-seat position
fixture. An earlier dialogue driver also demanded idle action while pulsing
Jump; its native forward transfer is recorded, but that driver's result is
not counted as a pass.

This fix ships in v1.0.1 Stable Public Beta. The v1.0.0 tag and download remain
unchanged. Release tracking: `beads-eio.9.211`.
