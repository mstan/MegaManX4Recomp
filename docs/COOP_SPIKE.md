# X4 simultaneous X / Zero spike

Work branch: `spike/mmx4-split-netplay`, continuing `spike/mmx4-coop` at
`49336d8`. Tracking: central Beads `beads-eio.9.1` and `beads-eio.9.5`.
The published `spike/mmx4-coop` branch remains the base of the separate HUD PR.

The first milestone uses X6's native player-context projection and retained
graphics approach. X1 informs shared-camera and survivor behavior; its SNES
implementation and split-camera prototype are not transplanted into X4.

## Plan and implementation

1. Verify X4's native player, terrain, projectile, pickup, vehicle, menu and
   stage boundaries against the original SLUS-00561 executable. Keep the
   shared world update at one pass per frame.
2. Add a private counterpart context: X campaign gives P2 Zero; Zero campaign
   gives P2 X. Retain both characters' original disc graphics, use independent
   controller ports, and render both actors and their shots.
3. Route shared collisions through each living player, preserving consumable
   attacks and enemy HP/invulnerability. Separate per-player persistent-hit
   tokens and moving-solid contact history. Keep native vehicles independent.
4. Project each player's HP, weapon/ammo, upgrades and native Start menu.
   Keep story, retries and acquired tank contents shared. Preserve survivors
   and corpses across same-stage sections; respawn the team after a team wipe
   or a new stage. Use one camera for this milestone.
5. Add a trusted `mmx4.coop` profile to standard recomp-net: two players,
   deterministic delay sync, fixed 4:3 OpenGL presentation, CPU-authoritative
   online rendering, and a digest of the mod's simulation state. Disable
   rollback, rewind and save states until host state has snapshot support.
6. Validate original-disc loads in both campaigns, native menus and tanks,
   independent input, deaths in both orders, both Ride Chasers, stage
   transitions and a private two-process network session. Review findings
   adversarially and fix failures before calling the milestone qualified.

## Playing the spike

Build the game with its pinned framework submodule and regenerate from your
original disc after changing `game.toml` hooks. In the launcher Mods page,
enable **X + Zero Co-op (Development)**, select **OpenGL**, and start a fresh
game. The package is disabled by default. Choose either native campaign and
bind the second controller through the normal controller settings.

The prepared Windows playtest build exposes both local controller cards.
Development builds permit explicitly assigning the same physical controller to
both players; normal builds retain exclusive assignments. Automatic controller
allocation still chooses separate devices. The private `Play X4 Co-op.cmd`
opens the launcher and uses `build-coop/playtest-saves`: card 1 holds the X
campaign, card 2 the Zero campaign. Both generated native-format saves have all
eight Mavericks cleared and story progress 5, immediately before the final
stage chain. They retain base HP and do not grant optional armor/hearts/tanks.
`tools/prepare_mmx4_playtest_save.py` recreates them in a new directory from
the owner's original US executable and refuses to overwrite existing cards.

Each player's Start opens that character's original menu. The world pauses;
only the owner controls the open menu. A tank used there heals that owner and
depletes the shared pool once. The native pickups contribute to the same
pool. A heart upgrade belongs to its collector; X owns armor upgrades.
Both characters receive the corresponding defeated-boss ability.

One player's death leaves the survivor playing. A team wipe consumes one
shared retry. While P1 is alive, P2 may hold Select for 90 native ticks
(1.5 seconds at the game's normal rate) to withdraw, then release and tap
Select to rejoin beside P1. Rejoining waits for a safe native landing.
Enrollment and foot teleporting use each character's original beam-in,
landing, departure and beam-out animation. The partner is excluded from
combat during that sequence, and native HP/ammo remain intact.
Select withdrawal is disabled throughout Marine Base's bike sequence and
while mounted. Shared scripted
events take the nonowning living partner out of simulation and return them
after the door and any chained dialogue release control. Fallen or voluntarily
absent partners remain absent.

Online, select the trusted `mmx4.coop` profile. Its compatibility identity is
`mmx4-coop-delay-v3:<source fingerprint>`. CMake hashes the co-op callbacks,
headers, camera/rendering framework contract and hook configuration so incompatible spike builds cannot share
the same profile identity. Online policy enables the required plugin, fixes the
supported renderer/aspect, and excludes offline mod selections from gameplay.
Both peers need the same build and original disc. **Netplay cameras** offers
**Unified** (default) and **Split**. The room host's choice applies to both
peers without changing their saved offline preference. Offline uses Unified.

The current private candidate can be opened with
`build-coop-split/Play X4 Co-op.ps1`. It uses the latest executable, copied
controller bindings and separate `build-coop-split/playtest-saves` cards.
Select **Split** under **Netplay cameras** for the host's room; the playtest
preference starts at Unified. The helper does not stop an existing game.

## Validation scope

The stage fixture requests the original load/init dispatcher at a native
frame boundary. It is available only in development builds and is rejected
online. It does not simulate a completed campaign. Boot and dialogue
navigation use controller input. Private captures contain original game
imagery and stay under ignored build/cache directories.

| Native stage ID | Original stage | X campaign, section 0 | Zero campaign, section 0 |
| --- | --- | --- | --- |
| 0 | Sky Lagoon | Load + independent P2 input | Load + independent P2 input |
| 1 | Jungle / Web Spider | Pass | Pass |
| 2 | Snow Base / Frost Walrus | Pass | Pass |
| 3 | Bio Lab / Split Mushroom | Pass | Pass |
| 4 | Volcano / Magma Dragoon | Pass | Pass |
| 5 | Marine Base / Jet Stingray | Two Ride Chasers | Two Ride Chasers |
| 6 | Cyber Space / Cyber Peacock | Pass | Pass |
| 7 | Air Force / Storm Owl | Pass | Pass |
| 8 | Military Train / Slash Beast | Pass after safe-spawn fix | Pass |
| 9 | Memorial Hall | Pass | Pass |
| 10 | Final-stage entry | Pass | Pass |
| 11 | Final-stage entry | Pass | Pass |
| 12 | Final-stage entry | Pass | Pass |

Zero campaign stage 12 section 1 also reached the original Sigma conversation.
After native confirmation, both characters remained alive and P2 moved
independently in the encounter. This is encounter access, not a Sigma defeat.

Live checks confirmed separate native menu ownership, world freeze, shared
tank depletion with owner-only healing, withdrawal/rejoin, both death orders,
one retry per team wipe, and short Ride Chaser survivor continuation in both
death orders. Both menu portraits/themes and HP bars were visually reviewed
after correcting native DMA uploads from retained character data.

The focused C tests cover asset bounds/decompression, combat ownership,
invulnerability/token behavior, moving-solid history, vehicle handoff,
inventory, menus, pickup ownership, and section/death bookkeeping. They model
native boundaries; they do not replace the original-disc runs. All 952
compressed X/Zero sprite frames also decoded successfully from private assets.

Original instruction evidence and narrower qualification limits are in
[combat/vehicles](COOP_COMBAT_VEHICLE_EVIDENCE.md) and
[lifecycle/menus](COOP_LIFECYCLE_MENU_EVIDENCE.md).

## Independent HUD layout

P1 keeps the original upper-left HUD. P2's native health/weapon-energy HUD
is stacked 104 native pixels below it, with the sprite and filled-bar packets
moving together. Both use the same left anchor. Split presents the viewed
character's original single HUD. HP and ammo remain independent; unlimited
buster/saber attacks do not require an ammo bar. Native OpenGL captures were
inspected in both X4 campaigns. The isolated change is
[PR #17](https://github.com/mstan/MegaManX4Recomp/pull/17); the matching X6
change is [PR #41](https://github.com/mstan/MegaManX6Recomp/pull/41), with
native 4:3 and 21:9 captures inspected.

## Remaining qualification

This is an experimental spike, not a full-campaign certification. Complete
stage clears, all boss/weapon combinations, moving platforms, Ride Armor
acquisition/destruction, long sessions, and character-specific sound parity
need broader playtesting. Unified retains the inspected aimed-AI targeting;
Split also selects the nearest eligible player for each original enemy/shot
callback, restoring canonical contexts for collision and shared-event calls.
Host-state snapshots remain deferred. Split's bounded native evidence and
remaining limits are recorded below.

The corrected Zero-campaign intro section transfer, later bike section and
Sigma section have passed native load/control checks. The later bike section
now mounts both players. Private delay and jitter pairs passed for both
campaigns on the earlier checkpoint. The resumed spike also validates both
players leading a native Jungle boss door and either sole survivor advancing
the ensuing conversation under their own controller. Latest Select/bike and
netplay reports are recorded in the narrower evidence documents.

## Downloaded owner playtest saves

The owner playtest found that the generated cards omitted the native button
masks at save-record bytes 8 through 39. Native load `8001C210` copied those
zeros into `800EE430`, so movement and attacks were discarded for both players
after loading a save. The generator now retains the original default bindings,
with regression coverage for both campaigns and existing-card preservation.

At the owner's request the playtest uses the existing US GameFAQs save
`mega-man-x4.26805.gme`, fetched from the Internet Archive collection
`game-faqs-saves-2024-09-05`. Its card payload is imported unchanged into
`build-coop-next/online-playtest-saves`, selected by the launcher helper.
Earlier cards remain separate. Native Continue slot 1 is Zero, slot 2 is
Fourth Armor X, and slot 3 is normal X. All have HP 32, all eight boss rewards,
story state 5 before Spaceport, and valid button bindings. Native teleport
visuals and uninterrupted nonlethal saber recovery still need owner playtesting.

## Jungle boundary correction

The Zero-campaign Jungle left edge now applies original `80027BE4` bounds to
the other living player after the shared camera pass. In the reported pairing,
both Zero and P2 X stop at X=8.5 instead of X walking off the map. Separate
Right input still moves each player independently. Duplicate co-op hook
registrations that had silently disabled the camera filter are consolidated
and regression-tested. The live evidence and limits are recorded in
[lifecycle/menus](COOP_LIFECYCLE_MENU_EVIDENCE.md).

## Optional Split netplay milestone

Tracking: central Beads `beads-eio.9.5`. The owner's new request supersedes the
earlier deferral. X1's reference is
`F:/Projects/snesrecomp/MegamanXRecomp/docs/netplay-independent-cameras.md`
and its `mmx_coop_view.c`, native placement rescan, and actor/cull hooks.
X1's actual independent setting is netplay-only; offline remains Unified.

Required contract:

- Add a co-op camera dropdown: **Unified** first/default, **Split** second.
  Match the host's choice on both peers and include it in content agreement.
  Offline remains Unified, matching X1; this is not couch split-screen.
- Split displays only the local controller seat's own view. The simulation
  never reads that local seat to decide which actors exist or get updated.
  Derive and clamp each view to the original stage/room limits; remove the
  mutual camera tether without allowing either player past map boundaries.
- Activate the union of two complete rectangles, not the bounding box spanning
  the gap between distant players. Overlap produces one native enemy, one HP
  value and one update per world frame; separating views does not clone actors.
- Despawn only outside both eligible regions. Track successful placement
  visits through overlapping/separated views so a kill cannot immediately
  respawn during a stationary scan. Leaving both regions restores the native
  re-entry policy. Allocation failure must remain retryable; native collected
  flags and finite actor pool capacities remain authoritative.
- Route inspected native AI queries to the nearest eligible player with a
  stable tie-break. Shared doors, boss conversations, menus and section changes
  keep one event owner. A transported/withdrawn/fallen player watches that
  owner/survivor rather than independently driving a second story script.
- Preserve Unified behavior and the previous boundary/HUD work. Use the
  existing sandboxed local-view service and complete authored-map background
  renderer, not a second world tick or peer-dependent RAM writes.

Implementation sequence:

1. Establish tested camera/rectangle/visit/nearest-player policy. The new
   `mmx4_coop_views.c` helpers use native follow dead zones and placement
   extents. Pure-policy tests pass; they are now used by the private candidate.
2. Extend the trusted netplay profile to carry only its explicitly permitted
   camera option through room agreement/commit. Implemented with a read-only
   canonical settings fingerprint, strict package/feature whitelist, temporary
   host selections and conditional native-aspect renderer activation. Ten
   framework policy/runtime/real lobby-codec tests pass. Recorded native room
   launch adoption/rejection evidence is listed below; a live Internet lobby
   was not exercised.
3. Integrate per-seat camera state, dual-region native placement scans and
   lifetime checks. Keep each native pool and script update once per frame;
   include new shared state in the co-op digest and reset it on stage/retry.
   `mmx4_coop_split.c` now stores independent native camera records and visit
   state in mod memory, runs directional native scans for each eligible view,
   retries ordinary native allocations, and unions complete native lifetime /
   draw predicates. The mutual tether is disabled only in Split. Ten game
   CTests pass, including a deterministic native-call model covering distant
   cameras, parallax records, disjoint XY/gap rejection, allocation failure,
   stationary kill suppression, re-entry, scripted-owner fallback and native
   left bounds, independent room-limit targets and survivor camera ownership.
   This model is not original-executable gameplay evidence.
   Intro searchlight scans now use both view/layer records and union the
   original full-XY origin/centre visibility predicate; pure beam tests pass.
   P2's private controller/projectile/terrain pass uses its own layer origins,
   restoring the shared origins afterwards without discarding native shared
   camera configuration or script changes. Remote shots use the union of both
   views for native lifetime checks. Bounded native separated-play evidence
   is recorded below.
   General nearest-player enemy/shot dispatch is now integrated at the two
   original pool `jalr` instructions. It executes one native callback with
   the selected full player context and keeps each seat's real hit pause.
   Collision/solid/common-event callbacks temporarily suspend that scope to
   preserve the existing two-body ordering, then resume it. Union lifetime
   queries read the real two actors even while P2 occupies the singleton.
   Focused models cover these scopes, per-seat writes/freezes, corpse cases,
   ties and projected XY union. Bounded native distant camera/spawn/cull
   evidence is recorded below; every actor family is not certified.
4. Add sandboxed seat-view drawing at a verified completed-frame boundary,
   reusing original scene/HUD producers and full-map foreground/parallax.
   Connected at the native main-loop OT submit, with deterministic shared
   synchronization before the local port is read. Own-view redraws journal
   RAM/GPU state and restore private player/inventory contexts; immutable
   presentation-only texture banks cannot change canonical bank numbering.
   Actual two-peer intro evidence and inspected OpenGL captures show Zero's
   own view/HUD on seat 0 and X's own view/HUD on seat 1. This is close-player
   evidence, not distant-stage or scripted-handoff qualification.
5. Verify the default is unchanged, then a bounded actual two-peer separated,
   overlapping/rejoining, death and script-handoff run. Inspect each peer's
   pixels, native allocations/despawns and matching simulation digests. Do not
   call Split ready based only on pure policy tests or a manifest dropdown.

### Resumed native qualification, 2026-10-09

The private `build-coop-split` candidate implements both camera choices.
The earlier P2-led Jungle dialogue stall is fixed and validated in eight
offline campaign/owner/survivor combinations (see the lifecycle evidence).
That offline result does not establish online Split boss-door qualification.

`qa-stacked-hud-unified/report.json` passes both campaigns' original-disc
delay pairs, native movement, both menus, disconnect and matching core/mod
CRCs through X 3908 / Zero 3564. Both stacked HUD captures were inspected.
`qa-stacked-hud-split/report.json` passes Zero's own-view presentation,
disjoint intro enemy interests, overlap/rejoin and remote-enemy despawn through
tick 3847. These reports record the preceding `677af0d2...` executable.

`qa-room-independent-bounds-commit/report.json` uses the corrected executable's identity
publisher and native room-launch decoder/profile commit. Host Unified adopts
over guest Split, and host Split adopts over guest Unified; each reaches
native tick-0 admission with unchanged persisted offline preferences. Forged
fingerprints and a correctly hashed undeclared choice are rejected. The
recorded launch messages use reserved loopback SFU endpoints, not an Internet
lobby or relay match. The initial non-SFU replay failure is retained at
`qa-room-launch-commit`.

The added P1-first native death case found a real camera-bounds bug:
`qa-split-jungle-p1-first` shows P1's room-target limits dragging the neutral
P2 across Jungle and killing it. Native `80027AFC`/`80027B70` ease current
limits toward camera targets at `+24..2B`, then `80027BE4` clamps/kills the
player. Independent records now retain both current and target limits
(`+1C..2B`). Shared script/gate records still synchronize fully. On camera
ownership changes, the survivor's saved camera is restored before the native
follow/clamp pass. The native-call clamp regression fails before the fix and
passes afterwards (central bug `beads-eio.9.8`).

The corrected executable SHA256 is
`c12c9b09a1e7857d787b029e511b86f27cb972aaad4feff383e2612a796a9f5e`.
`qa-split-independent-bounds-survivor/report.json` passes both campaigns'
original-card Continue into Jungle, controller-only P1 death, survivor view,
second native death and shared retry. Neutral P2 stays at X=8.5 and HP=32
through P1's entire traversal/death. The team spends exactly one life (2 to 1),
both characters revive and the campaign stays unchanged. CRCs agree through
X 8619 / Zero 9124, including both menus and native disconnect. Survivor and
retry OpenGL captures were inspected. Up to 5567 verified local passes per
peer record zero verifier mismatches, watchdogs, VRAM leaks or disabled passes.
Earlier P2-first runs pass in both campaigns on `677af0d2...`; their reports
are `qa-split-jungle-survivor-climb` and
`qa-split-jungle-survivor-x-plain-controls`.

`qa-split-independent-bounds-p2-timed/report.json` repeats P2-first native
death/retry in both campaigns on the corrected executable, with CRCs matching
through X 9054 / Zero 8768. Both peer menus, survivor view and native disconnect
pass; the fallen peer's survivor captures were inspected. Added debug-request
timing records show menu/presentation screenshots taking at most 47 ms (X) /
31 ms (Zero). The preceding `qa-split-independent-bounds-p2-first` report stays
failed: after matching death/retry CRCs through 8497, both peers exited with
the native peer-disconnect reason during the first menu capture. Its cause is
unresolved; the timed repeat is not a causal fix or a relabeling of that run.

The same corrected candidate passes both intro campaigns under 25 ms simulated
latency plus 10 ms jitter in `qa-split-independent-bounds-separation/report.json`.
CRCs match through X 3944 / Zero 3574. Both peers' separated OpenGL captures
were inspected: each shows its own character/HUD and authored background.
Disjoint interest windows activate remote native soldiers at X=872/1042;
the horizontal gap snapshot is empty and remote soldiers despawn or remain
inside an eligible view after overlap/rejoin. Both native menus and disconnect
also pass.

Framework commit `8fff59f8` preserves the trusted-option agreement and local
render transaction fixes. Ten title CTests and twelve focused framework tests
pass. Native peer runs use isolated writable directories, owned process
handles and original controller input; online fixtures/RAM writes are not used.

Qualification remains bounded: online boss-door/script transitions, full
native kill/re-entry/pool exhaustion, every stage/actor family, long sessions
and a live Internet room/relay match still need playtesting. Historical failed
reports below remain failures and identify the exact candidates they tested.

### Earlier Split integration evidence

Native Split evidence: `build-coop-split/qa-split-render-phase-fixed/report.json`
passes intro enrollment, both movement/attack inputs, own-view verification,
both menus and disconnect, with matching core/mod checkpoints through 3552.
Inspected `split-intro-presented.png` captures use the actual OpenGL own-view
buffer, not the canonical software VRAM. Seat 0 records 226 committed/verified
passes and seat 1 records 227, with no watchdog, verifier, VRAM or private-state
leaks. The report records its exact binary; later boss-HUD/compatibility-hash
changes still need native reruns.

The subsequent native targeting candidate reached matching core/mod samples
through 3698, with native AI counters for both seats (1630 P1 / 630 P2 at the
first own-view probe). Its inspected guest capture retains X's own view/HUD.
Evidence: `qa-split-native-targets-address-fixed/report.json`. The full report
is failed: after the intentional guest disconnect, the harness requested a
status from the blocked emulation thread before the native disconnect reason
was published. The native log does report the disconnect. The harness now
waits only for that required native reason and process health; two focused
tests preserve failure when the reason is absent. The earlier
`qa-split-native-targets` failure records an invalid synthetic function alias,
fixed to the runtime's physical `0F000000..0FFFFFFF` aperture with an address
regression. These failed reports are retained, not relabeled as passes.

The latest native `qa-split-separated-native/report.json` passes the exact
`88c8941b...` candidate through matching core/mod tick 4376, including
input-only separation, overlap/rejoin, both menus and native disconnect.
Both separated OpenGL captures were inspected: Zero remains in his original
area with a nearby native enemy, while X sees the distant authored foreground,
parallax and two native soldiers around his own body/HUD. The foreground
camera origins are 67 versus 785/793 (asynchronous observations), and player
positions are 227.5 versus 953.625: complete activation windows are disjoint,
and X does not pull Zero. The remote soldiers occupy native slots 0/3, type
49, at X 1042/872. After returning both views those remote soldiers are gone;
the near-player enemy in slot 10/type 3 retains its shared HP/identity and
continues to update. No observed native enemy occupies the horizontal gap.
Successful-visit and repeat-suppression counters advance during the trip.

This qualifies the bounded intro separation/response/rejoin path, not every
stage/family or the full kill/re-entry/allocation-exhaustion lifecycle. The
harness now explicitly asserts remote enemy interest/gap/returned-view
lifetime on future runs. This was the earlier checkpoint; resumed death/retry,
Unified and recorded room-launch evidence is listed above.

The first Split attempt is retained at `qa-split-render-first`: core first
forked at 2761 while RAM, mod and GPU authority still agreed. Framework issue
`beads-eio.3.318` fixes render freezes omitting the carried guest-cycle-scale
fraction and gates. The regression fails before the fix and passes afterwards;
five freeze/abort/sandbox/scaler tests and the actual Split rerun pass. The
verification hash now includes this state. Uncharged-loader semantics are
unchanged. This timing fix does not qualify the remaining separated-view AI,
allocation/despawn, overlap/rejoin, death or script-transition cases.

The rebuilt candidate's actual Unified/Zero private two-peer regression passed
intro enrollment, both movement/attack input paths, both native menus and
disconnect, with matching core/mod CRC samples through tick 3347. Evidence:
`build-coop-split/qa-unified-profile-diagnostic/report.json`. The first attempt
stopped during boot with a debug-response mismatch / peer disconnect; retain
`build-coop-split/qa-unified-profile/report.json`. The rerun does not erase or
causally explain that failure, and neither run qualifies Split presentation.

Original-executable integration points for the remaining work:

- The main frame loop submits its active OT at `80012078` through `800EA80C`,
  with `a0 = [80142F80] + 9C` and return address `80012080`. The native SDK
  table pointer at `8011E180` points at `8011E140`; its `+18` DMA callback is
  `800EC228`, which writes GPU linked-list CHCR at `800EC268`. Its `+08`
  queue callback is `800EC2C8` and `+3C` synchronization callback is `800EC9F0`,
  reached through `800EA20C(0)` in the main loop. The exercised local pass uses
  this submit/synchronize boundary, clears the native OT through `800EA714`,
  redraws `80023D68`, and submits directly through `800EC228` in the sandbox.
  `80024260` only stitches the OT; it is not a completed-frame boundary.
- Native enemy/shot pools dispatch one slot at `80021300` / `80021518`
  (`jalr v0`) and reconverge at `80021308` / `80021520`. The early per-slot
  reads are `80021288` / `800214A0`. A balanced nearest-target scope must
  preserve actual-seat positions/hit pause for all collision callbacks,
  maintain union lifetime queries, and restore through skip paths too.
- Texture banks are immutable: defining an existing ID with different pixels
  fails. Local drawing must not append to the authoritative sequential
  sprite/UI bank caches; reserve separate presentation-only IDs/caches.
  Restore private player/projection/inventory bookkeeping even on sandbox
  watchdog failure. Local rendering cannot change future canonical bank IDs
  or the gameplay digest.
