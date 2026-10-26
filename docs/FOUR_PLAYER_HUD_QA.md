# Four-player red damage trail

The owner's screenshot showed the native red lost-health trail staying visible
above the green health fill. Green fill matched actual HP. A read-only snapshot
of the owner's game found these independent values:

| Seat | Actual HP (`+5C & 127`) | Display trail (`+5D`) | Delay (`+5F`) |
| --- | ---: | ---: | ---: |
| P1 X | 29 | 29 | 0 |
| P2 Zero | 14 | 32 | 0 |
| P3 Ultimate X | 24 | 32 | 0 |
| P4 Black Zero | 15 | 32 | 0 |

Native `800253F0` emits the fill/trail, then `800254AC..8002555C` advances the
trail and delay. P1's canonical HUD commits those changes. Extra players only
drew their HUD inside private render transactions, which restore all state
after drawing. Their displayed-health trail therefore remained at its spawn
value forever. Previous full-health HUD checks did not expose that difference.

The canonical four-player HUD hook now advances each additional seat's trail
alongside P1's original HUD. The small portable helper matches the native
state-only update: healing catches up immediately, marked damage starts the
native delay, and the trail then decreases once every fourth native frame.
The same native HUD/world visibility gates apply. Actual HP, armor, maximum
health, death handling and every other body field are untouched. Private
rendering keeps its complete state restoration. View count and local/online
ownership do not select which seats receive updates.

## Evidence and scope

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-four-player`.

- `hud-qa-before/report.json`: the native retained baseline reproduces the
  failure. After health fixtures and 96 additional simulation frames, actual
  HP is `[29,14,24,15]`, but trails are `[29,32,32,32]`. Its full-health pixels
  pass; its damaged-state assertion fails. `failure.png` records the stuck red.
- `hud-owner-evidence`: read-only diagnostic bytes and screenshot pixel
  observations; no input or writes to the owner's process.
- `hud-portable.log`: GCC and Clang pass the helper fixture, including the
  owner's distinct values, damage delay, quarter-rate decay, healing and
  preservation of all body bytes except the trail/delay.
- `hud-ctest.log` and `hud-release-ctest.log`: all 45 tests pass on the
  instrumented and final Release configurations. The initial build driver
  referenced a nonexistent CTest beside the standalone compiler tools; both
  suites were then run successfully with the installed native CMake CTest.
- `hud-qa-final-4/report.json`: X/Zero/Ultimate X/Black Zero pass independent
  values, transient red after damage, green/critical fill heights, healing and
  progress while private views are refused. All 1,740 restore checks pass
  without mismatches or aborted transactions.
- `hud-qa-final-3-force/report.json`: the same checks pass with Force Armor X
  in the continuous bottom camera. Its 1,720 restore checks have zero
  mismatches or aborted transactions. Together these runs cover all five forms.

The first final-Release input smoke stopped when its old foreground guard
rejected an OS foreground-window change. It is retained as an interrupted
attempt, not a gameplay pass. The helper now checks that its input windows
remain hidden and no QA peer owns foreground focus, while allowing the owner
to switch unrelated applications. The shipping game is unchanged by this
test-helper correction. Evidence: `labels-production-qa-hud-final/report.json`.

The native HUD harness uses private hidden, muted copies and offline health /
damage-grace fixtures. It checks rendered green and critical fill heights,
red trail appearance and disappearance, and independent health changes. Those
fixtures isolate HUD behavior; they do not qualify damage collision, death,
doors or stage progression. The owner withdrew the suspected shared-death
report; this change addresses the red trail only.

The final Release binary is SHA-256
`8fd255da5480077857cd82604736603c27209cf636e679344c384f001a00d5fe`:
ENHANCED/HLE, debug tools and development netplay cheats off; framework
`a0738464afe8bdf8cb2e9e1eb939699e37027ac4` is unchanged.
`labels-production-qa-hud-final-r2/report.json` passes native shared-keyboard
movement, combat, four private menus and resume with the complete authored
background arena active. Its 180-frame idle and firing windows both measure
60 FPS on this run. This is not a controlled performance comparison with the
previous 34.39 FPS sample; host/route conditions differ, and heavy gameplay
remains tracked in beads-eio.9.229.

`labels-production-qa-hud-net4/report.json`: four native Release LAN peers pass
per-seat input/charging, P4 leading camera and private menu ownership/resume.
All 106 common core/mod CRC samples match through tick 3424. Native input is
window-local, all QA windows remain hidden, no QA peer may own OS foreground
focus, and guest state is observed read-only. Ordinary network liveness and
build/capability checks remain enabled. No new cross-build or cross-OS/CPU
compatibility claim is made.
