# MMX4 serial enhancement qualification

Tracking: `beads-eio.9.2`. Owner accepted X4 gameplay and authorized the final
searchlight repair to be accepted through a focused agent check, then merged.

`Play-MMX4.ps1` in `F:/Projects/psxrecomp/parity-review-20261006` selects
`build-native-review`, verifies its exact executable hash and uses separate
review memory cards. Framework: `bf4246f2`; recomp-ui: `03d58aa0`.
Earlier interpolation/fixed-16:9 candidates are retired.

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

The subsequent enemy/CPU-budget changes were owner accepted. A focused native
door check placed the destructible door 674 pixels ahead of the camera and
reduced its original HP from 32 to 29 using real shots. The disabled damage
cheat preserves the original calculation. Guest cycle scale is 2 during
gameplay only; device clocks keep their original timing. OpenBIOS boots the
current review build.

The intro controller has a separate twelve-record searchlight table. Its
original scanner/allocator/latches now cover each layer's adaptive activation
rectangle, including stationary aspect changes. The original beam visibility
function's two horizontal tests widen with the shared view margin; vertical
tests, geometry, animation and retirement remain native. At 4:3 both hooks
fall through or make no changes.

A bounded original-disc/OpenBIOS check observed three native beams at 4:3 and
five after a stationary 32:9 reveal. Two active/drawn beams failed the original
4:3 visibility tests. Restoring 4:3 reduced the set to three; widening again
restored five. The wide image was inspected. Guest frames advanced, the modern
renderer remained at its 1080p preset, and the process exited cleanly. Evidence:
`receipts/MMX4.searchlights.json` and `MMX4-searchlights-wide.png` in the review
folder. Release-config checks passed; this is a light targeted check, with
audio/controllers disabled, not a performance benchmark or a full stage sweep.

Possible sound delay remains unmeasured. Broad stage/platform coverage and a
published release remain separate work.
