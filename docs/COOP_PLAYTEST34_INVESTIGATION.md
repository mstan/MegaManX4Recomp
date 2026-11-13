# Post-Playtest33 report qualification

These are development findings, not a published release. Tracking:
`beads-eio.9.176`; the new saber investigation is `beads-eio.9.177`.

## 155: charged fire resets the game

On current33, ordinary simultaneous fire and a lethal overlap survive.
Fully charged X Rising Fire is the missing condition. Against native Frost,
X/P2 shot13 and Zero/P1 Ryuuenjin35 reproduce the reset on the first trial.
Native write tracing identifies decompressor80016FF4, store80017088,
RA80015F5C, advancing from staging8016FEA8 into PLAY801721C0. This is memory
corruption, not a normal defeat, intentional restart or host crash.

The preceding graphics call is deferred effect26 at800B28CC. Its B2924
parent lookup uses a guest pointer into the allocating player's projected
shot arena. During the later shared effects pass, that same address can
contain the other character's raw attack. Copying Zero's empty compressed
graphics pointer into X's effect feeds unrelated data to decompression.

The existing effect-allocation ownership now also binds800B28CC to P2's
context. Both development and release configs include its generated entry;
regeneration changes shard53. The regression models two live shots at the
same guest address and verifies that an expired private parent cannot attach
to the other player's still-live shot.

Native post-fix qualification uses real controller attacks after setup:

| Arrangement | Charged overlaps | Result |
|---|---:|---|
| Zero/P1, X/P2 |23| Frost48 to0; native victory and return; both32HP |
| X/P1, Zero/P2 |18| Frost48 to0; native victory and return; both32HP |

Flames, charged uppercut and boss burn frames were inspected. The first
receipt reports24 trials because its loop included a final no-attack break;
23 actual issued overlaps is the corrected count. This does not establish
that every trial deals damage: native range, weakness phases and recovery
still apply. The Sigma arena was not matched by the bounded setup, so no
fresh matched Sigma fight is claimed. The corrected attachment is shared
across bosses and does not alter their health or weakness logic.

Private receipts: `feedback34-charged-fire-before`,
`feedback34-charged-fire-alloc`, `feedback34-fire-return-after-final`,
`feedback34-fire-return-x-first` under `build-pr23`.

## 156: refight wall return

Three recordings bind the symptom to Slash's return pad:
1792257869050172/52938,1792260039189280/23869,
1792260904456189/11674. A current-source native Slash defeat reproduces
X/P2 returning at1960/1195 and dying to a crush while Zero/P1 is safe at
1936/1194. Invulnerability does not prevent this native crush.

Hub-return checkpoints11–18 now use the existing grounded-partner arrival
gate. Native standing-box terrain validation and character foot alignment
precede visibility and the incoming beam. P1's authored checkpoint and
portal progression remain native.

Actual Slash defeat and return pass with each character in each seat. Both
heroes finish controllable at1936 with native X/Zero foot heights,32HP and
cleared personal locks. A P1 death before P2's final blow also returns safely;
the existing checkpoint refill revives P1. Unit coverage exercises both
rosters across all eight hub-return checkpoint records and rejects unsafe
terrain before releasing the beam. It is not native gameplay coverage of
all eight refights.

Private receipts: `feedback34-return-native-before`,
`feedback34-return-native-after`, `feedback34-fire-return-after-final`,
`feedback34-fire-return-x-first`, `feedback34-return-survivor`.

## 126: Owl double capture strands the other body

The original stage7 double-capture and target-switching controls release both
heroes. Moving the same investigation to the actual fortress refight12/0/6
reproduces the failure: flags1/1 at7919 become0/1 at8006. P2 remains in
captured action17 through8989 while Owl resumes several later attack cycles.
Keeping the free hero near Owl exposes the target-switching condition.

The correction extends Owl's own six native release/abort/weakness stores to
the other captured body. It clears only capture command1, preserving negative
pending damage/throw commands. It neither runs Owl twice nor adds a timer
that forcibly unlocks players. Both configs emit the instruction callbacks;
their original words were verified against the disc executable.

Post-fix native refight qualification with Zero/P1 and X/P2 observes both
captured at5195, both released at5284, then both moving under controller input.
One partner-release callback fires. Health remains17/26: this proves a healthy
release, not immunity to the grab's native damage. Before capture, diagnostic
fixtures heal ordinary damage and restore the two nearby floor positions
between attempts. After capture, no HP/position/capture writes occur; only
normal contact protection is enabled, which does not prevent Owl's grab damage.

Two earlier candidate setup failures are retained. One lost both heroes to
ordinary attacks before a grab; another protected the grab window and never
admitted a grab. Neither is counted as a passing capture test. Unit coverage
checks all six release sites, both rosters/targets, command preservation,
wrong-actor and outside-update guards, and exactly one AI update. Windows and
Linux GCC combat tests pass. This does not claim every interruption sequence.

Reversing the roster also passes an actual double grab, release and subsequent
both-seat movement, leaving32/23HP with one partner-release event. The release
can therefore occur with either seat being Owl's selected target.

Private receipts: `feedback34-owl-refight` (before),
`feedback34-owl-refight-after-pin` and `feedback34-owl-refight-after-x-first`
(after), plus the earlier stage7 controls.

## Verification shared with the saber investigation

All35 Windows title checks pass across the suite and focused reruns. A startup
helper timed out under initial load and passed when rerun serially; it is not
an uninterrupted35/35 initial run. Linux GCC builds and runs the balance,
combat and lifecycle regression executables. The recompiler emits the new
function/instruction callbacks in development and release configs, checked
by the hook and release-config tests. No new published build is implied.

See [the saber investigation](COOP_HITREG_FEEDBACK_20261017.md) for the separate
visual-feedback correction, real two-peer Split qualification and unresolved
limits of the original missed-hit report.
