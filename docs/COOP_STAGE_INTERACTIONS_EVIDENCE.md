# X4 shared stage interaction evidence

Static evidence uses the owner's original US executable `mmx4/SLUS_005.61`
and its trusted generated C. No original executable bytes or game assets are
included. Native runtime qualification is separate from this trace.

## Military Train background updates — feedback item 28

Slash Beast's native introduction `8006DC24` sets authored foreground room
bounds. Those bounds do not stop the separate train scenery animation.
Stage 8's first background mode table `800F32D4` selects mode 5, whose
callback `800285E0` does nothing. Shared actors advance the scenery.

Real online Split qualifications QA79/85/86 kept reproducing the other pane's
frozen background while the canonical pane advanced 8/4 pixels per tick.
Both players were healthy and idle after P2's Slash entry. Earlier models
matched native function arithmetic but assumed the wrong physical actor
pool and kind numbering: neither the 20-slot `801410C0` pool nor the
32-slot `801435B0` pool contained the live boss scenery actor. Those
inferred pool gates rejected the actual callback, so their passing models
did not establish the requested native behavior.

QA87 supplies the decisive original callback/write trace. Repeated
`800D9218` entries have `a0 = 80169CB8` and `ra = 80021BDC`; the dispatch
table register is `s1 = 800F2AB4`. Native background writes at `800D9388`
and `800D93A4` use `s0 = 80169CB8` and advance background 1/2 by exactly
8/4 pixels. `800D8648` and `800D964C` are absent during this idle boss
loop. The trace is retained privately in
`qa-real-split-train-trace-87/train-background/native-traces.json`.

The original executable independently agrees with every traced address:

| Contract | Original instruction evidence | Result |
| --- | --- | --- |
| Pool base | `80021B38`: LUI `8017`; `80021B3C`: ADDIU `-6348` hex | `80169CB8` |
| Pool end | `80021B40` adds `C0`; `80021BE4` constructs `80170000 - 6288` | `80169D78` exclusive |
| Slot stride | `80021BE8` adds `30` | Four `30`-byte records; last slot `80169D48` |
| Table | `80021B60/64` construct `800F2AB4` | Kind 0 → `800D8648`; kind 2 → `800D9218` |
| Original caller | `80021BD4` executes JALR | Return PC `80021BDC`, matching QA87 |
| Matching allocator | `8002B048` constructs the same base, adds `C0`, advances `30` | Same four-slot pool |

The signed low immediate matters: `80170000 - 6348 = 80169CB8`.
The native dispatcher `80021B34` has a different pool/table from
`8002196C` and `80021A20`. The latter two loops cannot identify the
callback merely because their overlapping tables also contain `800D9218`.

| Live scenery kind | Native update | Background behavior |
| --- | --- | --- |
| 0 | `800D8648`, active `800D872C` | Initializes actor `+18` to `00080000`; adds that speed to background 1 X at `800D878C` and its arithmetic half to background 2 X at `800D87A8`. |
| 2 | `800D9218`, active `800D9330` | Initializes the same speed; adds it to background 1 X at `800D9388` and its arithmetic half to background 2 X at `800D93A4`. The outer update also calls `800D9B48` for boss-phase layer metadata. |

Native `80021158` calls foreground `80027850` at `800211D8`, background
helpers `80027D40`/`800281E8` at `800211E0/E8`, then the actual scenery
pool `80021B34` at `800211F0`. This ordering agrees with QA87's writes:
the background camera write at `80028238` precedes the train actor writes.
Thus live scenery runs after co-op camera snapshots; calling the mode 5
helper for the other pane cannot recreate those external world deltas.

Kind 2's region selector `800D993C` uses player X thresholds
5808/7648/9984/10000 and resets subphases when the region changes. Earlier
phase 1 calls full helper `800D964C` from `800D9480` only at `+6 == 2`.
If `+7 != 0` and `PLAYER+BC == 0`, it adds
`current_foreground_X - previous_foreground_X` to background 1 and its
arithmetic half to background 2. The stopped-train `+7 == 0` branch instead
decelerates or sets `+7 = 1` and shared limits; that transition tick adds no
foreground displacement. This earlier-phase helper is additional original
static evidence, separate from the directly observed idle boss loop.

The revised Split adapter accepts only the trace-corroborated four-slot
pool and kinds 0/2. It calls each original update once and propagates captured
background deltas to both saved cameras without adding them to canonical
RAM again. Where native `800D964C` actually runs its foreground branch, the
other pane receives its own recorded foreground contribution instead of the
canonical contribution. Arithmetic half preserves native `sra` for negative
odd fixed units. Changed authored limits, dead zones, shake and layer metadata
are copied; foreground current/previous origins remain private. Shared actor
effects, particles, story commands and animation updates still run once.
Unified, player projection and local drawing bypass the adapter. Section
reset starts both snapshots from the new native origins.

Focused models now use the actual four-slot table and exact QA87 base-slot
kind 2 callback. Foreground/background camera snapshots precede shared pool
dispatch. They require one original 8/4-pixel update, absence of `800D964C`
during the idle loop, both panes advancing, and no second addition on the
next frame. First/last slots and rejection of both previously inferred pools,
below-base, exclusive-end and misaligned records are covered. Existing cases
retain independent origins, owner switches, fractional movement, freeze
gates, stopping transition, region reset, initialization metadata, uint32 wrap,
shared scenes, cleanup, wrong kind, projection, reset, local drawing, another
stage and Unified. This revision has source inspection and a clean whitespace
diff. Root owns model execution and repeat native raster/CRC qualification;
item 28 remains open until that requested behavior is validated.

Native qualification requires a healthy connected two-peer Split match:
load stage 8, section 1, checkpoint 3; place both players at X9360/Y395 and
use native Right to cross X9472 with either leading seat while the other
remains at X9360. After the warning/introduction, keep both living players
stationary and compare completed-frame captures over several seconds. Both
saved scenery panes must advance while foreground stays inside boss limits.
Read four records at `80169CB8 + slot*30`, especially the observed base-slot
kind 2: bytes `0/1/4/5/6/7/14` and word `18`. Inspect canonical backgrounds
`80141A04` and `80141A58`, current X/Y at `+8/+C`, previous X/Y at
`+14/+18`, and the saved camera state published by `mmx4.coop.split-state`.
The `mmx4.coop.split-train-background-ticks` counter should advance during
the observed original loop. Repeat both leading seats and campaign rosters;
verify matching deterministic state on both peers.

## Lightning Web wall-jump anchors — feedback item 22

The requested enhancement concerns X's Lightning Web weapon, earned from Web
Spider. The boss's damaging/capturing web actors are different: secondary
types 22/23 (`8009D74C`/`8009DD40`) have a null solid pointer and use native
player damage/capture. They are outside the requested enhancement.

### October 13 follow-up — feedback item 56

Zero repeatedly alternates Fall7 and WallGrip9 at unchanged XY, including
frames3837–3863,4695–4747 and4926–4951. The adapter accepted Fall7/FF and
WallSlide11/FF but rejected the intervening WallGrip9. Original80034968 sets
action9 and an eight-tick timer while inheriting the rising/falling control
byte. Original80031B90 abandons the grip if contact disappears; otherwise its
timer eventually selects WallSlide11. Rejecting the grip therefore creates a
Fall7/Grip9 loop with zero velocity. The older single-jump check missed this:
an immediate Jump edge can escape during an accepted Fall7 tick.

Accept WallGrip9 with either inherited1 orFF, for the native X owner and Zero
partner. Also accept Zero's native double jump6/FF: original80039D9C returns
to action6 without replacing the falling control byte. X's invalid6/FF and
the existing scene/death/warp/vehicle guards remain excluded. Geometry,
contact history, snapshot format and body top/bottom cleanup are unchanged.

The strengthened native probe first requires real side contact to reach
WallSlide11 and move downward, then checks Jump10's upward/away trajectory.
It fails on the unchanged Playtest11 debug executable with the same Grip9
loop. The focused regression also fails against the original web adapter and
passes the changed source. The probe additionally supports `--top-contact`
to compare X and Zero falling onto the web.

That comparison exposed the separate top-contact defect: original800931A8
checks the previous contact's vertical bits and calls80093260 to expire the
web before its next animation update. The adapter's partner-only history
never reached that check, so Zero was pushed back above the web repeatedly
instead of collapsing it as X does. A narrow filter on800931A8 temporarily
includes the partner's previous vertical bits in that original check, then
restores the owner's contact byte. It runs the original update exactly once;
side contact and each player's carry history remain private. Focused tests
cover top/bottom contact in both rosters, one native expiration, no early
expiration for side contact, register/owner-history preservation, local-view
exclusion, recursion and reused shot slots.

Final hidden native runs `native-final-x` and `native-final-zero` each pass
four cases on executableSHA256
`4af7617c67e962dc0a6e2f21310167a236ccd79130b37ebcd53555586f0cb301`.
Both sides reach WallSlide11 with downward motion, then Jump10 with upward
and away motion. X and Zero each collapse the web and fall through its top
in17–28 ticks after observing its opening, before its normal timeout; Zero's
new native expiration counter advances exactly once. Both rosters retain
HP32. The unchanged Playtest11 native baseline fails sustained side contact
and independently leaves Zero atY86.789 above a web atY130 during the top
case. All32 Windows title checks pass. No visible windows or desktop input
were used. Only generated shard42 changes for the new trusted hook, listed
in both development and release game configurations. This is source
qualification for the next release; published Playtest11 remains unchanged.

Lightning Web is player shot type 1, native update `80092F08` (dispatch table
`800F25D8`). Its opening animation installs solid hitbox `80108800` at `+68`
and enables `+75` in `800932A0`. The update calls original solid contact
`8002E184`, then clears top/bottom contact bits from `PLAYER+71` according to
shot `+72` (`80092FA8`–`80092FFC`). Horizontal contact remains available to
the player's ordinary wall-jump action. Expiry clears the solid eligibility.

Native solid comparison `8002C36C` writes shot `+72` and player `+71`/`+4A`
from actual contact geometry; it has no X/Zero character check. Co-op keeps
each player's 16-shot pool private. The existing canonical solid retry reaches
P2 for P1's web, but a P2-owned web updates while projected, so the canonical
retry is skipped and P1 never contacts that web. The native post-contact
top/bottom cleanup also applies only to the singleton owner body. A narrow
enhancement must offer each live partner only the web's solid contact, retain
its per-seat contact history, and perform the same per-seat top/bottom cleanup.
It must not update the projectile animation, lifetime, movement, or damage
twice, and must not change Web Spider's enemy web behavior.

The separate `mmx4_coop_web.c` adapter handles only an active player shot 1
with enabled `+75` and the verified `80108800` solid pointer. It calls the
original `8002E184` once for X and offers the other living normal Zero body
the player portion of that collision: `8002E294` position response,
`8002E380` carry when that partner had prior contact, and `8002C36C` side
comparison. These are full native function entries in the trusted ranges;
each takes the victim as explicit `a1`, and carry takes prior side bits as
`a2`. The adapter preserves the caller's `a2` and original return value.

The noncurrent Zero body is copied to one deterministic guest mirror while
its owner and both private shot pools stay in place. Only the helpers' proven
mutable bytes return to the saved body: integer XY at `+A/+E`, separation
XY at `+6C/+6E`, wall-contact kind at `+4A`, and sides at `+71`. Fractional
coordinates, action, HP, inventory and campaign state are not copied back.
Top/bottom cleanup matches X's original web update. History is indexed by
allocating seat and the 16 native shot slots, so identical guest shot addresses
in different private pools remain distinct. Native allocator `8002AC0C`
returning a reused slot invalidates only that seat's old contact; stage reset
clears both sets, and the deterministic digest includes every history byte.

Queries exclude incoming actions 0/1, hidden/dead/vehicle bodies, scene and
door holds, unknown `+67` movement states, other relevant native holds,
and local drawing. `+67` is also used for normal airborne movement, so the
adapter accepts the inspected action/control pairs: Jump 6/1
(`80034538`), Fall 7/FF (`80034604`), WallGrip9 with inherited1/FF
(`80034968`), WallJump 10/1 (`800349F4`) and WallSlide11/FF (`80034AFC`),
plus Zero's double jump6/FF (`80039D9C`) and ordinary control0. Treating every FF value as a
script lock would disable web contact while Zero approaches or slides along
the anchor. Native `80033B34` requires Jump edge and positive wall-contact
kind at `+4A`; `800349F4` then starts actual action 10 with upward velocity
and horizontal velocity away from the surface. Both player arrangements,
active combat pose 48, and Jump/Fall/WallSlide/WallJump contact progression
have focused model coverage. Additional cases cover real contact geometry and
carry ordering, opposite wall sides, shot-pool aliasing, allocation reuse,
vertical cleanup, field/return/register preservation, story/arrival gates,
unopened/expired/other projectiles, reset and allocation failure. No projectile
update runs twice and no enemy web path changes.

Root owns integration, compilation, model execution and native wall-jump
qualification. Native qualification should select Lightning Web weapon 1 for
the X seat using the existing development weapon command, fire with original
controller input in an ordinary stage, then have Zero approach each opened
web side and press native Jump while moving into it. Verify actual normal
wall-jump action/trajectory in both rosters and camera modes, expiry/reuse,
and scene/warp exclusions. Counters `mmx4.coop.zero-web-queries` and
`mmx4.coop.zero-web-side-contacts` distinguish an offered native query from
accepted horizontal contact. The `mmx4.coop.zero-web-body-mirror` counter
publishes the existing guest query mirror address for read-only contact
inspection; it allocates no extra diagnostic sidecar. The owner has not yet visually qualified this
enhancement.

`tools/mmx4_web_probe.py` requires an explicit existing private TCP port and
never launches a process. With `--scenario web --campaign 0` or `1`, it uses
the observed Jungle platform at X56/Y139, native weapon-selection fixture 7
and Square input to create a real web. It reads both live solid boxes to align
Zero beside the opened anchor, requires direction input to produce actual
side contact, and then checks native Jump for action 10 plus an upward/away
trajectory. The safe setup is bounded to the observed initial platform's
X32–256/Y75–139 air column; a web outside that column is reported as an
unqualified setup rather than teleporting into unobserved terrain. For X/P2,
the existing diagnostic tail publishes one completed opened-web record with
WEB1 header at `+6F0` and the native `9C`-byte shot at `+700`. X/P1 uses the
original shot pool. The helper releases inputs on failure and retains JSON
observations and a native action 10 screenshot for root's visual review.

The same helper's `--scenario train --owner both` drives the original Slash
entry for each leading seat and compares background X movement in both saved
Split cameras during an idle fight. An existing private invulnerability
fixture prevents combat knockback during the measurement and is cleared
afterward. Both scenery panes must advance while the foreground remains
within one pixel of its initial camera origin. Source syntax was inspected
without connecting to or controlling a runtime; root owns these qualifications.

Actual Split views require an online match; requesting Split in an offline
configuration keeps Unified behavior. `tools/mmx4_netplay_runtime.py` supplies
`NetplayRuntime(existing_development_match)` for the unchanged native probe.
Read/capture requests use peer 0, setup fixtures use the existing admitted
`NetplayDevelopment` packets, and seat input goes to that seat's peer on the
host input layer. A both-player teleport uses one fixture 4/seat 2 command.
Direct RAM writes and other raw mutation requests are rejected. Its eight
offline fake-peer tests verify routing and completed-body reads, with sockets
prohibited; live Split/native CRC and visual qualification remain with root.

Root native QA88 qualifies the corrected range using two connected hidden
peers with real Split rendering and synchronized development fixtures.
The live base-slot actor is type2, state[1,5,0,0], speed00080000. Original
Slash entry is exercised with each leading seat. During two-second idle
windows, both saved scenery panes advance equally: BG1/BG2 +496/+248px
for owner1 and +464/+232px for owner0, while foreground X changes0px.
Both peers' before/after screenshots show the moving train structures.
Core/mod CRC samples match after each qualification, through tick8672.
All18 title CTests pass. This corrects the reproduced Split scenery freeze;
it does not claim complete campaign or production/display FPS coverage.

Native Zero wall-jump qualifications QA76/77 also pass in both player
arrangements. A real X Lightning Web is fired with native controller input;
Zero enters action10 and moves upward and away from the contacted anchor.
Those cases qualify the enhancement, beyond model-only contact fields.

## Checkpoint loading and retained-player cameras (2026-10-11)

Hidden original-disc QA15/16/17 reproduces a healthy P2 becoming a carried
corpse after a native Bio Lab section0 checkpoint1 load. P2 has HP32 at
8001FC20, then is marked dead by the next 80035240 initializer. This happens
without a native P2 damage/death update. The co-op camera constraint checks
the outgoing P2's Y against incoming room limits during loading; its direct
HP128 write is subsequently retained as a real death.

Camera target, owner scope and team constraints now require mode6/minor0
and an initialized canonical PLAYER dispatcher. A dying/dead primary
(states2/3) still permits the survivor's camera. Loading mode4/5 and the
temporary mode6/state0 boundary use the native camera. Partner drawing also
excludes that uninitialized boundary, avoiding old-body rendering in the
new room. Carried-death counters distinguish this bookkeeping from a plugin
failure. There is no opt-in setting for the correction.

QA18 replays the formerly failing checkpoint with both HP32, concurrent
arrivals, clear controls and unchanged shared lives. The reusable
tools/test_mmx4_coop_checkpoint.py then passes both pause owners, voluntary
withdrawal across a checkpoint and Select rejoin, section1/checkpoint1
arrival, P1 death with a living P2 executing its native jump, and a full
team retry spending exactly one life (2 to1). All18 title checks pass.

These are native loader/controller qualifications. The owner's exact Web
Spider enemy-shot/HUD disappearance and staircase teleporter clip remain
unqualified; this checkpoint result does not close those reports.

## Executable-relative playtest diagnostics, October 11

Actual packaged Playtest6 creates default-on recordings, but its recording
directory follows the configured memory-card root. A development launcher
probe showed that root resolving outside the presumed package saves folder.
The next title recorder therefore uses the actual application directory,
independent of cwd and card configuration. Windows uses the running module
path; native Linux uses `/proc/self/exe`; AppImage uses the portable image
path preserved by AppRun in `RECOMP_APPIMAGE_PATH`, outside the mounted image.
`diagnostics-location.txt` beside the application names the exact session or
a recording error. The input display shows ERROR on write failure.

Every session retains accepted inputs, starting cards or absence markers,
mod choices, available settings and network/build/execution metadata. The
offline replay helper uses isolated cards/settings and rejects marked network
traces before launching anything; network logs remain inspectable.

Native Windows nextQA30 presses the real hidden launcher Play button, boots
X/P1 and Zero/P2, drives movement/attacks and each native pause owner, and
exits normally. It produces2896 input records; offline playback verifies2895
input-state hashes. QA31 boots the opposite roster, produces2551 records and
verifies2550 on playback. Both deliberately store memory cards separately
under private-cards while recordings stay beside their executables.
Real two-peer LAN QA32 produces2521 records per seat and95 matching core/mod
CRC samples through3166 while each seat moves/attacks. These are local LAN
captures, not qualification of full internet replay. All18 Windows checks
pass. Linux g++ also builds/runs the recorder, replay, error and real native
executable/AppImage path tests; full Linux gameplay qualification is separate.

## Split Mushroom lift and staircase review, October 11

Commit11da504 fixes an immediate co-op fatality at the circular lift's native
section1/checkpoint2: it sets destination camera maximum720 while the actual
camera remains2047 and riders are nearY2200. The death plane now uses the
deeper of the current camera and authored room maximum. QA35 passes the
native limit change, team retry spending exactly one shared life, and continued
ascent in both rosters. X/P1 reaches camera1746 with bothHP32 atY1900/1899;
Zero/P1 reaches camera1748 with bothHP32 atY1901/1902. Reports are under
build-pr23/qa-mushroom-parity-35/{x,zero}-custom/lift/report.json.

QA34's11.75-second recording separately shows centered idle riders meeting an
overhead panel atY1233. The owner watched it and confirms authentic behavior:
players need to destroy those panels. This is no longer treated as a bug.
The reusable elevator check stops before the obstacles instead of requiring
idle riders to survive the entire ascent. No crush policy was weakened.

## Offline co-op states and diagnostic baselines, October 11

Offline co-op now registers a versioned, pointer-free host snapshot provider.
It preserves both native player contexts, lifecycle/menu ownership, combat/web
state, private audio headers, Split/widescreen state and immutable texture-bank
descriptors alongside the guest/device snapshot. Resource initialization runs
before a possible first-PC load. Immutable SPU sample banks wait for hardware
initialization and rebind once per hardware epoch. The provider excludes live
guest-call/projection stacks; shared save/load and rewind defer until the
continuation context is safe. Network user state jumps remain blocked.

Native Windows QA39 uses X/P1 and QA40 uses Zero/P1. Both drive the mapped F7
panel and actual Square Save/Cross Load, move both bodies, restore positions,
restore P2 after a fatal HP fixture, and restore a settled P2-owned native pause
menu that P2 can unpause. They select/accept and cancel real F8 rewind history,
then start a new process with slot0 auto-load and attack with both characters.
QA44 repeats the final shared safe-context changes with X/P1. All retain both
healthy active bodies and the original roster; inspected warm/cold/rewound
screenshots preserve complete sprites and aligned HUDs.

Successful offline state loads and rewind start a fresh diagnostic segment
with their complete baseline.pst. Checked loaders notify only after accepting
the restored PC; failed loads roll back quietly and do not split recordings.
The replay runtime compares the actual restored image against the required
baseline before admitting input rows. QA41 cold-load playback verifies152
hashes; QA42 raw-rewind playback verifies48. QA45 repeats cold-load playback
on the final X/P1 build and verifies155 hashes. Native Linux focused serializer,
no-provider handoff, arena reset, RAM geometry, diagnostics and replay-preparation
tests pass. Direct AppImage playback by the helper is rejected with extraction
instructions; extracted native Linux payloads use copied libraries and private
mod/settings/data roots. This is not full Linux gameplay qualification.

Final two-peer native LAN QA43 reaches original gameplay, exercises movement
and attacks for each seat, and matches99 core/mod CRC samples through tick3201.
Its last closed-menu save-unavailable assertion was invalid: that field reports
the currently displayed message, not capability while the menu is closed.
Network request refusal remains covered by the shared title-policy test. This
LAN check does not qualify a Discord login, WAN path or full network playback.

Session review also found enhancement allocation cursors and packet/texture
registrations surviving lobby teardown. The runtime now retires those resources
after renderer teardown and before each activation. Focused tests qualify
five large identical allocation layouts,20 packet/bank reuses, active-pass
refusal, journal retirement and preservation of ordinary RAM/scratchpad. A
complete UI-driven rematch remains additional coverage rather than a claimed
native result.
# Independent character codes and complete room plans (October11)

The next build adds separate default-off Ultimate Armor X code and Black Zero
packages, plus corresponding host-agreed co-op options. The original X code
selects PL02 resources and makes a capsule grant the armor; it does not grant
armor at stage start. Black Zero retains original stats and palette13. P1
continues to own the campaign and saved code byte; P2 uses its selected code
only within its projected context. Both X archives are prepared before cold
restore, share verified-identical UI/audio, and occupy the existing fixed
reservation. Sprite descriptors record the archive variant; the co-op host
snapshot schema is now2.

Private QA46 (X/P1) and QA47 (Zero/P1), both codes enabled, pass native mapped
save/load, repeated loads, P2 death restore, P2 pause ownership, rewind
accept/cancel and fresh-process load followed by attacks. Inspected restored
images show X's purple code helmet and black Zero. QA49 replays QA47's exact
cold baseline and verifies158 hashes. QA51 separately boots each solo campaign
with both standalone packages enabled, observes the selected original code
body and attacks, and confirms neither campaign receives armor at stage start.
The original-EXE fixture executes resource selection, palette copy, selector
clearing, capsule eligibility/grant and Nova gating/energy instructions; it
passes on Windows and Linux. It also rejects duplicate hooks and tests P2 actor
scopes that retain P1's campaign selector.

Native QA54 publishes all six camera/HUD/code combinations with distinct
settings fingerprints. Its real launcher adopts Unified/Split host choices
with codes enabled and disabled despite opposite local preferences, rejects
wrong fingerprints and undeclared choices, and leaves persisted preferences
unchanged. The initial QA48 query failed because the shared START encoder
silently truncated its640-byte nested field. Shared commit5a2332b6a replaces
fixed scratch limits throughout encoding, queueing and inbound framing with
the declared12-package capacities, and rejects truncation before publication.
Focused actual-metadata and maximum-plan tests pass on Windows with and
without ICE and on Linux with ICE. QA53 boots two real hidden LAN peers with
both options enabled: each seat moves/attacks and100 core/mod digests agree
through tick3250. This does not qualify Discord authorization or WAN/NAT.

All21 Windows title checks pass. No game window was shown, and every native
run used explicit private memory cards. Full Linux gameplay remains additional
coverage. These changes are not in the published Playtest6.
# GitHub25 intro warning review (October11)

A fresh hidden debug build from Playtest7 source8207f27, using the original
disc, OpenGL/OpenBIOS and private cards, passes all four checkpoint2
campaign/dead-seat combinations. An original fatal-HP request runs the native
death; the survivor then crosses the authored gate through controller input.
The original Eregion actor8 initializes withHP48 and renders; the corpse stays
dead and shared lives stay unchanged. Checkpoint setup alone does not qualify
the complete first-approach warning, so additional runs retain checkpoint0,
use the observed3936/443 pre-boss floor, and let the original camera settle
before death and controller-only traversal. No boss HP, script phase, gate,
completion flag or warning timer is written.

Private `qa-github25/settled-x-p1-dead` shows the actual WARNING in inspected
approach08, its disappearance in approach14 and the Eregion/body meter in
approach19. General24 progresses phases0,1,3,4 and retires. X remains atHP0,
state3; Zero enters normal action2 withHP32 atX4452.5/Y442 and lives2. A
near-gate fatality and a fatality during the real WARNING also finish. The
reusable `tools/test_mmx4_coop_intro_survivor.py` repeats the latter, requires
the warning actor to retire before success and proves the survivor moves
after the boss appears. Final output is `qa-github25/reusable-warning-test`.

The existing generic sampler's field named `warning` reads80141BDC, which is
the shared fade/busy state initialized by original800129F0. It cannot prove
the rendered WARNING occurred. The new tool records the actual General24
state and captures the screen; this distinction limits older field-only
claims. Development fixtures require this tool for reproduction rather than
input-only diagnostic replay. Current code passes the reported setup, but a
matching Playtest6 baseline failure and full original route were not reproduced.

PR17 was also reviewed and closed as obsolete, without merging its prototype
branch. Its original commit e7b7079 is not an ancestor, but its complete HUD
behavior is included and superseded by ancestor e2952be: defaultX0/Y104,
optional Side by sideX56/Y0, and unshifted own-player Split HUD. Master also
includes the later native anchoring and pause drawing fixes. No runtime change
or new release is needed for this review. Central review issue: `beads-enkq`.

## Renewed online startup report, October 11

The suspected completed-agent race is not established: the server requires
the guest's completed-connection path report before starting a two-seat room.
Next evidence should identify the exact PR/build and capture both peers'
startup stderr, including any launch refusal and the host's admission stall.
The title input recorder starts after native input execution, so its absence
during a pre-boot white screen is expected. Keep checklist32 and central
`beads-eio.9.42` open.

## October 11 independent-view fix and new Mushroom footage

The linked Mushroom video RZd3pBBAKfA was downloaded at720p and reviewed over
its complete356.85s timeline, with denser2fps review of109-144s and1fps
review of298-322s. Private originals, contact sheets and excerpts are under
`qa-mushroom-0783`. It shows a green-portal entry around110s, P2 dropping
out of the destination platform, a subsequent death/retry around138-144s,
and P1's failed arrival while P2 remains on the platform. The later purple
staircase remains the reported item36. The recording reaches the green lift
near348s and cuts to black at the end; it does not show a later P2 lift death.
Distinguish those visible symptoms from exact native causes and from the
separately qualified earlier destination-plane correction/authentic panels.

Private native `qa-mushroom-portal-60` probes authored stage3 checkpoint1 in
both sections. Its short observation includes native initialization/arrival,
not a complete P2-led portal-and-team-retry reproduction. Both bodies retain
HP32; section1 finishes atX2912/2936,Y427/426 with collision boxes and ground
contacts. Section0's final sample still has P1 in incoming action0 while P2
is in action2; this is insufficient to qualify completed retry collision.
No new Mushroom runtime patch or fix claim follows from that probe.
Checklist44/45/46 and `beads-kmsf` remain open.

## October 11 Mushroom collision and P2 exit fixes

Extended hidden native QA61 reproduces checkpoint1's platform defect after
complete arrival: X/P1 is atY187 with terrain ground, while Zero/P2 is atY170
on the portal. P1's jump returns toY187 and native team retry repeats it.
Retail800C2BE0 dispatches the portal state, then calls8002E184 atC2C20.
When P2 owns the portal, that solid pass executes inside the story projection,
where the combat wrapper declines and only P2 receives collision. Defer this
specific nested pass, restore the canonical actor's four contact fields, then
run the native solid wrapper once for both players. The portal's story still
updates once using its owner and independent previous contact history.

QA64 now keeps X/P1 atY171 on the same platform, including jump/landing and
team retry. Opposite-roster QA73 keeps Zero/P1 atY170 and X/P2 atY171.
The reusable `tools/test_mmx4_coop_mushroom.py` derives the platform top and
each player's foot from their actual native collision boxes, rather than
assuming a shared character height. It checks landing, jump/landing and a
native fatal team retry with exactly one life spent. Enemy invulnerability
and health are set after checkpoint initialization; QA72's earlier failure
was a legitimate setup enemy hit (X/P2 HP29), not a platform failure.

QA65 reproduces the horizontal P2-first exit failure with P1 atX3380/Y203:
P2 walks fromX3480/Y138 toX3619/Y267 and dies, leaving checkpoint1 active.
Mushroom's native General8 camera controller800B7078 selects its phase from
PLAYER X/Y inB7528. B7394 advances to checkpoint2 atX3536; B7440/B74EC
handle the leftward return at the lift's top. These routines write shared
camera/script fields without writing the player. Run that single native
update with the progressing seat's coordinates, then restore P1 immediately.
Choose the rightward leader before the lift and the leftward leader afterward;
keep existing story ownership, dead-seat, projection and local-render guards.
The hook is in both game configurations and locally regenerated codegen.

QA67 and opposite-roster QA73 now take that real P2-controlled exit to the
authored lift checkpoint, both HP32 with no transition life spent. QA73 also
passes the existing second-lift arrival, destination-before-camera, native
retry and continued-ascent checks. All21 Windows title checks and Linux
combat/lifecycle/split ownership checks pass. Checklist44/45 are complete
in source; this does not publish a new build or close the staircase/lift report.

QA71 uses the supplied armor choices and1080p/4x settings, a development
skip across the native preceding section exit, and original native background
rendering. The deliberately skipped exit touches solid geometry, so this is
not boss-completion qualification. After section1 initialization, X/Zero's
grounded heights follow714-floor(X/4) and713-floor(X/4) atX400/800/1200.
The inspected images have not reproduced the reported floating offset.
Both riders remain healthy and carried throughY1776/1775 in the subsequent
lift ascent. The custom-map checker is not a valid renderer comparison here
because the supplied settings disable that renderer. Preserve the captures,
leave item36 open, and qualify the later top/exit separately for item46.

The stronger green-portal return case exposes another floor failure before
the checkpoint retry: P2 remains below the entrance atY186/X2382, while P1
stands atY171/X2344. QA82's native solid trace observes incoming P2 atY178,
thenY186, with no partner solid pass: `mmx4_coop_alive(1)` excludes an incoming
warp. That combat exclusion must not suppress the world-solid floor needed
by the original arrival controller. Allow the solid wrapper to process a
healthy, visible, initialized incoming body; outgoing/withdrawn/dead bodies
and enemy interaction retain their existing exclusions. Negative Mushroom
portal returns also wait for P1's completed ground contact and preserve that
platform height instead of358A4's terrain landing height. Character-native
box centre/half-height offsets align both feet at enrollment and shared-room
returns. Cancellation origins retain the passenger's actual placed position.

Final QA84 (X/P1, Zero/P2) and QA85 (Zero/P1, X/P2) use actual P2 green-portal
entry with neutral controls during the return. Both bodies land on the native
platform, then pass jump/landing, a one-life team retry and the actual P2-led
horizontal exit. The reusable helper derives each contact surface from native
boxes and enables this additional regression with `--portal-return`.
Windows's21 checks and Linux combat/lifecycle/controller checks pass; native
full-stage initial beams retain their original concurrent enrollment path.

Full-ascent QA74/75/79 do not qualify the later elevator report: attempted
attacks leave ceiling geometry intact and both riders reach native contact
bits12 atY1233 (orY1313 when positioned farther left). This matches the earlier
owner-confirmed authentic crush and is not evidence for changing crush policy.
No new P2-specific elevator failure or staircase-offset cause is established;
items36/46 remain open. All owned native probes are hidden and use private
cards, without desktop input or changes to the user's running game.

## Metadata6: intro survivor crusher shaft (2026-10-14)

Item88 / beads-eio.9.120. Exact Playtest21 local recording1792032378566405
verifies all9,313 retained native input/state checks. P1 Zero dies at X863/Y394;
X/P2 reaches the crusher floor at X3758/Y955 with23HP. All22 solid type3 panels
remain in state1: original C0558 loads only PLAYER.y at C0570 before comparing
with the authored Y944 threshold. Moving only the corpse coordinates in a
private diagnostic branch starts the panels, confirming the singleton predicate.

The C0574 instruction callback substitutes living P2's signed Y when P1 is
unavailable, only for the original intro section1 type3 waiting panels. It keeps
original movement, tile updates and shared collision handling. No invulnerability
or crush policy is changed. Both character arrangements and eight ineligible
contexts pass the focused lifecycle test.

From the exact recorded endpoint, the corrected continuation uses ordinary
Right/jump pulses to climb and leave the shaft at X4175/Y443, with P2 HP23 and
P1 still dead/unmoved. The initial idle probe was inappropriate: those panels
remain a real crushing hazard; the successful check climbs during their movement.
Private evidence: build-pr23/feedback21-metadata6-20261015/{exact-intro2,
inspect-intro2-b,hole-before-probe,hole-native-escape-final}. The inspection build
translated only its private debug identity header; all original state checks
remain, and the separate original production replay verifies without translation.

## Metadata6: intro act1 paired-door admission (2026-10-14)

Item91 / beads-eio.9.123. First recording1792032245331228 reproduces a nonfatal
story stall; its6,569 retained checks verify. The native D0580 camera comparison
admits the act1 exit when cameraX reaches4848 even if Both-player traversal is
still waiting on the partner. Command21 aborts that crossing before its new
minimum commits. Native34150 then walks toward cameraX+64 while the camera
follows left; P1 reaches the wall at4874 and C0 never finishes. P2 stays hidden.

The instruction predicate retains the original wait through door phases1-3.
Once physical clearance commits minimum4848, the authored walk ends at4912.
A verified pre-admission recording prefix supplies the native A/B: identical
controller approach stalls on the old build (min4368, P1X4872, P2 hidden), while
the fixed build crosses and completes the story into stage0/section1. Focused
checks cover phases1-4, unrelated/no crossing and stale serials. No cutscene
or native story is skipped. Private evidence: feedback21-metadata6-20261015/
{inspect-intro1,intro1-prefix,intro-door-before,intro-door-fixed,intro-door-final}
under build-pr23. Older item53's reported act2 process crash stays unconfirmed.

## Metadata6: shared Maverick victory poses (2026-10-14)

Item90 / beads-eio.9.122. A defeated main Maverick's native command14 now
keeps both living/present players for the shared end sequence. Each runs native
34F7C/33368 character cleanup, pose39 and outgoing beam. P2 omits only the
shared music request; P1 retains the campaign theme and one reward/save path.
Native stage termination waits if the other character's outgoing animation
still runs. Midbosses18/33/70, story/movie endings and unavailable partners
retain their existing flow.

Actual native P2 attacks defeat reduced-health Jet Stingray in both campaigns.
Both bodies reach action25/pose39 together: captures show X's raised fist and
Zero's salute, without partner departure/rearrival. One scene and one P2 pose
counter fire; the reward and Continue cache retain the native Jet clear.
An early assertion inspected animation-event+46 instead of pose+17; the native
run was already completing successfully. A pose savestate also reloads and
finishes using the same normal confirm input as the initial completion.

The paired flag is reset on campaign/stage changes, included in deterministic
digest and serialized in X4P7 snapshots. Older X4P6 host states are rejected;
normal campaign cards are unchanged. Both-roster/negative eligibility, shared
termination and snapshot roundtrip checks pass, as do all34 Windows title tests.
Private evidence: feedback21-metadata6-20261015/{victory-x-7,
native-victory-restored-confirm,victory-zero-final} under build-pr23.

## Metadata6: Storm Owl wall qualification (2026-10-14)

Item89 / beads-eio.9.121 remains open. The full original recording reaches
Storm Owl; a captured native checkpoint resumes its last10,572 input/state
checks successfully, retaining all original expectations. In the boss room,
left movement reaches the authored solid wall at X2154/2155 and cameraX1994/1995,
exposing large wall panels across the view. Target camera min/max remain0/2240.
The same pan occurs with P1 alone after P2 is made unavailable; its camera
filters then pass through the original native routines. This narrows the
problem beyond a P2-specific camera selection, but does not prove the admission
bounds match an untouched solo run.

No tile/collision or speculative room clamp is changed. The cause of the
uncommitted arena minimum still needs investigation. Attempts to replay a
forced checkpoint reload with the old private harness were not successful
qualifications and are not counted as a new gameplay defect. Private evidence:
feedback21-metadata6-20261015/{storm-tail-replay,storm-arena-native,
storm-arena-left,storm-solo-native-camera}. Playtest22 does not claim this fix.

### Original solo checkpoint comparison (2026-10-16)

A bounded hidden, muted comparison uses the current Playtest28 gameplay source
and the original stage7/section1/checkpoint3 spawn list. One private process
explicitly disables co-op; the other enables local Unified co-op and Both-player
doors. Both enter from the native checkpoint at2640/512 and settle atY683.
No actor phase, terrain, or camera bound is written by the test. Left controller
input activates the original kind60 introduction, with native confirmation for
dialogue. Both runs finish the introduction, restore normal movement, and lock
the current and target horizontal camera limits to2240. Continued left input
stops the player atX2248 rather than reaching the recordedX2154/2155 wall.
Inspected composed images retain the same authored arena scenery in both runs.
The co-op setup uses invulnerability after loading; the solo run retains native
damage and ends with20HP. The tests use private cards/settings and stop their
owned processes normally.

This establishes that checkpoint3's current admission works, not that the older
full-stage route has been fixed. The recorded0/2240 limits differ from both fresh
entries; qualify the transition from the lower encounter/elevator before changing
bounds. Item89 remains open. Private receipts:
`build-pr23/storm29-{original-solo,current-coop}/report.json` and their
`native-solo-end.png` captures. No gameplay source change or additional release
is required by this comparison.

### Storm Owl arena admission fix (2026-10-16)

Item89 is now reproduced and fixed on current source. Native stage7/section1
camera area3 at8010AC70 covers X2640..2656 and supplies minimum2240; adjacent
area4 at8010AC7C covers X2672..2688 and releases that minimum to0 for ordinary
backtracking. A trailing player can activate the release after the leader
crosses the entry. The boss introduction then transports that partner past
the entry trigger, leaving the live fight at0..2240. A private original-disc
checkpoint3 run reproduces those wrong limits with P1 at2640/683 and P2 placed
at2680/683, without writing actor phases, terrain or camera fields. Normal
Left input and native dialogue confirmation start the actual kind60 fight.
This explains the retained Metadata6 wall state without inventing extra tiles.

At the original750FC admission call, recommit the authored target minimum2240
for this stage/section and command only. Keep the current camera origin and
native limit movement; other admissions, boss rush and ordinary backtracking
remain unchanged. No persistent state or extra native stage update is added.
The development checkpoint whitelist also admits the four verified original
Storm Owl section1 entries, allowing synchronized setup on both peers.

The old source fails the new ownership-boundary regression. The fix passes
both campaigns, Unified/Split, both owners and either survivor, plus eight
negative contexts. All34 Windows checks and the Linux lifecycle check pass.
In local Unified/Both, the same native rear-release setup still produces0
before admission, then locks the arena at2240; continued Left input brings
both players toX2248 with32HP, clear locks and normal controls. In a hidden,
muted two-peer Split/Both match, both leading seats finish the real introduction
and partner return. The views start with independent rear/entry minima0/2240,
then retain current/target arena bounds2240..2240 during movement. Players
remain visible and32HP nearX2271..2275, and inspected composed views retain
the authored arena. Core/mod checks match through6390 for P1 leading and8938
for P2 leading. These native checks use the X campaign; the reverse campaign
has boundary-test coverage. Owned processes are stopped normally.

Private evidence: `build-pr23/storm29-rear-partner-{before,after}`,
`build-pr23/storm29-split-session/owl-arena-result.json`, the composed PNGs,
`build-pr23/storm29-unit-{before,after}.log`, `storm29-windows-tests.log`,
`storm29-plugin-audit.log`, and `storm29-linux-test.log`. Stalled ladder
navigation and an initial Split setup assertion that incorrectly expected
both cameras to share a minimum are excluded. Tracking: `beads-eio.9.121`.
This fix is newer than Playtest28 and has not yet been packaged.
