# MegaManX4Recomp — Issues

Current state (v0.0.1-alpha): the game boots and plays — intro cinematics
decode, the title screen and menus respond, attract demos run, and a game can
be started — with working (digital) controller input and no known crashes on
the covered path. It has not been verified deep into stages or to the end.

---

## #1 — Early preview: uncovered code regions halt loudly — OPEN

X4's boot EXE still has ~2,804 regions the recompiler currently classifies as
data (down from 7,373 before the control-flow-aware extent fix). If gameplay
reaches one that is actually code, the program **fail-fasts with an
"unknown dispatch" report** rather than silently misbehaving — that is by
design. The covered path (intro → title → attract → game start) is clean. If
you hit a halt, note where you were and what you did; each report pins the
exact address the recompiler needs to classify.

One unreproduced silent exit was observed minutes into a long attract-cycle
soak during bring-up (no fail-fast banner); it has not recurred and is
presumed stale. Tracked here until a reproduction exists.

---

## #2 — Widescreen (true 2D wide field of view) — RESOLVED (experimental opt-in)

X4 now offers the launcher's experimental Widescreen toggle while retaining
authentic 4:3 as the default. The port widens the three-layer background tile
window and streamer, the shared actor activation/despawn funnels, and the
screen-space draw cull. Sky Lagoon verification showed populated 16:9 margins,
enemy sprites active beyond the native 320-pixel edge, safe packet occupancy,
and zero dispatch misses. The player health/weapon and enemy/boss health HUD is
source-filtered to its dedicated packet arena and re-anchored to the respective
16:9 boundaries, without touching world sprites or the default 4:3 path. See
`annotations/widescreen_bg2d_sites.md` for the reverse-engineered sites and
runtime evidence.

---

## #3 — Memory-card save/load not verified end-to-end — OPEN

The SIO/memory-card hardware layer is exercised and healthy at boot (the game's
card probes complete normally), but an actual in-game save + reload cycle has
not been verified in this build. X5/X6 save/load work on the same framework
path, so this is expected to work — it needs a verification pass, not new
machinery.

---

## #4 — Generated full.c bloat (~225 MB) slows source builds — OPEN (dev-only)

A recompiler alias-promotion pass mints ~40 overlapping alias bodies over X4's
ARC filename/pointer tables, tripling the generated `full.c` (76 → 225 MB) and
pushing the single-TU compile to ~40 minutes. Dead code — correctness is
unaffected, players are unaffected; it only slows source builds. Needs a
recompiler-side fix (tracked in the framework), then a regen.
