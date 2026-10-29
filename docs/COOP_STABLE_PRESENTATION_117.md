# Stable presentation follow-up

Tracking: central Beads `beads-eio.9.280`. Experimental X1 characters and
Four Player Co-op remain separate from this stable cleanup.

## Unified Widescreen pause menus (`beads-eio.9.233`)

The stable runtime reproduced black paired pause menus with Unified local
cameras and Widescreen enabled. The same native menu sequence without
Widescreen displayed normally. The personal menus were live but their GPU
draws used the native-wide world surface, while local-pair presentation
captured the empty native rectangles.

Widescreen now identifies personal menus and their shared Options/Quit
frontend through the existing native-scene predicate. This classification
remains valid through nested GPU submission and presentation. It does not
change menu input, inventory, simulation clocks or world fades; gameplay
returns to its usual widescreen classification on resume.

Current original-disc native validation, hidden/muted with isolated cards:

- Inspected composed X/Zero menus: both visible, with their correct portraits
  and blue/red backgrounds. Baseline visibility ratios were `[0,0]`;
  candidate ratios were `[0.9977,0.9984]`.
- Full simultaneous-menu sequence passed independent weapon/tank management,
  healing while the partner navigates, held-Start rejection, either-player
  resume, paused save/load, P2 Quit confirmation, shared Options return,
  surviving-P2 management and native firing after resume.
- 2,736 render transaction restoration checks: no mismatch, abort, watchdog
  or VRAM leak.

Private evidence and exact capacity/source/binary receipts:
`G:/Temp/Codex/psxrecomp/x4-train-pillar116/`, particularly
`baseline-menu-unified`, `baseline-menu-unified-stock`,
`candidate-menu-unified-r2` and `candidate-unified-management`.

## Train foreground pillars (`beads-eio.9.279`)

Report: <https://youtu.be/ucIsvq2lM3k?t=2032>. Native D9A58 queues a type20
foreground pillar while running the progressing hero's train controller.
Its deferred CB940 initializer subsequently reads the shared camera at
CAMERA+(352,128), after the hero's projection has ended. A leading P2 can
therefore receive a pillar at P1's older camera origin, inside P2's screen.

The original CB8F8 dispatch now initializes that pillar once with the
allocating hero's saved camera and layer. The stage assembly at SCRATCH+20
stays intact; projecting character resources here would corrupt its graphics.
Live pillar movement/culling, train physics, scene timing and shared updates
remain native and run at their existing rate.

Both-owner unit checks cover initialization, body/camera restoration,
stage-resource preservation, ordinary live dispatch and unrelated/Unified
paths. The generated hook is declared in both authored game configurations;
only generated shard61 changes after regeneration with the pinned emitter.

Native baseline/candidate checks use controller movement from the original
checkpoint through the actual stop, followed by P2's native run/jump/dash.
Both complete the stop, recover controls and retain32HP through P2 reaching
X>6720. Native CB990 write traces show baseline repeated births at P1's
5472 origin while P2 advances; the candidate uses advancing P2 origins
5553,5878,5910 before the shared stop. Four P2-owned initializations run in
the candidate. Inspected post-stop captures place the first observed pillar
at P2 screenX310 versus baseline280, nearer the viewport edge. This is a
bounded equivalent route, not deterministic playback of the old netplay
recording. Host capacity/configuration and raw writes/images are retained in
`candidate-pillar-route-r2`, `baseline-pillar-route-r2` and
`pillar-comparison.json` under the private evidence directory above.

Earlier teleports ahead skipped moving train collision progression; those
failed fixture routes are excluded from passing evidence.

## Intro dragon camera lock (GitHub87, `beads-eio.9.281`)

Native solo reproduced the apparent return to4:3: the renderer still used a
426px canvas, but the first dragon-door lock set minX=maxX2976, producing
53px of empty padding on each side. The camera lock does not erase the
authored stretch behind the hero. During that exact lock, retain the original
0..2976 presentation extent, borrowing the full106px reveal on the left.
Native camera limits, physics and the unopened door's far side stay unchanged.

Inspected baseline/candidate native captures show the removed empty margins.
The original door takes native buster hits and is crossed; the dragon's real
screen-relative corner and return phases run. The final Eregion introduction
also completes, then native Right input moves X15.5px. Its brief scripted walk
left before the boss is native positioning; an endless walk was not reproduced
with co-op disabled. Evidence: `solo-intro`, `solo-intro-fixed-r2`.

## Other report dispositions

Current native Zero/P1 and X/P2 death sequences show the native death burst
on both composed panes, with the survivor retaining HP. Both Slash boss
entrant orders keep near/far scenery scrolling while foreground idles.
Evidence: `effects-native`, together with the older two-peer survivor check
recorded in `beads-eio.9.55`. Existing hover-jet correction3ac1021 remains
delivered; diagnostics recording stays optional/off. These checks did not
reproduce the remaining visuals from GitHub32 (`beads-eio.9.193`).

## Remaining qualification

### Frost Walrus paired-door introduction (`beads-eio.9.187`)

The current native Both-door route reproduced the old cosmetic warp. Walrus
constructor72240 created WARNING and issued command20 at7227C while the
paired crossing was still in phase3, before camera arrival. That command
aborted the crossing and withdrew/rejoined P2 although both heroes had entered.

Apply the existing paired-boss admission barrier to Walrus's complete native
constructor. It now waits for physical clearance and camera arrival, allowing
the original closing and subsequent introduction to finish in order. The
passenger remains visible and receives neutral scripted controls; this does
not enable free movement in cutscenes or change the separate experimental mod.

Original-disc CP3 to real inner door, WARNING, first-entry conversation and
fight release passed. Both heroes remain visible with32HP throughout; native
Walrus57 reaches48HP and both heroes recover ordinary idle state with no
script/door lock. Baseline had one departure/rejoin; candidate has none.
Evidence: `frost-native` and `frost-native-fixed` in the private task directory.
The first harness receipt mislabeled Walrus as enemy16; retained raw native
rows correctly identify57 and are not discarded or rewritten as a passing run.

The stable presentation tracker retains the older unlocated Frost enemy-sprite
report and Quit/Continue qualification with their individual evidence/caveats.
Cosmetic severity alone is not a reason to defer a bug.
