# Local side-by-side co-op

Local co-op now has a separate Local co-op display dropdown: Unified (default) or
Side by side 8:3 (Experimental). Each pane uses a native4:3 camera and the
corresponding character's HUD. The shared world still updates once. Online
camera selection remains the separate Netplay cameras option.

The title reuses its existing independent-camera world admission and authored
map rendering. Two own-view draws start from the same CPU/RAM/device/VRAM
checkpoint; both must finish before a complete pair is published. A failed
pair falls back to the canonical frame. Native menus and shared story locks
use one canonical view; normal rendering resumes after the menu closes.
The game restores host contexts around each callback and around the entire
pair, including callback watchdog unwinds. The generic API rejects online
sessions even when generic temporal replay has been enabled.

## Qualification

Original-disc, hidden-window, muted Windows check in
build-pr23/local38/native-input:

- Both characters visible with their own HUDs in the1280x480 composition.
- P1 stationary at500/427 while controller-driven P2 reaches1194.75/333.15,
  with both healthy. The setup fixture positions the nearby pair; the distant
  separation itself uses jump/dash/right inputs.
- Native pause UI uses one4:3 view; unpause resumes two panes.
- Resizing to960x720 preserves8:3 composition with letterboxing.
-848 verified render transactions,0 mismatches, watchdogs, aborts, rejected
  device reads or VRAM leaks;424 complete pairs.
-36/36 title CTests pass. Windows and Linux sandbox/abort helpers pass,
  including failure of the second view, scope cleanup, recovery and online
  refusal. This is not a full Linux title or cross-platform netplay qualification.

The first pause check sent Start before the opening animation finished. The
corrected check waits for the native menu and resumes with controller input.
The first distant screenshots preceded the camera's catch-up after a700px
teleport; later checks wait for each player to be inside its own viewport.

## Remaining limits

Shipping as an opt-in experimental display in Playtest38, authorized by the
owner after offline native validation. A human two-controller feel/performance
check remains useful; the automated movement uses accepted controller input.
This does not claim retail-wide visual perfection.

A band of incorrect-looking tiles at the top of Jungle's distant view remains
visible with normal P2 movement. The same band appears in a same-location
native tile-ring Unified control with Custom Renderer disabled (both players
at1296/395). That control also used a position fixture, so it does not prove
retail authenticity or conclusively identify the cause. This observation is
recorded rather than presented as a fixed rendering bug. A speculative
CPU-authority policy change was removed after this control; the normal offline
GPU policy is retained. CPU mirror synchronization before a local capture is
retained for callbacks that read native texture/palette bytes.

Source review also identified native-wide per-layer view caches absent from
the existing GPU checkpoint. Their relevance to the4:3 band is unproven; this
prototype does not claim to have repaired that separate cache concern.

Private screenshots and reports are retained beside the native run, including
local-close.png, local-distant.png, local-pause.png and local-resized.png.
GitHub32/39/41 intake is recorded separately in COOP_GITHUB_REVIEW38.md.

The optional32:9 request is tracked as beads-eio.9.195. Native-wide per-pane
capture and background-cache isolation require further implementation; changing
only the output aspect would stretch the existing4:3 panes.

## Release38 offline follow-up

The additional hidden, muted original-disc check explicitly confirms
netplay_status.active=false. Controller-driven P2 separation reaches1221.625
while P1 remains500. Both retain32HP and their own HUDs. Pause/resume and resize
pass;880 verified render transactions report0 mismatches, aborts, watchdogs or
VRAM leaks. A native fatal-health request kills P1; P2 moves and the complete
pair continues publishing, following the living seat, with no host-state leak.

A separate native intro arena approach enters a shared script: pair mode0 and
no stale pair during the scene, then valid mode2 after both players regain
control. Composed screenshots are in build-pr23/local38/story-smoke.
The first survivor assertion read the diagnostics envelope instead of its
published field; the corrected run is release-smoke-v2. The long walking
intro probe did not reach Dragoon within its time bound and is not shared-scene
proof; the authored arena approach above supplies that check.

Windows production packages and the Linux AppImage are built from the same
committed source and framework pin. Windows and Linux helper tests cover the
sandbox/abort/online-refusal behavior. A human two-controller feel check is
still welcome, particularly for performance at higher internal resolutions.

## Follow-up: offline performance report

Enhanced builds now skip the retail 21x16 background packet producer **inside
protected offline view callbacks**: the authored-map renderer replaces that
list immediately afterwards. The ordinary canonical draw and online paths keep
their native producer. Invalid map setup falls through to the original function;
REFERENCE builds compile the optimization out. Both branches compile on Linux.
No frame skipping, simulation timing changes or weaker rollback checks are used.

Hidden, muted original-disc Web Spider comparisons on a Ryzen 7 9800X3D and
RTX 3080 Ti, with the 1080p setting and render verification disabled for timing:

| Same-toolchain build | Nearby players | Separated players | Guest draw per view, nearby / separated |
| --- | --- | --- | --- |
| Original producer | 58.84 FPS | 59.73 FPS | 0.810 / 0.750 ms |
| Optimized producer | 59.97 FPS | 59.96 FPS | 0.505 / 0.563 ms |

All 36 title tests pass. A separate native-resolution run enables full render
verification and checks player separation, pause/resume, resize and P2 surviving
P1's death with no state mismatch, watchdog, aborted pass or VRAM leak. Baseline
and candidate composed screenshots were inspected. Private measurements and
images are under `build-pr23/local38-perf/`.

For a new report, request CPU/GPU, stage and location, video settings, whether
Unified is smooth, and the entire matching `saves/diagnostics/session-*` folder.
Unified remains the default and the immediate workaround. This follow-up is
not a new published release.
