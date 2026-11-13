# Playtest36 burndown: implementation evidence

## Original local scaling controls

An instrumented executable was built from unchanged36 gameplay sources with
`PSX_DEBUG_TOOLS=ON`, `MMX4_COOP_NETPLAY_DEBUG=OFF`. It retains the production
execution identity and co-op compatibility prefix/hash. Original diagnostic
header, mod-plan, input and recorded state checks remain enabled.

These results qualify inspection of the supplied local encounters. They do
**not** reproduce the owner's reported ON-worse-than-OFF contrast, and are not
acceptance of157. Boss HP is not part of the diagnostic player-state hash;
separate native write traces and captures establish the observations above.
The moving encounters diverge after hits, so aggregate hit totals are not a
fair gameplay-equivalence metric. Do not replace the missing contrast with
those totals.

Rendered contact sheets and full captures were inspected. Later attack
presses also occur during hurt states or while the existing aerial attack is
still running. Such presses are not automatically fresh connected blades.
A specific missed-attack timestamp was requested asynchronously; other
burndown work continues independently.

## Native independent-cooldown expiry gap

A new near-simultaneous local native test exposes a distinct pipeline defect.
With scaling/separate mode active, a native X buster hit at backend frame5121
installs the shared rejection table `801060F0` with countdown1. At5122 Zero's
fresh class24 blade reaches that rejection table and receives reactionFF,
although Zero's private timer is ready. The native caller expires its shared
timer afterward; the blade finally damages at5123. The earlier twelve-frame
overlap checks could not establish correctness at this boundary.

The candidate caches the actual table used by an accepted hit and the inspected
SB/SH post-hit timer field. Only the pending native one-tick expiry window with
the exact common rejection table is eligible for projection. A ready seat's
native contact sees the cached table and zero shared countdown, so its accepted
result reaches the original boss caller for one ordinary reaction. A no-hit or
duplicate restores the original table/countdown for native expiry cleanup.
Completed windows are disarmed; unrelated tables and real invulnerable phases
remain native. Global Sigma timer stores are not projected as actor fields.

The cache uses fixed-width serialized/digested guest values, is cleared on
activation/slot reuse, and rolls back for Cyber's discarded damage. Snapshot
format becomes X4PA. The existing health scale, timer durations and blade
lifetime policy have not been represented as resolved by this correction.

| Check | Evidence |
|---|---|
| Counterexample regression | Disabling table projection fails the ready-partner admission assertion; candidate passes both owner orders, no-hit cleanup, genuine phase, later phase, slot reuse and snapshot restoration. GCC builds without warnings. |
| Native local candidate | Adjacent X/Zero hits now register on consecutive backend frames5480/5481; two projected admissions in the bounded trial. The inspected native reaction uses the normal table rather than FF. |
| Real two-peer Split, X/P1 and Zero/P2 | Both peers record the projected ready-partner hit; actual combat core/mod checkpoints match through5924, both heroes remain healthy and no local-render state leak is reported. Captures retained. |
| Real two-peer Split, Zero/P1 and X/P2 | Reversed roster also exercises ready-partner projection on both peers and matches actual combat core/mod checkpoints through5667. |
| Linux portable unit | The same regression/counterexamples compile with GCC and pass under WSL Ubuntu; no cross-OS match compatibility is inferred from compilation alone. |
| Initial LAN geometry control | X20px from the boss did not exercise the one-tick window. It is excluded as coverage; X28px adds the native bullet travel tick and exercises the boundary. |
| Windows title suite |34 of35 pass in the initial run. The detached-pointer-unavailable logging fixture hits its10s child deadline during heavy evidence copying; its isolated rerun passes. Final release validation still needs a complete clean suite. |

Remaining slash cases, weaknesses, and broader burndown validation are still
pending. This correction alone does not close157.

## Evidence locations

Private source scripts and intake: `build-pr23/followup36/` and
`build-pr23/observe_local36.py`, `analyze_observed36.py`, `compare_observed36.py`,
`fast_overlap37.py`, `overlap37_lan.py`, `balance37_counterexample.py`.

Durable initial archive: `build-pr23/followup36/qualification37/`,623 files,
1,554,030,205 bytes verified against source hashes. It contains original local
observations/captures and the initial counterexample. Later native overlap
and LAN receipts originated under
`C:/Users/Matthew/AppData/Local/Temp/mmx4-feedback37/`; the incremental manifests
below now retain those receipts in the qualification directory. The local
candidate has its own `overlap-candidate-manifest.json`. Failed/insufficient
setups remain distinguishable from passing evidence.

## Storm Owl leading-player exit and interrupted death

A real two-peer Split36 baseline uses native dash-jump input from the observed
safe final ledge6240/426. It does not teleport past the native6352 exit test.
With P1 alive at the start, P2 crosses6376 without a forward fade even after
five world updates. With P1 killed immediately before the crossing, P2 reaches
6352 and finishes its healthy outgoing beam, but section0 remains stalled for
240+ netplay ticks. P1 is HP0/state1/action17: the real shared fade suspends the
unfinished death animation. Both baseline peers remain synchronized.

Two separate native predicates need correction. Owl's background may inspect
the farther living P2 X coordinate while retaining its original threshold and
one shared update. At stage_normal's original80020018 branch, a positive
forward-transition flag and a healthy completed P2 beam can complete the exit
when P1 is dead. This changes only the loaded predicate register, neither
body nor campaign data. Negative retry flags, temporary hidden passengers,
voluntary withdrawal, other modes and live P1 retain their original behavior.
The instruction site is emitted from both development and release configs.

Qualification currently includes:

- Local Unified, X/P1 and Zero/P2: leading-P2 and newly dying-P1 cases reach
  section1; both heroes return active, visible, healthy and controllable with
  the same lives. The new `storm_exit_race` scenario avoids the older test's
  wait for P1 to finish dying, which concealed this boundary.
- Real two-peer Split with both X/P1-Zero/P2 and Zero/P1-X/P2 rosters: both cases
  reach section1 and restore32HP, visible controllable bodies and unchanged
  lives. Matching actual core/mod checks through6400 and6113 respectively.
  Captures from both seats were inspected.
- Lifecycle regressions cover both campaigns, the incomplete P1 death, genuine
  forward completion and rejection of no fade, retry, live P1, dead P2,
  unfinished P2, withdrawn/hidden P2, render scope, other modes and inactive
  co-op. Existing passenger/catch-up and native-owner departure cases pass.
- Combat, balance and lifecycle CTests pass; regeneration emits the guarded
  native branch callback. A complete release suite remains pending.

Receipts are under the private `mmx4-feedback37` root in
`owl-exit-confirm-before`, `owl-exit-after-valid-disc`,
`owl-exit-after-zero-p1` and `owl-local-after`. The initial walk-only setup
fell normally into a gap before reaching the trigger and is excluded.
An initial candidate launch misspelled the private disc filename and exited
before boot; it is a setup failure, not gameplay evidence.

This qualifies checklist25. It does not establish that Owl's separate
post-miniboss and Buster-upgrade/death reports166/167 share the same cause.

An incremental verified archive now adds115 files (57,194,007 bytes), covering
Owl baseline/candidate receipts, recent overlapping-hit LAN receipts and the
initial Cyber portal investigation. See `incremental-owl-cyber-manifest.json`
under the qualification directory. The full Cyber qualification adds176 files,
77,745,601 bytes and22 executable hashes in `cyber-qualification-manifest.json`,
including excluded/failed setups as well as the qualified native receipts.

## Cyber timed-course portal (investigation in progress)

The old hub-only tests pass but do not cover the reported timed-course portal.
Native Split reproduction starts P1 at72/203 and moves P2 from the observed
last platform1712/154 to1792 with ordinary Right input. P2 enters command20;
P1 completes its temporary outgoing passenger beam. Both peers stay in sync,
but the course actor (kind38,80143028) remains in phase4. Its wait timer wraps
because +0x16 is never signaled. Captures match the supplied portal scene.

The original targeting graphic is particle34, native800CE894, linked to this
controller at +0x50. It follows canonical P1. At the P2 portal handoff it
reaches native camera cull8002B160 from800CEA8C, disappears, and never reaches
its ordinary completion write800CEF6C. Trace/captures establish this; no timer
or completion flag was manually forced.

An initial owner-body projection alone still allows that cull. A scoped camera
copy is also insufficient in the native test. Neither trial is accepted as a
fix. Temporary guard instrumentation confirmed correct P2 ownership and has
been removed. The current candidate keeps the one native assessment update
with the actual P2 scene owner and protects its completion producer from that
specific bounds-based deletion during projection. The original grading,
animations, timers, completion write and final self-deletion remain native.
The protection is delegated inside Split's already registered bounds handler,
before its ordinary projection/availability gate. A separate registration was
rejected by the runtime's one-handler-per-plugin/address contract. Unit stubs
now reject duplicate addresses and test this pre-availability delegation.
A literal/macro source audit finds no remaining same-plugin function-hook
collisions; different plugin IDs may intentionally share an address.

Actual native before/after now qualifies the full assessment and return with
both Split rosters. With X/P1-Zero/P2, the original S-rank result renders and
checkpoint1 loads with both heroes visible32HP/idle, no personal locks and
matching core/mod checks through4578. Reversing the roster also completes,
with matching peers through4323, and subsequent ordinary input moves both
heroes from72/96 to87.5/111.5 with32HP. Captures are retained. These are real
native grading, checkpoint loading and movement, not forced completion flags.
The expanded bounded harness covers the subsequent native100-frame result
animation; one earlier short trial stopped in this active phase5, not another
softlock. Local Unified also qualifies both rosters: the timed P2 assessment completes,
both heroes return healthy/visible and ordinary input moves each after return.
The existing hub1 P2 handoff and hub5 P1 section handoff still pass. The reversed
local roster also repeats both Owl exit-race cases successfully. Checklist165
is qualified by this bounded before/after evidence. The separate all-course P2 targeting-graphic
feature172 remains open regardless of this portal correction.

Local test setup is part of the evidence boundary: an instantaneous P1
teleport could delete the initial targeting graphic before its opening scan
finished. Another setup waited for that scan before crossing its actual native
entry threshold400. Both failed setups are excluded. The tracked native test
now uses observed36 route positions in bounded steps, waits through the opening
scan on its first platform, then uses ordinary P2 Right input across1792.
It does not force a grade, completion flag, checkpoint or warp state.

All35 Windows title CTests pass in one complete run (12.75s), including the
previous detached-pointer logging fixture. Linux portable lifecycle and Split
regressions also pass; the latter reports two existing misleading-indentation
warnings in the fixture. This is not cross-OS match compatibility qualification.
Production-package validation and the rest of the burndown remain pending;
157 is still open and no release has been authorized as validated by these
portal/Owl checks alone.

## Frame-scheduled slash controls and remaining instrumentation gap

The next discovery controls use the debug server's queued, vblank-counted pad
route instead of alternating short TCP presses with diagnostic reads. Basic
Zero/P1 air, single, combo and cancel trials complete with scaling OFF and
Separate ON. OFF admits 2/2/3/3 native damage stores; ON admits 5/4/17/6.
Those totals do not reproduce157: Frost walks past the fixed-facing player,
so many later swings are visibly out of range. Initial AI states also differ.
Neither totals nor these controls qualify the owner's contrast.

A sustained upgraded class34 aerial at a safe left-side position also completes
OFF and ON, with one initialized blade and one accepted hit in each. Captures
show the blade and impact; the existing lifetime guard rejects eight later
ON contacts, but the OFF control does not accept a second damage opportunity.
This case therefore does not prove that removing that guard fixes157. The
first two wall setups use unqualified floor coordinates; the second records
both setup deaths. They are explicitly excluded. The successful setup uses Zero1120/X1144 and
retains native boss movement. It still has different initial boss AI timing.

Source/native executable checks narrow the remaining investigation:

- Native801060F0 rejects all inspected Zero types24-28/34/35. Frost's normal
  80107A78 table gives those blades2/2/3/2/2/3/5 raw damage respectively.
  Fractional scaling therefore cannot explain a zero-integer-damage basic
  Frost slash: these particular accepted blade hits have at least two raw HP.
- Native8002DD04 normalizes the child7E duplicate result to zero before the
  co-op wrapper considers the other seat. A private immunity rejection does
  not by itself skip P2 through a nonzero outer result. Do not patch this
  ruled-out hypothesis.
- The existing queued route only drives guest port1. It cannot establish
  frame-scheduled P2 controls merely by passing a port argument it ignores.
  Shared debug-tool task `beads-eio.3.358` adds a validated1/2 selector,
  preserving port1 by default. Source-function tests cover port isolation,
  exact segment boundaries, deferred start, invalid ports, clearing and the
  legacy route on Windows and Linux. Native P2 input qualification is recorded
  below.
  This guest override is not an authoritative netplay host input route.

Private discovery archive: `qualification37/route-investigation/` beneath
`build-pr23/followup36/`, with source-script snapshots and SHA256 manifest.
No gameplay fix, acceptance of157, or release is claimed from these controls.

### Native P2 route qualification

The rebuilt debug executable830e110c46b62404a47075c06f6f8f90086e258f1717798cd76eeecb73194efd
runs the same queued basic-air pattern in fresh scaling-OFF and Separate-ON
Frost encounters, with Zero/P2 and X/P1. Both native diagnostic recordings
contain190 consecutive verified input frames: five identical38-frame cycles
(4 Cross,3 neutral,4 Square,27 neutral). P1 is neutral throughout. This verifies
actual native P2 delivery rather than accepting the new command's response.
Input receipts are `pad-route-verification.json` beside each report.

The OFF encounter admits class27 then class24 (48->44); ON admits five class27
hits, exactly38 backend frames apart (48->43). Native captures show basic green
blades, repeated jumps and impact/white-palette reactions. The OFF hit pause
changes the later player's attack state; not every Square pulse there becomes
a fresh aerial. Initial AI timing also differs. This comparison qualifies the
instrumentation and the bounded ON contacts, **not** the reported failure or
acceptance of157. Source-function checks also run on Linux GCC, without claiming
native Linux controller or cross-OS netplay qualification from those checks.

The next negative control disables only4985's ready-table projection in a
separate local executable, retaining the normal working binary and source.
Its provenance explicitly distinguishes it from an unchanged36 build; it must
not be used as a qualified netplay/replay identity. Repeat the now-verified P2
route to determine whether that correction explains the observed admissions.

The local-only no-projection negative control also completes with five class27
hits,38 backend frames apart (48->43). Its native190-frame P2 input window is
verified against the same pattern with P1 neutral. Thus4985's shared-table
correction does not explain this particular solo aerial control, even though
its independent paired-hit defect was reproduced and fixed separately. This
narrows the next comparison to the remaining upgraded/ground/cancel cases and
the actual reported encounter; it does not resolve157.

Shared input-route implementation: framework commitf2344be7,
[PR686](https://github.com/RetroPortingToolKit/psxrecomp/pull/686), assigned to
TechnicallyComputers. Its base retains the existing X4 framework line; unrelated
framework master changes have not been pulled into this investigation.
Native source/capture/input receipts are retained in the verified private
`qualification37/p2-route-controls.zip` with a separate size/SHA256 receipt.

## Storm Owl post-midboss camera release (166)

The supplied36 paired Owl recording1792318160615607 reaches part2 and ends
with living P1 around2814/971 and P2 around2552/970. The stalled P2's cause
cannot be determined from those input/position rows alone. Fresh native
Split/Both fixtures reproduce it after defeating the original Generaid Core,
kind70. The fully retired boss leaves canonical/P1 maximum X=2560, but P2's
saved current/target maximum remains2240. Holding Right clamps Zero/P2 at
2552.5 with32HP and no C0/C4 lock, while P1 proceeds to2833. Both peers still
agree. Captures show P2 pinned to the old view's right edge.

Native death handler800891C8 writes2560 to current maxX at891FC and target
maxX at89204, before continuing the original tile/death finalization. Mirror
exactly those two writes into saved views whose target interval is still the
2240 enclosure. The hook checks the actual kind70 death, zero HP, native2560
value, stage7/section1, live simulation scope and actor-pool alignment. It also
works during a legitimate P2-targeted actor update. Other camera fields,
origins and unrelated room intervals remain unchanged; native AI, death and
map edits still run once. No new persistent snapshot fields are introduced.
Both configs declare the checked A42219D4 instruction; main generated shard39
was regenerated. This fixes166, not the separate capsule/death report167.

| Check | Evidence |
|---|---|
| Counterexample | A unit models the two ordinary native stores even without the new hook. Original code fails the saved-view-byte assertion; candidate passes15 cases, including projected actor context, out-of-scope rejects and whole-view byte preservation. |
| Portable check | The same Split fixture passes MinGW and Linux GCC. Its two pre-existing misleading-indentation warnings remain; no cross-OS gameplay compatibility is inferred. |
| Real Split, X/P1 Zero/P2 | Both final-hit owners complete the original death and following movement. Both saved maximums become2560, both heroes stay visible32HP/unlocked, and matching peer state is retained through8197. |
| Real Split, Zero/P1 X/P2 | Both final-hit owners also pass, including a P2-nearest projected death update. Native release counter records both views for each death; matching peer state through7972. |
| Local Unified | The source-owned storm_midboss_exit case passes both finishing owners. Both heroes proceed past2780, reach native checkpoint2, and settle healthy/visible/idle with no personal locks. |
| Windows title suite | All35 tests pass in one14.76s run; build/test logs are owl-midboss37-all-build.log and owl-midboss37-all-tests.log under build-pr23. |

Only setup uses invulnerability, observed safe positions and the existing
synchronized enemy-HP fixture6. Native attacks supply the final hit, and the
original death animation fully retires before the tested exit. The scenario
is available as tools/test_mmx4_coop_regressions.py --cases storm_midboss_exit
for local or an attached NetplayRuntime; private two-peer captures also retain
the distinct saved views and CRC checkpoints.

Excluded attempts remain separate: an initial probe expected the wrong
actor73; a headless present-shot request failed; a network damage-multiplier
setup did not finish combat. Those failures do not count as reproductions or
acceptance. The later baseline explicitly waits for complete corpse retirement
and records both saved camera intervals. The local pre-fix control already
allowed P2 to follow; this was a Split camera release fault.

Private native inputs, observations, screenshots, source-script snapshots,
executable digests and an archive manifest are retained in
build-pr23/followup36/qualification37/owl-midboss-controls.zip with a separate
size/SHA256 receipt. The main checklist now has139 checked/34 unchecked of173.
Item157 and the remaining progression/feature scope remain open; no release
is qualified by this correction alone.

## Further native slash controls: no acceptance of157

The development checkpoint whitelist now admits all four verified Frost part2
spawns. Its pointer list occupies800F417C..800F4188; the next section's list
starts800F418C. Checkpoints2/3 point to800F3674/800F3698. The original stage's
boss checkpoint3 now loads through the ordinary initializer, with both heroes
healthy. This is development setup support, not a gameplay/slash correction.
Actual combat in both the original stage and fortress uses live kind57; a
probe that waited for kind56 in the original stage never began its trial.

| Control | Observation | Limit |
|---|---|---|
| Unified, P2 upgraded ground-to-jump cancels | Ten native blade initializations in each fresh fight. Scaling off accepts3 damage events; current Separate accepts10. Captures show ground and spinning blades. | Does not reproduce scaling-on being worse. Initial AI substates differ. |
| Unified, sustained/double-jump schedule | OFF has1 initialized spinning blade/1 hit; current ON has2/2. | Native hit pause changes the later attack state despite the same scheduled buttons. This is not a matched blade-lifetime experiment. |
| Private local-only removal of the lifetime veto | With the existing four-tick recovery unchanged, the cancel schedule admits30 hits from10 initializations; the double-jump schedule admits13 from2. | Demonstrates that deleting this guard alone changes repeated damage and aerial hit pause substantially. Does not establish the owner's failure or qualify a fix. Never used in netplay. |
| Actual two-peer Split, X/P1 Zero/P2 | Six requested jump/slash cycles per skill set. Current Separate accepts6 basic and6 upgraded hits, both heroes healthy. OFF has3 and4 health-decreasing cycles. Native traces include27/basic and34/spinning blades. | Authoritative host inputs and synchronized setup; wait-tick/capture overhead remains. Not an exact source replay. The unfiltered write dump limit also omits later individual stores, so use per-cycle health/counters rather than claiming a complete store trace. |
| Original Frost stage, local Unified | OFF accepts27 then24 (48→44); current ON accepts three27 aerials (48→45). The inspected video shows Frost moving out of the later fixed-facing swings. | Additional attacks/initializations are not proof of additional overlaps. Again no ON-worse failure. |

The real Split peers retain matching core/mod checkpoints through9539 with
scaling enabled and9475 disabled. Both option states are verified by the native
scaling activation counter. Guest input_route was not used to bypass network
input ownership. No boss positions, timers, hit tables or attack outcomes were
written during these combat trials.

Rejected setup attempts remain excluded: the old whitelist rejected original
checkpoint3; the kind56 predicate never found the live fighter; an exact
ground-Y predicate missed the one-pixel character-root difference; several
setups requested a teleport before P1's post-intro arrival was complete. The
final local setup waits for both healthy, visible, idle heroes after the actual
fight starts. These are not reported as game reproductions. A second private
half-native-timer variant also stopped in setup and supplies no gameplay result.

The idle Windows suite passes all35 checks in2.20s. An earlier concurrent run
had a detached logging child exceed its10s fixture deadline (34/35); it is
preserved alongside the clean idle run. The source change here only broadens
the verified development spawn list.

Private receipts, inputs, captures, script snapshots and variant provenance
are retained in qualification37/slash-controls-continued.zip with a separate
size/SHA256 receipt. The silent6.87s comparison video is
build-pr23/slash-comparison37.mp4: it explicitly labels fresh fights with
different boss movement, not a deterministic matched-world comparison.
Item157 remains unresolved; checklist139/34 is unchanged. No release is cut.

## Cyber course analysis graphic for P2 (172)

The supplied magenta brackets and cyan readouts belong to the course's native
particle34, not Cyber's missile attack. Native800CE894 also advances assessment
timers, grading and the completion signal to its parent kind38. A pre-change
original-disc control shows only P1's graphic and fails the new indicator case.
Its native assessment runs once per observed frame; simply running it twice
would alter gameplay.

The new read-only helper copies the96-byte particle for presentation, retaining
its native scan offset, previous position and animation. It translates current
and previous coordinates by the player-position difference, selects P2's world
layer, and uses the same native80024334 draw path. A separate temporary draw
address gives the copy its own interpolation identity. All temporary bytes are
restored before leaving the draw scope. The original assessment, parent link,
timers, grade producer and native actor pool remain single instances.

Only Cyber part1's live course graphic in opening/free-course phases qualifies.
Story ownership, a hidden/dead partner, projected actor updates, unrelated
particles, screen-relative layers and completion/flyaway phases retain the
existing behavior. The new scratch region is beyond the existing diagnostic
shot dump; no diagnostic bytes are borrowed. The source is included in the
strict co-op compatibility identity.

| Check | Evidence |
|---|---|
| Local Unified | Both character assignments show both indicators in inspected native captures, then complete the original P2-led course portal with both heroes healthy, visible and able to move. Opening and free-course drawing both occur. |
| Actual Split, X/P1 Zero/P2 | Both nearby indicators render; P2's own presented view retains its indicator384px ahead of P1. Only one native particle34 exists. Both heroes complete the course and move normally; matching core/mod state through9057. |
| Actual Split, Zero/P1 X/P2 | The reversed assignment also retains P2's graphic384px ahead, completes the course and preserves both heroes; matching core/mod state through8611. |
| No extra gameplay update | Free-course traces show exactly one800CE894 update in each of44/104 local and39/33 Split observed frames. Temporary draw bytes restore exactly, and neither Split trial reports a local-view state leak. |
| Portable/source checks |22 helper cases cover translation, all-byte state preservation and scope exclusions. MinGW and Linux GCC pass; Linux builds with Wall/Wextra/Werror. All36 Windows title checks pass in6.85s. |

The source-owned cyber_indicator regression uses synchronized setup fixtures,
native assessment entry and read-only function observation. It does not mutate
grading or completion state. Grade/checkpoint results may differ between fresh
setups; no claim of identical course timing is made. Private captures, input
receipts, script/source snapshots and executable digests are retained in the
verified qualification37/cyber-indicator-controls.zip and its SHA256 receipt.
No original game assets or executable bytes are included in that archive.

Item172 is qualified. The numbered checklist is140 checked/33 unchecked of173.
This feature does not resolve157 or qualify a release by itself.

## Intro falling-platform admission (168)

The supplied paired Intro recording1792323436112052 leaves P1 near1671 while
P2 traverses part2. A fresh native Split run reproduces suspended General1
platforms5..10 while P2 advances beyond2500 and living P1 stays1671. That
walking-only observation ends at an ordinary terrain obstacle; it is not
proof of the later boss-entry softlock2.

The bounded native counterexample walks both heroes from original checkpoint1
along the raised floor, stops one below the trigger and advances the other.
P1 crossing2000 moves subtype5's Y from288 to408.83. Repeating with P1 stopped
at1881.5 and healthy P2 crossing2033 leaves that same platform at288/phase0.
Both heroes remain32HP. This establishes the P2 trigger exclusion without
placing either body beyond the relevant terrain or writing any platform state.

Native BEED4 loads only PLAYER.x atBEEF0, compares it with the subtype's
authored threshold at8010C164, then starts one original fall/map sequence.
The checked BEEF4 instruction hook extends only that loaded X to a farther
living P2, or to the surviving P2 when P1 is dead. It checks Intro part2,
the actual aligned General pool, live type1/subtype0..13 and the waiting phase.
Native threshold comparison, falling movement, tile edits, effects and sound
still execute once. It adds no persistent state and applies by default.
Both source configs declare the exact00041100 instruction; generated shard57
was regenerated from the original executable.

| Check | Evidence |
|---|---|
| Local Unified | Both rosters pass with each seat leading. Subtype5 leaves288 and moves through its native falling phase; both heroes remain healthy/visible. |
| Actual Split, X/P1 Zero/P2 | Both leaders trigger the original fall, no local-view state leak, matching peer core/mod state through6628. |
| Actual Split, Zero/P1 X/P2 | Both leaders also pass with the reversed assignment; matching peers through6212. Presented captures show the changed platform/floor and living P2. |
| Scope/counterexamples |16 new lifecycle cases cover leading P2, a P2 survivor despite a farther stale P1 coordinate, P1 already ahead, and exclusions for dead P2, projections, presentation, stage/section, pool alignment, type/subtype and phase. Platform bytes and the other CPU result register remain unchanged by the predicate hook. |
| Regression checks | MinGW lifecycle and Linux GCC lifecycle pass; the Linux fixture's existing missing-field-initializer warning remains. All36 Windows title checks pass in13.00s. |

Excluded setups remain recorded: two fixtures used the lower corridor's443
root in this raised floor and died before the crossing; another airborne
placement did not settle. The qualified scenario uses native walking from
the authored checkpoint instead. No excluded attempt is counted as a game
reproduction or acceptance. The source-owned case is intro_platform_leader.

Private native controls, input receipts, captures, exact source/script
snapshots, generated-hook provenance and executable digests are retained in
the verified qualification37/intro-platform-controls.zip and SHA256 receipt.
Item168 is qualified;141 checked/32 unchecked of173. Intro boss approach2,
scaling157 and the remaining release scope remain open.

## Saved-fight scaling controls: preparation, not acceptance

Fresh OFF/ON fights have repeatedly diverged in native boss movement, even
with the same requested buttons. A private development fixture6/253 now
selects a reference OFF or Separate ON control without changing public mod
preferences or any native actor bytes. It resets the existing balance
bookkeeping, retains its existing mode in the snapshot/digest, and refuses
when an actor/contact update, projection or local presentation is active.
It requires configured Separate scaling and the explicit development build;
production always refuses. There is no new shipping toggle or cooldown rule.

The native original Frost scenario balance_saved_control saves an active
48HP fight, admits the OFF control, returns to ON and successfully loads the
real saved image. Both heroes remain visible32HP, the same kind57/slot0 boss
retains48HP, and save/load completion generations1/2 report success. Snapshot
validation stays strict: the unit rejects restoring the ON image while the
current mode is OFF, and accepts it after returning to ON. No native HP,
boss AI, position or hit timer is edited by this mode fixture.

Actual Split peers agree after OFF and ON fixtures through3041/3140, with
both heroes healthy and no local-view state leak. Production and development
balance checks pass MinGW and Linux GCC; Linux uses Wall/Wextra/Werror. All36
Windows title checks pass in2.96s. Private source/native/check receipts are
retained in verified qualification37/scaling-mode-controls.zip and its SHA256
receipt; saved images, original assets and executable bytes are excluded.

This only qualifies the comparison setup. It has not yet produced a matched
slash trial or resolved157. TCP acknowledgements and host input-route start
times are asynchronous; the runtime's route frame counter is host history and
is not restored with a game snapshot. Starting a queued route at the actual
load boundary would remove that remaining timing ambiguity. The existing
source-function route tests and native P2 route support remain separate from
authoritative network input replay. Checklist141/32 is unchanged; no release.

## Load-boundary inputs now qualify in both seats

Framework c647d27c adds optional input_route_start after_load=1. The queue
stays dormant until the actual successful native load callback, independently
of fingerprint collection. It then starts at that restored guest boundary.
Stop/clear cancel the waiting route; another port's clear does not. A positive
at_frame cannot combine with after_load, and existing default routing remains.
Source-function checks pass Windows MinGW and Linux GCC, including cancellation,
port isolation, fingerprint independence and ordinary deferred starts.
[PR686](https://github.com/RetroPortingToolKit/psxrecomp/pull/686) remains open,
assigned to TechnicallyComputers, on the existing X4 framework line.

The source-owned balance_saved_slashes case now records OFF/ON controls from
one real original-Frost saved fight. Both use348 identical scheduled native
inputs:60 neutral setup frames, six jump/slash cycles, then a neutral tail.
Zero is separately qualified in both seats. Each pair's first60 recorder rows
also match the restored guest clock, both heroes' observations and the full
diagnostic PLAY/player-state hash. The idle partner is neutral throughout.
Mode fixtures complete inside the neutral prefix; loading still requires
returning to the snapshot's configured ON mode. Only setup moves players or
selects skills. Combat changes come from native controller input.

This removes the earlier fresh-fight/start-time ambiguity, but does not close157.
Both pairs first differ in their full diagnostic player-state hash at row77,
around the first connected spinning attack. Later native motion differs.
The P2 pair records one original damage store OFF and two Separate stores ON;
the P1 pair records one each. Inspected captures show native spinning slashes
and ordinary combat. These controls still do not demonstrate the reported
ON-worse follow-up suppression, and store counts are not gameplay acceptance.
Asynchronous sample.frame/write-trace timestamps are host observations; use
the recorded N rows for the matched guest clock/input basis.

All36 Windows title checks pass in3.54s. Private inputs, captures, native
function/health-write observations, exact executable digests and source/check
snapshots are retained in verified qualification37/saved-slash-controls.zip,
with a separate SHA256 receipt. Saved images and original game assets are
excluded. The comparison returns slash_acceptance=false deliberately.
Checklist141/32 is unchanged; no release is qualified.

Two further bounded controls retain the same348-input/60-setup-row checks.
A deliberately relinked local-only variant removes both the blade-lifetime
veto and four-tick override, retaining half-native private recovery. Its own
saved-fight pair admits two class34/spinning damage stores in each mode:
OFF48 to42, ON48 to45. This supplies the gameplay result missing from the
earlier half-native variant's failed setup. It does not reproduce suppression
or qualify this variant for production/netplay. Exact altered source,
compile/link commands and executable hash remain in its private provenance.

A separate current-code control places idle X initially closer to Frost than
Zero. That saved pair admits two class34 stores OFF and three ON. These are
different saved setups, so do not compare totals between the variant and
current-code experiments as though they shared a world. Neither demonstrates
the reported ON-worse behavior. Private verified native-half-saved-controls.zip
and x-near-saved-controls.zip retain their inputs, observations, captures,
source snapshots and separate SHA256 receipts.157 remains open.

## Survivor-camera controls and a teleport setup correction

A native Web camera-area crossing now passes both with P1 alive and with P1
dead. At the authored Y416..432 region, P2's saved camera bounds change from
5584/5376/1280/256 to6048/5376/256/256. The survivor case also retains the
canonical camera update. Both actual Split peers match through4581/6306.
The projected stage-pool guard alone is therefore insufficient evidence of
a survivor-camera defect; no broad camera-guard change was made.

The first Frost opening-ice setup instead exposed a developer-tool error.
Teleporting idle players to2213/300 and2253/300 clears PLAYER+2C along with
velocity. P1 lands, but P2 stays healthy atY300/action7 for the entire10-second
wait. Native ordinary fall paths32558/32608/326C0 install4200 in that vertical
acceleration field. This failed setup is not a reproduction of169 or170.

The development teleport now preserves nonzero acceleration and supplies that
native fall acceleration only to normal on-foot actions2..15 with a zero
value. Mounted, beam and story movement remain excluded. The source-owned
teleport_fall case includes both Jungle and the affected Frost setup; both
players fall and land through native physics. The old Jungle-only control
already passed before the correction, so it is not presented as a failing
baseline for the Frost problem.

The corrected real Split Frost control reaches the recorded opening-ice death
location: P1 at2213/443, P2 at2253/410. After P1's native death, native P2
movement reaches2742/431, with both saved camera origins following to2582/266,
32HP and matching core/mod checks through8326. Inspected before/after captures
show the surviving player and stage correctly. The corresponding both-alive
control also completes. This bounded route does not reproduce169/170 and does
not qualify the full rising-ice shaft or post-respawn camera report.

All36 Windows title checks pass in6.04s. The helper correction is confined to
the synchronized development fixtures; no new production camera fix or release
acceptance is claimed.157 and the remaining progression reports stay open.

## Matched basic-air and grounded follow-up controls

Two additional Zero/P2 controls start each OFF/ON pair from the same native
Frost saved fight. The basic-air pair has576 scheduled input frames; the
grounded pair has588. Both qualify the exact selected-pad sequence, a neutral
partner and60 identical initial recorder rows, including the restored guest
clock and full diagnostic player-state hash. Skills are disabled for these
controls; fixed leftward inputs attempt to retain contact as Frost moves.
Neither uses attack, cooldown, damage, terrain or AI edits during combat.

The basic-air control admits native class27 damage once OFF and twice ON;
both finish at46 displayed HP because ON halves each accepted two-point hit.
The grounded control admits two native damage stores OFF and ten ON. The ON
sequence contains successive24/25/26 blade classes, with the first ten stores
13 write-trace frame ticks apart. Those trace timestamps are host observations;
the matched native controller sequence is qualified separately above.
Captures show the basic aerial arc, grounded
chain poses and connected impacts. Later positioning diverges after native
reactions, and many later attacks are visibly out of range.

## Owner playtest handoff

The owner requested a short manual comparison instead of further unattended
reproduction attempts. A private development copy is prepared under
`build-pr23/manual37-frost`, with a visible Frost Walrus fight, normal system
audio, fresh private saves and a separate reset panel. The existing saved
native fight supplies the baseline; neither this setup nor its ready image
constitutes a new successful slash reproduction.

Zero is P2 with basic skills and normal incoming damage. Idle X is protected
so a second operator is unnecessary. Arrow keys move Zero; Z attacks, X jumps
and C dashes. The reset panel restores the same native image, applies scaling
OFF or ON with independent cooldowns, releases setup input overrides and
pauses for the owner. It restores ON before loading because snapshot
compatibility validation remains strict. The Resume button uses the native
pause input; no removed TCP pause API is required.

The requested comparison is repeated basic jumping slashes and standing
combos in each mode. Look for connected follow-ups being rejected or requiring
extra delay when scaling is ON; reduced integer health-bar movement alone is
expected from doubled durability. Diagnostics and input recording use the
private `saves/diagnostics` directory. `manual-events.jsonl` records reset mode
and wall-clock time, since the persisted mod settings still describe ON while
the development fixture temporarily selects OFF.

The fight and reset panel were verified live and the ready image inspected.
No owner result has been received. Item157 remains unresolved, and no release
is qualified. Further unattended gameplay reproduction is stopped for this
handoff; the owner's active instance must remain untouched.

## Owner playtest result

This closes157 for the current release on bounded human acceptance alongside
the native adjacent-partner correction and recorded control cases. It does
not retroactively prove the historical ON-worse sequence reproduced, qualify
every boss/attack combination, or restore the withdrawn Playtest36 automated
acceptance. Reports2/107/109/167/169-171 remain explicitly unconfirmed.

Framework PR686 merged as32eefd90, with a tree identical to qualifiedc647d27c.
Production Windows and Linux packages must disable both PSX_DEBUG_TOOLS and
MMX4_COOP_NETPLAY_DEBUG; package qualification remains a separate release gate.
