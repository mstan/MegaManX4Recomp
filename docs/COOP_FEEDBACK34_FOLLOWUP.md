# Playtest34 follow-up: saber contacts, local flashes and Frost ice path

Playtest35 packages these corrections. The exact missed-hit report157 remains
unconfirmed; the evidence and limits below are retained.
Tracking: `beads-eio.9.177`, `beads-eio.9.179`, `beads-eio.9.180`.

## Evidence intake

`Single slash hit 34.zip` is CRC-clean, SHA256
`fe0a982977b095ef5cc9f8676bb3940deea65454d29e12fb2f893c7237b2a11e`.
Its two sessions identify Playtest34 (`7669a86f`), Split netplay and boss
scaling with the default per-player cooldowns. The excerpts have P2 attacking
and P1 idle. They enter the Frost refight, not the outdoor Frost stage.

## Saber contact lifecycle

The outer ENEMY_HIT (`8002DD04`) filter skipped an immune attacker's entire
native overlap scan. Native DD04 clears the target's last-contact byte at+65
when no shot overlaps. Skipping that cleanup can preserve an old saber token
across a gap and incorrectly reject a new same-key swing after immunity ends.
These are reusable native keys: Zero's `800981AC` derives shot+64 from
subtype/pose, rather than assigning every new swing a unique sequence number.

Move private immunity admission to the child `8002DE30`, after DD04 finds
overlap and before it stores a token, consumes the projectile or damages the
target. Its native duplicate result7E blocks damage while retaining a held
contact. Empty overlap still runs native cleanup. Both character seats use
their existing independent histories and cooldowns; their durations do not
change in this patch.

The new both-seat regression fails the Playtest34 implementation at the fresh
same-key swing and passes the correction. A held overlapping swing remains
one hit after another cooldown interval. Existing hit-pause, consumption,
partner, damage-reaction and snapshot checks pass.

Fresh native Frost trials use the original disc, setup-only checkpoints,
positions and health protection, then actual controller inputs. With the
recorded P2 cadence5710..5980 transplanted into fresh fights, the baseline
accepts2 hits/6 native damage units; the correction accepts3/9. Repeated
upgraded aerial trials accept10 hits/30 units in420 frames in both builds.
The new source trial's sprites, blade arcs, recovery flashing and successive
damage were visually inspected. The boss remains free to move and attack.

Actual Split LAN also completes held-swing48->47, ground48->37 and aerial48->44
trials, with matching core/mod CRCs through9362. The later mixed-attack phase
of that script failed to land an accepted partner-cooldown overlap. A fresh
both-alive mixed trial also failed its overlap assertion. Both failed receipts
are retained (`lan-ice-hit35/` and `lan-overlap35/`); neither full script is
reported as passing. The ice and earlier combat phases remain separately
qualified at that point.
A separate fresh local mixed trial does accept both native buster and saber
hits, including three hits during the partner's cooldown: nine accepted hits,
18 pre-scaling damage units, 48->39 HP in 240 frames. Its native HP-store trace
includes X and Zero hits two frames apart. Receipt: `mixed-native35/`.

A corrected actual Split LAN control now qualifies the missing overlap. The
earlier probe interleaved full diagnostic reads and screenshots with short
input pulses: its supposed aerial slash arrived after Zero had landed. Keep
those observations outside the short input sequence. Native HP stores now
accept X's class0 buster at5845 and Zero's class34 aerial at5857, during the
partner cooldown, taking48->46 HP. Both rendered views were inspected. Nine
core/mod CRC samples match through5906. This changes the private test's input
schedule, not gameplay to accommodate a test. Receipt:
`build-pr23/feedback35-continued/corrected-controls/`.

A further basic green aerial trial requests low hops every24 observed frames
for320 frames. Baseline accepts six class27 hits; the correction accepts seven.
These asynchronous fights again differ in native motion and input timing,
so the extra hit is not a causal before/after proof. The native contact-write
trace does demonstrate the cleanup change: the baseline first runs its
post-hit empty-overlap cleanup37 frames after the hit; the correction begins
that cleanup nine frames after the hit, while private immunity still runs.
Ground recovery, ordinary green blades, landing and hurt flashes were reviewed.
Receipts: `feedback35-continued/basic24-before/` and `basic24-after/`.

Private receipts: `build-pr23/feedback35-intake/air34-before/`,
`air35-after/` and the captured review sheets beside them. The original
netplay recordings retain their identity and are not relabeled as offline
replays.

## Later Frost ice bridge

The separate native stage controller type21/subtype3 (`800BA340`) starts its
path sequence when signed PLAYER.x reaches3168. The prior opening-cycle fix
at `800B9F74` cannot satisfy this second predicate.

Baseline: P1 remains144/347 and P2 waits at3246/361. Across594 observed native
frames the later controller remains at phase0. Moving P1 across3168 starts
the sequence. The new `800BA39C` callback allows an eligible living P2 to
satisfy just that initial predicate. The native timer, particle generation,
tile updates and platform allocation still run once.

The corrected native run advances the bridge while P1 stays at144. In actual
two-peer Split LAN, it creates six native type17 platforms; ordinary P2
movement reaches3717/475 on the ice path while P1 remains144. The inspected
P2 presentation shows Zero standing on the formed path. Core and mod CRCs
match through5081 for activation and crossing. Both-seat character orders,
the exact3167/3168 boundary, negative coordinates, inactive/dead partners,
other areas and presentation/projection exclusions pass the lifecycle tests.
The committed `frost_late_path` native case also passes with X as P2 in Zero's
campaign, while Zero remains at spawn.

Private receipts: `build-pr23/feedback35-intake/ice-path-before/`,
`ice-path-after/` and `lan-ice-hit35/ice-rows.json`.
The reusable native harness case is `frost_late_path`.

## Distant intro red flashes

The owner promoted report158 to active work. The native type28 explosion
controller (`800BBC14`) centers explosions around the canonical camera in
`800BBDF4`, but applies red31 to the shared palette in `800BBEA4`. Intro part2
also has a type2/subtype0 palette program (`800B5960`) that cycles red1..3.
The latter reproduces a shared red pulse with P1 ahead and P2 still at the
authored starting position. The stronger type28 branch was identified from
native code and covered by unit controls, rather than claimed separately
reproduced by that capture.

During a private camera draw, remove these red contributions when their
source viewport is distant. Rebuild and upload the unlit native working
palette inside the existing render transaction. Near or overlapping views
retain the flash. The transaction restores the shared palette, RGB values,
upload flags and VRAM afterward. Unified play and simulation stay native.
Other palette programs, non-red effects, white flashes and scene fades retain
their existing behavior.

An actual two-peer Split LAN run leaves P2 at168/443 and advances P1 from4100
past5000 in intro part2. The distant peer performs210 scoped corrections;
the near peer performs none. Both images were inspected: the near view keeps
its red pulse and the far view keeps normal colors. Fifteen matching core/mod
CRC samples continue through5179. Each peer commits907 private views, with
zero VRAM leaks, device-read faults, watchdogs, verification mismatches or
local-view state leaks. The observed pulse is qualified; no claim is made
that every scripted intro explosion has been traversed. Receipts:
`feedback35-continued/corrected-controls/intro-blast-*` (before the pulse
correction) and `feedback35-continued/red-after/`.

All35 Windows title checks pass after the palette change. Linux GCC split,
combat and lifecycle executables also pass. Automated launches are hidden/headless and use a dummy audio output.
Production Playtest35 qualification and publication receipts are recorded
under `build-pr23/release35-qa/`; these developer receipts do not by themselves
claim cross-platform gameplay compatibility.
