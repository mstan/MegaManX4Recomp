# Three / four-player co-op spike

This branch starts from stable 1.0.6. It adds a separate, default-off
`mmx4.coop4` mod. Offline play uses independent split cameras. Online play has an individual
4:3 camera and HUD for each participating peer. All peers must enable this
mod, use the same build, and agree on the roster before starting.

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
independent personal menus, Internet/ICE, spectators, cross-OS/CPU netplay and
long-session cache pressure remain unqualified. The existing two-player
engine's stage-specific fixes do not automatically cover this separate engine.
P1 retains the shared campaign/menu role. P2–P4 enroll after P1's initial READY
arrival has installed the landing geometry. This is a spike, not a stable release.

Local play uses an 8:6 canvas: P1 top left, P2 top right; P3 spans the bottom in
three-player mode, or uses bottom left with P4 bottom right. The bottom camera
in three-player mode covers twice the horizontal world area. It is drawn as two
contiguous native-width tiles from one camera, preserving pixel and glyph shape,
with one HUD at the left edge. Placement/culling includes that wider visible area.
All four tiles must succeed before a new frame is published. Each draw restores
the same simulation checkpoint; drawing more views never ticks the world again.
Shared menus/movies and unavailable private draws use fresh canonical images in
the same canvas. Local unified play is deferred and has no selectable option.

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
