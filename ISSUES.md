# MegaManX4Recomp — Issues

Current state (v0.0.2-alpha): the game boots and plays — intro cinematics
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

## #2 — Widescreen background-ring corruption — RESOLVED (experimental opt-in)

X4 offers an experimental Widescreen mod while retaining authentic 4:3 as the
default. Its widened renderer previously exposed stale or incorrectly populated
background-ring columns in multiple stages, visible as diagonal staircase
tiles, repeated rectangular bands, and black voids after clean stage entry.

The fix configures X4's actual layer records at `0x801419B0`, follows the native
streamer's width-only map indexing, and disables MMX5/MMX6-style parent-layer
scroll composition. The resulting full-window refill updates all 87 columns
across the three layers and validates all 1,134 visible ring cells with zero
mismatches. A clean-boot Jungle replay and manual end-to-end testing confirmed
that the corruption is gone while authentic 4:3 remains unchanged. See
`annotations/widescreen_bg2d_sites.md` for the reverse-engineered evidence.

A clean-boot input route for this Jungle location is checked in at
`tools/routes/mmx4_jungle_widescreen.json`; use
`tools/replay_input_route.py` as documented in the README. It reaches the
regression location with turbo loading enabled, avoiding the unreliable
save-state path.

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
