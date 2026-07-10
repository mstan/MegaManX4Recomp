# Mega Man X4 widescreen background hook sites

Verified against `SLUS_005.61_no_header.bin` in Ghidra at image base
`0x80010000`. X4 uses the same Capcom three-layer tile renderer lineage as X5
and X6, with the X5-style inline loop compare and 32-column tile ring.

## Renderer: `0x80026AA0`

The renderer reads three layer records at `0x801319B0`, stride `0x54`, and emits
21 columns by 16 rows of 16x16 background tiles. Its live packet pointer is
scratchpad `0x1F800108`; the shared per-frame tile counter is
`0x1F80011C` and is capped at 1000.

| Setting | Address | Original instruction | Widescreen behavior |
|---|---:|---|---|
| `count_site` | `0x80026C98` | `sltiu v0,t2,0x15` | compare against `21 + 2*LEFT` |
| `startcol_site` | `0x80026B20` | `andi s5,v0,0x1f` | subtract `LEFT`, retain mask `0x1f` |
| `startx_site` | `0x80026B10` | `subu s6,zero,v0` | subtract `LEFT*16` screen pixels |

`LEFT = ceil(per-side reveal / 16)`, which is four columns for the 426-pixel
16:9 surface. The widened loop therefore draws 29 columns and preserves every
native column's original screen position.

## Tile-ring streamer: `0x8002728C`

The streamer updates the leading column on both sides of each of the same three
layers. Moving its edges by `LEFT*16` keeps the renderer's reveal columns from
reading empty or stale 32-column ring entries.

| Setting | Address | Original instruction |
|---|---:|---|
| `stream_left_site` | `0x800272D0` | `addiu a1,s1,-0x10` |
| `stream_right_site` | `0x800272E4` | `addiu a1,s1,0x150` |

The right immediate already includes the native 320-pixel width (`320+16`).

## Packet budget

The driver at `0x80026648` selects a double buffer at `0x8015D9D0` with a
`0x4000` stride (1024 16-byte packet slots). The renderer's cap compare is
`0x80026BB0: slti v0,v1,1000`. Widened dense-stage occupancy must be measured
before raising or relocating this buffer; no unproven RAM relocation is part of
the initial hook. A live Sky Lagoon Area 1 run with the 29-column loop reached
652 packets (`0x28C`) in the sampled dense frame, leaving 348 slots below the
1000-packet guard; the cap was not hit.

## Object draw cull

The shared object-primitive visibility function `0x800D46F4` checks four
world-to-screen vertices. Each horizontal coordinate is accepted when unsigned
`screenX < 0x140` and each vertical coordinate when unsigned
`screenY < 0xF0`. `[widescreen.cull] auto_screen_x` with X4's `0xF0` vertical
signature routes only the four horizontal compares through the aspect-derived
margin helper. This widens draw culling to 16:9 while leaving vertical culling
and every unrelated use of 320 unchanged; the helper is identity at 4:3.

## Object activation and despawn windows

The shared family `0x8002B160`, `0x8002B1E8`, `0x8002B288`, `0x8002B318`, and
`0x8002B3C0` compares object world coordinates relative to the selected layer's
camera (`0x801319BA/BE + layer*0x54`). It either returns the outside-window
predicate or sets the object's visibility/activation byte at `object+3`.

The fixed horizontal windows are exactly `320 + 2*margin` for margins 64, 32,
and 96 pixels; their vertical partners are based on 240 and remain unchanged.
The two parameterized functions use caller-supplied horizontal and vertical
margins separately. Configured `bias_sites`, `range_sites`, and `a1_sites` add
the per-side reveal only to the horizontal margin/range. This keeps enemies and
other actors active through the 16:9 reveal area and delays horizontal despawn
accordingly, while preserving byte-identical comparisons at 4:3.

## Live 16:9 verification

The native-wide compositor produced a 426x240 Sky Lagoon view with populated
background tiles across both reveal margins. After moving right, enemy sprites
were visibly active in the right reveal area. The primitive census for frames
26200-26225 recorded 6,550 primitives outside the native 320-pixel bounds;
non-background textured-sprite packets included X positions 320 through 338.
At the same time the dispatch trace showed the stage calling the parameterized
activation/despawn funnels `0x8002B1E8` (64-pixel caller margin) and `0x8002B318`
(32-pixel caller margin) every frame. The run accumulated zero dispatch misses.

## HUD packet arena and wide anchors

The same 26-frame census isolates the player HUD's double-buffered fixed
16x16 sprites at physical GP0 sources `0x00139834..0x001398A4` and
`0x00139A34..0x00139AA4`. The solid health fill is a mono quad alternating at
`0x00139E34` and `0x00139EAC`. These commands belong to the dedicated
`[0x80139800,0x8013A000)` HUD packet arena; the stage/world packets observed in
the same run begin outside that range.

`nw_hud_corners` is therefore source-filtered to this arena. In native-wide it
translates every piece in a left-third HUD command by `-reveal` and every piece
in a right-third command by `+reveal`. Player health and weapon pieces reach the
true 16:9 left boundary, while enemy/boss health pieces retain their native
layout at the true 16:9 right boundary. The transform is identity at 4:3.

## Review-build performance

The original diagnostics build sampled 256 live frames at 34.94 ms/frame
(about 28.6 fps). External PresentMon capture then established that GPU render
completion was normally below 1 ms; the missing time was on the emulation
thread. A non-invasive stack sample landed in
`psx_dispatch_game_compiled -> dirty_ram_dispatch -> psx_cyc_load_word`.

The cause was X4's generated dispatch source: 13,063 compiled function entries
plus CPS continuations produced 59,904 sparse switch cases. At `-O0`, GCC
lowered that switch to a roughly 72 MB linear compare chain, so common guest
transfers could walk thousands of comparisons. The framework now emits one
sorted `{addr,resume_pc,fn}` table and performs binary search (at most about 16
iterations) for both dispatch and the non-destructive entry probe.

After strict regeneration, the unoptimized Debug build sustained 59.95 fps
over 477 presents in an active 16:9 attract-mode gameplay scene (16.68 ms
average, 17.49 ms p95, 0.15 ms average GPU completion). It accumulated
4,915,433 static dispatch hits with zero misses. The optimized Release build
independently sustained 59.81 fps over 477 presents (16.72 ms average,
0.40 ms GPU completion). Release also defines `PSX_NO_DEBUG_TOOLS`, removing
the TCP/per-block tooling and unused OpenGL timing queries.
