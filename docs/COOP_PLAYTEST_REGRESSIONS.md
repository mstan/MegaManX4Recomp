# Co-op playtest regression queue

Updated 2026-10-10. The owner says all new reports below are secondhand.
Treat them as reports to reproduce, rather than confirmed causes or passes.
Builds and gameplay runs are serial; two owner-authorized read-only reviews
cover Cyber Peacock routes and save ownership. Reproduction uses hidden runs,
debug stage skips, teleporting and invulnerability where useful. Existing user
windows, settings and saves are preserved. A checkmark means the requested
outcome has been validated, not merely that a patch compiles.

## Previously validated and shipped in playtest 2

1. [x] Windows/Linux lobby and two local controller selections (9.18).
2. [x] P2 incoming damage after pause in the earlier owner playtest (9.22).
3. [x] P2 death particles originate at P2 and use the character's color (9.24).
4. [x] P2 character voice sample ownership (9.23). Overlapping channels are #12.
5. [x] P2 HUD anchoring matches P1 (9.19).
6. [x] Stacked default HUD with side-by-side option (9.21).
7. [x] P2 is hidden on the Quit Game screen (9.20). Quit authority is #15.

## New regression work

8. [x] P2 pause/resume corrupts graphics and disables attacks; later report
   says P2 opens P1's menu and P1 must resume it (`beads-eio.9.26`). Canonical
   world drawing and palette refresh handling are patched; ownership tests
   pass. Native menus/resume were checked in both rosters; X/P2 fires full
   charged shots after either seat pauses. Native Zero/P2 saber damage also
   passes after P2 resumes its own menu (enemy HP16 to HP11). X/P2 native
   projectile damage also passes (HP16 to HP15). Unreleased.
9. [x] X/P2 charge glow and charged projectile; P2 dash afterimage/glitch
   (`beads-eio.9.27`). Native charge actors and trail update/draw were missing
   from private context. A native reproduction also found the shared delayed
   charge emitter running as P1 and exhausting P2's shot counter. The emitter
   now uses allocation ownership. Native glow/full projectile and counter
   recovery pass before/after either seat pauses. Both rosters produce three
   private dash trails, with native screenshots checked. Unreleased.
10. [x] P2 Web Spider water effects (`beads-eio.9.28`). Native water callbacks
    now use allocation ownership; focused ownership tests pass. Exact visual
    comparison passes in both rosters: each seat has visible native splashes
    at its own X +/-8 and native floor height. Screenshots checked. Unreleased.
11. [x] P2 intro boss door softlocks (`beads-eio.9.10`). Unified actor updates
    now honor script ownership. Native P2-first intro arena approach releases
    into the boss fight in both rosters; native-burn-42 repeats the P2-first
    route on the current build. The intro uses a scripted arena entrance;
    its original placement lists have no separate C4 boss door. Unreleased.
12. [x] Jump/hurt sound from one seat interrupts the other (`beads-eio.9.29`,
    framework `beads-eio.3.323`). Private P2 voices and scoped native driver
    queues are implemented. Actual SPU mixing, independent stop/reuse and
    snapshot replay tests pass. Isolated original MIPS VAB/key-on calls pass
    for both characters. Native staggered jumps use logical channel20 on
    independent voices20/44: P2 starts before P1 ends naturally, with no P1
    KEYOFF. Native 44.1kHz PCM captures retained. Unreleased.
13. [x] P2 does not trigger enemies (`beads-eio.9.30`). Existing nearest-player
    native actor context now covers Unified; ownership tests pass in both
    camera modes. Native-burn-29 checks the authored Jungle enemy at X704:
    P2 alone triggers its native attack while P1 stays X500, outside the
    144-pixel threshold. The swapped-seat control also passes. Unreleased.
14. [x] P2 Web Spider secret-area camera (`beads-eio.9.31`). The owner narrowed
    the trigger to P2 dropping into the hole first. Camera-area and
    checkpoint triggers now consider P2 in Unified. The region ownership test
    passes. Native P2-first drop opens area5 while P1 stays outside its box;
    P2-first area6 entry locks the secret-room camera. Both alive, screenshots
    checked. Unreleased.
15. [x] P2 Quit Game promotes its character into P1 when continuing with
    current data (`beads-eio.9.32`). Select-to-Quit is blocked for P2's menu;
    both-roster ownership tests and native Select/Start menu checks pass.
16. [x] Jet Stingray Ready appears twice when P2 participates (`beads-eio.9.33`).
    P2 bike reuses the native READY actor. Two hidden native runs show one
    actor, the same wait pointer on both bikes, and normal riding. Unreleased.
17. [x] Jet Stingray boss spawns at the wrong location after a boss door (`beads-eio.9.34`).
    Both leaders in both campaigns now place the initialized boss in the same
    native room and return both seats alive at native floor height. The native
    entrance flies to a camera-relative position. Screenshots checked. Unreleased.
18. [x] P2 death/lives state across Jet Stingray's two sections (`beads-eio.9.35`). The owner
    cannot confirm whether P2 revived or only the HUD still showed 2 lives.
    Current retry lives are shared and consumed on team wipe; reproduce
    corpse carry before changing that policy. Native section-transfer tests
    now preserve P2 HP0/state3 and hide its new bike; P1 remains healthy.
    Native-burn-42 repeats corpse/vehicle carry on the current build. The
    counter intentionally remains shared team retries, not personal lives.
19. [x] P2 returns after P1's victory departure, plays victory again and leaves
    (`beads-eio.9.36`). Passenger return now waits for ordinary grounded play,
    with no fade/termination result. The voluntary rejoin path now has the
    same normal-play/control gates. Native P2/Zero and P1/Zero victories finish
    with zero passenger returns during victory; native Save and stage selection
    also complete. Unreleased.
20. [x] Boss completion lags by one boss, then grey portraits revert to color
    (`beads-eio.9.37`). Owner clarified one P1-owned campaign and shared
    persistent upgrades. Removed private permanent inventory; native record
    writers run canonically and reward refreshes Continue-current-data.
    Focused bookkeeping tests pass. Native Zero/P1 victory, immediate Continue
    cache, original Save UI record and grey Jet portrait all agree on clear
    bit16; X/P1 victory/cache also pass. Native-burn-25 cold original-card
    Continue retains Zero/P1, clear16 and grey Jet. Independent audit found
    upgrade sync targeting transient A6 instead of native B8; corrected with
    regressions. Native-burn-41 adds Web Spider via X/P2 attacks: clears16 to17,
    immediate cache/card/both grey portraits agree. Native-burn-43 cold reload
    retains17 and both grey portraits. Native stage-select clearing also used
    to carry a false P1 corpse into fresh same-stage entry; its carry reset now
    passes native-burn-41 with both alive and17 retained. Unreleased.
21. [x] Cyber Peacock portal passenger returns before room changes (#11).
    Native-burn-42 traverses authored controller38 handoffs: P2 hub1 to
    checkpoint2, P1 hub5 to section1. No passenger arrival occurs before
    the native room changes; both return alive/grounded at its floor height.
    The same runs qualify healthy outgoing-owner state3 carry. Unreleased.
22. [x] Cyber Peacock boss-door passenger appears above room and dies (#11).
    Native outer/inner doors complete with either seat leading; both return
    active and alive at Y2507/2506, the characters' native floor heights.
23. [x] Cyber Peacock missiles do not target P2 (`beads-eio.9.39`). Native
    missiles home toward surviving P2, and toward P2 while moving away from
    living P1 on the opposite side of the arena. Both-live test retains 25
    missile samples with seven distinguishing steering samples. Unreleased.
24. [x] Cyber Peacock yellow-orb hit followed by despawn/respawn (#11).
    Voluntary withdrawal now respects personal control locks, and rejoin
    respects ordinary-play/scene/fade gates. Native-burn-38 captures Zero/P2
    in the original orb (BA capture and personal67 lock, orb phase3). Holding
    Select for 1.7 seconds while captured produces zero departures. Unreleased.
25. [x] Safe incoming spawn position, including IMG_0685.mov (#11). Removed
    the unconditional 96-pixel upward body relocation. Prefer the owner's
    native landing height and set P2's player/bike facing right on enrollment
    and incoming handoffs. Both fresh rosters, Jet Stingray bike enrollment,
    and Cyber Peacock's outer/inner handoffs passed the native height checks.
26. [x] Default-on diagnostic mod with both seats' input display, starting
    memory-card captures and replay trace (`beads-eio.9.38`, framework
    `beads-eio.3.324`). Display/recording/card capture implemented; earlier
    native offline replay matched 5,131 input records. The final standalone
    helper now passes with 4,964 current-build input records and private starting
    cards/mod choices. Current regenerated build passes native-replay-final-44
    using the cold combined-clear17 card load and original starting cards/mod
    choices. Native-burn-42 host bitmap checks accepted inputs and idle release.
    Fresh installs also capture default mod choices. Projected callbacks label
    canonical P1 correctly. This is input/state-subset replay, not a full
    GPU/SPU machine replay. Unreleased.

New details under #8/#9 are additions to their existing issues. All new reports
remain open until their requested gameplay outcome has been validated.

## Current validation boundaries

The first Windows candidate contains #8/#9/#10 changes only. The current hidden
developer build includes the later audio, ownership, save and diagnostic work.
All 16 title CTests pass. Native screenshots show X/P2's full projectile after
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
