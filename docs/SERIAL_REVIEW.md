# Mega Man X4 serial review candidate

This candidate targets SLUS-00561 (USA), with owner validation deferred.
Preparation is serial and solo, with at most two owner review passes.

The enhanced build serves nine verified blocking callsites from a resident
catalog of all 138 ARC table records (130 typed archives and eight raw files).
The original receive callbacks parse the headers, copy RAM, populate the
deferred GPU/sound queue and finish with their own Pause/completion protocol.
The adapter drains that original queue and runs DrawSync before reusing sector
buffers. Loading-screen waits remain in the game; their drive transfers are
batched instead of accelerated through generic CD timing or host pacing.

The two asynchronous actor loads retain original drive delivery. Movies and
XA audio are outside the resident catalog. Unknown callers, modified assets,
relocated table entries, or changed loader code use the original loader before
any resident transfer. Nine callsite instruction sequences, the file table
record and SHA-256 hashes of the loader and SDK protect the ABI contract.

The fixed ENHANCED build selects `mmx4_resident.cpp`. REFERENCE selects the
matching registration without HLE filters and retains the original CD loader.
There is no runtime execution-profile selector. The generic sector view and
resident cache live in psxrecomp; title addresses, contracts and queue policy
remain here. Mode 2 EDC/ECC are synthetic and the guarded callback ignores them.

Default review features are 1080p internal rendering, adaptive scene reveal
and resident asset loading. The old fixed-16:9 tile-ring expansion is retired
under `development/retired-widescreen`, outside active configuration/catalog.
Custom Renderer 1.1.0 selects Adaptive by default. Its fixed 16:9, 21:9 and
32:9 choices use the same replacement renderer.

The shared psxrecomp authored-map renderer emits original stage tiles into a
double-buffered host DMA arena. X4 supplies its three layer layouts, native OT
groups, integer parallax mappings and draw bounds. The original 21-column
renderer, 32-column ring and packet capacity are unchanged. Ordinary actor
placement and draw/lifetime bounds expand with the view; special/boss placement
retains original policy. Screen-relative objects retain HUD tagging. The 2D
sprite/tile renderer uses nearest sampling; 3D PGXP is not applicable.
Damage remains one.

Compiled source is `a0ed6c3`, framework `e740b81c`. Focused shared geometry and
release-config checks passed. Bounded hidden normal New Game reached actual X
intro-stage gameplay (state 6), generated additional stage tiles with positive
wide reveal, served resident requests and exited cleanly with no invalid-layer
errors. Audio/controllers were disabled; sound delay, full-stage transitions,
wider ratios and long-play performance remain for owner review.

Interpolation and generic loading source/packages are archived outside the
compiled sources and bundled catalog. Their baseline is preserved on
`archive/mmx4-interpolation-20261007`. They are unavailable in this build.

Owner checks: start as X and Zero, run/jump through the intro, watch background
edges and the HUD, cross a boss door, die/retry, save/reload, and listen to music
at startup and a stage transition. First-use resident preparation can take
longer than subsequent launches. Do not count this source preparation as
owner acceptance or full-game loader, audio and performance qualification.
