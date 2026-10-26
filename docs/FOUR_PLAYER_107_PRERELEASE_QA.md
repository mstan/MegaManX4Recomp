# Four Player Co-op preview 1.0.7-4p-spike.1

This Windows x64 prerelease belongs only to `spike/four-player-coop` and
framework branch `spike/mmx4-four-player`. Stable 1.0.7 remains separate and
latest. The new preview incorporates stable 1.0.7, centers a native-width P3
pane in three-player mode, and qualifies stored-shot HUD ownership across all
four seats. It remains an experimental spike, with known stage gaps below.

## Qualified runtime

| Component | Identity |
| --- | --- |
| Runtime game source | `b672ed435bc614948a47d932fdd6134f546e16b8` |
| Framework | `2f3f38403250b2937cf61179a17dc648c3392765` |
| recomp-ui | `b964eabe5a3438b8ed121064354ba27c1accef3f` |
| Development executable SHA-256 | `f6ae5f45f8f81b061d56703ba4f8d8c4d66e4255c804fd61b5b06ff24b17e47c` |
| Production executable SHA-256 | `dfbe5f2b5fafc010369a8065b66986a502939a0f59a8b759ad04516f39673570` |

Both builds use ENHANCED execution, HLE GL batching, four-player capacity,
and the public release channel. Both pass 45/45 CTests. Production disables
debug tools and netplay development cheats. Later feature commits add tests,
evidence and documentation; `BUILD_INFO.json` separates the runtime source
commit from the release commit. The framework pin remains unchanged during
qualification.

## Native checks

- Three-player and four-player real keyboard controls, independent leading
  cameras, private X/Zero menus and individual cursors, pause/resume, fresh
  fallback and resize pass. Actual composed PNGs establish centered 4:3 P3
  with black pillars and the unchanged 2x2 grid at 960x720 and 1280x720.
  Offline/online camera-option visibility also passes. See
  [the presentation evidence](THREE_PLAYER_CENTERED_QA.md).
- Force Armor X charges four native stocks and consumes 4/3/2/1/0 through
  distinct normal keyboard input in every owner seat across four legal
  three/four-player rosters. Only the owner pane shows the original icons;
  Zero/Black Zero and other X forms do not inherit them. Refused private
  passes preserve the correct canonical HUD. All 13,681 restoration checks
  pass without mismatches, aborts, watchdogs, VRAM or state leaks. See
  [the stock-HUD evidence](COOP_STORED_CHARGE_HUD.md).
- The declared offline intro door/free-cutscene scenario passes, including
  non-owner control during the frozen scene and actual native next-section
  transition. Its setup and HP/damage-grace limits are recorded in the
  presentation evidence; it does not establish every door or script.
- Both two-player rosters pass damaging Magma descents with either leader,
  and Web Spider's P2 survivor-ladder descent/return after P1 dies. Those
  checks apply to Two Player Co-op, not automatically to Four Player Co-op.
- Fresh extracted production copies boot the original USA disc using the
  authored portable configuration, with no loose-EXE rewrite or RAM writes.
  All three/four seats enroll alive, move independently, resize the hidden
  client, open personal menus and resume. Gameplay pixel PNG evidence is
  development-build evidence; production checks use read-only native state
  and window observations, with recording off.
- Four independent production LAN processes use the extracted public payload,
  original-disc bootstrap and native keyboard input. X/Zero/Force X/Black Zero
  pass independent movement/attacks, P3's native 0/4/3/2/1/0 stock cycle, P4's
  leading camera, private menu cursor ownership and resume. All 113 common
  core/mod CRC samples match through tick 3616. No simulation RAM writes,
  debug fixture commands or online liveness bypasses are used. This is a
  same-host Windows LAN check, not Internet or cross-platform qualification.

## Portable package

The clean catalog contains 15 packages. Four Player Co-op, Free Cutscenes and
diagnostic recording default off. CD Speed and Fast Loading are absent.
Authored settings retain windowed OpenGL, native internal resolution, the
render worker and normal audio. Fresh production launcher/default/camera
context checks pass. Private automated tests are hidden and muted.

The package contains no original disc, Japanese media, personal save/binding
or machine path. Its native toolchain uses the exact feature emitters. Their
actual LLVM DLL dependencies and license are staged; obsolete copied GCC DLLs
are removed. With PATH restricted to Windows system directories, both
packaged emitters, embedded Python/helper CLIs and a compiled/executed TCC
probe pass. The generic shared staging helper's missing LLVM DLL closure is
tracked separately as `beads-eio.3.399`; this archive includes the correction.

The final archive's runtime/configuration/catalog payload is checked against
the qualified candidate. Later documentation changes do not qualify a
different executable. Published archive hashes are in `SHA256SUMS.txt`.

## Known limits

With P1 waiting at the beginning of Magma Dragoon, P2 can descend the slope
without activating the native volcano emitter: no boulders or slope rumble
start. The matched P1-led control activates the same loaded emitter and deals
real boulder damage to the waiting seats. This is a confirmed Four Player
Co-op gap, tracked as `beads-eio.9.244`; the imported two-player correction
does not fix Four's separate stage-controller path.

Web Spider has a separate Four camera-area ownership gap: after equal camera
settling, P1 crossing the shaft trigger admits its native bounds, while P2
crossing the same trigger with the same area actors loaded does not. Tracking:
`beads-eio.9.245`. The actual Four P1-death/survivor-ladder case remains
unverified because P2 could not enter its prerequisite camera area. See
[the bounded stage evidence](FOUR_PLAYER_107_STAGE_QA.md). This is not a claim
that a Four ladder death was reproduced.

Full campaign and all boss/door/script paths, vehicles, all weapons, long
sessions, Internet/ICE and cross-OS/CPU matches remain unqualified. Heavy
gameplay can slow down; no new performance benchmark or full-speed guarantee
is claimed. Local unified play remains deferred. Four-player save states and
rewind remain blocked; native memory-card isolation is unchanged.

Tracking: `beads-eio.9.239`, `.241`, `.242`; known stage follow-ups `.244`/`.245` and
existing performance work `.229` remain open.
