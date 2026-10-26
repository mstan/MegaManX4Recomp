# Initial four-player follow-up qualification, 2026-10-24

Production source: game `495efa4`, framework `aaf4fb1` on the existing
experimental branches. The subsequent game commit `4446ca8` changed only
qualification scripts and documentation. This is the earlier LLE handoff;
[FOUR_PLAYER_RENDERING_QA.md](FOUR_PLAYER_RENDERING_QA.md) records the later
renderer and launcher changes. Windows development executable SHA-256:
`d745e2b59779570283cd1f335454c1f9bea77492dc6afcb195799fbe2bcdbac7`.
The build uses ENHANCED execution, the reference LLE OpenGL batcher and native
internal resolution. It is not a stable release.

The private evidence root is
`G:/Temp/Codex/psxrecomp/x4-four-player`. Automated game processes use hidden
windows, muted output and disposable cards. Original discs are referenced,
not copied. Reports distinguish native gameplay from explicit offline setup.

## Passing current-binary checks

- `ctest-followup-current.log`: 44/44 Windows tests.
- `portable-followup-current.log`: portable core/package compilation, roster,
  profile, legacy policy and render sandbox/watchdog checks. This is not
  Linux gameplay qualification.
- `followup-current-free-scenes-r2/report.json`: the separate optional addon
  in two-player Split, with Zero/P1 and X/P2 owning the native Slash Beast
  scene. P1 moves during the live scene and an explicitly injected world
  freeze. Native dialogue releases the scene; both heroes remain healthy.
  A subsequent physical Jungle door retains withdrawal, carry and grounded
  return. No scene-completion flag is written.
- `followup-current-rt-local-four-r2/report.json`: four separate controllers,
  independent cameras, resize, refused-draw fallback, individual pause menus
  and cursors, and resume. The addon then preserves all four heroes through
  the original Intro C4 door. P2–P4 move during an explicitly frozen live
  scene while P1 remains its owner. Native dialogue performs the actual next
  section load; all four re-enroll alive and controllable. There are 6,988
  verified redraws and zero state-restore mismatches, aborts, watchdogs or
  VRAM leaks in the final cutscene receipt.
- `followup-current-rt-local-three/report.json`: three-controller input and
  cameras, the continuous bottom viewport, centered P3 pause artwork,
  independent cursors, resize/refusal and resume.
- `followup-current-rt-two/report.json`: the existing two-player private-menu,
  local-pair and local-canvas scenarios pass, including individual weapons and
  tanks, either-seat resume, shared quit/options, a surviving P2 menu, and
  current-build save/load while paused. Native Jungle cameras stay independent
  across the tile-ring boundary and pause/refusal/resize cases.
- `followup-current-rt-online-4/report.json`: four native LAN processes with
  Zero, normal X, Fourth Armor X and Ultimate X pass independent movement,
  attacks, charged X shots, a separated leading P4, private menus/cursors and
  resume. There are 115 common matching core/mod CRC samples through tick
  3,680, with zero renderer state-restore mismatches.
- `followup-current-rt-online-3/report.json`: the default three-hero roster
  passes the same native LAN scenarios, with 117 common matching core/mod CRC
  samples through tick 3,744. These LAN probes never write simulation RAM.

The four-player pause images were visually inspected: each X form uses X's
native menu, each Zero form uses Zero's menu, and Ultimate X has Nova Strike.
The three-player image centers the unscaled P3 menu in the wide bottom pane.
The gate-release image shows four healthy heroes in the real second section.
`followup-current-rt-hud-gate/arrival-45.png` through `arrival-52.png` show
the first HUD-enabled startup frames with one meter in each quadrant, the
concurrent colored incoming beams and the original READY exit pair. Its
`indoor-seams.png` and the local run's separated-camera images were inspected
for the reported stale parallax margin. These images cover the sampled Intro
locations, not every campaign background.

The render worker is enabled for the `-rt-` runs. It is a host presentation
setting, not a changed simulation profile. Its source default remains off.
The owner playtest enables it explicitly.

## Performance and scope

Synthetic extra-player and camera execution no longer consume the canonical
world's cycle allowance in ENHANCED. Their bounded fallback retains charged
execution when necessary. The canonical world remains charged; REFERENCE
retains its original charging policy.

The remaining busy-area slowdown is not solved. Under the same current host
conditions, the retained control measured 20.42/23.08 FPS idle/firing and the
current render-worker build 22.00/22.33 FPS. Vblank deltas approximately equal
game-update deltas. These measurements point to host rendering rather than
extra simulated PSX frames; they are not a 60 FPS claim. Evidence:
`followup-perf-control-current-host` and
`followup-perf-current-render-thread`. Further rendering work is tracked in
`beads-eio.9.229`.

The HLE batcher was evaluated but is excluded from this handoff: its native
pause/save-load run failed, and a wide-surface digest difference remains
unexplained. Earlier HLE/old-framework reports are historical evidence only.

Full campaign, overlapping scenes, vehicles, Internet/ICE and cross-OS/CPU
gameplay remain unqualified. The cutscene addon remains a separate default-off
package. Local unified is deferred; four-player save states and rewind remain
blocked. Native memory cards are preserved.
