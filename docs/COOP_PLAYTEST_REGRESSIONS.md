# Co-op playtest regression queue

Updated 2026-10-26. The original 2026-10-10 reports below were forwarded playtest reports.
Reproduce each report before marking its cause confirmed or its fix passed.
Gameplay scenarios run one at a time in private profiles. Reproduction uses hidden runs,
debug stage skips, teleporting and invulnerability where useful. Existing user
windows, settings and saves are preserved. A checkmark means the requested
outcome has been validated, not merely that a patch compiles.

## 1.0.12: independent Ride Chasers and pause Select

Tracked as `beads-eio.9.266` and `beads-eio.9.267`. Fixtures prepare positions,
inventory and damage protection; original controllers, collision, scene tasks
and boss AI establish the outcomes below. Four Player Co-op and the X1
character replacements stay on their separate branches.

- [x] Both campaigns, either leading rider: the leader completes native
  ejection and landing while the trailing rider remains mounted with a camera
  more than 10,000 pixels behind. The trailing rider then completes its own
  native exit. Both retain full health and the shared life count.
- [x] Current-build save/load with one exit controller finished and the other
  pending preserves their separate flags and permits both native landings.
- [x] Eight dash cases cover both riders in both bike sections and campaigns.
  The neutral rider stays at six pixels per simulation tick. A dash retains
  its native independent world/camera advance; riders can choose different
  positions.
- [x] Paired bike exit through actual Jet boss admission in both campaigns:
  both land, move by controller input, cross the door, finish the timed
  WARNING/introduction and regain control while the live boss moves.
- [x] Both-player door policy: either leader, with the partner crossing
  immediately or waiting outside, passes in both campaigns with both healthy
  and controllable inside the boss room.
- [x] Default Either-player door policy with a still-mounted distant rider:
  return both heroes on foot through the original scene, with unchanged
  health/lives and restored controller movement, for either leader in both
  campaigns. The passenger departs from its distant route at X3765..3869;
  admission does not wait for that rider to finish the bike route. Native
  source-bike cleanup uses the existing floor-validated foot arrival.
- [x] Magma Dragoon's upper foot route with either lower-route Armor rider
  passes from a fresh boot, including the boss scene, passenger return and
  controller movement. Falling below the actual lower boundary remains lethal.
  A sequential run after the survivor-menu scenario reloaded an unrelated
  stage; that fixture-contaminated result is retained separately.
- [x] Select from either player's pause menu in both campaigns: presented
  frame rings retain black through the handoff into the original blue
  Continue/Quit frontend. The pause image does not reappear. Returning restores
  the correct independent X/Zero panels. Private-pass verification reports no
  mismatches, VRAM leaks, aborts or watchdogs.
- [x] Native independent-menu controls, tank consumption, selected weapon
  use, paused save/load, modal ownership and surviving-P2 control pass in both
  campaigns.

The timed native scene checks wait for actual completion, rather than assuming
a fixed wall-clock delay supplies enough simulation ticks in instrumented
builds. Automated runs hold neutral virtual pads between button pulses to keep
physical controllers from influencing private QA processes.
The distant-rider door probes fast-forward the original scene sequence and
verify restored movement at normal pacing; they do not qualify performance.

These checks cover the listed routes and handoffs. Full-campaign and cross-OS
netplay coverage, Jet boss performance and the Frost Walrus refight camera
report remain separate work.

## Previously validated and shipped in playtest 2

1. [x] Windows/Linux lobby and two local controller selections (9.18).
2. [x] P2 incoming damage after pause in the earlier owner playtest (9.22).
3. [x] P2 death particles originate at P2 and use the character's color (9.24).
4. [x] P2 character voice sample ownership (9.23). Overlapping channels are #12.
5. [x] P2 HUD anchoring matches P1 (9.19).
6. [x] Stacked default HUD with side-by-side option (9.21).
7. [x] P2 is hidden on the Quit Game screen (9.20). Quit authority is #15.

## New regression work

New details under #8/#9 are additions to their existing issues. All new reports
remain open until their requested gameplay outcome has been validated.

## Current validation boundaries

Playtest 3 includes the verified audio, ownership, save and diagnostic work.
All 16 title CTests pass on Windows and Linux production builds, with debug
tools and synchronized cheats disabled. Packaged Windows/Linux room offers
match; Linux AppImage launcher/hidden boot checks pass through frame531.
Native screenshots show X/P2's full projectile after
P2 pause and both Jet Stingray riders facing right at the same height.
The SPU overlap test checks two seats using the same logical channel, instead
of the earlier test's two distinct hardware channels. Original VAB/key-on
calls were executed in an isolated MIPS harness against owner-provided files;
that verifies the register calculation, not a complete stage playthrough.

The broader SPU fidelity test reports four volume-sweep assertion failures on
both the unchanged framework HEAD and this candidate (same 137-check run).
The sample-bank/overlap, Gaussian and end-without-repeat checks pass. Those
baseline failures are not counted as a candidate gameplay pass.

Two bounded older software-headless boot probes did not reach title/gameplay.
A developer build with hidden OpenGL/TCP reached native Intro with both players
using controller input. The earlier probes merely waited through opening FMV;
they did not establish a boot defect. This successful boot is not a regression
pass. No visible user window was opened or focused for these probes.

## Development netplay fixtures

Owner requested finishing implementation/reproduction before handing remaining
human checks over. `beads-eio.9.14` now has a separate build option
`MMX4_COOP_NETPLAY_DEBUG=ON` (requires `PSX_DEBUG_TOOLS=ON`). It uses a distinct
compatibility ID and admits checksummed indexed commands through synchronized
P1 input. Stage/checkpoint loads, health, invulnerability, player/bike teleport
and bounded enemy-health setup run at the native dispatcher. Ordinary release
builds ignore the command channel. `tools/coop_netplay_debug.py` drives existing
private peers without RAM writes or visible windows. Native teleport and load
checks match both peers' core/mod CRCs. `netplay-debug-final-45` passes the complete
development harness: native teleport/invulnerability, stage/checkpoint load,
P2 health, and Jet shared READY, with agreement through tick 6253. Earlier
failed harness probes are retained, including one watchdog abort with matching
CRCs through tick 5024; that intermittent liveness issue remains open.

`tools/test_mmx4_coop_regressions.py` runs bounded hidden native scenarios with
private cards. Native-burn-1/2 cover Jet shared READY, corpse/vehicle section
carry, P2-led Cyber outer/inner doors, and P2-first intro scripted arena entry.
The intro approach releases normally; a separate C4-door qualification is not
inferred from this arena-script probe.

## October10 follow-up

The owner approved a new18-item scope in [project issues](https://github.com/mstan/MegaManX4Recomp/issues). Items8,9,11 and20 above are reopened after the new reports; their earlier tests document bounded evidence and do not cover every new route. The new list distinguishes visual evidence from text-only reports and keeps the original check numbers.

## Opening boss admission, October 25

`beads-eio.9.251` tracks the renewed 1.0.8 report with either player leading.
Strict replays of the original release verify 10,457 and 8,977 recorded input
intervals and reproduce the stall. When both heroes approach Eregion, its
background position makes the trailing hero nearest. Selecting that hero for
the native WARNING/action15 commits the introduction to the wrong camera.

At Eregion's original type8 phase1/9/0 admission, use the leading living hero
and that hero's saved camera. Keep an already committed scene owner and retain
nearest-player targeting during combat. `intro_near_entry` now checks both
leaders in both campaigns: original boss initialization to48HP, survival,
passenger return and actual movement by each seat. All four cases pass with
minimum32HP per hero. The previous distant-partner regression also passes;
Windows title checks pass39/39. These are bounded admission checks, not a full
opening-stage or four-player qualification.

## Marine Base acceleration and boss door, October 25

`beads-eio.9.252` covers a partner's Ride Chaser dash changing the other
player's speed in both sections. Native3C148/3C430/3C68C write camera+47;
the additional player pass now retains that speed in its own saved view and
restores the canonical speed. `jet_dash` passes both sections, either dashing
seat and both campaigns: every neutral partner update remains6px while the
actual dashing bike exceeds11px/update.

`beads-eio.9.253` covers the partner dying at the boss door. Native camera
area17008..17024 releases the lower floor, but an overlapping Both-door
contact could restore the old upper-room bounds. Retain the native expansion
in the existing serialized admission bounds and publish it to both admitted
views. The native bottom test uses that authored destination while the camera
eases; it still kills below the destination room. `jet_door` passes either
leader, entering together or waiting outside, in both campaigns: eight native
admissions, minimum32HP per hero, no life consumed, original48HP boss and
independent movement after the scene. The same runs pass the four renewed
opening-admission cases and all eight independent-bike cases.

The long original Jet recording was replayed only partially: it ran30minutes
without an observed hash mismatch, then hit its wall-clock timeout. Its report
is not a completed replay pass. Controlled original-disc scenarios establish
the acceleration and door results above. Jet boss performance remains tracked
separately in `beads-eio.9.254`; no performance improvement is inferred from
these correctness checks.

## Dragoon lower-route Ride Armor, October 25

`beads-eio.9.256` tracks the1.0.8 lower rider/upper partner report. The native
current build reproduces Armor402C4 writing fatal80 to the rider's health at
800403CC while the rider remains on the safe672 platform. The additional
player pass projected camera origins but left P1's room limits installed.
Native21C14 dispatches the vehicle before the ordinary player/shot updates;
project the rider's saved current/target limits only around that real vehicle
pass, then restore the canonical limits. Native death and scene rules remain.

`magma_armor_entry` passes either upper-route leader in both X/Zero campaigns.
It boards the authored checkpoint Armor, prepares observed approach positions,
then uses native upper jumps and lower punches. All four admissions retain
minimum32HP per hero, initialize the original48HP Dragoon, complete native
dismount/departure/return, preserve lives and restore both movement controls.
Separate below-room900-position checks still trigger native rider death in
both campaigns; this is not an invulnerability or pit-death bypass.

Rejected early camera-handoff and wrongly placed projection prototypes are
not fixes. Two overlapping QA runs reused a control port and were discarded;
the final isolated runs use18556/18557. The runner now rejects occupied ports
before it can launch or send controls/cleanup to another test process. Full
original162,996-input recording replay and four-player Armor admission remain
unqualified; these are controlled original-disc checks of the reported route.

## Sigma follow-up from 1.0.8, October 25

`beads-eio.9.257`: Double has no retail damage entry for Zero's first standing
slash, type24. The existing counterpart collision handler now uses his authored
type26 damage for that missing entry, as it already does for Kuuenzan. Attack
identity, consumption, duplicate-hit handling, damage scaling and native
cooldowns remain intact; the retail table is unchanged. `double_standing`
requires isolated standing inputs and actual type24 writes by native8002DF0C.
Both campaigns pass: Double48 to42 with Zero in P2, and48 to41 with Zero in P1
after the native Iris encounter. A later combo cannot satisfy the assertion.

`beads-eio.9.259`: the replay confirms a leading P2 dying at1121.5/593.8 while
P1 waits at1056/459. An immediate checkpoint-arrival drop reproduces this on
the current build; waiting for an idle arrival hides it. Native camera strips
open the lower destination to592, but the easing current limit can remain274.
At native27CE0, use the authored destination for the lethal bottom-test operand
only in this exact Sigma2 lower room, in addition to the existing paired-door
case. Do not change camera position, easing, terrain or health. The native
`sigma_lower_route` test holds input through arrival, drops either leader in
both campaigns with the partner still above, then walks into the actual lower
pit to require a native death. All four landings retain32HP per hero; all four
real pit checks kill only the falling hero.

`beads-eio.9.260`: nativeD2420 admits Sigma's corridor scene atX2321. Either
living player may now reach that threshold first; P1 retains simultaneous
admission priority and an admitted owner remains latched. `sigma_entry` passes
P1 survivor, P2 survivor, both/P1 leading and both/P2 leading in each campaign:
eight native yellow-pad, corridor-scene and forward-section checks, preserving
lives and proving movement afterward. Prior refight clear bytes are bypassed
prerequisites. Zero's interstitial movie uses the native global pad1 Start
skip; these checks do not qualify full movie playback or P2-only movie skip.

`beads-eio.9.261`: after the upper Sigma forms, native855CC/855D4 expands the
final arena targets to maxX1728/minY512 before36B18 releases the scene. Saved
Split views could restore the upper1088/256 targets, preventing final boss
admission. Publish those two native target changes to matching upper views
before the handoff; retain origins, current limits and native easing. The
`sigma_phase3_survivor` scenario passes both campaigns: actual attacks defeat
both upper forms, P1 dies before the second defeat, P2 falls into the lower
room, final type74 initializes at48HP and the survivor moves afterward with
32HP and unchanged lives. One-HP, position and protection fixtures prepare
combat; they do not set defeat, room-limit or admission flags. This qualifies
the transition, not a complete final-boss victory.

`beads-eio.9.258` remains open. In the original Frost Walrus refight recording,
P2 loses6HP at a time, then dies at native108038 near Walrus. Both living
private cameras remain inside the authored arena limits; P2's view follows P1
after death. The owner cannot confirm whether the reported camera glitch
preceded that death. No camera fault causing it has been established, and no
speculative damage immunity or camera correction is included.

Evidence is retained under `G:/Temp/Codex/psxrecomp/x4-sigma-111`, including
original replay receipts, private-view observations, rejected probes and final
native reports. Unit coverage includes standing-hit cooldowns, entrance-owner
latching, excluded rooms/render passes and byte-for-byte camera preservation.
Four Player uses separate stage logic; these two-player checks do not qualify
its Sigma routes. Existing performance and pause-flash issues remain open.

### Sigma2 paired-door corridor camera (1.0.13)

`beads-eio.9.268`: the supplied 1.0.11 recording reproduces the missing P2
camera lock through 6,341 complete native input intervals, with identity,
read order and state hashes verified. After both heroes clear the outer door,
native C1D90 commits minimum X1744 only to the installed owner's camera.
P2's saved view retains minimum256. It continues following P2 inside the
corridor instead of panning into the authored1744 lock. The same failure is
independently reproduced on the 1.0.12 development baseline with both heroes
at32HP and unchanged lives.

Publish that completed door's destination minimum once to both saved views,
including inside a P2-owned native projection. Retain their origins, current
limits, easing, vertical bounds and scenery. Keep the committed enclosure in
the door's admission record so later contact cannot restore its old minimum.
This runs only after both collision boxes physically clear the entrance;
it does not change the earlier Both-player waiting rule or player positions.

The `sigma_story_corridor` native scenario exercises either leading seat in
each campaign. Both views reach1744 with target interval1744..1744 before
continuing through the actual inner door, original Double/Iris introduction
and restored fight controls. Health and lives are retained. Earlier stage
traversal is bypassed with stage/position fixtures. Setup uses the recorded
platform foot heights331/330 and camera origin1440 while paused; it never
writes the tested1744 destination, door state or boss admission flags.
Earlier probes placed heroes inside terrain and are excluded as invalid
fixtures. Native composed captures and RAM observations are retained under
`G:/Temp/Codex/psxrecomp/x4-sigma-door113`.

Unit checks require physical clearance before publishing, one publication per
door, unchanged player bytes and every other camera byte, P2 projection
support, and no changes in Unified, private draw or non-gameplay contexts.
These local two-player checks do not qualify Four Player or cross-OS netplay.
