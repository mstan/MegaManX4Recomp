# First-entry Maverick roll-call corruption

Owning issue: `beads-eio.9.288`. Branch: `fix/splash-roster`, based on
`e515d54f960c4a07b990d02f1c58c5246b47f7c8` (v1.0.25).

The report is the first-entry roll call on the stage-select board, native
`PLAY=[3,10,...]`. The later full-screen boss-name splash is phase11. Earlier
Web Spider splash checks exercised phase11 and missed the reported scene.

## Reproduction and fix

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