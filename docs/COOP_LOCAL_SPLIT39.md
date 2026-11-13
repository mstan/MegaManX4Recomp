# Local split-screen follow-up for Playtest 39

Tracked centrally as beads-68fs and framework beads-eio.3.360.

The selected offline 8:3 layout now lasts for the game session. Fresh private
camera pairs show each player's view; menus, movies, shared scenes and refused
or stale pairs show the current canonical image in both 4:3 panes. The guest
camera and simulation retain native 4:3 geometry. Disabling the layout or
starting another session resets the presentation request.

Local split now requires the authored-map renderer, even when the standalone
Custom Renderer switch is disabled. Unified and disabled co-op retain the
player's renderer preference. This is a rendering dependency of independent
cameras, not a new enhancement switch.

## Reproduction and qualification

Hidden, muted native original-disc A/B checks hold P2 on Jungle's first
platform at (56,138), then move P1 to the waterfall at (500,427). With Custom
Renderer disabled, the first platform breaks into floating fragments while
P2 remains stationary. With the authored-map renderer enabled, its complete
terrain stays intact. Both players retain 32 HP in these checks.

The reusable `local_canvas` scenario checks actual composed 8:3 presents from
startup, separated gameplay, forced pair refusal, pause/resume and resizing.
It captures the panes for visual inspection and checks protected render-pass
state restoration. Run with `--local-cameras split --hidden-window`; omit
`--custom-renderer` to exercise the newly required renderer dependency.
Production builds do not expose these development fixtures.

The separate severe-lag report remains unconfirmed (beads-eio.9.196).
Residual Jungle background banding and native-wide 32:9 panes remain separate
work; this change does not claim all experimental split-screen visuals fixed.

Windows native candidate qualification passed with the standalone renderer
explicitly disabled: the dependency supplies authored tiles, and visual
inspection confirms P2's complete stationary platform and two matching pause
panes. Startup, pair refusal, pause/resume and resized composed viewport records
retain 8:3. The run completed 624 protected-view verification checks with zero
mismatches, aborts, watchdogs or VRAM leaks. Source catalog resolution also proves
the requirement does not force the renderer for Unified or disabled co-op.
