# 1.0.5 Volcano effects and Sigma1 survivor camera

Tracking: `beads-eio.9.221` and `beads-eio.9.222`.
Private evidence: `G:/Temp/Codex/psxrecomp/x4-magma-105/`.

## Volcano

The part2 pillar sequence is category3/type34 at native `800BCE48`, separate
from the ordinary type33 emitters qualified earlier. `BCEA8` arms its authored
timers only when the singleton player's signed X exceeds576. Allow a living
P2 to supply that coordinate during initialization. Native code still owns the
nine timers, emitter allocation and actual terrain destruction. The temporary
coordinate is restored before returning, and the existing scene guard rejects
snapshots during that scope. Unified retains the same native admission; private
drawing, other stages/sections, checkpoints, actor types and dead P2 are excluded.
The callback entry is emitted in both development and release game configs.

The old fireball camera binding also kept the allocating player's view throughout
its life. A placed rolling ball can instead hit terrain beside the other hero.
Its native quake guard then reads a distant view's clear bit and can restart
the nearby view's timer on each update. Use allocation provenance only for the
unplaced falling initialization; placed fireballs use the nearest living view.
Native collision, motion, timer updates and one shared actor update remain intact.

Native part2 comparison:

| Build / leader | Sequence armed | First scripted fireballs | First pillar tiles removed | Both HP |
|---|---|---|---|---|
|1.0.5 / P2|No|None|0|32|
|1.0.5 / P1|Yes|X192,320,672|16|32|
|Candidate / P2|Yes|X192,320,672|16|32|
|Candidate / P1|Yes|X192,320,672|16|32|

The pillar fixture protects the moving hero above the native pillars. Only
position and damage protection are prepared; native actors and `DABE4` perform
the16 terrain edits. This qualifies the event, not ordinary traversal survival
on a pillar as it disappears. Evidence: `before-mechanics/` and the first
candidate probe `--exe-mechanics/` (the directory name is a runner-label error).

The corrected quake check uses native walking from the original arrival,
with the other living hero waiting. The native timer progresses and the nearby
camera alternates between X0/4. Both retain32HP. Evidence:
`after-sigma-mechanics/continued/` and `final-zero-campaign/`.
The new model check explicitly leaves the remote view's quake bit clear and
verifies that a second impact update cannot restart the nearby timer.
The original walking controls also contain intermittent quake commands and
camera motion; they do not show a blanket absence of shaking. The scope of
the correction is the proven misplaced-camera/timer-guard contract, with
native local quake behavior checked afterward. The wider part1 end audit
remains open in feedback194.

## Sigma1

After P1's death, the shared stage-script pool runs with P2's body. It did not
also bind and retain P2's camera. The narrow ladder return strips therefore
failed to release the upper enclosure reliably during that survivor transition.
Scope this one Sigma1 pool pass to the surviving P2's saved camera and commit
the original camera writes. Preserve shared stage palette resources and one
native pool update. Other stages and Unified retain their existing paths.

The baseline native setup first admits the actual upper camera areas, walks
P2 toward the first door without entering it, then takes a native P1 void death.
After aligning with the original return strip, Down drives the native ladder
and terrain controller. Baseline P2 dies at X3928/Y1000.5 while the camera and
its maximum Y remain736, matching the supplied log's X3928/Y1000 death.

The candidate releases target maximum Y to2048, scrolls beyond736 and lands
Zero/P2 at Y1018 with32HP. The X/P2 control lands at1019 with32HP. The dead
partner remains dead and shared lives do not change. Composed captures show
the original next platform and lower shaft. Evidence: `before-mechanics/`,
`after-sigma-mechanics/`, `final-zero-campaign/`.
The final X-campaign run also checks the P1-survivor control. Native quake
checks with the heroes separated near X1000 pass for either leader in
`final-x-campaign/`; the corresponding original controls are retained in
`before-quake-separated/`.

## Validation and limits

All39 Windows development CTests pass. New model checks cover the native quake
bit/timer guard, either allocating hero, the pillar admission boundary and
exclusions, restored P1 coordinates, snapshot quiescence, and the Sigma survivor
camera transaction with Unified/other-stage/living-P1 controls.
The signed integer threshold also has explicit fractional-coordinate checks:
P1 at576.5 must not suppress an eligible P2; P2 below577 must not arm it early.

Native scenarios are exposed as `magma_quake`, `magma_pillars` and
`sigma_ladder_survivor` in `tools/test_mmx4_coop_regressions.py`. Dragoon's real
upper arena admission remains a regression control: checkpoint3, both living
heroes visible and initialized native boss, with the pan starting near P2.

Excluded setup attempts are retained as diagnostics, not passes: early quake
teleports crushed the moved hero; one ladder setup overshot into the first door;
another reused an occupied debug port; one stopped at X3918 and never attached
to the ladder. An initial candidate assertion incorrectly required descent past
Y1400, overlooking the real platform at1018; its healthy landing and released
bounds establish the narrower reported behavior.

All runs use hidden muted windows and private cards/settings. No new LAN,
cross-OS gameplay or release qualification is claimed. The earlier unreproduced
Magma process exit remains separate.
