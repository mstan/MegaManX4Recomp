# Four Player Co-op preview: stable 1.0.7 integration

Version: `1.0.7-4p-spike.1`, experimental Windows x64 prerelease.
Tracking: `beads-eio.9.239` (centered three-player layout) and
`beads-eio.9.241` (stable release integration).

This feature-only integration merges stable master
`d68a03a4d01f1f1901dd77d48efe2a2990980ef2` into the four-player feature head
`1e8d17d7a307c971e905b988e599945ae729acc0`, preserving both histories.
The published stable branch and 1.0.7 release are not modified by this work.

The feature framework stays at
`2f3f38403250b2937cf61179a17dc648c3392765`; recomp-ui remains
`b964eabe5a3438b8ed121064354ba27c1accef3f`. The centered three-seat presenter,
four-seat grid, seat-aware stored-charge HUD ownership, per-player resources,
HLE textured batching and bounded extra-player/camera execution are retained.
No save-state migration or new release loading mod is introduced.

## Stable corrections retained

Magma's two-player first-slope rolling rumble and Web Spider's survivor
stage-pool camera correction apply without production source conflicts.
The seat-aware feature HUD work already contains the stable two-player
stored-charge correction and its native entry hook. Both development and
release hook lists remain aligned.

The only runtime changes relative to feature head `1e8d17d` are in
`mmx4_coop_lifecycle.c` and `mmx4_coop_split.c`. Shared regression-harness
conflicts retain all stable stage cases, the stored-charge case and the
feature-only free-cutscene fixture. Version and presentation documentation
now describe the centered three-player pane instead of the retired wide
bottom view.

The stable native evidence in `COOP_107_INTEGRATION.md` and the two-player
stored-charge document describes the tested stable executable. Four Player
Co-op has its own progression engine; those checks do not establish its
stage/boss parity or the new preview binary's correctness.

## Source and model checks

All six focused model executables pass against the retained feature framework:
roster, HUD, texture cache, combat, lifecycle and split. The release configuration
suite passes 8/8 checks and hook registration passes 3/3. A separate registration
audit verifies all 41 four-player native function hooks and 10 instruction sites
are emitted. Production plugin syntax checks pass both ENHANCED and REFERENCE
execution branches with debug tools disabled.

The C99 model builds use Clang with `-Wall -Wextra -Werror`; lifecycle retains
the existing narrow exceptions for typedef redeclarations and partial aggregate
initializers. Feature-manifest checks retain the unique roster, P4-only None,
forced conflicting-mod suppression, default-off experiments/diagnostics and
the clean windowed preview settings. Exact commands and results are recorded in
the private integration receipts under
`G:/Temp/Codex/psxrecomp/x4-preview107-integration/checks/`.

A reused stable build cache must explicitly select `PSX_GL_BATCH_IMPL=HLE`
for this preview's ENHANCED build; an existing explicit LLE selection is
deliberately preserved by CMake. This merge does not silently override it.

The source catalog retains diagnostics, Four Player Co-op and free cutscenes
off by default. Public packages must exclude CD Speed, Fast Loading and
development cheats. Windowed presentation and normal owner-facing audio
remain the preview defaults.

## Fresh native and package qualification

The rebuilt development and production configurations pass all 45 CTests.
Native checks pass centered three-player gameplay and black pillars, every
seat's matching private menu, fallback/resume, window resize and the unchanged
four-player grid. Native keyboard charging/consumption covers Force Armor X
in every possible seat across legal three/four-player rosters, including
canonical fallback. The follow-up ownership gate in `mmx4_coop4.inc` keeps
extra-seat stock icons out of P1's canonical fallback HUD.

The clean production candidate passes fresh original-disc boots, independent
keyboard movement, resize, private menus and resume in both local layouts.
Four production LAN peers also pass native movement/attacks, Force X's complete
stock cycle, P4 leading with an independent camera, personal menus and resume;
113 common core/mod CRC samples match through tick 3616. Both two-player
rosters pass the imported Magma and Web Spider native regressions.

Four Player Co-op has a confirmed separate Magma controller gap: P2 leading
does not activate the volcano emitter or its boulders/rumble, while the matched
P1 control works. Tracking: `beads-eio.9.244`. Stage prerequisite checks must
not be presented as completed survivor-ladder or campaign qualification.
The matched Web Spider approaches also establish a separate Four camera-area
ownership gap: P1 admits the shaft bounds, P2 does not (`beads-eio.9.245`).
The actual Four survivor-ladder case is unverified because this prerequisite
was not met. See [the bounded stage checks](FOUR_PLAYER_107_STAGE_QA.md).
Detailed scope and exact binaries are recorded in
[FOUR_PLAYER_107_PRERELEASE_QA.md](FOUR_PLAYER_107_PRERELEASE_QA.md).

The old preview's performance numbers remain historical evidence. No new
full-campaign, Internet/ICE, cross-OS/CPU, all-weapon or full-speed qualification
is claimed. Four-player save states and rewind remain blocked; native
memory-card saves retain their existing separate isolation contract.
