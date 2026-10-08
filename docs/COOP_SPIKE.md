# X4 simultaneous X / Zero spike

Work branch: `spike/mmx4-coop`, based on fetched `origin/master` at
`e13b654f`. Tracking: central Beads `beads-eio.9.1`.

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

Each player's Start opens that character's original menu. The world pauses;
only the owner controls the open menu. A tank used there heals that owner and
depletes the shared pool once. The native pickups contribute to the same
pool. A heart upgrade belongs to its collector; X owns armor upgrades.
Both characters receive the corresponding defeated-boss ability.

One player's death leaves the survivor playing. A team wipe consumes one
shared retry. P2 may hold Select for 90 native player ticks to withdraw, then
release and tap Select to rejoin at the grounded P1. Withdrawal is unavailable
while riding or when P2 is the only survivor.

Online, select the trusted `mmx4.coop` profile. Its compatibility identity is
`mmx4-coop-delay-v1`. Online policy enables the required plugin, fixes the
supported renderer/aspect, and excludes offline mod selections from gameplay.
Both peers need the same build and original disc. Independent/split cameras
remain a later enhancement.

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

## Remaining qualification

This is an experimental spike, not a full-campaign certification. Complete
stage clears, all boss/weapon combinations, moving platforms, Ride Armor
acquisition/destruction, long sessions, and character-specific sound parity
need broader playtesting. Only inspected aimed AI routines choose the nearest
living player while both are alive; other native AI retains its original
target preference. The shared enemy pools follow the survivor if P1 dies.
Split cameras and host-state snapshots are intentionally deferred.

The corrected Zero-campaign intro section transfer, later bike section and
Sigma section have passed native load/control checks. The later bike section
now mounts both players. The first private X-campaign netplay pair also passed
independent controls, menus, matching core/mod digests and disconnect; the
remaining campaign/jitter matrix is in progress.
