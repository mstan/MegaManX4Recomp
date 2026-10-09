# X4 combat, solid contact, and vehicle evidence

Evidence source: the user's original US executable `mmx4/SLUS_005.61`,
statically decoded as little-endian MIPS. This document contains addresses and
behavioral descriptions, no executable bytes or copied assets. Observations
below are static evidence; gameplay assertions still need live qualification.

## Combat ownership

### Split native target scopes

The original enemy pool `80021234` contains 48 slots at `8013BED0`, stride
`9C`; enemy shots `8002144C` contain 32 at `8013F328`, stride `9C`. Their
whole-pool and per-slot `PLAYER+BC` reads precede dispatch. Native callbacks
are dispatched by `jalr v0` at `80021300` / `80021518` (word `0040F809`),
with NOP delay slots and reconvergence at `80021308` / `80021520`. Original
global freeze checks and flag-8 exceptions remain in the native pool.

Only Split scopes the real per-seat hurt gate and nearest eligible full
body around that one callback. A frozen target takes the original visible
bounds-only `8002B3C0` path, not another AI update. P1 wins distance ties;
shared scenes retain their event owner. P2 projection pins the shared
campaign for story decisions and restores its own identity before inventory
capture. Collision/solids/pickups/rewards/scene callbacks temporarily return
to canonical context, preserve accepted-event ordering and resume P2 AI.
Private-player projections are not mistaken for AI scopes. Real two-body
coordinates remain available to union lifetime checks throughout.

The registered synthetic dispatcher is `8F7FFF00`, normalized physical
`0F7FFF00`. An earlier `9F` alias was outside the supported function aperture
and produced a native dispatch miss; that failed report is retained. Guarded
instruction emission precedes the generated target latch, verified after
real regeneration. Focused ownership models pass; broad enemy-family and
distant-stage original-executable qualification remain required.

| Address | Verified role | Relevant behavior |
| --- | --- | --- |
| `8002D9BC` | Enemy/projectile contact against native player; `a0` is the attacker | Resolves body collision, native damage/armor reduction, knockback direction, and hurt state. Returns zero on no accepted contact, one on accepted contact. |
| `8002BB80` | Contact hitbox comparison; `a0` attacker, `a1` victim | Uses attacker `+50` attack box, victim `+54` hurt box, positions, facing, and victim `+61` invulnerability. Does not update actor AI. |
| `8002DD04` | Player attacks against an enemy; `a0` is the enemy | Iterates 16 native attack slots beginning at `801406F8`, stride `9C`, checks collision at `8002BD58`, applies damage through `8002DE30`. |
| `8002DE30` | Accepted player attack against an enemy | Uses enemy weakness table at `+58`, attack type at shot `+1`, enemy HP at `+5C`, and enemy hit token at `+65`. May consume/change the shot through `8002DF7C`. |
| `8002DF7C` | Attack acceptance/deflection/consumption | Dispatches by attack type. Preserve this function's original behavior for buster shots, saber attacks, and persistent weapons. |

`8002DD04` is **not** an enemy attack against the player. Its return contract is
zero for no accepted attack, a positive attack-type result for a hit, and a
negative result for a kill. An internal `7E` result from `8002DE30` means a
duplicate hit token and becomes a zero result in the outer routine. A `7F`
result represents deflection and is returned positively.

The implementable X6 precedent is a function replacement with a recursion
guard: invoke the original routine against P1; on a zero result only, project
P2's complete body/attack context and invoke it again. Return the selected
result to the original actor caller. The caller then performs its AI, death,
pickup/effect creation, and projectile consumption once. Do not replay the
attacker's entire update. A nonzero contact must not be retried against P2,
because many contact callers consume the attacking projectile.

Examples of original callers with separate AI/result handling:

- `80042290` checks enemy damage; `80042298` selects the death path; the same
  actor updates its behavior before `800422F4` checks player contact.
- `80044540` checks player contact and `80044548` checks enemy damage; the
  branch at `80044550` applies the kill result once.
- `8004086C` checks enemy damage; its caller retains a special enemy state and
  applies invulnerability itself at `80040894`–`800408D4`.

On-foot player contact uses native `801418C8`. If signed player `+C5` is
negative, `8002D9E8`–`8002DA04` instead selects vehicle `80173A30` as victim.
Vehicle HP/hurt state must therefore be part of a player projection. For
normal contact, the original routine writes player `+5C`, `+63`, `+A5`,
`+BA`, and sometimes ammunition at `+A8`. Enemy damage can set player
`+BD` at `8002DEC8`. Swapping only coordinates is insufficient.

The native enemy `+65` hit token is shared across all its accepted attacks.
Equal nonzero signed shot `+64` and enemy `+65` values are rejected at
`8002DE3C`–`8002DE58`, before weakness or damage calculation. X charged attacks
use tokens 1/2 at `80092DB4`/`80092A80`; Zero saber derives its token from an
animation command or attack subtype at `80098198`–`800981AC`. These domains
overlap. In addition, a no-overlap P1 check clears enemy `+65` at `8002DE08`,
which otherwise erases P2's persistent attack history before its retry.

The combat module retains separate native token bytes per enemy and seat,
projects the appropriate byte around the original `8002DD04`, and restores
P1's byte afterward. Each native call still applies its own no-overlap reset,
zero-token behavior, and duplicate rejection. Enemy `+61` invulnerability,
`+7A` lock, weakness data, damage/kill return handling, and attack consumption
remain shared and native. P1 first/only-miss still limits an invocation to
one accepted attack. Original allocators and stage reset invalidate token
records; the deterministic digest includes both tokens in address order.
Boundary tests cover equal seat tokens, a persistent P2 attack with P1
missing, shared invulnerability, zero tokens, and target-slot reuse. Live
weapon/boss qualification remains required.

## Terrain and moving solids

### Owner nonlethal Zero saber report, 2026-10-08

A read-only capture of the owner's paused process recorded P2 Zero in action
`30`, with HP 13, `BC=5` and `BD=1`; P1's freeze bytes were clear. The native
player routine at `800311EC` consumes `BD` by resetting `BC` to five, otherwise
counts `BC` down before progressing the action. Saber damage at `8002DEC8`
reasserts `BD`. Native enemy pools at `80021238`/`80021450` stop checking
collisions while the canonical player's `BC` is set. The co-op P2 retry was
missing that per-seat gate, allowing its still-active saber to restart the
pause continuously while the shared world ran for P1.

Both contact and player-attack callbacks now honor each seat's native `BC`
gate. This preserves the original five-tick pause and shared enemy update
count while allowing its owner's animation to advance. Combat regression
coverage verifies frozen P2 cannot be polled/retriggered, P1 still interacts,
and P2 resumes when its own pause clears. The paused owner process was left
running; its RAM and private player contexts are private diagnostic artifacts.

The owner's X6 report matches its corresponding native `CC`/`CD` fields:
`80034DCC` restarts the five-tick pause, `80020088` gates the enemy pool, and
the existing P2 enemy-hit retry has no such gate. This separate X6 follow-up
is tracked centrally as `beads-xuao`. At the owner's request the equivalent
X6 fix was submitted separately in
https://github.com/mstan/MegaManX6Recomp/pull/40, without rebuild/tests.

The X4 combat regression passes. A controller-only encounter probe did not
reach a nonlethal saber contact and is not a combat pass. A private replay of
the owner's captured actors showed the countdown progressing and the saber
pose ending, but a subsequent native story transition interrupted it; that
also does not certify an uninterrupted recovery to idle. The exact enemy
encounter therefore remains an owner playtest check for the new build.

`80035EA4(a0=PLAYER)` installs player `+68`, the terrain box pointer: X uses
`800F8BC4`, Zero uses `800F8BC8`; negative player `+C5` disables the on-foot
box. Normal spawn calls this at `800359E4`. A prototype that directly forces
the initialized player to normal movement must invoke this routine explicitly
or leave normal spawn to finish; otherwise `8002C614` immediately skips terrain
when `+68` is null.

`8002C614(a0=actor)` resolves actor terrain against the shared map. It uses
temporary calculation storage at `8013B7D8` and nearby addresses and resets
actor `+70`/`+79`. Sequential player passes are compatible; these temporaries
are scratch, not independent persistent player state.

`8002E184(a0=solid)` resolves moving/solid actor contact against three native
victims:

| Victim | Victim address | Solid previous sides | Solid carried flag |
| --- | --- | --- | --- |
| Native player | `801418C8` | `+72` | `+76` |
| Double body | `80175D58` | `+73` | `+77` |
| Vehicle | `80173A30` | `+74` | `+78` |

The wrapper performs overlap/depenetration through `8002E294`, displacement
carry through `8002E380`, and exact surface-contact calculation through
`8002C36C`. It does not advance the solid's own AI or position.
`8002C36C` sets victim `+71` and can set victim `+4A` from solid `+75`.

Recommended implementation: retain P2 `+72`/`+76` history in a host record keyed
by solid actor address and allocation identity. Save P1's bytes, project P2's
previous bytes, resolve P2, store P2's resulting bytes, and restore P1's bytes.
During a P2-only on-foot replay, hide native Double and vehicle active flags
so those victims are not carried twice. With independent bikes, retain P2's
vehicle `+74`/`+78` history too and resolve it once against that solid.
Do not repurpose the Double slot as P2; X4 uses it for Double's native body.

Pool/allocation evidence useful for clearing contact records:

- Ordinary enemy pool `8013BED0`, length `1D40`, stride `9C`, 48 slots;
  allocator entry `8002AB74`, update `80021234`.
- Secondary attack/actor pool `8013F328`, length `1380`, stride `9C`, 32 slots;
  allocator `8002ACA4`, update `8002144C`.
- Stage-object pool `80165A30`, length `1180`, stride `8C`, 32 slots;
  allocator `8002ADBC`, update `8002174C`.

There are additional actor families, so use the actual argument to
`8002E184` rather than assuming every solid belongs to one pool. Clear
sidecars at stage reset and whenever an actor slot is newly allocated.

## Ride Chaser and Ride Armor

There is one native vehicle actor at `80173A30`, size `B0` (cleared bytewise
by `8002AA5C`–`8002AA78`). Dispatch table `800F2AD4` selects by actor `+1`:

| Type | Update | Initialization | Evidence |
| --- | --- | --- | --- |
| `0`, Ride Chaser | `8003B3DC` | `8003B470` | Spawn inherits player position/HP, mounts automatically, stage exits clear rider state; main behavior uses bike movement/jump/attack modes. |
| `1`, Ride Armor | `8003D3F8` | `8003D4C8` | Initializes separate HP `20` hex, on-foot collision/jump mounts through `8003E0D0`, dismounts separately. |

Stage ID `5` is Marine Base / Jet Stingray in root's original-disc QA. Native
spawn `80035694` branches specially for stage `5` at `80035704`; the route
selects `80035A24` through spawn table `800F8B44`.

`80035A24(a0=PLAYER)` enables the vehicle actor and installs the player's
vehicle movement state. It writes player `+C5=-1`, `+D4=29` hex,
`+5=12` hex, `+6=1`; the Chaser initialization subsequently changes
player `+C5` to positive one at `8003B5E4` and installs bike animation/mode
`41` hex through `8003B458`. Starting a second bike with an ordinary
on-foot `+5=2` overwrite bypasses this path.

Native per-frame order in `80021F34` is vehicle `80021C14`, player controller
`800311EC`, player attacks `80021340`, shared actor updates, then player
terrain `8002C614`. `80021C14` updates only the vehicle and can be used in a
second private player pass without advancing enemy AI or the stage scroll.
It honors the original gameplay freeze flags. The Chaser reads native player
input at `80141944`/`80141948` into its `+8A`/`+8C`; the Armor reads the same
values into `+88`/`+8A`.

To make two Chasers, maintain `vehicle[B0]` in each player context and project
it at the singleton alongside the owning player and attacks. Initialize P2
with its native vehicle spawn path, then update it through `80021C14` in the
original order. The shared map and scrolling stage update remain once. Native
vehicle rendering occurs at `80024158`, calling `80024334(a0=vehicle)`; a
second renderer pass can render the private vehicle with the shared stage
vehicle assets.

Chaser HP shares the rider's health behavior: initialization copies player HP
at `8003B564`–`8003B5B4`; at `8003B7EC`–`8003B818` a zero Chaser or rider
HP writes fatal player HP and selects vehicle death. Its own normal update
copies accepted Chaser damage back to player HP at `8003B82C`–`8003B844`.
Suppress shared team death until both players are fallen; do not revive or
reset the surviving vehicle. Stage end state is read from player `+D9`, and
`8003BA24` clears player `+C5` while continuing the vehicle exit animation.

Vehicle-linked effect ownership requires additional care. For example,
`8003D638` creates a shared effect with its source pointer in effect `+50`.
P2's source is the projected singleton address; restoring P1 makes that
address point at P1 again. Route these effects to a stable private P2 vehicle
mirror or retain an owner sidecar. Bike attack slots use the ordinary player
attack pool, so each player's pool projection preserves those separately.

Implemented ownership uses allocator `8002AD3C`, which returns one of 32 shared
effect slots at `8013E510`, stride `70` hex. Both players' allocations clear a
reused slot's owner, including private enrollment before `ready()`. At the end
of a private projection, root refreshes a stable vehicle mirror and invokes
`mmx4_coop_combat_project_end(mirror)` before restoring P1. Only active,
P2-owned effects whose `+50` still equals the native vehicle address are
redirected; hitbox and other source pointers retain their native values.

Verified following-source stores: Chaser shot effect type `0C` at
`8003C8A0`, vehicle start effect type `0D` at `8003D224`, damage effect type
`0F` at `8003D30C`, and Armor effects at `8003D6CC` and `8003D774`. Type
`0C` follows source state/XY in `800B1A48`; type `0D` follows source
frame/XY in `800B1B74`; type `0F` follows source XY/state in `800B1DE4`.

## Native stage identifiers

The resource catalog establishes numbered ST00–ST0C archives, not English
stage names. Earlier inferred name tables were wrong. Root's native-load QA
and review of its screenshots establish the following practical map; these
are original-disc visual observations, distinct from an EXE name table.

| Native ID | Stage / Maverick |
| --- | --- |
| 0 | Sky Lagoon intro |
| 1 | Jungle / Web Spider |
| 2 | Snow Base / Frost Walrus |
| 3 | Bio Lab / Split Mushroom |
| 4 | Volcano / Magma Dragoon |
| 5 | Marine Base / Jet Stingray |
| 6 | Cyber Space / Cyber Peacock |
| 7 | Air Force / Storm Owl |
| 8 | Military Train / Slash Beast |
| 9 | Memorial Hall |

ID 2's snowy colossus, ID 3's green staircase, ID 4's volcanic cave, and
ID 6's circuit-board room are visible in root's
`build-coop/qa-stages-0/campaign-0-stage-NN-0.png` evidence. Root separately
identified the bike, Air Force, Military Train, and Memorial Hall loads.
This evidence worker did not execute new stage loads.

## Character graphic coverage

Original PL00/PL01 metadata verifies that the main compressed member `2` and
assembly member `9` are not the whole character graphic set. Both archives
contain raw graphic member `1`, type `10206` hex, size `1B800` hex, and raw
graphic member `5`, type `10007` hex, size `8000` hex. Their contents differ
between X and Zero. Header family counts also differ: 14 compressed graphic
families versus 22 assembly families for X, two versus seven for Zero.
Do not assume matching header indices identify corresponding families.

Native raw graphics loader `800142BC` indexes placement table `800F1614`
with the type's low byte. Entry 6 begins at VRAM word `(320,176)` and entry
7 at `(832,256)`. `10206` mode 2 uploads consecutive 2048-byte chunks as
64-word by 16-row rectangles. Rows start at Y 176, 192, 208, 224, and 240,
then X advances 64 words and Y returns to 176 (`800144E0`–`80014500`).
Its 55 chunks therefore cover VRAM words X 320–1023, Y 176–255.
Member `5` mode 0 covers 16 chunk rows at X 832, Y 256–511.
These original bytes can supply private static texture banks without
overwriting P1 VRAM.

## Unoccupied Ride Armor handoff

The combat module wraps native Armor update `8003D3F8`, invokes that full update
once for the world/P1, and offers P2 the same actor only if it remains active,
type 1, state `+4=1`, idle mount mode `+5=9`, and `+97` bit `40` hex is clear.
Both player `+C5` fields must be clear, and P2 cannot already own a vehicle.
P1 therefore wins a simultaneous mount. The retry invokes only
`8003E0D0`: its initialization has already run (`+6` is retained); the original
routine checks player overlap/eligibility and commits `+5=0A`, `+97|=40`, and
player `+C5=1` at `8003E22C`–`8003E258`.

An accepted mount transfers the world vehicle into P2's private context and
clears the world actor's active/visible bytes. Its next private Armor update
is skipped once because the world already advanced that same actor during the
transfer frame. Following effects and previous solid vehicle-contact bytes
follow the transferred owner. No world Armor actor is cloned.

Native dismount clears player `+C5` and vehicle `+97` bit `40` at
`8003EF90`–`8003EFA8`. At projection end the same unoccupied actor, with its
remaining HP/position/state, returns to the saved world vehicle context if
that context is empty. P2's active/visible vehicle bytes are cleared, and
following effect pointers and solid vehicle-contact history return with it.
This requires core accessors for the second vehicle and the saved first
vehicle; the latter is valid only during projection. Destroyed/ridden-death
armor recovery still needs live qualification.

Concrete texture-selection hazard: native Zero saber initialization
`80097FC4` shares the body assembly pointer at `80098020`/`80098028`, but
chooses the saber's own animation through `800980C8`/`800980DC`. A renderer
fallback may inherit the body graphic table while retaining the saber actor's
own `+47` frame; replacing that frame with the body's frame is incorrect.
The root implementation corrected this during adversarial review.

Generic X buster initialization `80092314` uses assembly header index 1
at `80092348`/`80092370` and leaves compressed-graphic pointer `+38` null.
Its preuploaded static artwork therefore requires the raw graphics path.
Other weapons have explicit, different family mappings; for example
`80095254` uses compressed header index 5 and assembly index 13.
Full weapon/effect graphic parity still requires original-backed mapping and
live visual checks.

## Aimed AI and qualification limits

The combat module opts in only inspected functions `80040CCC`, `800419B8`, and
`80042824` to nearest-living-player aiming. These routines write the enemy's
movement/state and do not call contact/combat routines or write the player.
Scoped player XY substitution lets the original AI run once and restores XY
before returning. Equal distances retain P1. Other AI still needs mapping.

When P1 is fallen and P2 survives, the module wraps primary enemy pool
`80021234` and secondary attack pool `8002144C`, substitutes only the survivor's
XY during their single native update, and restores the fallen body's XY
afterward. Contact/damage wrappers remain active and project the actual live
victim. Both pool entries read player `+BC` (`80021238`, `80021450`) and skip
their entire pool when it is nonzero; the survivor wrapper temporarily clears
the fallen owner's freeze byte, then restores it. Script/boss-transition
pool `8002166C` is not part of this change. No additional actor AI pass runs.

Enemy aiming is not centralized in the contact routines. Verified examples
read player coordinates directly: `80040CFC`, `80040D44`, `80040D68`,
`80040DA4`, `800419C8`, `8004251C`, and `80042840`. These update only the
enemy once and need a separate target-selection policy. A stable nearest
living-player selection per actor update is suitable, but projecting P2's
coordinates through that update also affects native contact checks unless
those checks explicitly restore/project the actual victim contexts. Do not
claim that adding contact retries alone makes aimed AI target P2.

Required live scenarios after integration: both death orders on bikes,
simultaneous bike attacks, bike-to-foot transitions, contact projectiles
consumed once, moving platforms carrying both players, differing facing and
overlapping saber/buster hit tokens, Ride Armor exclusive ownership, and both
campaigns' bosses/terrain. These observations provide hook boundaries; no
live validation was performed by this evidence worker.
