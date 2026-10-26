# Four Player Co-op rendering and launcher follow-up, 2026-10-24

This supersedes the earlier LLE handoff in
[FOUR_PLAYER_FOLLOWUP_QA.md](FOUR_PLAYER_FOLLOWUP_QA.md). The branch remains
experimental; this is not a stable release.

The later [heavy-gameplay follow-up](FOUR_PLAYER_HEAVY_GAMEPLAY_QA.md)
supersedes this artifact and its idle-skip/performance conclusions. This file
retains the earlier qualification evidence and remaining campaign scope.
The subsequent [layer/menu correction](FOUR_PLAYER_LAYER_QA.md) found that
four-player packet allocations silently prevented the authored-map renderer
from activating. Earlier visual checks and timings do not establish that path.

The launcher now calls the renderer mod **Widescreen** and the roster mod
**Four Player Co-op**. Offline settings expose Local cameras; the Netplay flow
exposes Online cameras. Returning to the dashboard restores the offline
controls. Context is presentation metadata: hidden selections remain stored
and participate in the existing committed plan. The shared UI source did not
change; the framework uses its existing netplay-mode callback and filters both
package and feature option counts/getters. Package tests cover the context
matrix, legacy defaults and invalid context rejection.

Fresh ENHANCED X4 builds now select the existing HLE GL textured batcher.
Fresh REFERENCE builds retain LLE, and explicit LLE builds remain supported.
The generic framework default is unchanged. Private world draws reject actors
outside their camera before packet assembly, avoiding needless foreign-view
sprites and wrapped GPU coordinates. Canonical simulation, personal menus and
screen-relative layers are excluded from that cull. No new live cycle-scale
or idle-skip setting is shipped.

## Artifact identities

The instrumented Windows regression executable is RelWithDebInfo, ENHANCED,
HLE batching, debug tools enabled, with SHA-256
`7359e09c6eaf17f8b0bbc9ed569189a41a68496d4310ec7223709ad3d38fbef5`.
It supplies the debug-control, CRC and render-restore evidence below.

The optimized owner playtest is Release, ENHANCED, HLE batching, with
`PSX_DEBUG_TOOLS=OFF` and `MMX4_COOP_NETPLAY_DEBUG=OFF`. SHA-256:
`621ba85f496ff30ca755eadc694e91e9031a5f3c444cc401673dc9f39ca6a354`.
Optional Input Display and Recording remains available and default-off.
Normal audio, windowed mode, native cards and the owner's identical keyboard
map across all four seats are preserved. The render worker is enabled in the
playtest settings. Each handoff includes its execution manifest and provenance.

These binaries were built before the final commits from the same runtime
source content; final changes after compilation were documentation, commit
metadata and the framework submodule pin. The framework source is `3748851`.
The exact game head and binary hashes are also in the private qualification
receipt and handoff provenance.

## Measured behavior

Host: Ryzen 7 9800X3D, RTX 3080 Ti, NVIDIA 616.86, 165 Hz panel. Native internal
resolution and the render worker are used. The previous owner game remained
open and paused throughout these measurements; other host load varies.

| Repeatable instrumented Intro setup | Idle FPS | Holding attack FPS |
| --- | ---: | ---: |
| LLE control, `labels-performance-before-rt` | 17.96 | 18.46 |
| Final HLE + private cull, `labels-performance-final-rt` | 31.81 | 35.36 |

The fixture places the four heroes at x=1100 plus per-seat spacing and protects
them. It is setup evidence, not an input-only campaign playthrough. A separate
audit of the exact PsyQ VSync wait loop skipped about 3.05 billion guest cycles
but measured only 18.21/19.15 FPS. That override is excluded. World-update and
VBlank deltas are approximately equal in the baseline and candidate, pointing
to host execution/rendering cost rather than extra simulated frames.
Synthetic player/camera passes already have the scoped ENHANCED cycle policy
documented in the earlier follow-up.

`labels-production-qa-r2/report.json` exercises the actual optimized executable
without a debug server or simulation-memory writes. It boots through native
menus, enrolls the default four heroes, moves all four using the same Windows
keyboard events, uses native attacks/movement to progress, and opens/resumes
their menus. All remain alive. The naturally reached later Intro combat area
measured 31.75 FPS idle and 25.38 holding attack. This is a different live world
from the protected fixture and is not a direct A/B comparison. Its observer
uses read-only process access and window-local messages for its own hidden
QA process; it does not inject global keyboard input or touch the owner's game.

Busy 4P gameplay remains below full speed. No 60 FPS claim is made. Remaining
host-rendering work stays tracked in `beads-eio.9.229`.

## Qualification evidence

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-four-player`. Automated
processes are hidden, muted and use disposable cards. Original discs are
referenced in place.

- `labels-final-ctest.log`: 44/44 Windows tests pass.
- `labels-final-portable.log`: portable core/package compilation and
  roster/profile/policy/sandbox checks pass. This is not Linux gameplay QA.
- `labels-final-gl-blend.log`: all 22 real-context blend/mask checks pass.
- `labels-final-gl-stream-hle.log`, `labels-final-gl-stream-lle.log` and
  `labels-gl-cross-tier.json`: the 80-frame GL stream passes at 1x/4x, with
  the render worker off/on and dynamic scale changes. Native readback, VRAM,
  hires and actual pre-swap composed hashes match across tiers. All six
  nontrivial composed images have zero changed RGBA bytes. Raw wide backing
  textures differ in unused composition content; this is not a presented
  pixel difference. The fixture now explicitly captures the retained composed
  frame before resolution cleanup invalidates it.
- `labels-final-local-4/report.json`: independent input/cameras, private
  menus/cursors, resize/refusal/resume, optional cutscene movement, the native
  Intro C4 door and actual next-section load pass.
- `labels-final-local-3/report.json`: the three-seat layout, wide bottom
  camera, menus/input, resize/refusal/resume pass.
- `labels-final-two/report.json`: existing personal-menu, local-pair and
  local-canvas cases pass, including current-build paused save/load.
- `labels-final-free-scenes/report.json`: the separate addon passes native
  scene/frozen movement, scene release and physical-door coverage for 2P.
- `labels-final-online-4-r2/report.json`: four native peers, including normal,
  Fourth Armor and Ultimate X, pass per-seat movement/attacks/charge, separated
  cameras and private menus/resume; 113 common matching core/mod CRC samples
  through tick 3616. No simulation RAM is written online. The initial run
  lost peer liveness under concurrent build load with no preceding CRC
  disagreement. The retry uses the default timeout and passes; the failed
  receipt remains available.
- `labels-final-online-3/report.json`: the default three-hero roster passes
  the same native LAN movement/attack/charge, separated-camera and private
  menu/resume checks; 113 common matching core/mod CRC samples through tick
  3680. The default liveness timeout is retained.
- `labels-ui-final`: screenshots from the optimized executable show the
  requested names and offline/online/offline-return option visibility.
  Diagnostic recording is unchecked. Stored roster/camera selections survive.
- `labels-production-qa-r2/report.json`: actual Release keyboard/menus/speed
  check passes as described above. The earlier probe's fixed pause-init delay
  was insufficient; it also stopped at an uncleared barrier. Its report is
  retained as failed and is not used to qualify the corrected run.

The final independent-camera and four-menu composed images were visually
inspected, including matching X/Zero artwork and Ultimate X's Nova Strike.
The offline-return screenshot restores Local cameras and keeps the requested
roster. These are bounded Intro/launcher observations, not full campaign QA.

Full campaign, overlapping scenes, vehicles, Internet/ICE and cross-OS/CPU
gameplay remain unqualified. Local unified remains deferred. Four-player
save states/rewind remain blocked; native memory cards are preserved. The
cutscene addon remains a separate default-off package. CD Speed and Fast
Loading packages are absent from the handoff.
