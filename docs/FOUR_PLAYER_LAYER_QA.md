# Four-player scrolling layers and Options correction

The reported blue-background seam came from failed renderer initialization.
Four-player co-op reserved eight 256 KiB sprite slots and a 64 KiB HUD area.
The authored-map renderer then requested six 1 MiB planes: all three scrolling
layers in both native buffers. Their combined 8.0625 MiB exceeded the fixed
8 MiB GPU DMA aperture, so renderer activation silently returned. Selecting
Widescreen in the launcher did not prove that its hooks were running. The
game continued using the shared native rings, whose distant-camera contents
could leave a fixed seam during scrolling.

Each sprite slot now reserves 128 KiB. The existing bound remains 1,000 native
40-byte quads plus two expanded 40-byte triangles per quad: 120,000 bytes total.
No primitive limit is reduced. All eight texture-cache scans use the same
shared stride. Four-player sprites, HUD and every background plane now fit in
7.0625 MiB, in either activation order. Allocation failures are logged and
counted; successful activation and each plane's actual tile production have
separate counters. Native local and LAN harnesses require these counters.

Working renderer activation also exposed an order dependency: Widescreen's
adaptive setting could replace co-op's native cell aspect after co-op activated.
Offline Split keeps native 4:3 camera cells while installing all background
hooks. The compositor retains its 8:6 canvas, including the continuous wide
bottom camera for three players. Ordinary unified play retains adaptive reveal.

Black Zero in Button Config was a separate world-sprite leak. Native address
`0x800241E8` clears object ordering lists for gameplay and frontends. The
four-player hook appended teammates after every such call in mode 6, including
Options. Depending on their world coordinates, a teammate appeared over the
menu; it was not a character preview. Both four-player sprite hooks now use
the existing two-player world/menu predicate. Options and Quit exclude these
sprites, while native pause fades can still draw the real world.

## Qualification and boundaries

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-four-player`. Automated games
are hidden and muted with private cards. Owner game state was read without
input or writes; its process had already closed before the updated handoff.
No native card or owner setting is discarded.

- `layers-combined-ctest.log`: all 44 Windows tests pass on the instrumented
  Release build. The background fixture reproduces the old allocation failure,
  exercises both activation orders and resolves all three planes in both
  buffers while their native rings contain unrelated tile IDs. Texture-cache
  tests retain live IDs through all eight resized slots and restored timelines.
- `layers-release-ctest.log`: all 44 tests also pass on the delivered Release
  configuration, with debug tools and development netplay cheats disabled.
- `layers-portable.log`: the background and cache fixtures pass under Clang
  and GCC on Linux. This is portable code coverage, not Linux gameplay QA.
- `layers-qa-final-3/report.json`: three-player native follow scroll crosses
  2016 → 2128 for planes 0/1 and 1008 → 1064 for plane 2. Every plane produces
  authored tiles, cell widening is zero, and 2,940 render-restore checks have
  zero mismatches or aborted passes. The bottom camera spans both tiles.
- `layers-qa-final-4-r2/report.json`: the same explicit boundaries pass with
  four independent camera panes. All planes are active; 2,920 restore checks
  have zero mismatches or aborted passes. The fully faded Button Config image
  contains no gameplay hero. Native UI frames advance without any added world
  sprite pass. Start returns to the four X/Zero menus and then to gameplay.
- `layers-qa-two-split/report.json`: legacy two-player Split personal menus
  pass independent weapon/tank management, healing locks, paused snapshot
  restoration, either-player resume, P2 Quit, Options return and firing the
  selected native weapon. Its 3,482 restoration checks have zero mismatches.
  The subsequent moving-pair scenario reaches separated cameras but times out
  closing pause; the remaining canvas scenario does not run. These cases are
  not counted as passes. An additional Unified/Widescreen menu capture is black
  on both the current build and the retained baseline. Follow-up tracking is
  beads-eio.9.233; neither failure is hidden by weakening capture assertions.

These render fixtures position actors on observed Intro street coordinates,
then let native camera follow cross the ring boundaries. This is rendering
coverage, not validation of bypassed barriers, bosses or stage progression.
The blue glow itself is authored artwork, also present in the original native
reference. The correction removes stale camera content rather than the glow.

An intermediate allocation-only build widened the cells and repeated the
canonical view despite successful transaction counters. It is excluded from
qualification; the final check requires native cell geometry and independent
views. Initial short captures caught native fades or unsettled cameras and
are retained only as failed/incomplete evidence. The first instrumented HLE
cold launch hit its startup watchdog before a guest frame; corrected native
resolution launches pass. No watchdog is disabled for qualification.

Earlier four-player timing figures lacked renderer-activation assertions and
describe the fallback path. They do not qualify full-map performance. Full
campaign content, vehicles, overlapping scenes, Internet/ICE and cross-OS/CPU
gameplay remain unqualified. Local unified remains deferred; recording remains
optional and default-off. This stays an experimental branch, not a release.

The final Release binary is SHA-256
`00f1ba31a4c83a3396684505d4546d9a5ebc844efac6b4f809bca3fa06127c28`:
ENHANCED/HLE, debug tools and development netplay cheats off, framework
`a0738464afe8bdf8cb2e9e1eb939699e37027ac4`. Its native input-only smoke
(`labels-production-qa-layers-final/report.json`) requires the complete 6 MiB
background arena, moves every seat with the shared keyboard, attacks, opens
all four private menus and resumes. The 180-frame windows measure 59.70 FPS
idle and 34.39 FPS during charge/fire. These are one route on this host, not
full-game performance qualification. Heavy gameplay remains beads-eio.9.229.

The first final-Release LAN attempt aborted during boot when the private-input
harness observed a foreground-window change. It stopped its own four peers;
no CRC or gameplay pass is claimed for that attempt. The foreground guard
remained enabled for the retry. Evidence: `labels-production-qa-layers-net4`.
The second attempt repeatedly pressed Start after reaching gameplay and paused
the initial arrival before its lock cleared. Its own peers were stopped after
read-only observations confirmed the setup error. The helper now stops boot
input at gameplay and waits for the native arrival lock to clear, then checks
real per-seat movement and attacks. No simulation state or success flag is
written to bypass arrival. Evidence: `labels-production-qa-layers-net4-r2`.

Four final Release LAN peers pass native enrollment, per-seat movement/attacks, X charging, P4 leading camera, private cursor ownership and resume. All 107 common core/mod CRC samples match through tick 3424. Every peer has the full authored background arena active. The ordinary liveness and foreground guards remain enabled; peers use native input and read-only observations. Evidence: `labels-production-qa-layers-net4-r3/report.json`.

The subsequent red damage-trail correction and latest artifact are documented
in [FOUR_PLAYER_HUD_QA.md](FOUR_PLAYER_HUD_QA.md).
