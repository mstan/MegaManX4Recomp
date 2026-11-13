# Playtest36 recurrence review

## Intended scaling behavior

- A boss requires twice its original total damage to defeat.
- Each player owns their damage cooldown. A hit by X must not impose Zero's
  cooldown, and a hit by Zero must not impose X's cooldown. Both can be active.
- Cooldowns are shorter than the original game: the owner's requested starting
  rule is half the native duration, not half the attack rate.
- Normal grounded combos, aerial slashes, cancellations and weakness reactions
  retain their native meaning. Scaling must not add a new restriction on which
  connected attacks count. Native invulnerable attack/story phases remain.
- The original balance remains available when scaling is disabled. The
  alternate reduced-shared method remains a separate selectable policy.

An illustrative native32-frame cooldown would become16 frames for the
attacker, while the partner remains eligible. This example does not assert
that every boss/attack uses32, or that a timer alone defines native admission.

## What the current code actually does

`mmx4_coop_balance.c` halves accepted damage with a fractional carry, retaining
native displayed HP ranges and phase thresholds. It intercepts selected native
post-hit cooldown stores, tracks per-seat countdowns, and gates accepted
overlaps in8002DE30. Native collision cleanup at8002DD04 still runs.

In Separate mode the native shared timer receives the smaller private timer,
with a minimum of one tick for native expiry cleanup. Boss hit handling also
uses duplicate-contact keys, damage/vulnerability tables and native reaction
states. Private timers alone therefore do not constitute independent native
hit handling.

Playtest36 additionally caps recognized Zero blade recovery at four boss
updates and records one damage admission per native projectile lifetime,
target, health bank and seat. That lifetime limit is an implementation choice,
**not an owner requirement**. It must not be treated as correct merely because
tests enforce it. Native8002DE30 rejects a repeated nonzero contact key;
8002DD04 clears that key when no contact remains. Native800981AC also changes
blade contact keys. Consequently one projectile lifetime is not, by itself,
proof of one native damage opportunity.

Fractional damage can produce an accepted hit without changing an integer HP
byte. Whether that affects the particular reaction being reported is still a
hypothesis, not an established cause. Native damage returns and reactions must
be inspected as well as HP. Replacing it with literal doubled HP would require
preserving native phase thresholds, health bars, multi-pool bosses and death;
it is not a safe one-line replacement.

## Why the prior acceptance was insufficient

In particular, the prior fresh scaling-off control accepted fewer attacks than
its scaling-on counterpart. That did not reproduce the owner's observed
contrast and should have prevented closing157. Improved hit totals in another
moving fight, synthetic held-blade protection and passing unit tests do not
repair this missing comparison. Historical results remain evidence for their
bounded observations, not acceptance of the reported experience.

The review must follow attempted connected attacks through actual overlap,
native contact key, private cooldown, native vulnerability table, accepted
attack return, HP/fraction and reaction. Ground single slashes, full chains,
cancelled follow-ups, basic aerials and upgraded aerials need separate controls,
with Zero in either seat. Partner buster/fire overlap, weaknesses, deaths and
snapshot restoration are necessary counterexamples. Matching counters without
matching visible blade/impact/reaction behavior is insufficient.

## New evidence intake

Both ZIPs pass CRC checks and contain the published36 execution identity
`93f56a5e8f8d35fdee77384cb5e3772b5dfbd9b973dcbcd76206bba14f883dc5`
and co-op profile `4944a647210cafcb6e9c1a22f8ae2b3e9565eef1`.

| Archive | SHA256 | Relevant contents |
|---|---|---|
| Storm Owl36.zip | b21405deec1ab0731871b10ea60650729a80e0a24a6590b4e9b1539a608eb3e3 | Paired network sessions, local sessions, scaling-on and scaling-off settings, Owl and Cyber routes. |
| Opening stage Web Spider Frost Walrus36.zip | cb76eb51321af46e537d3991099b99604d563ef09f6d627e2903af09abb2d8e4 | Multiple sessions including repeats from the first archive; use recording identity rather than counting the repeats as separate reproductions. |

One paired Owl session, `1792318160615607`, records P1 dying at2905/524 while
P2 is6348/426 at input20539. P2 also dies by20898. A later retry reaches part2.
The existing `storm_survivor_exit` substitution applies only when P1 is dead
and preserves the native X>=6352 test. It does not cover a living P2 reaching
the exit while P1 remains alive behind. The recorded6348 position is also short
of that threshold; a teleport beyond6352 is not proof that a walking player can
reach the trigger. These are investigation leads, not a validated new fix.

## Current burndown additions

Correctness intake is `beads-eio.9.188`, scaling remains `.177`, Cyber reticle
is `.189`, and local split feasibility is `.190`. Existing issue references
remain in the main numbered checklist.

## Local split feasibility

The existing code already maintains two native cameras and shared-world enemy
admission, with scoped render transactions to protect simulation and VRAM.
`present_local_view` currently chooses one seat from the netplay local port and
commits one view for that process. Local8:3 is therefore feasible, but needs
two view targets and a window compositor, viewport/aspect/resize handling,
per-view HUDs and appropriate shared-scene behavior. Simulate the world once;
two views must not run AI, damage, stage events or timers twice. Verify state
restoration and performance before offering it as an optional local mode.

Local8:3 is deferred for this correctness release: the presentation/compositor
work above would delay the active progression and hit-registration fixes.
The feature remains tracked in `.190`; no local split implementation is claimed.
