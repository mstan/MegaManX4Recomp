# X4 co-op: lifecycle, menus, pickups, stage fixture evidence

Evidence is from disassembly of the user's original US `SLUS_005.61` executable.
Addresses describe this revision only. This document contains metadata and
interpretation, not executable or asset bytes. `PLAY = 0x801721C0`,
`PLAYER = 0x801418C8`, `MENU = 0x801754A0`.

## Native state and stage lifecycle

The major dispatch table at `0x800F23E8` is read by the game loop at
`0x8001FB50`. PLAY+0 is the major mode and PLAY+1 its minor mode.

| Major | Routine | Behavior |
| --- | --- | --- |
| 0 | 8001FBB8 | Initializes lives to 2, max HP to 32; advances to title |
| 1 | 800299EC | Title |
| 2 | 8001FBD4 | Advances to character selection |
| 3 | 8002FC38 | Character selection / new game story |
| 4 | 8001FBE0 | Calls native asset loader 80013014; resets checkpoint/spawn mode; advances to 5 |
| 5 | 8001FC20 | Native stage initialization, including 80035240 player initialization; advances to 6 |
| 6 | 8001FF50 | Actual gameplay; dispatches minor state |
| 7 | 8001FAFC | Stage completion / reward presentation |
| 8 | 80020AC8 | Game over |
| 9 | 80020D98 | Stage selection / story progression |
| 10 | 80020464 | Section transition |
| 11 | 80023A54 | Scripted scene |
| 12 | 800204AC | Alternate dispatch |

Gameplay minor 0 (`8001FF8C`) checks the native Start edge then runs the world
once (`80021158`). Minor 1 (`80020060`) handles stage termination/death after
fade. Minor 2 (`80020368`) updates input and the native pause menu; it does not
run the ordinary world. `80031064` runs one world update while exiting pause.

Important PLAY fields:

| Offset | Evidence-backed meaning |
| --- | --- |
| +0C / +0D | Stage ID / section |
| +0F | Stage termination result: zero = death, positive = completion, negative = transition preserving resources |
| +10..+1A | Shared update/script gates; do not privatize as a block |
| +1C | Pause suppression; 8001FF8C refuses Start when nonzero |
| +1D | Checkpoint index |
| +1E | Spawn mode; 0/-2 = full HP and ammo, -1 = restore saved HP/ammo |
| +1F | Script/transition control |
| +24 | Script field |
| +43 | Campaign character: 0 X, 1 Zero |
| +44 | Remaining retries/lives |
| +45 | Saved HP for preserving a section transition |
| +46 | Maximum HP |
| +47 / +48 | Native armor / additional upgrades |
| +49..+58 | Saved ammo for preserving a section transition |
| +59 | Completed Maverick bits |
| +5A..+5B | Heart / tank acquisition flags |
| +5C / +5D | Two E-tank energy fields: bit 7 acquired, low 7 bits energy 0..32 |
| +5E | W-tank stored energy 0..32 |
| +5F | Shared story progression |
| +60 | Saved selected weapon |

The death decrement is at `800202B4`: PLAY+44 decrements once; signed underflow
sets major 8. Otherwise it selects a native restart state. A co-op survivor must
prevent the shared death transition until both players have died. Ordinary
script transitions must not revive a dead partner.

`80033BC8` checks native damage/crush flags and HP. Instant death marks HP zero,
sets PLAY+1C, and enters player state +4=2. Player-state dispatch at
`800F8980` maps state 2 to death animation `80035A6C`, state 3 to `80035DDC`
(empty), and state 4 to `80031410`. Gameplay minor 0 sees canonical
PLAYER+4==3 and begins shared death transition. Keep the dead player's native
animation while preventing it from terminating the surviving team.

## Pause/menu ownership

Native menu dispatcher `8002FCAC` uses MENU+4 (main state); `80030128` uses
MENU+5 (substate). MENU+14 is selected item. Main 0 initializes, main 1 is
interactive, main 2 exits. MENU+28 and +2C are ongoing E-tank/W-tank operations;
MENU+30 is escape-stage request.

Opening routine `8002FD70` reads PLAYER+2 character, PLAYER+B9 abilities,
PLAYER+A8..B7 ammunition and PLAYER+93 selected weapon. Menu art is therefore
character-specific. Project the owner before the menu initializer and every
menu update/render. Serialize requests with P1 winning a simultaneous Start.
`80020368` runs `80035EF0` before menu update; owner input must be restored
after that canonical input refresh. Native menus consume the shared edge word
at `80166C0C`, with Start mask 0x800.

E-tank menu commit `80030A2C` selects E-tank 0/1 from item index 0xA/0xB,
consumes one energy point and adds one HP per update until full/depleted,
with PLAY+46 as cap. Both tank fields retain acquired bit 7. Native routine
can compact the second tank into the first at depletion; retain that behavior
in the single shared pool. W-tank commit `80030C54` fills PLAYER+A8..B0 while
consuming PLAY+5E. `80031064` applies selected-weapon/animation changes on exit.

Tank ownership bits are PLAY+5A halfword bits 0x1000, 0x2000, 0x4000; EX tank
is bit 0x8000. Native pause setup reads these directly. E-tank energy flags
and acquisition bits must not be privatized with Heart Tank flags.

## Pickups and asynchronous healing

General pickup actor dispatcher is `800BF730(a0=pickup)`. Spawn/init uses actor
class byte +0=0x21 and type +1=2; `800BF76C` initializes the actor.
`800C00BC` chooses canonical PLAYER or the native vehicle, checks nonzero HP,
and calls overlap routine `8002C160(a0=pickup,a1=recipient)`. Pickup+7C is the
normalized subtype; table at `80011238` dispatches the reward.

| Subtype | Reward |
| --- | --- |
| 0 | 4 HP, 1 E-tank energy |
| 1 | 16 HP, 2 E-tank energy |
| 2 / 3 | 4 / 16 weapon ammo |
| 4 | Shared life, capped at 9 |
| 5 | Full HP, 4 E-tank energy |
| 6 | Full ammo (48) |
| 7 | Heart Tank; raw actor+2 = 7..14 |
| 8 | E-tank; raw actor+2 = 15/16 |
| 9 | W-tank; raw actor+2 = 17 |
| 10 | EX tank; raw actor+2 = 18 |

Health reward handler `800BFF0C(pickup, hpAmount, tankAmount)` starts native
asynchronous healing: actor state+4=2, +80=remaining heal, +81=2-tick interval.
It calls `800C03BC(1)`, freezing PLAY+10..+17. Actor flag bit 8 allows healing
to continue in the frozen world. Healing update `800BFBD0` adds canonical
PLAYER HP every two ticks, then unfreezes via `800C03BC(0)`.

Consequently pickup owner selection must persist across collection and later
healing updates. Project the collector for both. Marking ownership only during
the overlap call heals the wrong character on subsequent ticks.

Native health rewards immediately fill the first acquired nonfull shared
E-tank by their fixed tankAmount even when the recipient was missing HP.
This differs from an overflow-only rule; the co-op plan's overflow-only
behavior requires an explicit adjustment rather than assuming native logic.

Ammo reward handler `800BFCC0` reads PLAY+43 and PLAYER selected weapon and
abilities; project collector inventory and character there. E-tank shared
fields remain global. Heart reward `800C01E8` sets PLAY+5A bit
`(rawSubtype-7)` and increments maxHP PLAY+46 by 2. Privatize low eight Heart
flags and maxHP for collector-only upgrades; retain shared tank bits.
Pickup initialization also reads acquisition flags to suppress collected
permanent items, so define which player's heart flags should suppress each
world placement. Shared placement consumption is a separate decision from
private character acquisition bookkeeping.

## Dual rewards and campaign identity

Completion routine `8001FA24` ORs bit `(PLAY+26-1)` into both PLAY+59 completion
and PLAYER+B9 abilities. PLAYER+B9 is initialized from PLAY+59 at `80035240`.
For co-op, each character should receive that same native stage ability bit:
X interprets it as its weapon, Zero as its corresponding technique. Completion
and shared progression advance once. Preserve selected campaign PLAY+43 for
story/reward scenes; private projection is required only for character-local
behavior and inventory.

Native save record routines `8001C07C` and `8001C3E8` store one character,
maxHP, upgrades, completed stages, story state, and heart/tank flags. They do
not provide a two-player save format. The development build should not save
P2-projected state into the original single-character record.

## Native stage-load fixture

At a gameplay frame boundary in an established game, set PLAY major/minor/sub
to `4,0,0,0`, set +0C stage and +0D section, and leave campaign and inventory
unchanged. The native loader/initializer then executes modes 4 -> 5 -> 6.
Mode 4 itself resets +1D checkpoint/+1E spawn mode. Do not bypass mode 4 and
write gameplay mode directly: stage assets and terrain would remain stale.
The development fixture dispatcher applies this request before the original
gameplay dispatcher runs. Private live tests below exercised IDs 1 and 5 after
input-only boot through the intro; the broader stage matrix is recorded by the
integrating worker. A successful load is separate from a completed stage.

Original file catalog verifies ST00 through ST08 each have sections 00/01;
ST09 and ST0A section 00; ST0B and ST0C sections 00/01. IDs 13..15 are menu/
story/ending resource sets and must not be treated as ordinary gameplay.

Only visually confirmed stage names are assigned below. Other IDs retain an
explicitly unverified name until a screenshot or native title confirms it.

| ID | Confirmed stage or unverified candidate | Section fixture |
| --- | --- | --- |
| 0 | Sky Lagoon intro | 0/1 |
| 1 | Jungle / Web Spider | 0/1 |
| 2 | Snow Base / Frost Walrus (combat worker's live visual check) | 0/1 |
| 3 | Bio Lab / Split Mushroom (combat worker's live visual check) | 0/1 |
| 4 | Volcano / Magma Dragoon candidate | 0/1 |
| 5 | Marine Base / Jet Stingray | 0/1 |
| 6 | Cyber Space / Cyber Peacock (combat worker's live visual check) | 0/1 |
| 7 | Air Force / Storm Owl (integrating worker's live visual check) | 0/1 |
| 8 | Military Train / Slash Beast (integrating worker's live visual check) | 0/1 |
| 9 | Memorial Hall (integrating worker's live visual check) | 0 |
| 10 | Space Port candidate | 0 |
| 11 | Final Weapon candidate | 0/1 |
| 12 | Final Weapon / Sigma candidate | 0/1 |

Stage 5's native vehicle behavior is corroborated by stageinit `8001FDBC`
setting spawn mode 1 on checkpoint 0, player init's dedicated stage-5 branch,
and vehicle/camera exception `80027C00` checking stage 5. Private live screenshots
show both characters on separate Ride Chasers in this stage.

Native checkpoint setup `80028BF0`, called by `8001FC20` before player init,
indexes `800F42B4` by `(stage*2+section)*4`, then indexes that pointer list by
PLAY+1D. The resulting original record supplies player position and camera
bounds. Stage 1 section 1 list `800F4150` contains six checkpoints: index 4
record `800F3530` spawns at (3408,1280), while index 5 record `800F3554`
spawns at (4480,1280) with camera X locked to 4320, a boss-room candidate.
After loading that section normally, mode 5 can rerun native initialization
with checkpoint and spawn mode 0 preserved. This exercises native actors,
camera, terrain and boss setup rather than changing boss HP or completion.
The stage 12 section 1 list `800F4298` contains exactly seven entries (0..6)
and ends at the pointer table itself; larger checkpoint indices are invalid.

## Implemented ownership boundaries

`src/mods/mmx4_coop_lifecycle.c` wraps the original routines above and keeps
shared tanks' native contribution/use behavior. Private inventory projection
changes PLAY+43 only while a character-local pass runs, then restores campaign
identity. The native full-load entry `8001FBE0`, together with stage/section
identity and shared life count, distinguishes a new stage from a same-stage
script/section initialization. Section transition `8002044C` also runs full
loader mode 4, so a changed section in the same stage must preserve either
fallen partner. Both P1 and P2 corpses are reapplied after native initialization.
Native stage initialization `8001FC20` calls `8002A7D0`, which clears the
canonical PLAYER through `8002A728` before calling `80035240`. P1 survivor
status therefore must be captured at `8001FC20` entry, before that clear.
Checking HP at `80035240` incorrectly treats newly cleared initialization
storage as a corpse. The module keeps and hashes this pending status until
reset consumes it; focused regressions model the actual clear for both
section changes and same-assets checkpoint initialization.
An increased native life count from a 1-up also preserves a fallen partner
on section transfer. A decreased count, native 0-to-255 underflow, or both
previous bodies being unavailable triggers team respawn. The last condition
also covers game-over Continue, which resets lives from 0 to 2 and must not
be mistaken for a 1-up that preserves two corpses.

Additional capsule evidence: main capsule dispatch `800C62DC`, child
handlers `800C6B84`, `800C6C2C`, `800C6CE4`, and upgrade commit `800C6EDC`
read the canonical player. Initializer `800C63BC` removes a capsule for Zero,
so Zero campaign requires projecting X/P2 over the entire capsule family.
Native armor commit at `800C712C`/`800C7138` stores PLAY+48/+47 from X's body.
Dead or withdrawn X cannot perform a capsule acquisition.

The paused render path stays inside `8002FCAC`: `80020368` calls input then
that menu dispatcher only. Palette upload `800170B0` reads scratch+38;
background renderer `80017100` reads +34; sprite-menu renderer `800175AC`
reads +3C. Original resource tag pointer table `800F15BC` maps tags 16..21
to scratch +34/+38/+3C/+40/+44/+48. Native archive graphics type 1 is handled
by `800142BC` and uploaded in 64x16 chunks by `800148EC`; type 2 is audio
and must not be interpreted as a VRAM image. `800170B0` palette upload is
at x=0, y=484, width=256, height=2.

The focused test `tests/test_mmx4_coop_lifecycle.c` uses a deterministic
native-call model to check the ownership boundaries, deferred healing,
survivor handling, menu serialization, capsule ownership, section death
carry, and withdrawal/rejoin bookkeeping. It does not certify original
gameplay, stage traversal, or native resource rendering; the live original
executable matrix remains required.

## Survivor progression, portals, and script ownership

`8002166C` is the shared stage-script pool dispatcher: 32 actors starting at
`80142F98`, stride 0x30, type table `800F285C`. It runs no player/controller,
projectile, or enemy pool. When P1 is dead and P2 is alive, the lifecycle
module presents P2 during this one original dispatch and pins PLAY+43 to the
selected P1 campaign. It does not run the world twice. This includes stage
controller type 38 (`800BD654`).

General portal dispatch `800C2BE0` is placed type 11 in the `80165A30` pool
(table `800F2910`). Its entry behavior `800C2E20` checks the previous frame's
actor+72 bit 8, moves the canonical player to the portal, and requests native
script action 0x14 through `80036AE4`. `800C31C4` finishes the portal action
and writes the checkpoint from actor+2. The original dispatch calls solid
contact `8002E184` after its behavior, so independently stored P2 contact has
the same previous-frame timing. P1 wins a simultaneous contact; P2 may lead
when only its contact bit is present. Actor ownership remains fixed throughout
the resulting script.

Boss-door type 8 in the same pool uses dispatch `800C1994` (table entry
`800F2930`). Its waiting handler calls original read-only bounding/state
query `800C1E7C`; successful contact sets personal PLAYER+C4=1 at `800C1BF4`.
The door's `800C1D90` auto-walk advances that owner's X, and completion clears
C4 at `800C1DD4`. This path does not call the general C0 script command pair.
The lifecycle wrapper queries living P1 first and P2 only on a miss, keeps
the door owner through native auto-walk, and treats C4 begin/end as shared
control/transport boundaries. P1 wins simultaneous eligibility; a sole living
P2 can enter. Focused regressions verify those ownership cases and preserve
a P1 corpse while P2 finishes the modeled door.

Checkpoint/door controller `800BD654` compares player X against the original
threshold table `8010C02C` and requests actions 0x14/0x15. The leading living
player may own its trigger; that owner persists through the script. Shared
checkpoint/story state advances once. Native script commands `80036AE4` and
release `80036B18` are redirected to P2 if their caller still addresses a
dead canonical P1.

At script begin, the living passenger leaves simulation with their own
vehicle. Their HP and inventory remain intact; controller, projectile and
terrain passes exclude that passenger. The passenger returns only after
native C0/C3/C4, shared script gates and chained dialogue release control,
and the owner has reached a grounded position. Original pose 2 provides the
return animation. Dead and voluntarily withdrawn partners are excluded.
The scripted owner's native auto-walk proceeds with one shared camera/world
update. Broader portal, track-clear and final-stage sequences still require
qualification beyond the native Jungle door cases below.

## Private native validation, 2026-10-08

The lifecycle worker ran a separate copy of the current development executable
under `build-coop/qa-lifecycle-live-0`, PID 19644, TCP port 14625, with private
mods state and memory cards. The original disc remained the asset source.
Boot and game-over Continue used controller input only. Stage requests used
the bounded frame-boundary fixture; HP requests changed only the selected
player's HP (0x80 requests the original fatal path). A shared E-tank was seeded
with PLAY+5B=0x10 and PLAY+5C=0xA0. No boss HP or completion was written.
Private `validation.jsonl` and live GPU screenshots hold the observed evidence;
they contain diagnostic/asset material and are not committed.

| Check | Observed original-game behavior |
| --- | --- |
| Native Start, both seats | Each seat opens minor 2. The other seat's Start/direction input cannot close or move that menu. Both players' positions and HP remain frozen. |
| P1 tank use | P1 HP 8 becomes 32, P2 HP 14 stays 14, shared tank 0xA0 becomes 0x80 through one native use. |
| P2 tank use | P2 HP 14 becomes 32 after exiting its menu, P1 stays 32, shared tank 0xA0 becomes 0x80 through one native use. The original routine drains the chosen tank while clamping HP. |
| Withdrawal/rejoin | P2 holds Select for 110 observed frames and becomes inactive, retaining HP 32. P1 moves from X=56 to 77.5. A released/repressed Select rejoins P2 at X=77.5, HP 32. |
| P1 dies first | Native death reaches state 3, HP 0 and hidden sprite. P2 remains state 1, HP 32 and moves X=77.5 to 115 under its own input. Shared lives do not change. |
| P2 dies first | Native death reaches state 3, HP 0 and hidden sprite. P1 remains state 1, HP 32 and moves X=56 to 89.5 under its own input. Shared lives do not change. |
| Team wipe, P2 first | One corpse leaves lives 0 unchanged. After P1 also dies, native minor 1 leads to game over, with lives 0 becoming 255 exactly once; still 255 after another 180 frames. |
| Team wipe, P1 first | One corpse leaves lives 2 unchanged. After P2 also dies, lives become 1 once and native initialization respawns both characters at HP 32/state 1. |
| Stage 5, two bikes | Both riders have native C5=0xFF, action 18, and separate active type-0 vehicle actors. Wide screenshots visibly show X and Zero on their own Ride Chasers. |
| Stage 5, either rider dies | A fatal HP request removes only that rider's bike. Over the next 35 observed frames plus diagnostic sampling, P2 advances X=1581.5 to 1916.25 after P1 dies; P1 advances X=1607 to 2057 after P2 dies. Survivor HP 32/state 1 and shared lives persist. This is bounded continuation, not a track-clear claim. |

The tested build's menu graphics were severely corrupted for both owners,
despite correct input, freeze and healing ownership. The integrating worker
identified native GPU DMA truncation of retained Expansion1 data and is fixing
that rendering boundary; these tests do not certify the repaired menu art.
P2's diagnostic body is stale while native gameplay ticking is paused, so P2
healing was checked after menu exit. Individual TCP RAM reads are asynchronous
and may catch a temporary player projection; canonical campaign assertions
require a native boundary or the scoped tests, not one arbitrary RAM read.
Portal/boss-room progression, native rewards, a track clear, and Sigma remain
separate live validation requirements.

A repaired build was copied to `qa-lifecycle-live-1`, PID 51480, on the same
private port. Live captures confirm legible blue X and red Zero native menu
portraits after the retained-source LoadImage repair. Loading Jungle section 1
then exposed the pre-clear P1 corpse bug above: the live load did not produce
two playable characters and eventually wiped the team. The added `8001FC20`
capture and regression are being integrated before continuing the native
checkpoint/boss test. Do not count that failed section load as a validation
pass.

## Resumed validation, 2026-10-08

The section-load capture fix and retained-source menu rendering fix are now
integrated. The failures above remain historical evidence. Both campaigns
passed four native Jungle door/conversation cases: P1 owns the event, P2 owns
the event, P1 is the sole survivor, and P2 is the sole survivor. The living
passenger stays hidden through chained native locks, returns with preserved
HP, and does not resurrect a fallen partner. The P2-owned conversation uses
P2's controller input. Reports are private under
`build-coop/qa-resume-final-x/doors-warp` and
`build-coop/qa-resume-final-zero/doors`.

P2 enrollment now preserves all three native camera enable flags. A camera
pass projects the surviving or scripted owner as needed; the shared camera
can scroll after enrollment and follow P2 through its door event. Door
passengers are excluded from actor, terrain, projectile and shared contact
processing until their native return animation is ready.

The foot Select check observed withdrawal at 90 native ticks, retained HP 32
and all recorded ammo, and rejoined beside the moving P1 after release/tap.
The latest native bike check held Select for 112 ticks: both actors remained
active, mounted and at HP 32, with the shared lives unchanged. This prohibition
covers Marine Base checkpoint zero even during transient unmounted frames;
foot withdrawal is available after the bike checkpoint has ended. The
checkpoint policy and a survivor rescue during an incoming return animation
also have lifecycle regression coverage. Reports are private under
`build-coop/qa-resume-policy-x/select`.

Zero's resumed stage-entry matrix passed all 13 entries with independent P2
input. These are load/input checks, not completed stages. The candidate passed
all seven game CTests and the focused two-peer checks recorded in
`COOP_NETPLAY_EVIDENCE.md`. Candidate executable SHA256:
`a9758f53f6d365b6ac6b8850f7e2071992c252e4990e3b7ee23fe66591b458f4`.
This build is ready for the owner's first playtest; full boss/stage clears,
moving-platform and armor behavior, a bike-track clear, and extended play
still require qualification.

## Native teleport animation follow-up

The owner requested actual Zero arrival/departure animation. Foot enrollment,
Select transport and scripted passengers now use original `80035848`/pose 1
arrival, `80031410` touchdown/pose 2, and `80035048`/`80031540` departure with
poses 3/4. Arrival uses the previous terrain flags just as `800312F8` does
before native action dispatch; omitting that refresh left the beam resting
above the floor indefinitely in the first private check. Camera enable flags
and the owner's shared pause-suppression byte are preserved around the
passenger's arrival routine. Controls/collisions stay excluded through the
animation. A mounted scripted passenger keeps its existing scoped suspension,
and voluntary departure remains prohibited during the bike sequence.

Select release/tap can queue the return while the outgoing animation finishes.
Death of the scripted owner restores a living passenger safely during either
arrival or departure. Lifecycle tests cover the new outgoing phase and the
existing inventory/death/script invariants. The new build is staged separately
under `build-coop-next` so the owner's running build and paused scene remain
available.

The final candidate passed controller-driven foot withdrawal/rejoin with all
recorded HP/ammo retained, and held Select for 112 bike ticks with both riders
still mounted. Native enrollment reached controllable Zero after its arrival.
All seven game CTests passed. The owner encounter's uninterrupted combat
recovery remains a playtest check (see the combat evidence's replay limits).
Executable SHA256:
`8eabd7b62c2d14be02ec2e7f8c092135409b4fed2e133199c12a3ed8c4d3b8ed`.
