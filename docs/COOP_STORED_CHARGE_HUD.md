# X armor stored-charge HUD ownership

The charged-shot stock icons below X's health bar are personal HUD artwork.
They must follow the X that owns those stored charges, and must never appear
below Zero's health bar. This applies to both campaign characters, local Split
and Unified, and independent online cameras.

## Original executable evidence

The supported SLUS-00561 executable creates a shared particle of type `0x36`
only for X at `80035558..80035584`. Its deferred updater is `800B2C8C`:

- Initialization at `800B2CA0..800B2CE8` sets screen-relative layer `FF`,
  coordinates `(32,52)`, CLUT `780A`, and the current character's assembly
  member at `SCRATCH+1C` plus `30`.
- `800B2CEC..800B2D3C` hides the actor when the native HUD is hidden or
  `PLAYER+A6` is zero, otherwise it uses frame `31 - PLAYER+A6`.
- The particle belongs to the shared `80173CA0..801754A0` pool, with stride
  `60`, drawn by the world loop at `800240D0..800240F4`.

It is outside the native health-HUD routine `80024E70`. Therefore redrawing
the shared world for another Split camera also redrew X's icons under that
view's own health bar. When P1 was Zero, the deferred controller additionally
read Zero's counter and character resources instead of the allocating X's.

## Fix

Reuse the existing snapshotted particle allocation owner. Run a P2-owned stock
controller with P2's projected native context. At draw time, admit the actor
only into its owner's private HUD, or apply the same offset as that player's
health group in Unified. Resolve the original X artwork through the immutable
UI bank and retain its left HUD anchor. Restore temporary draw coordinates and
contexts before returning. No charge counter, gameplay body or native code is
patched, and private views do not advance the controller.

This correctness change is independent of the experimental four-player mod.
Additional X seats need the feature branch's seat-aware ownership adaptation.

## Validation

The strict Clang combat boundary suite covers X as P2 with four and zero
stored charges, canonical Zero isolation, unchanged player/charge bytes,
private-view refusal, allocation reuse, and actor-pool bounds. Native visual
and replay qualification is recorded separately when performed; these boundary
tests alone do not establish that a gameplay image is correct.

Both development and release recompiler hook lists include `800B2C8C`;
regenerate the native code before testing this updater interception.

Independent online cameras use the same private HUD scope as offline Split:
`present_local_view` selects the local netplay port, then calls
`psx_mod_render_local_view`. In the pinned stable framework, that operation
sets `psx_mod_local_view_scope()` before its draw callback. Consequently each
peer filters the other player's stock actor and uses its original HUD anchor;
Unified offsets apply only to the shared draw. This is source-contract
verification, separate from an actual two-peer gameplay run.

## Native qualification, 2026-10-24

The combined stable-fix bundle at `7cedeb269282a29ee7f896998b34413f26089cd8`,
with framework `a251a98616f0c8f9e45c90c7ca71d20da71b8b21`, passed the
`stock_charge_hud` scenario in four hidden, muted, isolated profiles.
The instrumented Release ENHANCED/LLE QA binary has SHA-256
`b4fff6d171cc69476e3ca33035e6e3a5c6f6bc9e46e17fd15f5265a7d58e4395`.
Debug tools and development fixtures were enabled for these checks.

| X seat | Presentation | Composed image inspection | Restore checks |
|---|---|---|---:|
| P1 | Local 8:3 Split | Four icons below X's left HUD, none below Zero's right HUD | 888 |
| P2 | Local 8:3 Split | Four original X icons below X's right HUD, none below Zero's left HUD | 930 |
| P2 | Unified, stacked | Four icons below X's lower HUD, none below Zero's upper HUD | Not used |
| P2 | Unified, side by side | Four icons below X's second-column HUD, none below Zero's first-column HUD | Not used |

Only setup granted the stock-charge armor and placed the players on a safe
ledge. Actual Square input produced four charges and consumed them
`4 -> 3 -> 2 -> 1 -> 0` in every run. Zero's unrelated byte stayed zero,
both players retained their health, and the display hid after consumption.
The P2 controller used X's private assembly and counted X's stored shots
while Zero remained the canonical player. Temporary Unified offsets restored
to the original actor position `(32,52)` after drawing.

Both Split runs reported zero render verification mismatches, aborts,
watchdogs, VRAM leaks or co-op state leaks. Unified does not use private
render transactions in these cases, so it has no restore-check count.
Evidence is retained in `qa-x-p1-split`, `qa-x-p2-split`,
`qa-x-p2-unified-stacked`, and `qa-x-p2-unified-side-by-side`, each with a
`report.json` and empty/charged/consumed composed PNGs. This qualifies the
two-player correctness fix; four-player and actual online gameplay remain
separate checks.
