# October 12 co-op feedback assessment

Assessment: `beads-eio.9.70`. Existing checklist numbers and qualifications in
[project issues](https://github.com/mstan/MegaManX4Recomp/issues) remain unchanged.
That checklist has 49 stable items, 41 checked. New evidence below does not
revalidate earlier staircase, elevator, graphics, or netplay fixes.

## Timestamped observations

## Burndown

| Order | Issue / status | Evidence and next useful action |
| --- | --- | --- |
| 1 | Empty automatic startup capture — existing item48 / `beads-eio.9.71`, fixed and shipped in Playtest 9 | Confirmed by both archives and independently reproduced using the actual public8 executable. Under a detached/no-console Windows launch, the stderr FILE reports `_fileno=-2`; replacing fd2 leaves that FILE unattached, so stdio writes fail while begin reports success. Reopen the actual FILE and attach raw/native stderr, check header/location writes, and report capture failure. The production release captures startup and later application errors while preserving redirected stderr. Nine Windows logging cases, all 31 title checks and four Linux cases pass. Published Playtest 8 assets remain unchanged. This is a logging fix, not a gameplay fix. |
| 2 | P2 plus HUD removal — existing item31 / `beads-eio.9.61`, fixed and qualified for Playtest9 | QA111/112 reproduce the removal at Jungle's opening waterfall with both players at HP32 and failure8 (`actor-texture`); native R1-only QA115 confirms the reused-slot cause. Rendering now follows the native static-artwork flags instead of decoding the stale Gravity Well pointer. QA116 passes two P1 cycles; strengthened QA117/X-P2 and QA118/X-P1 each pass18 attacks, including visible full charged busters, healthy bodies, enrollment and failure0 throughout. QA118 retains the original stale pointer while its shot and both HUDs remain visible. Commit1903e75 is on master. Item31 is checked for this reproduced weapon-reuse case; other disappearance locations are not independently qualified. |
| 3 | Missing visible staircase shot — new item49 / `beads-nbnc`, deferred and unresolved | QA113 shows ordinary and three charged travelling X/P1 shots on the green and purple stairs. QA114 shows ordinary and three charged X/P2 shots on the purple stairs. Its first-section hurt-lock setup is excluded. The supplied missing-shot state remains unreproduced; no projectile damage was measured. Keep separate from staircase-height and elevator reports. |
| 4 | Green cone appearance — comparison only, no new bug issue | Compare the same weapon and armor state against native solo rendering if this appearance is reported as wrong. Do not classify an aiming effect as malformed geometry from its size alone. |
| — | Existing items34/36/46 and online items30/32/42/43 | No new matching evidence establishes the previously reported teleporter corruption, floating pickup/feet, later elevator failure, WAN boot failure, independent-view switching, or performance cause. Their existing qualifications and unresolved limits remain intact. |

The original video alone cannot distinguish intentional withdrawal from co-op
failure. The independent native reproduction below establishes the latter path
for the matching Jungle weapon-cycle scene. Its failure occurs at 271–281 sprite
banks, excluding the 4096-entry cache limit for this occurrence. The green aiming
cone is not the diagnosed failure.

## Reproduction sequence supported by the video

1. Launch the matching Windows package with X/P1 and Zero/P2; preserve the
   original armor, display, co-op and controller choices if recovered.
2. Use native Continue, load memory-card slot 1 / Data 1, then select Jungle.
   Do not substitute the previous boss-door or secret-waterfall test route.
3. Complete the normal arrivals and remain near the opening waterfall ledge
   with both players alive. Repeatedly cycle P1's weapons and fire/charge as
   shown from roughly 00:42 onward; keep P2 present through the sequence.
4. Record both seats' actual inputs, including Select, and capture presence
   transitions, co-op disabled reason, player health/position and resource
   counts while approaching the removal near 01:48.45.
5. Separately reach the native rotating staircase for the MOV report. Repeat
   ordinary and charged X shots with the bodies separated for observation,
   then with the overlapping arrangement shown in the clip.

This is a route recipe, not a deterministic replay: the exact starting cards,
settings, mod configuration and input timing are missing. The initial media
assessment did not launch gameplay; the native follow-up is recorded below.

## Native follow-up and disappearance fix validation

QA111 and QA112 load a generated native all-clear card through Continue, select
Jungle, then cycle P1 weapons0–8 while charging/releasing at the opening ledge.
Returning to the buster reproduces the video's visible P2/body-HUD removal.
Both players retain HP32 and the co-op failure code is8 (`actor-texture`), with
271–281 sprite banks rather than cache exhaustion. These runs initially use
development fixture7 for weapon selection.

QA115 removes that fixture: native R1 input alone cycles all nine weapons,
followed by a charged buster release after returning to weapon0. The same
failure occurs at frame2845. The rejected P1 shot is `801406F8`, frame38,
CLUT`7803`, page`002A`. Its retained table is `9F02BEF4`; original table
`801A47A4` minus native sprite base `801789A0` is offset`2BE04`, PL00's
compressed family9 used by Gravity Well. Native and immutable frame entries
agree (`0CA98711`), so this is not disagreement between the archive and live RAM.

The original buster initializers at `80092328` and `80092840` set actor flags
`21` without clearing a reused slot's `+38` pointer. Native drawing at
`800244D0`/`8002452C` uses flags`40`/`20` to select static artwork and map its
UV/page. The private renderer classified any pointer within the retained
compressed range as compressed, then interpreted the stale Gravity Well entry
as a buster frame. Texture rejection set failure8, disabling P2's control/HUD.

The implemented fix follows those native flags: static actors ignore stale
compressed pointers and use the existing immutable raw-artwork path, including
its original UV/page and owning-character palette. Parent-table fallback and
compressed remapping remain available for actors that use compressed artwork.
The fix does not clear native actor RAM, suppress an error, or introduce an
opt-in setting. Commit `1903e75` is on master and is qualified for Playtest9.
The retained `weapon_cycle` scenario uses native R1 input, two cycles of all nine
weapons, and frame-based charge/release windows; it supports either X seat.

QA116 passes the first two P1 cycles after the fix. The strengthened retained
helper then passes QA117 with X/P2 and QA118 with X/P1:18 attacks in each run,
including required visible full charged buster type`0x12` with the native
static-artwork flags. Both runs assert healthy active bodies, co-op enrollment
and failure0 throughout. QA118's second-cycle P1 buster retains the original
stale table `801A47A4`, yet its projectile and both HUD groups remain visible.
All three runs exit normally0. Item31 is checked for this reproduced
weapon-reuse failure; the earlier boss-door enemy-fire, Mushroom midboss and
intro disappearance reports are not independently qualified by these runs.

Private evidence: `build-pr23/qa-items31-111/`, `qa-items31-112/`,
`qa-items31-115/`, `qa-items49-113/`, and the corresponding QA114/116/117/118
captures/reports.
Other parked feedback items remain outside this focused reproduction work.

Final production builds pass all31 Windows and25 Linux title checks, with
development controls disabled. Hidden packaged launcher and room-offer checks
pass. Final unredirected Windows logs contain655/658 bytes, and the redirected
case659 bytes, including the real application error; redirected stderr remains
preserved. Both gameplay and logging changes are qualified for Playtest9.
The release is published; these checks do not qualify the parked WAN/Discord
reports or item49's unreproduced missing-shot state.

## Logging fix validation

The same final Windows fixture compiled against the old logger reproduces
zero-byte logs with genuinely unattached FILE=-2, absent/null/invalid native
handles. The repaired logger records its header and over20KB of subsequent
stdio, raw fd2 and native Windows writes while still running; redirected
stderr retains those messages. Failure cases reject unusable capture/location
paths and expose error text instead of a stale success pointer. The detached
pointer-failure test additionally asserts the actual error message.

Independent production application checks use private copies, hidden windows,
and isolated settings. The public8 binary produces0-byte logs with both
DETACHED_PROCESS and CREATE_NO_WINDOW and unredirected stdio. The repaired
production candidate writes669/670 bytes containing its header, main entry
and a deliberate real renderer rejection. Its redirected case writes671 bytes
and preserves original stderr. A detached hidden launcher has440 bytes at
startup, grows to1355 after a deliberately refused loopback connection, and
exits normally0. This validates failure logging, not the parked online-startup
or Discord authentication reports.

All31 Windows title checks pass; four Linux logging cases pass. A separate
read-only adversarial review found no blocking correctness issue; its
pointer-error visibility gap was addressed in the retained test. The inherited
file-handle fixture qualifies redirected-stderr preservation, not an
interactive console window. Private reports are
`feedback-20261012/package-logs-before/report.json`,
`feedback-20261012/package-logs-after/report.json`,
`feedback-20261012/package-logs-live-after/report.json`, and
`startup-log-investigation/retained-before-after.json` /
`linux-focused-report.json`, beneath `build-pr23/`.

The initial logger-only qualification used `build-playtest9-logfix-production/`,
with development controls off; the final production qualification above also
includes the gameplay fix. Logger fix commit: `ea81d58`. Playtest 9 is published,
its uploaded asset checksums are verified, and no older public release files were replaced.
