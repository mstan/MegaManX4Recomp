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

The two-player correctness change is independent of the experimental
four-player mod. On the feature branch, the four-player engine separately
tracks all 64 particle slots with a 1-based allocating seat, resets ownership
on allocation/stage initialization, and includes it in its synchronized
digest. Each X variant's controller uses that seat's context and each private
view draws only its own stock display. This works with the centered
three-player presenter, whose draw callbacks are exactly seats 0, 1 and 2.
The canonical four-player fallback contains only P1's native HUD, so it also
admits only P1's stock display. Extra-seat stock cannot attach to P1's meter
when private rendering is refused. Two-player Unified retains its separate
two-HUD layout and offsets.

## Validation

The strict Clang combat boundary suite covers X as P2 with four and zero
stored charges, canonical Zero isolation, unchanged player/charge bytes,
private-view refusal, allocation reuse, and actor-pool bounds. Native visual
and replay qualification is recorded separately when performed; these boundary
tests alone do not establish that a gameplay image is correct.

The supplied recording (`Charged shots hud display bug.zip`, session
`1792889506553145`) was replayed with its exact original preview binary.
All 16,902 inputs in the bounded photo-near prefix verified. A read-only
2 MiB RAM capture at completion shows X with `PLAYER+A6=4` and the live
controller at `80173CA0`: type54, visible, layer255, position32/52,
CLUT780A, frame27. This confirms that the reported icons are this controller.
It establishes the original reproduction and actor identity, not a pass for
the modified build.

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

## Native three- and four-player qualification, 2026-10-24

`tools/test_mmx4_coop4_stock_hud.py` passed four legal complete-form rosters
on game `b672ed435bc614948a47d932fdd6134f546e16b8`, framework
`2f3f38403250b2937cf61179a17dc648c3392765`, and UI
`b964eabe5a3438b8ed121064354ba27c1accef3f`. The private Release
ENHANCED/LLE QA binary had debug tools and development fixtures enabled,
with SHA-256
`f6ae5f45f8f81b061d56703ba4f8d8c4d66e4255c804fd61b5b06ff24b17e47c`.

| Roster, P1 through P4 | Stock owner | Layout | Restore checks |
|---|---|---|---:|
| X, Zero, Force X, None | P3 | Three players, P3 centered below | 2667 |
| Zero, Force X, Ultimate X, None | P2 | Three players, P3 centered below | 2670 |
| X, Zero, Ultimate X, Force X | P4 | Four-player grid | 4172 |
| Force X, Black Zero, X, Ultimate X | P1 | Four-player grid | 4172 |

Each isolated, hidden, muted process used four distinct normal SDL keyboard
bindings. A guarded window-local input helper verified the executable and PID,
required a hidden drawable, and never changed foreground focus or sent global
input. The debug TCP input override was used for P1 boot navigation only;
it does not support P3/P4. Fixtures protected the players and aligned their
positions/cameras on the opening-stage platform. The legal roster supplied
Force Armor X's stock-charge arm. No stock count was written.

Actual hold/release input generated `0 -> 4` stocks; subsequent presses consumed
`4 -> 3 -> 2 -> 1 -> 0`. Charging normal/Ultimate X and attacking with
Zero/Black Zero left the owner's count unchanged and every unrelated count
zero. Composed PNG stock-region comparisons and image inspection put the
original four X icons exclusively below the owning HUD, and verified their
removal when empty. This includes Force X in the centered third pane.

Every case also refused private rendering while input consumed the first
stock. Canonical screenshots showed only P1's stock when P1 was Force X,
and no stock below P1 when an extra seat owned it. The foreign-stock rejection
counter increased during refusal in all three extra-seat cases. Restoring
private rendering returned the three remaining icons to the owner's pane.
All 13,681 render restoration checks passed, with zero mismatches, aborts,
watchdogs, VRAM leaks or co-op view-state leaks.

Evidence directories are `labels-production-qa-stock-3p-force-p3-v2`,
`labels-production-qa-stock-3p-force-p2`,
`labels-production-qa-stock-4p-force-p4`, and
`labels-production-qa-stock-4p-force-p1`, with reports, composed and canonical
PNGs, runtime logs, and build provenance. The initial P3 run stopped before
charging because its test crop included one final X/Zero portrait row; the
corrected stock-only crop starts at Y=97. That harness calibration is retained
separately in `labels-production-qa-stock-3p-force-p3`.

These checks qualify local stock-HUD ownership and render restoration in this
opening-stage setup. They do not establish an online gameplay session or
broader campaign progression.
