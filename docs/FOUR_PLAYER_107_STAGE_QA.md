# 1.0.7 Four Player preview: bounded native stage checks

Native qualification on 2026-10-24 used source
`b672ed435bc614948a47d932fdd6134f546e16b8` and the immutable Windows DEV
snapshot SHA-256
`f6ae5f45f8f81b061d56703ba4f8d8c4d66e4255c804fd61b5b06ff24b17e47c`.
It used the feature framework pin `2f3f384`, HLE OpenGL textured batching,
debug tools and offline fixtures. The processes were hidden and muted.
This is functional DEV evidence, not production-package or performance QA.

## Legacy two-player fixes in the preview

Two fresh local Split sessions exercised both campaign rosters: X/Zero and
Zero/X. All four harness cases passed, comprising six seat-specific checks:

| Check | X P1 / Zero P2 | Zero P1 / X P2 |
| --- | --- | --- |
| Magma first slope, P2 leads | Native shake timers 1–7; HP minima 30/42 | Timers 1–7; HP minima 24/42 |
| Magma first slope, P1 leads | Timers 1,3–7; HP minima 42/30 | Timers 1–8; HP minima 42/30 |
| Web final part 1 ladder, P1 dies | P2 descends to Y609.5, HP32; camera Y256→465 | P2 descends to Y614, HP32; camera Y256→470 |

Both Web runs climbed back through the native return strip, retained the
dead partner and preserved the shared life count. Magma damage remained
enabled; the waiting partner was hit by native boulders. These checks establish
the two-player fixes in this feature build. They do not establish equivalent
Four Player stage behavior.

## Four Player Magma: confirmed missing P2 activation

Roster was the default X, Zero, Ultimate Armor X, Black Zero. Stage4/part0
was initialized through native stage selectors. All seats began with HP48;
only the leading seat received Right+Dash. No invulnerability was enabled.

The matched P1 control reached the slope bottom, recorded 93 descent samples,
40 samples with native shaking, shake timers 0–7, and HP minima48/30/40/42.
The native type33/subtype0x10 emitter at `0x80142F98` advanced through its
authored phases0/1/2 and emitted damaging secondary actors.

With P2 leading and P1 waiting at X128, P2 also reached the bottom. Across
95 descent samples there was no native shaking or shake timer; all four seats
retained HP48. The same authored emitter was loaded, but remained in phase0
while P2 crossed its activation window. This is a reproducible Four Player
activation gap, tracked as central issue **beads-eio.9.244**.

A bounded follow-up can examine a Four Player equivalent of the two-player
`volcano_controller` projection in `src/mods/mmx4_coop_split.c`: choose an
eligible living seat for the existing authored emitter update and its camera,
while retaining one native timer/emission pass. The current Four Player actor
projection covers enemy/secondary pools, not this stage-controller update.
Emitter ownership, boulder camera context and shake audience all need native
verification; a broader stage-engine change was not attempted here.

## Four Player Web: eligibility before the death test

The fixture loads stage1/part0/checkpoint2, places the leading seat at
X5543/Y1402, waits for native shaft area4 to enter that seat's view, and uses
native movement to cross X5570. It does not write camera bounds, actor state
or scene-success flags.

With P2 leading, the settled run reached the native area4/area7 controllers at
X5568/X5552. P2 crossed to X5588.375 while its camera reached X5352, but the
shaft's vertical bounds remained1280 instead of admitting the native256
upper limit. The route stopped at the explicit prerequisite assertion
`Shaft camera area was bypassed`, before climbing the final ladder or killing
P1. This ownership contrast is tracked as central issue **beads-eio.9.245**;
it is not a passed or failed reproduction of the later survivor death.

With the same settling allowance, the matched P1 control admitted the shaft's
upper256 bound and the upper enclosure, descended to Y602 with HP32 and camera
Y256→458, and climbed back. This confirms the P1/P2 camera-area eligibility
contrast without relying on the initial timeout.

The P1 control also exposed a separate failure before the planned partner
death: its before-death snapshot already has P2/P3/P4 at HP0, at lower-route
Y1466–1467, with their cameras inheriting the upper256 room bounds. The reused
two-player helper returned `passed` because it assumes the waiting partner is
alive before its explicit void-death setup. Inspection invalidates that
interpretation here: this run establishes P1 camera admission and movement,
but does not qualify the planned partner death or preservation of the other
three seats. These observations do not establish a causal sequence from a
specific bounds write to a seat's death. The retained private runner now asserts all four seats are alive
at that prerequisite, so future runs cannot repeat that false-positive claim.

The source candidate for both Web findings is the Four Player camera path's
shared room bounds and canonical P1 ownership. A safe follow-up must admit
the native room transition for its real leading seat and preserve each
distant living seat's applicable bounds; simply running the whole stage pool
multiple times would change shared timers and stage events. This audit makes
no stage-engine change.

## Evidence and limits

Retained local evidence root:
`G:/Temp/Codex/psxrecomp/x4-preview107-integration/checks`.

- `feature-2p-stage-results.json` and `feature-2p-stages-campaign0/1`:
  command lines, native body/camera observations, captures and reports.
- `feature-4p-p1-control`: matched Magma control, sampled controller pools.
- `feature-4p-p2-settled`: completed P2 Magma endpoint and settled Web
  prerequisite failure, including native actor observations and captures.
- `feature-4p-web-p1-settled`: P1 Web camera admission and the unexpected
  waiting-seat deaths. Its raw helper success is superseded by the audited
  prerequisite failure described above.
- `run_four_stage_checks.py` and each settled profile's `spider_fixture.py`:
  retained private harnesses. The initial Web adaptation extends the two camera
  settling allowances from20 to75 seconds; native admission assertions remain.
  The runner's final revision adds the missing pre-death alive assertion;
  the per-run fixture files retain the exact revisions actually executed.

Initial short-budget Four Player attempts are retained for audit only. Their
timeouts occurred while cameras were still easing toward the position-only
fixture jump; those timeouts do not establish gameplay failures. The later
settled observations above supersede them. No claim of a frame-rate regression
is inferred from wall time under debug verification and concurrent QA.

This work does not qualify full three/four-player campaigns, remote stage
progression, Magma part2 pillars, Sigma or the Four Player Web survivor death.
Other workers own HUD, layout and online checks. No release/master code was
changed by this stage audit, and no cross-build save-state compatibility is
asserted. Private runtime copies, assets, mods, BIOS, settings, cards and caches
were removed after all seven owned profiles' processes closed. Reports,
captures, logs and small reproduction evidence remain; cleanup paths and
logical byte counts are recorded in `stage-profile-cleanup.json`. Four Player
private executables were hard links to the immutable central snapshot, so
removing them is not reported as physically freeing additional binaries.
