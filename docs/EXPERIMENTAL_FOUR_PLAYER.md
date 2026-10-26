# Four Player Co-op spike

This branch started from stable 1.0.6 and now incorporates stable 1.0.9.
The current preview is `1.0.9-4p-spike.1`; its fresh native integration checks
are tracked in [FOUR_PLAYER_107_INTEGRATION.md](FOUR_PLAYER_107_INTEGRATION.md).
It adds a separate, default-off
`mmx4.coop4` mod. Offline play uses independent split cameras. Online play has an individual
4:3 camera and HUD for each participating peer. All peers must enable this
mod, use the same build, and agree on the roster before starting.
The launcher shows **Local cameras** offline and **Online cameras** in the
Netplay flow. The hidden control's stored selection is preserved.

| Seat | Default |
| --- | --- |
| P1 | X, without armor |
| P2 | Zero |
| P3 | Ultimate Armor X |
| P4 | Black Zero |

Every seat can choose X, complete Fourth/Force Armor X, Ultimate Armor X,
Zero, or Black Zero. A form can appear only once. Only P4 has **None**, which
creates a three-player session. Partial armor combinations are excluded.
Fourth Armor uses the native stock-charge arm; Ultimate uses its native
plasma/Nova Strike form. The selected complete forms apply from enrollment.

Enabling this mod turns off two-player co-op and the standalone Ultimate
Armor/Black Zero mods. Those mods cannot be reenabled while this mode is
active. Each hero's native code flag belongs to that seat instead.

## Implementation

The world advances once. Extra seats run native character/controller, terrain,
weapon and animation passes against private body, projectile, trail, charge,
inventory and camera contexts. Shared enemies choose a nearby living hero;
native contact and attack admission visit the eligible seats. Solid contact
history and attack tokens belong to each seat. Deferred charge effects retain
their allocating hero. Fatal damage preserves the surviving teammates' world.

Normal/Force X and Ultimate X keep separate native body/assembly/color banks.
Zero and Black Zero share immutable artwork and preserve separate working
palettes. These allocations fit the existing 1 MiB guest mod-memory contract.
Each seat has two distinct draw arenas; cache reuse pins all eight arenas.
Individual views use the framework's reversible render transaction. Private
draws restore host contexts as well as guest state and do not tick the world.

Each seat has its own native pause menu, weapon selection and tank cursor.
Ultimate X uses X's menu artwork; Black Zero uses Zero's. Start from a living
seat pauses or resumes the team. Quit and Options retain one shared campaign
frontend. In three-player split, P3's menu is centered in the bottom pane.

Added seats join the native incoming beam while P1 is arriving. Each body
retains its own descent countdown and collision-box installation; only P1's
arrival releases the shared stage lock. Enrollment precedes the first HUD
draw, so each meter starts in its own cell rather than first stacking in the
canonical view. The retail overlapping READY exit animation is preserved.
Native C4 doors gather living teammates
at the entering hero's feet and protect them through the one authored door
walk. The door releases their original bodies and collision boxes together;
dead seats stay dead.

Each camera updates all three native foreground/parallax layers before its
view is saved. Local cells reuse the same synchronized CPU VRAM checkpoint.
ENHANCED execution charges the canonical world normally and runs added player
and camera passes through the bounded uncharged native-call service. An exceeded budget
falls back to charged execution so device waits can complete. REFERENCE builds
keep charged native player passes. This policy is fixed by the build profile.

Fresh ENHANCED builds select the existing HLE OpenGL batching tier; fresh
REFERENCE builds retain LLE. An explicit `PSX_GL_BATCH_IMPL=LLE` build remains
available. This is a fixed host-renderer choice, with no live cycle-scale
override. Private world draws cull actors outside that view before assembling
their packets; canonical simulation and screen-relative menus are excluded.
See [FOUR_PLAYER_RENDERING_QA.md](FOUR_PLAYER_RENDERING_QA.md) for measured
performance, renderer comparisons and the distinction between development
qualification and the optimized playtest executable.

The roster selects a separate trusted netplay profile. Three/four-seat capacity
and complete option tuples enter the existing content handshake. Profile and
roster selection stay fixed during the match; mismatched configurations are
rejected before simulation. Existing two-player sessions retain two seats.
This mode uses delay synchronization. Rewind and user save states are blocked;
native memory cards retain their separate existing save-isolation contract.

## Qualification and remaining work

Windows native LAN checks cover three and four independent processes, native
intro enrollment, all five forms across two rosters, per-seat movement and
attacks, charged X shots, independent HUD/camera draws, and a leading last seat
more than 250 pixels from P1. Matching core/mod CRC samples and render-restore
checks accompany the screenshots. An offline setup lowers P1 to one HP, then
real enemy damage runs the native fatal-hit/death sequence while P2–P4 continue.
Directly setting HP to zero is not a valid substitute for native fatal damage.

The existing two-player native netplay suite also exercises movement, Split
views, both pause-menu owners and disconnect, with renderer rejection checks.
Unit checks cover 180 legal roster tuples, malformed/duplicate selections,
forced mod suppression, profile selection and texture reuse across eight
arenas. Linux checks compile the portable core/package code and run roster,
profile and legacy netplay-policy tests; they are not a Linux gameplay audit.

Local three/four-player checks exercise separate seat input, a leading last
seat, the full canvas through resize/refusal/pause/resume, and composed images.
Both layouts pass render-restore verification with no state leaks, watchdogs
or aborted passes. Framework tests cover declined/capture-failed cells and a
watchdog in the last cell, including recovery without partial publication.

Full campaign, boss/door/script parity, vehicles, all weapon upgrades,
Internet/ICE, spectators, cross-OS/CPU netplay and
long-session cache pressure remain unqualified. The existing two-player
engine's stage-specific fixes do not automatically cover this separate engine.
The current preview confirms one such gap: with P1 waiting behind, P2 leading
in Magma Dragoon does not activate the native volcano emitter, boulders or
slope rumble. The matched P1 control works (`beads-eio.9.244`).
Web Spider also retains a Four camera-area ownership gap: P2's shaft approach
does not admit the native bounds that P1 admits (`beads-eio.9.245`). See
[the current qualification record](FOUR_PLAYER_107_PRERELEASE_QA.md) for the
new binary's checked behavior and remaining boundaries.
P1 retains shared campaign ownership. The intro C4 door and individual pause
menus have bounded native regression coverage. This is an experimental build.

Local play uses an 8:6 canvas: P1 top left, P2 top right; P3 is centered below
them in three-player mode, or uses bottom left with P4 bottom right. Every pane
is 4:3, including P3's three-player camera, with black pillars on either side.
Three-player play captures three native-width views; four-player play captures
four. Every view must succeed before a new frame is published. Each draw restores
the same simulation checkpoint; drawing more views never ticks the world again.
Shared menus/movies and unavailable private draws use fresh canonical images in
the same canvas. Local unified play is deferred and has no selectable option.

The optional `mmx4.coop-free-cutscenes` package is a separate experiment for
both co-op engines. It removes ordinary two-player withdrawal/neutral input
and keeps non-owner controllers/cameras active during a world freeze. Doors,
actual room transports and shared campaign/dialogue ownership are retained.
See [COOP_FREE_CUTSCENES.md](COOP_FREE_CUTSCENES.md).

## Reproduce the bounded online checks

Use a development build with `PSX_DEBUG_TOOLS=ON`, OpenGL, the bundled BIOS,
and the original supported USA disc. The script starts hidden, muted processes
with isolated settings/cards. It never writes simulation RAM online.

```powershell
python tools/test_mmx4_coop4_netplay.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --players 4 --report-dir qa-four
python tools/test_mmx4_coop4_netplay.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --players 3 --report-dir qa-three
python tools/test_mmx4_coop4_netplay.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --roster all-x --report-dir qa-all-x
```

Each report directory must be new. Reports, runtime logs and own-view images
are evidence; the copied executables, assets, private cards and caches are
disposable after review. No original disc is copied or included in the build.

Local split checks use the same hidden, muted isolation:

```powershell
python tools/test_mmx4_coop4_local.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --players 4 --report-dir qa-local-four
python tools/test_mmx4_coop4_local.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --players 3 --report-dir qa-local-three
```
