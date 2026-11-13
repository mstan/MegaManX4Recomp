# Marine Base and Mushroom release prerequisites

October 22, 2026. Next-release gate: `beads-eio.9.213`. The owner requires
both report groups below to be resolved before another public release.
The published 1.0.1 archive/tag is preserved. These fixes ship in 1.0.2.

## Marine Base bike exit (211)

Native 1.0.1 reproduces the failure at stage 5/section 1's final Chaser exit.
`8003BA24` clears the rider's mount and calls `80036AE4(0x17,0x40)` from
`8003BA84`. Co-op mistakes that personal ejection pose for a shared room
scene and hides the other rider before its vehicle can eject it. After P1
lands, P2 returns at P1's on-foot Y171 while still mounted. Its delayed native
ejection then begins below the platform; Zero falls to Y267 and loses all HP.

Let the original Chaser issue and release each rider's pose independently.
The exception is restricted to the native begin/release callers `8003BA84`
and `8003BAC4`, stage 5/section 1 and a live Chaser. Other vehicles, commands,
stages and shared scenes retain their existing handling. No health, lives,
landing coordinates, ejection physics or stage-success flags are patched.

The ownership regression fails on the original implementation and passes
with the fix. Native X/Unified and both rosters in local Split complete both
ejections and land at 32HP each without spending a shared life. Both Split
rosters then move under controller input, cross the actual Jet Stingray boss
door, finish its native introduction and regain controls.

The final Unified/Either widescreen continuation also exposed early Jet
allocation. Its authored category 0/type 56 placement is at X17146/Y138 in
table `800F6E78`. Extra-view scanning could allocate it before the physical door;
native `8006FD50` sees C4 zero both before entry and after completion, starts
the intro with the preceding room's bounds, and the boss disappears.

Both supplemental placement paths now leave that record unvisited until
the door establishes minimum X17024. The ordinary moving-edge scanner keeps
retail timing; delayed extra scans retry, including while stationary. Boss
initialization, flight, attacks and fortress refights remain native. An earlier
attempt to delay the allocated actor's AI failed and was discarded.

The final native Unified/Either check completes the actual door handoff with
both heroes healthy and controllable inside the arena. Jet remains alive and
moves across 34 subsequent samples over three seconds; the composed capture
shows the active boss and projectiles. Its extra-prefetch rejection counter
records the guarded attempt. Earlier allocation-only assertions and a test
that released held movement while confirming dialogue are excluded from this
continuation result. The reusable case now checks physical arena entry,
ordinary controls and a sustained live boss.

Setup skips the earlier bike course and positions riders and the autoscroll
camera on the last platform while the original pause menu is open. The
authored target and native dismount flag remain untouched. Early probes
that snapped back into the course or placed riders over the preceding gap
are excluded. Native on-foot checkpoint 1 establishes the platform's floor;
loading that checkpoint alone does not count as testing a dismount.

## Renewed Playtest 40 reports

### First opening-stage part2 drop (199)

The subtype 14 eligibility fix is already in 1.0.1. A fresh current native
check leaves P1 at X144 while controller-driven P2 wall jumps reach X634.
The lower piece 14 at X752 enters native state 1.3 and settles at Y392.75;
both heroes retain 32HP. Its object state, tile edits, fall and completion
are not forced. The initial QA assertion incorrectly demanded integer 392;
the corrected check retains the native fractional position. No additional
gameplay patch is needed. Existing source evidence is in[Feedback41](COOP_FEEDBACK41.md).

### Black Mushroom copies after P1 dies (203)

The stage-palette binding/upload fix is also already in 1.0.1. Fresh current
native qualification kills P1 before original boss admission, then lets P2
enter and drive the low-health copy pattern. All 128 working rainbow bytes
match their original stage source. Captures show colored green/pink/yellow
afterimages and normal copies, with P1 still dead and P2 at 32HP. Boss HP12
prepares the pattern; neither phases, clones nor palette colors are forced.
No additional palette gameplay patch is needed.

### Lift high jumps and late events (185)

Ordinary short jumps did not reproduce the report. Repeated native high
jumps/air attacks with Zero's acquired abilities do: both heroes remain32HP,
but the private camera accumulates 101 pixels of vertical drift. At observed
frame 46165, P1 cameraY1227 and P2 cameraY1328 show the shared lift and its
camera-dependent events at different heights. Earlier short probes did not
qualify this behavior.

The fix retains the canonical vertical scroll/current limits for another
view in the same shaft during native lift phase 1. It runs after personal
camera follow and before background parallax derivation. Rider physics,
horizontal views, panels and the one native lift/controller update remain
independent of this presentation correction. Native phase 2 releases the rail
at the top; another room, stage, section or actor does not use it. No new
host state or snapshot migration is introduced.

The composed boundary regression fails before the fix and passes afterward
for either owner, including fractional current/previous Y, parallax, top
release and unrelated-room/stage exclusions. The matched native observation
retains aligned cameras throughout the repeated high jumps. Idling into
the overhead panels still produces the authentic crush: those terminal
observation frames are not a successful full-ascent test.

## Final qualification

| Check | Result |
|---|---|
| Marine dismount | Native X/Unified and both Split rosters land healthy, preserve lives and regain controls. |
| Marine continuation | Both Split rosters pass the actual door/intro. Final adaptive-widescreen Unified/Either passes the door, controls and sustained live Jet check with the supplemental placement fix. |
| Lift high jumps | Both rosters, both-alive and P2-only survivor windows pass. All 1,538 observations retain exactly zero camera-Y separation; living riders stay 32HP and lives are unchanged. Composed captures show the aligned platform and backgrounds. |
| Opening object 199 | Both rosters pass actual P2-only wall-jump/drop checks with P1 stationary and both healthy. Existing 1.0.1 fix confirmed. |
| Mushroom palettes 203 | Both rosters pass P1-dead-before-admission native copy patterns with all 128 working bytes matching their original source and colored trails visible. Existing 1.0.1 fix confirmed. |
| Production | Windows Release builds with developer tools and synchronized debug commands OFF; all 38 CTests pass. The final expanded moving-edge allocation regression also passes. Python helpers compile and the diff passes whitespace checks. |

These report groups ship in v1.0.2 Stable Public Beta. Full Mushroom
ceiling-panel clearance is not claimed: native rise checks stop before the
authentic overhead crush, and native top-release behavior has composed boundary
coverage. No new online or Linux gameplay qualification is claimed. The
framework/UI pins, native memory cards and recording defaults are unchanged.
Release tracking: `beads-eio.9.214`. The v1.0.1 tag and download are preserved.

Private evidence: `G:/Temp/Codex/psxrecomp/mmx4-marine-dismount-20261021/`.
Reusable native cases: `marine_dismount`, `intro_first_drop`,
`mushroom_survivor_colors` and `mushroom_lift_high_jump` in
`tools/test_mmx4_coop_regressions.py`. All runs use owned hidden processes,
muted audio and private settings/cards/caches onG:.
