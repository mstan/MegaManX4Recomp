# Co-op cast and splash-screen feedback, 2026-11-01

Branch: `fix/coop-feedback124`, based on `bae367af9fbfb769e0bd0ba9a4cac0162a3b9cb5`
(v1.0.24). Owning cast issue: `beads-eio.9.288`.

The owner explicitly requires human validation of the Maverick name/sprite
splash screen and its reported corruption before merge or release.
Automated results alone do not authorize merge or release. The owner cleared
the visible candidate on2026-11-01: "all looks good at a glance. cut a new
release with all of this". This clears the requested human release gate; no
individual checkbox results were recorded, and the unreproduced reports below
remain open.

## Requested behavior and current disposition

## Evidence and review

The final candidate binary SHA-256 is
`3d887057be3abec145af5505dd598563b795a56748487b8102b2e1ad0bbfdbc2`.
`candidate-build.json` records source/patch identity. The preceding layout
candidate passed all44 title tests; only the horizontal inset was then changed.
Final native campaign checks and snapshots are in `review-final` and
`review-zero-validated`. The stage-select scenario now checks the added
companion appropriate to the campaign: Iris for X, Double for Zero. Its former
hardcoded Iris assertion incorrectly rejected Zero's native dialogue pose.

`review_panel.py` provides separate cast and Web Spider splash checks for both
campaigns, plus the pre-door camera comparison. It replays original native
confirmation and door movement. `human-validation-notes.json` stores separate
Pass/Fail/Deferred observations under the binary identity. Read these notes
before further fixes or approval decisions; an unreviewed item remains open.
Use a normal window with normal audio. Replay controls need no second pad;
they do not qualify physical controller assignment or netplay.

Fixtures use cloned native cards, change only disposable first-visit setup
prerequisites, and use existing development checkpoint/placement facilities.
They do not set the graphical result, camera values, door completion, rewards
or persistent owner saves. Gray stage portraits remain from the prepared
all-cleared card and are unrelated to the splash artwork under review.

Central Beads is unavailable: `database "beads" not found` at
`127.0.0.1:3307`. The task root retains the pending update; sync it into the
existing hierarchy when service returns. Do not create a second database or
close the cast/graphics follow-up while human checks remain unresolved.

Keep the final review profile, native snapshots, notes and necessary evidence
until the owner finishes. Remove superseded task-created runtime copies and
caches. Existing source/build workspaces, supplied cards/archive and delivered
v1.0.24 remain preserved.
