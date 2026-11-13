# Stable playthrough follow-up

The owner also reports flying enemies swarming in during solo intro play
but failing to appear in two-player co-op (`beads-eio.9.276`). The owner
confirmed the **outdoor first section**, distinct from the historical part2
falling-object groups. The native opening controller allocates ten type10
enemies atX34..54. Their subtype3..8 waiting phase49D60 checks whether
the hero is more than192px ahead. Nearest-hero projection continually picks
the waiting trailer, so the group never activates when the other hero leads.
The unfixed native control reproduces all ten remaining hidden in state1/2
as P1 walks from200 past400 while P2 waits at224. Only that waiting phase
now selects the furthest living hero; the original activation predicate,
one shared update, motion and later nearest-target AI remain intact.

Separate part2 controls show all three P2-led flying groups. A P1-led group
is allocated but remains hidden when the check stops at X3066; continuing
the actual approach makes those same allocated actors drawable. This is a
test approach correction, not an additional part2 spawn or AI fix.

This work is based on stable `c404f1d`, with framework `9868d30` and UI
`b964eab`. X1 replacements and Four Player Co-op are separate work.

## Native causes

Cyber Peacock's airborne admission retains `PLAYER+67 = FF`. The existing
Both-door passenger handler excluded that state, leaving its terrain box
active while the shared door moved it through a still-sealed wall. The
original crush bits changed to `13` and full-health P2 died at approximately
967/2485. Removing that exclusion applies the same native `C1C24` null-box
setup already used for grounded participants. Admission, movement, animation
updates, door timing and damage remain native; no immunity is added.

Dragoon's `7E3D0` releases its script before `7E400..7E430` writes the room
targets and follow speed. A P2 projection could restore the closed
5264..5264 enclosure over those writes. Extend the existing refight release
publication to the main fight's 5264..5104 targets and follow step 4. Only
matching closed views in the same vertical room receive those authored
targets; origins, current bounds and easing remain independent.

Final Sigma has the same ordering at `8F884`: `36B18` releases the scene
before `8F8C4..8F8D8` writes max 1152/min 1104. Publishing that completed
release prevents stale 1104..1104 shadows from trapping the survivor at
1416.5. There is a second gate: ending controller `D3048` reaches `D2B9C`,
which tests the singleton player's X against 1761. A dead P1 remains at 1127
in the upper arena while P2 reaches the exit. Reuse the existing scene-actor
ownership wrapper for that complete controller, scoped to stage 12/section 1.
It retains the entrant through camera wait, dialogue and native completion,
while preserving the original campaign identity. Both developer and public
configs emit the hook. Regeneration changes one game shard; no framework
change is required.

Web Spider's `D8ED4` selects the region and `D9138` manually advances mode 5
background parallax. Paired doors keep independent cameras, but the
background handler treated them as complete shared-camera scenes and skipped
private updates. P2's background X remained 4352 while its foreground moved
4376..4623, requiring background 4394..4578. Exempt paired doors from that
shortcut. Each saved view retains its own region phase and native parallax;
the canonical controller still updates once. Ordinary shared scenes still
reset private region initialization as before.

## Bounded qualification

`tools/coop_playthrough115_scenarios.py` provides `cyber_air_door`, `spider_air_door`,
`magma_entry_camera`, `spider_door_scenery` and `sigma_survivor_victory`.
Run them through `tools/test_mmx4_coop_regressions.py` with an original USA
disc and a development build. Use `--boss-doors both --cameras split
--local-cameras split --hidden-window`; use a fresh output
directory and unoccupied debug port for each run. Repeat with campaigns 0/1
to exchange the native heroes. Put Sigma last because it leaves gameplay.
Normal boot is used for the latest Web checks; two accelerated startup
attempts stalled in the BIOS before entering that scenario and are excluded.

Door checks use native input for admission, actual closing, boss introduction
and control recovery. Cyber's air dash prerequisites are prepared as learned
Jet Stingray ability only (Zero) or native foot armor (X); Cyber's clear flag
remains unset and damage remains enabled. Recorded native action 13 confirms
an actual air dash. The Spider check compares each
view's background with the original parallax formula during crossing.
Dragoon tests either entry leader. The existing `door_animation` case checks
ground walking/dashing, once-per-update animation and scripted movement.

Sigma prepares one HP on the currently selected live health pool, then
requires actual attacks against each native damage rectangle. The first
head's death returns the actor to combat; only the final victory scene counts
as defeating both. P1 remains dead through the native drop, final defeat,
dialogue and ending approach. No result flags or scene phases are supplied.
Protection prevents incidental combat damage but leaves native lethal terrain
active; movement up the final exit steps and ending admission use real input.
The first qualification passed with X as P2. Subsequent native validation
also passes with Zero as P2, using short saber attacks followed by a retreat
from the earth head's suction. Earlier melee/special setups were unqualified
and are superseded by this completed route. No native ending movie or full
credits playback qualification is claimed.

The reduced QA observer makes six RAM reads per sample instead of eleven,
and caches the fixed profile's diagnostic address. Its fields match the old
observer across 24 static fixtures (`observer-parity.json`). It does not
change runtime pacing or the game's update rate. Native text confirmation
prefers the body carrying the script marker when both heroes remain visible.

The door/camera runtime passes all 43 CTests. Native qualification on executable
SHA256 `ecbd879844d869e48aa68e73705afc926799c858a692f0d149f4ea8c520265cf`:

| Check | Evidence | Result |
| --- | --- | --- |
| Cyber inner-door genuine air dash; actual closing, boss introduction and controls | `zero-native`, `native-final-1` | Both P2 hero choices pass, damage enabled. |
| Dragoon first-visit introduction and native camera pan | `native-final-0`, `native-final-1` | Both heroes and both entry leaders pass; actual origins reach 5149. |
| Web paired-door background parallax and fight recovery | `zero-native`, `native-final-1` | Both P2 hero choices pass; 19/20 crossing observations. |
| Sigma upper phases, both final health pools and survivor ending handoff | `sigma-qualified-1`; later `sigma-release116/zero-native-fixed` | P2 X and P2 Zero pass on their separately recorded executables. |
| Additional Web outer air dash baseline | `original-web-readonly`, `web-recorded-buttons-baseline` | Full original replay and corrected short fixture both reproduce the airborne death. Earlier short passes are excluded from regression evidence. |
| Intro part 2 source group at 3167/3296 | `web-bee-approach` | P1 native allocation plus closer approach/drawing pass; no game fix claimed. |

The additional outdoor waiting-phase correction is built as executable
SHA256 `09d3c46cdd526430dcd1f9f37eb774b05c3727ea8c561d2df67c1f25dbc212f2`.
All 43 CTests pass (`swarm-ctest.log`). Its combined native
`final-web-outdoor` run passes:

- Web outer-door genuine Zero air dash with damage enabled: 29 observations
  retain airborne admitted action 19 with the correctly suspended terrain box;
  both real doors close, the boss introduction finishes and both 32HP heroes
  regain independent control. Both views reach native boss-room Y1280.
- Outdoor opening swarm with P1 leading and separately P2 leading: all ten
  original actors activate and become drawable while the partner stays at
  their original position. Composed captures were visually inspected and show
  the flying group entering from behind in the independent views.

`web-recorded-buttons-baseline` and `outdoor-baseline` fail those same checks
on the unfixed runtime. Earlier door/camera qualifications retain their own
frozen executable identity; they are not relabeled as new-binary tests.

The extra legacy `door_animation` run in `native-final-1` passes complete
grounded walking, scripted carry and independent control recovery. Ground
dash opening/carry/closing checks pass, but the renderer diagnostic run hits
the existing starvation watchdog during its boss introduction. It is not
counted as a complete dash pass. Previous stable release qualification is
retained; no watchdog or guest timing setting was weakened here.

## Sigma white explosion and Zero survivor ending

The owner requested a bounded white-explosion reproduction attempt, deferral
only if it did not reproduce, and native Zero/P2 ending validation before the
next stable release. The original runtime reproduces the rendering fault in
native final-boss death phase2/6: the scene reaches its full white interval,
but Zero remains fully colored in both panes. Six bright baseline captures
retain roughly5,600 colored hero pixels. The report was reproduced and was
therefore not deferred (`beads-eio.9.277`).

Private sprite banks read the original character COLs, whereas native
8002A484 applies lighting into the shared working palette at80141F70.
Use that same clamped additive/subtractive RGB transform when defining
private character and UI colors. Preserve the native per-CLUT enable mask,
transparent zero and STP bit. The existing cache key already includes the
exact colors, so lit frames and restored normal colors remain distinct.
No guest palette, scene flag, lighting amount, physics or timing is changed.

Corrected executable SHA256
`dfa0aadf26de2b814daeedabb62308c4b57cdd611f593c6c6e4ffaa4425f6961`
passes the full native Zero/P2 route: actual upper-form defeats, P1 death,
native lower-room drop, actual attacks defeating both final health pools,
victory text, camera release, real exit climbing and General dialogue,
followed by mode11 ending admission. Zero stays32HP, P1 stays0HP and lives
stay2. Stage/position/protection/one-HP fixtures prepare the route; no result
or scene flags are set. The prior initial trial's retreat teleport was
correctly rejected once victory locked the scene; a fresh corrected full
run then passed end to end.

Peak-white captures8..13 are entirely white in the gameplay area, with no
visible hero pixels. Zero returns with normal colors afterward. The adjacent
ramp frame contains two native blue explosion streaks outside the hero areas;
these are not character bleed-through. Captures and frame observations are
retained under `sigma-release116/zero-native-fixed`, with matched baseline
and candidate pixel measurements and build identities. Unit vectors cover
native RGB clamping, mixed subtraction, transparency and STP preservation.

Private evidence is under
`G:/Temp/Codex/psxrecomp/x4-playthrough115/`. Baseline reproductions are
`cyber-baseline-0`, `magma-baseline-probe`, `spider-baseline` and
`sigma-native-victory-geometry`. Rejected early Sigma attack fixtures and
premature post-intro waits are retained as failed setup attempts, not passes.
Final qualification results and binary identities are recorded separately in
that directory. These bounded local checks do not establish a fresh full-game
playthrough, LAN qualification, or Four Player coverage.
