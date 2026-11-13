# 1.0.3 stage feedback

The 1.0.5 release integrates these correctness fixes. Fresh generation and
native controls confirm the P2 corner and snapshot/projectile return; all
opening flying-group emitters are now qualified separately. The owner withdrew
the intermittent stutter report. Idle Dragoon slowdown remains unreproduced in
bounded current-host controls, with no performance fix claimed. See the
[source and tests](https://github.com/mstan/MegaManX4Recomp).

## Opening dragon

The earlier background-dragon fix used the canonical P1 camera to turn the
native screen-relative body and type10 attachments into world coordinates.
With P2 leading, the first corner attack consequently appeared beside P1.
Both native coordinate conversions also read that canonical camera:
`80041DF0` leaves world space and `8004205C` returns to it.

Record the nearest living participant while the actor still has world XY,
bind each original conversion once to that participant's saved camera, and
use the same origin in private drawing and background-shot initialization.
The shot hooks at99634/99650 run after the native camera loads, preserving
their original fixed-point arithmetic. Owner records live in allocated guest
RAM, included in the existing snapshot/digest contract. Drawing never changes
the records. A new world departure refreshes a reused native slot's owner.

Native before/after checks start before the first encounter and use controller
movement to trigger its actual phases. P2 leads at X3238 while living P1 waits
near X200. Before: camera origins40/2976 put the corner body in P1's viewport.
After: the body/attachments appear only beside P2; the subsequent native return
reaches world X2881. The P1-led control also completes the background attack and
return. This qualifies the first flyby, not a fresh audit of the later boss.
The other roster also restores a current-build corner snapshot after a fresh
stage clears the owner records, retaining the correct body/view and return.
The final headless original-disc check repeats that reset/load sequence and
observes four initialized background-shot samples at world X3070--3187 beside
the progressing P2, followed by the original return near X2881.

## Volcano fireballs

Three independent dependencies excluded P2:

- The bounded stationary spawn scan excluded category3/type33 stage emitters.
- `800BC748` tested the singleton player's position for its original timer and
  platform-admission predicates.
- Type33 secondary actors at `800A03B8` used the canonical camera, including a
  falling branch whose newly allocated body still has no world position.

Admit only this Volcano emitter family to the bounded scan, retaining shared
placement visits. Run one native emitter update with an eligible living hero
and its view. Existing serialized secondary allocation provenance supplies the
fireball's camera, including initial placement and later lifetime checks.
Original terrain-impact shake commands go to nearby living views; unrelated
camera origins, limits and distant players' shake state stay personal.

Initial native probes reproduce P2=0 fireball observations versus P1=35;
the candidate produces34 for each leader. The wider original-source control
adds the early rolling branch: P2=0 fireballs/0 quake observations versus
P1=304 fireball/73 quake observations. These are repeated samples of original
actors and commands, not counts of unique spawns. The candidate's corresponding
P2 control produces299 fireball/61 personal quake observations, with both heroes
healthy. The P1 control also retains its original rolling effects.

In part2, the original P2 control already produces platform fireballs, but
receives no shake and does not start the falling branch at X2050. The candidate
receives9 personal quake observations and actual phase1 falling fireballs near
X2049 in that player's world. Both leaders' part1/part2 controls retain32HP.
The highest simultaneous fireball count observed is16 of32 secondary slots.
Nearby effects can legitimately shake both views near the shared start;
distant views retain separate shake commands. A setup inside the lava at
X2688/Y700 was excluded.

## Elevator and performance

One recorded elevator death is located around30:23, near overlay frame106705.
The diagnostics contain two P1 deaths at almost the same place:6027/1361
(22HP to zero), then6030/1361 (32HP to zero), with P2 remaining alive. A neutral
native probe reproduces6030/1361 exactly despite ordinary damage protection,
while P2 at6052/1359 retains32HP. The terminal image places P1 beneath the edge
of an overhead blue panel. The final original-disc trace records native33C14
setting the fatal0x80 marker from32HP when PLAYER+89 has both vertical contact
bits (0x0C); native33C74 then clears HP. This is the original crush predicate.
P2 beside that panel remains healthy. The reported position is qualified as
an authentic panel-edge crush; no elevator collision or death rule is changed.

Intermittent stutters and idle Volcano part2 slowdown remain separate reports.
Neutral-start developer-build controls average59.5--60FPS, with occasional long
GPU frames on this host. These controls use the harness's own boot/mod layout;
copying video fields does not establish production-profile identity. A verified
native-resolution control has the same approximately14ms mean scene-timer
interval, so those timing fields alone do not establish a fill-rate cause.
No performance fix or cross-release performance
conclusion is claimed. Missing opening-stage flying bees remain open in the
existing intro audit.

## Checks and limits

All38 Windows CTests pass, including the final added boundary checks. Added checks
cover either dragon owner, complete camera restoration, fractional XY,
owner refresh, emitter provenance, nearby shake, trailing-player eligibility,
stationary emitter admission, duplicate suppression and unrelated modes/stages.
New native helpers are `intro_dragon_views`, `magma_effects_field` and
`mushroom_lift_crush` in `tools/test_mmx4_coop_regressions.py`.

The final shot/crush mechanics run uses headless OpenGL and a documented
diagnostic-only120-second host watchdog deadline, with no composed screenshots.
Earlier body/Magma composed checks use the ordinary deadline. Extra hidden
startup/IRQ waits were retained for the existing intro/diagnostic investigations
(`beads-eio.9.76`, `beads-eio.3.253`); they do not establish a new game regression.
The engine and release watchdog settings are unchanged. This is bounded
mechanic qualification, not a general startup/performance stability claim.

Fixtures prepare original checkpoints, safe positions and damage protection;
native actors, controller input, terrain and scripts produce the observations.
No event completion or actor phase is forced. Observation-only probes require
assessment of their JSON and composed images; their harness status alone is
not a mechanic pass. Runs are private, hidden and muted. No new LAN or Linux
gameplay qualification is claimed.