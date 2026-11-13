# Co-op follow-up to 1.0.2

Included in 1.0.3 Stable Public Beta: a correctness pass for the opening-stage, Slash Beast,
Storm Owl and Split Mushroom reports. Cyber Peacock was explicitly withdrawn
from this pass by the owner. Shared barrier hit palettes remain original behavior.

## Changes

- Opening-stage explosions use the progressing living hero's view for one
  original controller update. Native horizontal/vertical shake commands reach
  nearby living views. The closing-wall emitter represents the full 432 px
  wall, including the floor view. Distant views keep their own camera state. Restoring a
  temporary camera retains newly issued commands without restarting old timers.
- Eregion's native admission commits the entering hero's camera even after
  WARNING has frozen ordinary follow. This prevents the pan from starting at
  the distant living partner's position.
- Cannon beams retain their emitter's world origin. Native `9D100` (secondary20)
  and `A3B04` (secondary36) previously clamped distant emitters against P1's
  camera. The hooks replace only the loaded camera value with the emitter's
  nearest living view; secondary36's native culling extent uses the same view.
  Shared AI, collision, animation and lifetime still execute once.
- Slash Beast's separate category7/type3 midboss controller uses the progressing
  hero for native admission at X7009. Its later scenery subtype is unchanged.
- Tentoroid's native death stores at `5D7C4`/`5D80C` publish the original easing
  target to both saved views still in its enclosure. The original left limit
  opens from 4608 to 4352. P2 previously retained 4608 and stopped at 4616 while
  living P1 could leave. Camera origins and native limit easing are retained.
- Either player can initiate native Select/Quit or an eligible Escape through
  their pause menu. The initiating player controls Quit's original confirmations.
  Native Escape keeps its original direct transition. P1 retains Options.
  Dead bodies do not receive equipment/controller updates during Escape.

## Evidence and status

Native runs use isolated copies on G:, the original US disc/executable, muted
audio, hidden windows and local 8:3 Split. Checkpoint, position, ability,
invulnerability and enemy-health fixtures prepare scenarios. Actual inputs and
original game routines supply admissions, hits, death/retirement, camera releases
and frontend transitions. No success flag or collision tile is set to pass a check.

| Check | Evidence so far |
| --- | --- |
| P2-led Eregion with distant living P1 | Pass: original source pans from the beginning and loses P2; candidate enters a living 48 HP boss fight, both heroes healthy and P2 controller movement restored. |
| Opening corridor with either leader | Pass: original P2-led native shake samples P1/P2=93/0; candidate 13/109, repeated 19/109 on the final source. Both native leader controls pass. Final views retain the native 360 vertical limit and at most 4 px of original quake displacement. |
| P1 catching up through the opening slope | Original camera reaches Y352 at X3912 and Y360 at X3992. Candidate keeps Y256 at X3906 and follows to Y274 at X3992, then reaches the authored 360 endpoint normally. |
| First falling structure | Pass: 956 original and 923 candidate observations show P2 quake absent before and 45 positive native observations after. The lower piece still drops naturally. |
| P2 Quit and Escape with P1 dead | Pass: P2 can cancel, resume and move, then confirm native Quit to title. Dead P1 cannot confirm P2's popup. Native Escape reaches the stage board; campaign clear bits remain 16. |
| Slash midboss | Pass: actual initialized native enemy 45, restored idle state and real controller movement with either living leader and with P1 dead. No midboss defeat/full-stage claim. |
| Slash cannons | Pass: two emitters at 1658 and 4458 retain original beam offsets -119/-375. Original source placed the distant 4458 emitter's beams at 809/1065 near P1, with 3649 px worst error. |
| Mushroom former camera stop | Pass: original native kill/retirement leaves P2 clamped at 4616. Candidate completes the original death sequence, controller-driven climb and actual part2 transition with living P1; both finish healthy. The survivor comparison crosses the former wall to 4490 while preserving P1 HP0/P2 HP32. |
| Owl cannon directions and early part2 enemies | Pass: 192 native observations across three part1 cannon positions and three fresh part2 setups. Both secondary36 facings retain original offsets+/-144 and+/-400 from their emitters. Original source instead places beams at 48/304 beside camera 64 while their live parent is 1214. Both composed panes were inspected; both heroes retain HP32. |
| Closing-wall shake | Pass: native upper-platform approach and descent arm the actual walls with living distant P1. P2 receives 50 horizontal observations during closure and the original final vertical command (amplitudes 3/2, duration 32); all wall pieces retire, both heroes retain 32 HP and P1 keeps zero shake. Position-only ascending protection prevents an idle crush after the floor observation. This qualifies effect delivery, not full trap traversal. |
| Separated opening-world render verification | Existing warning: an identical bounded setup produces 19 mismatches in both unmodified 1.0.2 (632 additional checks) and candidate (578). Both have 0 aborts, watchdogs or VRAM-leak reports and 89 journaled uploads. This is not a clean-world-render claim; the identical-hash warning is tracked with framework issue `beads-eio.3.375`. Its upload cause is not separately proven by this comparison. |
| Menu render rollback and snapshots | Full simultaneous-menu scenario passes 3198 strict checks with 0 mismatches, aborts, watchdogs or VRAM leaks. Current-build paused snapshot restore, actual weapon fire after resume, native healing/equipment locks, P2 Quit control and survivor menus pass. Separate bounded original/candidate pause/resume checks also pass 704/714 checks. |

The supplied Mushroom archive contains seven offline 1.0.2 sessions. Its newest
57,920-input session covers the new video and includes the repeated Mushroom
attempts. The earlier opening-stage production replay verifies 35,895 native
frame/state hashes. These recording checks are distinct from the controlled
native reproductions above.

All 38 Windows CTests, including lifecycle/split boundary checks, and all nine
diagnostic-replay Python tests pass. The production build with both development-tool options disabled also passes
all 38 checks.
No fresh online or Linux qualification is claimed. Missing bees (item 200) remain
open; these camera/effect changes are not evidence that their spawning is fixed.

## Repeatable checks

`tools/test_mmx4_coop_regressions.py` provides named cases:

- `slash_midboss_entry`, `slash_cannon_origins`, `owl_cannon_origins`;
- `mushroom_midboss_exit`;
- `intro_effect_corridor`, `intro_distant_entry`, `intro_structure_shake`,
  `intro_closing_shake`;
- `p2_pause_escape`, `p2_pause_quit`, and the existing `personal_menus`.

Use a development build with original-disc fixtures, a new isolated output
directory, `--hidden-window --local-cameras split --cameras split`, and explicit
G:/F: temporary/profile/cache paths. Run one game at a time. Cannon position
protection qualifies original beam origins, not stage traversal. Native Quit
and Escape checks prepare eligibility and follow the original frontend flow.