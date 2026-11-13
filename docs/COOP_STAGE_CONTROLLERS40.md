# Playtest39 stage-controller follow-up

The five qualified fixes (183,184,190,201,202) are selected for Playtest40,
alongside the personal-menu/private-energy work based on commit `948f6a25`.
The burndown tracks each of the21 incoming symptoms separately (182–202).
Unqualified Dragoon and lift-command candidates are excluded from the release;
their private patch is retained under `build-pr23/feedback39/deferred-magma-lift.patch`.

## Native qualifications so far

## Candidates and remaining checks

- The deferred lift camera candidate treats+47 as a command written every tick, including
  repeated identical values. Its regression check passes. Short native ascent,
  jump and P1-death controls preserve P2 health; they do not yet qualify the
  reported high-jump offset or blue-panel mechanics.
- The deferred Dragoon candidate selects the progressing/nearby living hero. A newly
  allocated falling fireball has no coordinates and initializes relative to
  CAMERA; it must retain the allocating seat and that seat's camera rather than
  choose the nearest hero to0/0. Bounded before/after field qualification remains
  required. Do not confuse authored rolling fireballs with the random falling
  branch.
- An original39 control already spawns some part2 platform fireballs, including
  an832 source, with P1 behind. This bounded result does not reproduce item195;
  do not claim that report is fixed merely because the candidate also spawns
  them. The sampled part1 location1425 precedes the next controller's placement
  window and does not qualify the random falling branch.
- The Slash midboss actor spawns in a bounded P2-led survivor setup. Placement
  alone does not establish fight behavior. Part2 stop/start probes initially
  teleported into unsafe geometry and are excluded; those reports remain open.
- The initial opening-stage spawn check found static type7 wall enemies. The
  later flying groups are allocated by BF3B0 after falling-object phases, so
  that check does not clear item200. A walk blocked before a trigger threshold
  does not establish an object-trigger failure either.
- Long teleports can leave the eased camera/placement window behind the hero.
  Wait on safe terrain before movement. Initial pit/crush deaths from invalid
  setup positions are excluded from the reported regressions.
- Storm laser projection, remaining Mushroom traversal/panels, exact Chaser
  dash recurrence, Slash background/stop/start, Dragoon endpoints/shake and the
  remaining opening effects/spawns stay open. No LAN qualification is claimed
  for this batch yet.

## Engineering checks and private artifacts

The Windows runtime builds and all36 CTest checks pass. GCC/Linux lifecycle,
combat and split boundary executables also compile and pass, including the
negative carry override. These model checks supplement native qualifications;
they do not prove a stage mechanic by themselves.

Private evidence is under `build-pr23/feedback39`: intake manifests and unique
sessions; original39 surveys; `candidate-refined`; `candidate-focused`;
`candidate-controls`; `midboss-native-kill`; `candidate-final`; and the
`lava-original39` control. JSON observations include
native body mirrors, saved camera state, controller/actor bytes and original
enemy state. `*-pair.png` captures the composed8:3 presentation, while plain
captures show only the canonical view. Survey wrappers retain per-case errors;
their outer harness status must not be treated as an all-green result.

Tracking: `beads-t56p` and `beads-eio.9.197` in the central database.
