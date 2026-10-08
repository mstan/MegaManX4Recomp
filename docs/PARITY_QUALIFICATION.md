# MMX4 serial enhancement qualification

Tracking: `beads-eio.9.2`. Owner acceptance of this replacement remains open.

`Play-MMX4.ps1` in `F:/Projects/psxrecomp/parity-review-20261006` selects
`build-native-review`, verifies its exact executable hash and uses separate
review memory cards. Compiled source: `a0ed6c3`; framework: `e740b81c`;
recomp-ui: `03d58aa0`. Earlier interpolation/fixed-16:9 candidates are retired.

Defaults: OpenGL, 1080p internal resolution, nearest pixel-art sampling,
Custom Renderer / Adaptive and resident ARC loading. Interpolation is absent
from compilation/catalog. Generic CD/host pacing is unavailable. Damage is one.

The retired tile-ring widening path is replaced with the shared authored-map
renderer. X4 supplies original layer/OT/map layouts, integer parallax mappings,
HUD classification and ordinary placement bounds. Adaptive and fixed choices
all use this replacement. Original rings/packet capacities are unchanged.
See [serial review](SERIAL_REVIEW.md) for loading contracts and limits.

The runtime and historical native helpers compiled; the executable is bound to
its ENHANCED receipt. Focused shared geometry and existing release-config checks
passed. A bounded hidden normal New Game reached actual X intro-stage gameplay,
generated modern stage tiles with wide reveal, served resident requests and
exited cleanly without invalid-layer errors. Raw samples/screenshot are
`receipts/MMX4-modern-smoke.json` and `MMX4-modern-smoke.png` in the review folder.
Audio/controllers were disabled. This is not a performance benchmark.

Owner checks: X and Zero intro, run/jump through scrolling edges, resize the
window, inspect layers/enemies/HUD, pause/resume, boss door, death/retry,
save/reload, effects and music. At most two owner review passes.

Possible sound delay remains unmeasured. Wide-ratio transitions, special
placements, asynchronous actor loading, broad performance/stage coverage,
Linux packages and final integration remain outside this first pass. Earlier
"loads OK" feedback does not approve the replacement renderer.
