# GitHub review for local split development

Initially reviewed against Playtest37 (master06e940c) on 2026-10-18.
The issue39 fixes shipped in Playtest38. The subsequent issue41 follow-up
identified a distinct Zero/P2-first case, reproduced and corrected below.
Private attachments remain in build-pr23/issue-review38; the follow-up native
recordings are in build-pr23/issue41-followup.

## Issue39 implementation and validation

Aiming Laser uses a private type6 shot for its reticle, then968A4 creates a
separate shared type9 beam in801435B0's32-by60-byte pool. Its nativeD67DC
update checks PLAYER+93 for weapon6 and takes its origin from PLAYER. Restored
Zero/P1 makes that check fail, so the beam disappears before being drawn.
The new bounded filter projects X/P2 for that one deferred update. Native
lock acquisition, target health checks, geometry, flicker and expiry remain
native. X/P1, unrelated types and other actor pools bypass the filter.

Space Port's kind7/DA878 controller adds native scrolling speed and half the
foreground displacement. Final Weapon's kind6/DA7C0 controller changes a
four-cell atlas every eight ticks. Both execute after the saved player-camera
updates, and mode5's ordinary parallax helper does not recreate their writes.
Run each controller once, then publish the resulting animation to both saved
layers. Scrolling retains each view's own foreground displacement and Y;
atlas animation shares the actual native phase and dirty metadata. Neither
fix advances enemy AI, the animation clock or actor allocation twice.

The source and release TOML hook lists include all three callbacks. The
recompiler regenerated full_64/full_65; generated code was not hand-edited.
The first full test run caught the missing hook-list entries, which were
corrected before final native qualification.

Private evidence: build-pr23/issue-review38.

- lan-baseline: real two-peer Split, Zero campaign. Stationary P2's Space Port
  background remains24805376 while P1 scrolls. Final Weapon P2 remains0 while
  P1 changes through all four atlas phases.
- lan-aiming-baseline: real two-peer Split, healthy X/P2 locks onto the native
  Jungle plant, but no beam survives. Jungle supplies a repeatable target;
  its health is increased to24 through the synchronized setup fixture so
  target death cannot hide the beam lifetime. Both players retain32HP.
- x-control-gate: native X/P1 control locks the plant and produces its beam.
- lan-qualified: both saved views scroll in Space Port and cycle through
  0/512/1024/1536 in Final Weapon. X/P2's native beam survives and appears in
  the captured green-beam frames; native beam function traces agree on both
  peers (66 matching entry records each, including interception plus the guarded
  native call;33 world updates). The two peers match core/mod checksums after each
  case, through ticks4199,5573 and6951. These are same-build Windows loopback
  matches, not cross-build or cross-platform compatibility claims.
-36/36 title tests pass, including hook registration and staged mod validation.
  Linux GCC builds and runs the changed combat and camera helper tests.
  Tests cover owner restoration, unrelated actors, independent scrolling,
  signed odd displacement, all atlas phases and wraparound, native-once
  updates and private-render exclusion.

Final tested development executable SHA256:
3baf71bb3291ab1ff78ed39337d1f24ec6dbf1312c75fe1a43d5a8792b089359.
Stage/checkpoint/weapon/health protection and coordinates are setup fixtures;
actual targeting, firing and controller animation use native game logic.
An initial Space Port aiming setup lost X and is explicitly discarded as
weapon evidence. The existing Jungle rendering observation in the local split
notes remains separate; these checks do not claim to fix every visual defect.

## Issue41 current gate attempts

- Zero campaign, local Split:1858 native frames; final positions4042.5/4212.5,
  checkpoint2, both32HP, freeze0, story lock0 and camera maximum4544.
- X campaign, Unified:2354 native frames; final positions4216.5/4154.5,
  checkpoint2, both32HP, freeze0, story lock0 and camera maximum4544.

The earlier attempt held the attack button continuously and did not break the
gate; it is not accepted as traversal evidence. No correction is claimed for
41 because these actual gate traversals did not reproduce the endless loop.
Keep the issue open for a failing full session, rather than conflating it with
the separately fixed intro part2 falling platforms.

## Issue41: Zero/P2 enters first

Both heroes start on the observed floor before the first breakable gate;
P2 starts80 pixels ahead. The trailing hero moves only to keep a100-pixel gap.
Stage selection, position and invulnerability are development setup fixtures.
Actual gate destruction, traversal, auto-walk, dialogue and section loading
use the original executable and controller input, with no forced completion.

- Unchanged38 development executable3baf71bb: P2 enters the real door first,
  the scene locks at frame6656 and P2 reaches C0=FF at6678. The scene stays
  locked through9570, with explosions continuing and no part2. P1 is withdrawn
  at32HP; P2 remains visible at32HP. The composed image was inspected.
- X/P1-first control on the same executable advances to part2 at8008 and
  unlocks at8766. This control establishes section progress; its capture stops
  before the partner finishes the final arrival.
- Original D0548 commands action15 through80036AE4. That command is already
  redirected to the surviving scene owner, but D05B0 reads PLAYER+C0 directly.
  It therefore waits on withdrawn P1 after P2 finishes. The existing later
  D1248/D1284 rescue filters do not cover this act1 Dragoon actor.
- The fix adds D0374 to the existing scene-owner projection. Its native
  command and completion predicate now read the same body, while the campaign
  stays canonical. Both development and release TOML hook lists were updated;
  full_62 was regenerated by the recompiler rather than edited.
- Corrected native run fixed-zero-p2-dialogue reaches playable part2 at8321,
 4875 frames after setup. Both heroes are active/visible, state1,32HP, at
 144/443 and144/442; all story, script, door and freeze locks are zero.
  The final composed image was inspected. Separate confirm edges also finish
  part2's ordinary X dialogue; an earlier harness stopped confirming after
  the section change and is not accepted as final arrival qualification.
- All36 Windows title checks pass. The ownership model covers both seats in
  both campaigns, and also compiles/runs under Linux GCC. These model tests
  supplement the native reproduction; they are not a netplay qualification.

One hidden launch failed before frame0 because the Windows desktop exhausted
its window-object allowance. The successful control and corrected run use a
separate invisible desktop, with dummy audio. The private runner's watchdog is
30 seconds to tolerate captured frames; production policy is unchanged. No
visible window is opened on the owner's desktop.

Qualified executable SHA256:
3035c27de3bfcdacf3bd51bd86448921e908ff79716a64efdd260decbee527e9.
The separate draft local-render performance change is not part of this build.
