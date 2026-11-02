# First-entry Maverick roll-call corruption

Owning issue: `beads-eio.9.288`. Branch: `fix/splash-roster`, based on
`e515d54f960c4a07b990d02f1c58c5246b47f7c8` (v1.0.25).

The report is the first-entry roll call on the stage-select board, native
`PLAY=[3,10,...]`. The later full-screen boss-name splash is phase11. Earlier
Web Spider splash checks exercised phase11 and missed the reported scene.

## Reproduction and fix

The supplied `E:/Downloads/Stage select graphics.zip` records v1.0.25, local
Split co-op, Complete Story and Stage Select Cast enabled. Video reference:
<https://youtu.be/EexIkA9YJX8>. Its early X save is card2/data1, with no
Mavericks cleared. Native Continue from a private copy reaches the roll call.
The original cards, input trace and archive remain unchanged.

Exact v1.0.25 source reproduced scrambled added-character sprites in both
the canonical framebuffer and composed Split panes. `board_active()` excluded
phase10, so the added cast stopped receiving donor texture handling and drew
from the native VRAM atlas instead. Actor counts and idle-sequence assertions
still passed: they do not establish graphical correctness.

The fix includes phase10 in board ownership and idle-only added-character
animation handling. Phase11 remains excluded because it owns a different
scene and particle pool. The campaign's original dialogue actors retain their
native animation commands. No reward/save flags or pacing values are changed.

## Validation and evidence

Private evidence root:
`G:/Temp/Codex/psxrecomp/x4-splash-20261102`.

* Baseline executable SHA-256:
  `2a987b38e61956053ee00ae8614bdb6c08743d85562470019e488c1261beb751`.
* Candidate executable SHA-256:
  `905123d5e7d2902ec27bec2ab4afc1ea95bea93fb22b70f4a2f774ca6e99a748`.
* Both use debug tools with netplay debug fixtures disabled. The baseline
  retains the supplied recording's build/execution identity. Strict replay
  reproduced the fault without an observed mismatch through the captured
  prefix; the overall run timed out at240 seconds. This is not a completed
  replay of the entire recording.
* `baseline-replay/roster-1383-{canonical,pair}.png` and
  `baseline-rollcall/rollcall-0.png` show the corruption.
* `candidate-rollcall/report.json`: all eight X-campaign introductions,
  added cast idle, original board animations, current-build save/load,
  phase11 splash and native stage entry passed structural checks. All eight
  captured introductions were visually inspected and show intact cast sprites.
  Rendering uses the reported settings at explicit1080p, local Split,
  Complete Story, and render-pass verification.
* All44 title CTest checks passed (serial,55.25 seconds).
* `candidate-zero-unified/report.json`: the same two native cases passed in
  Zero's campaign with Unified presentation. All eight captured introductions
  were visually inspected and show intact cast sprites. This uses private
  copies of the supplied early card with only data1's campaign byte changed;
  clear/reward flags were preserved.

The reused native Continue helper now advances phase10 dialogue, and the
existing regression runner can observe all eight introductions using
`--card-seed <early-card-copy> --clears 0 --complete-story --cases
stage_select_rollcall stage_select`. It saves a current-build phase10 snapshot
in slot1. The recorded-input tool can use a hidden drawable for composed
captures; its identity and hash checks remain strict.

## Human gate and retention

The owner approved the visible candidate on2026-11-02: "looks good. cut it".
This clears the requested human visual and release gate for this candidate;
no individual checkbox results were recorded. `launch_review.py` opens the validated private X profile
at phase10 in a normal window with normal audio. `review_panel.py` offers
reload/advance controls and separate observations for the roll call and later
boss-name splash. `human-validation-notes.json` stores Pass/Fail/Deferred
and notes by binary identity. Read the notes before deciding acceptance.
This review does not qualify physical controller assignment or netplay.

Keep the candidate profile, snapshots, notes and decisive before/after
evidence until the owner completes validation. The supplied video was inspected
using storyboards and a decodable partial download, not watched in full.
Central Beads remains unavailable (`database "beads" not found` at3307);
the evidence root retains `pending-beads-update.json` for the existing issue.
