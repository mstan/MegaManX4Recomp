# Four-player heavy-gameplay follow-up, 2026-10-24

This artifact is superseded by [the layer/menu correction](FOUR_PLAYER_LAYER_QA.md).
Its four-player allocations prevented Widescreen's authored-map renderer from
initializing, despite its selected checkbox. The measurements here therefore
describe the retail-ring fallback and must not be presented as performance or
qualification of the complete authored-map path.

The experimental build now uses shared psxrecomp renderer improvements as well
as the previously selected HLE batching and title-owned private sprite culling.
The framework skips wholly clipped native GL primitives and keeps pending
batches through redundant draw-area/offset commands. Wide/precision paths keep
their previous clipping, and fills/transfers are excluded. Simulation and GP0
timing remain unchanged. See framework `docs/MULTICAMERA_RENDER_COST.md`.

The supported USA config also enables the existing strict PsyQ wait-loop
detector at `0x800E4F6C`. This exact store-counter loop can advance to the next
IRQ/device boundary without interpreting each countdown iteration. Clang and
GCC detector tests pass, including disallowed stores, MMIO and invalid loops.
The config is part of the co-op compatibility hash; old peers cannot silently
join this build. No global guest-cycle scale or framework default changed.

A proposed timing-helper inlining experiment was rejected: it slowed the same
native-input route from 55.38/47.81 FPS idle/firing to 46.63/40.42. The original
timing headers and BIOS generation fingerprint are restored. A separate
`PSX_GCS_BATCH` experiment was not selected. Neither is in this handoff.

## Artifact and measurement

The Release ENHANCED/HLE executable has debug tools and netplay development
cheats disabled. SHA-256:
`cfda094ac2a21925d3d4b7475e1d919afdf64f858d51094893d154c8113c92fb`.
Its execution manifest is staged and verified alongside the executable.
The framework runtime commit is `a073846`; the game commit and handoff/card
provenance are recorded in the private qualification receipt.

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-four-player`. Native QA uses
hidden, muted Release processes and private cards. It boots through normal
menus and sends keyboard messages only to its own process. Observations use
read-only Windows process access. No simulation memory is written. A separate
host-PC sampler briefly suspends only owned QA threads; sampled runs are not
used for the controlled FPS figures.

On Ryzen 7 9800X3D / RTX 3080 Ti / NVIDIA 616.86, native internal resolution,
render worker enabled, the first repeatable Intro route measures:

| Build | Idle FPS | Holding attack FPS |
| --- | ---: | ---: |
| Previous Release, `heavy-frame-before` | 55.38 | 47.81 |
| Previous Release plus audited wait loop only, `heavy-frame-before-idle` | 60.00 | 57.90 |
| Current Release, `heavy-frame-final` | 60.00 | 60.00 |

Reports use the `labels-production-qa-` prefix. Input segments and measurement
windows use guest-frame counts; each measurement spans 180 world frames.
These are bounded Intro observations, not a full-speed campaign claim.

A longer route with jumping and intermittent attacks reaches the later street
near x=1800-2300. `heavy-later-before` measured 52.13/45.89 FPS; the current
`heavy-later-final` measured 60.95/53.33. Both pass native movement, menus and
resume with all heroes alive. Enemy interactions and final hero positions
differ, so this supports a practical improvement on that route rather than a
strict identical-world A/B result. The short idle window can exceed the nominal
59.94 Hz slightly through pacing boundaries. Firing still falls below full
speed. The farther x=3000 area from the previous follow-up is not the same
fixture, and its older numbers must not be compared directly with these.

## Qualification

- `heavy-final-ctest.log`: all 44 Windows tests pass on the current artifact.
- `heavy-idle-check.log`: existing audited store-counter detector tests pass
  under Clang and GCC.
- `heavy-gl-hle-r2.log` and `heavy-gl-lle.log`: all six real-context stream
  configurations pass at 1x/4x, worker off/on and dynamic scale changes.
  Native readback, VRAM, hires and displayed composition hashes equal the
  previous fixture. Raw unused wide backing pixels remain tier-dependent,
  as documented in the previous follow-up.
- `heavy-gl-blend.log`: all 22 blend/mask oracle checks pass.
- `labels-production-qa-heavy-frame-final/report.json`: actual Release native
  boot, four shared-keyboard controllers, attacks, private menus and resume pass.
- `labels-production-qa-heavy-later-final/report.json`: the longer native route
  also passes with all four alive.
- `labels-production-qa-heavy-net4-r2/report.json`: four native Release LAN
  peers pass independent movement/attacks, normal and Ultimate X charging,
  P4 leading by more than 250 pixels with a separate camera, private menus,
  P1-only cursor movement and P4 resume. All 128 common core/mod CRC samples
  match through tick 4096. No online simulation memory is written. The first
  harness run checked resume after only 60 VBlanks; the corrected harness
  waits for the actual completed transition. That initial failed report is
  retained and does not qualify the artifact. No production code changed
  between these runs.

The earlier [rendering qualification](FOUR_PLAYER_RENDERING_QA.md) retains
local 3/4P, menu/door/scene/next-section, legacy 2P and multi-peer evidence.
Its old idle-skip exclusion and older FPS figures are historical. Full campaign,
vehicles, overlapping scenes, Internet/ICE and cross-OS/CPU netplay remain
unqualified. Local unified remains deferred. Native memory cards/settings are
preserved; diagnostic recording stays available and unchecked by default.
