# Opening briefing: partner departure pose

Issue: `beads-eio.9.288`. Branch: `fix/board-departure-pose`, based on
`7c44e09cec794070d16addda0d4b939ed766fcb3` (v1.0.26).

The original recording replayed completely with all5015 inputs verified.
In both campaign orders, native phase9 enters hero sequence1 through the
original dialogue animation script, without a new stage-confirm control
command. The added hero remained in sequence0: X-led native frame1200 and
Zero-led frame2815. Ordinary stage selection already put both heroes in
sequence1. The initial hypothesis about an earlier stage-selection phase was
rejected by this replay evidence.

The fix lets only the added hero follow the native hero into sequence1 during
phase9, advancing the animation normally once selected. Other dialogue
animations are still withheld from the partner; added companions remain idle.
The phase10 Maverick donor-rendering fix and ordinary stage-confirm path stay
in place. No additional host state, story control writes or reward changes
are introduced.

Evidence root: `G:/Temp/Codex/psxrecomp/x4-departure-20261102`.

* Baseline SHA-256:
  `3ab0d25727aaeaaed2f22b507ce33514bffd07673064cc0aedbed68f26b25a59`.
* Candidate SHA-256:
  `535b954c7c3c9504b8d5f17bb1170e10140f740948a789ea985e2b800e85abae`.
* Native `opening_departure` checks pass in both campaign orders, with input-only
  new-game navigation: partner idle during dialogue, both heroes in departure
  pose over multiple samples, and living co-op entry into the opening stage.
  Both composed captures were visually inspected and show intact poses.
* All44 title CTest checks passed in52.73 seconds.
* All eight first-entry Maverick introductions passed the idle/actor checks;
  the first composed capture was visually spot-checked for intact cast sprites.
  Ordinary selection, current-build save/load, phase11 splash and gameplay
  entry passed in `candidate-selection-canonical`.
* Current-build briefing snapshots are saved in slot0 of each private profile.
  These are for human replay and must not be replaced with old-build states.

The first selection run failed a raw `PLAY+0x43` gameplay read. The failure's
completed diagnostic mirror still showed campaign0 and player characters0/1;
live inventory projection temporarily writes P2's character into that address.
The check now uses the existing canonical mirror and verifies both player
assignments. The affected selection case then passed. The original failure
record remains in `candidate-rollcall`.

The owner accepted the visible candidate on2026-11-02: "seems ok. pass it".
This clears the requested human visual gate. No individual campaign checkbox
results were recorded; the two native campaign checks above remain the
automated evidence.
Central Beads is unavailable (`database "beads" not found` at3307); the task
root retains the update for the existing issue. Retain the decisive captures,
input recording, current-build review snapshots and source/build identities.
