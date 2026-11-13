# Single-hit slash investigation, 2026-10-17

Tracking: `beads-eio.9.177`, parent feedback task `beads-eio.9.176`.

The investigation reproduced a misleading recovery display: independent boss
cooldowns can remain active after the native white flash has already stopped.
The candidate restores the native white palette during that private recovery.
Damage, collision admission, combo timing and native boss AI are unchanged.
The broader report of incorrect hit registration remains only partly explained;
the supplied ground-attack cadence produced complete damaging chains in the
controlled reproductions described below.

## Source identity and viewing

The archive enables boss scaling, Split cameras, Both doors and both story
bosses. It omits `boss_cooldowns`; the matching manifest defaults to
`per_player`. This was checked against the baseline manifest.

P2 attack presses in the ground excerpt begin at source frames 5409, 5420,
5430, 5441, 5478, 5489, 5500, 5510, 5520, 5546, 5556, 5569 and 5578.
P1 supplies no attacks. P1 takes damage 32→28 at5431, 28→24 at5507 and
24→20 at5583; P2 remains at30 HP throughout that ground excerpt. These hurt
events therefore formed part of the adversarial reproduction.

## Native combat observations

Private evidence root: `build-pr23/feedback34-hitreg/`.
Each named run contains `report.json`, raw PNG captures and a runtime log.
Trials use the original SLUS-00561 disc and native Frost refight checkpoint
12/0/9. Setup fixtures admit the fight, grant the selected skills, protect the
heroes where stated, and position them once. During recorded combat, only pad
input and read-only observations are used. Frost moves and attacks freely.
All launches are hidden and muted, with private settings and memory cards.

| Run | Observation and visual interpretation |
| --- | --- |
| `cadence33-zero-p1`, `cadence33-zero-p2` / single | Each seat lands15 deliberately spaced ground attacks over600 game frames,48→33 HP. The blade, impact spark and HP changes agree. Frost continues his walk, punch, jump and sliding actions. |
| Same runs / combo | Native24/25/26 chains appear in both seats. P1 reaches20 HP and P2 reaches13 HP from48 in their separate600-frame fights. The continuously moving boss makes these totals unsuitable as a seat-equivalence assertion. |
| `cadence33-zero-p2` / combo, frames1144–1179 | Finisher26 damages Frost at1144. A new first slash24 visibly overlaps during1163–1169 while HP stays45. It falls within the attacker's32-tick private recovery, whose native flash has already expired. This is the concrete misleading-feedback example. |
| `right33-zero-p2` / combo | The long457→629 damage gap has a different cause: Zero remains near1260 while Frost retreats to about1102. The blade falls short. Damage resumes as Frost walks back. This interval is not evidence for an excessive cooldown. |
| `cadence33-zero-p2` / air | Upgraded Kuuenzan34 lands15 accepted hits,48→26 HP. One scheduled jump/attack during the charge produces no active34; it cannot be counted as a rejected blade contact. |
| `right33-zero-p2` / air | Sixteen accepted upgraded aerial attacks,48→24 HP, with approach and landing visible. |
| `basic-air33-zero-p2` / air | Without acquired skills, the green basic aerial slash is class27. Fifteen native HP stores use27, each for one scaled HP,48→33. Transient28 appears upon landing; no HP store in this trial uses28. The larger purple spin is34 and has three native damage units versus27's two. Both use the existing halved recovery. |
| `cadence33-zero-p2` / overlap | X's buster and Zero's aerial attacks both deal damage,48→23 HP. Fifteen accepted damage events occur while the partner's cooldown is active. Neither hero becomes stuck and Frost's native actions continue. |
| `source33-zero-p2` | Transplanting source5399–5631 P2 buttons into a fresh right-side fight yields three complete damaging24/25/26 chains,48→38 HP. Extra presses during existing swing/recovery do not each create a new attack. |
| `source33-hurt-zero-p2` / source | Same excerpt again gives three complete chains,48→38 HP. P1 is hurt32→26 and Zero32→26. Their observed hitpause maxima are0 and5 respectively. The setup immunity expires naturally after refreshing it is disabled. |
| `source33-hurt-zero-p2` / combo | Normal damage and boss movement reduce the number of useful contacts substantially; both heroes finish20 HP. This is retained as a less favorable combat trial rather than discarded in favor of protected runs. |

The normal-damage source run does not reproduce loss of whole P2 chains when
P1 is hurt. Independent main-loop inspection also shows the P2 tick hook runs
before the native P1 hitpause branch. The observations do not establish that
every possible hurt/input-edge interaction is correct.

### Original combat control

Scaling-off co-op runs still contain co-op combat hooks. Consequently an
additional control, `native-only-zero-ready2`, disables the co-op feature
entirely and boots Zero normally. Setup-only RAM writes request the original
stage/checkpoint initialization and establish a nearby starting position.
Its counter list contains no co-op activation or combat counters.

In that control, first ground slash24 damages Frost48→46 at native frame2457,
with timer63 and table `801060F0` immediately afterward. The next accepted24
is at2587,46→44. Earlier output from `native-only-zero` began during HP fill
and is excluded as a clean combat comparison; `native-only-zero-ready` failed
an overly restrictive setup predicate and is also excluded.

The original Frost routine calls `8002DD04` before decrementing its+94 timer.
The accepted ordinary ground-hit path installs `801060F0` and64 ticks at
`80072598`/`800725A0`. The inspected24/25/26/34 entries in that table are all
FF/0; expiry restores `80107A78`. The no-co-op observation supports that
behavior independently of the balance hook. Broad claims about Zero's
slash-dash cancel behavior should not replace this boss-specific evidence.
The TAS authors also distinguish attacks and bosses susceptible to that
technique in their [original submission notes](https://tasvideos.org/2528S).

## Corrected feedback

Independent admission retains one native expiry tick when the other seat is
ready. That tick restores temporary collision tables, while the attacking
seat retains its private4-tick opening-strike or32-tick finisher/aerial timer.
The native palette previously followed only that short shared timer.

`mmx4_coop_balance_flash_ticks()` exposes the longer remaining private timer
for the active health bank. The existing common sprite filter temporarily
applies the native+42/8000 white-palette flag, calls the original renderer,
then restores the complete halfword. Local-view reads do not advance timers
or change the balance digest. Off/shared modes and unavailable, inactive,
dead or ordinary actors return no override.

The generic renderer consumes this flag at `80024574` and chooses white CLUT
7840. The correction uses the existing render path and preserves the original
boss palette on alternate recovery ticks. Boss timers, hit tables, HP and
attack identities remain simulation-owned.

`flash-before33` and `flash-after34` record the same source-input excerpt in
fresh fights at a denser capture interval. Both first two chains contain six
accepted native24/25/26 HP stores and reach41 HP. Afterward the candidate boss
chooses a retreat/jump sequence: the third chain's first hit occurs at about
108 pixels of center separation, and subsequent slashes fall short. The
complete runs therefore end38 and40 HP respectively; they are not presented
as a deterministic before/after replay.

The candidate invokes the restored flash89 times during the ground trial.
Viewed contact sheets show white/normal recovery frames after the native
timer has returned to0; the baseline shows an almost immediate return to the
normal palette. The new flash makes the cooldown visible while preserving
the observed first two chains.

The candidate's `fire` trial exercises native Ryuuenjin35: five accepted HP
stores reduce48→36, enter weakness state1/10, continue into1/11, and retain
the red burning palette and flame reaction. Both players remain32 HP and
active. This is separate from the charged-X overlap corruption report.

Review artifacts:

- `flash-before-after.mp4`: 2.3-second side-by-side comparison of the first
  two chains, with before/after labels and source-cadence frame numbers.
- `flash-before33/source-review.mp4` and `flash-after34/source-review.mp4`:
  complete local excerpts, including the later divergence.
- `basic-air33-zero-p2/air-review.mp4`: basic green aerial saber and native
  landing transitions.
- `flash-after34/fire-review.mp4`: weakness reaction and subsequent recovery.
- `*-review-contact-*.jpg`: annotated capture sheets used for visual review.

Movie image durations follow observed game-frame differences divided by60.
Captures are asynchronous and can lag the sampled memory. One queued image
was absent in each dense ground run; exports skip it and retain the resulting
time gap. These are sampled review movies: they do not resolve every60 Hz
palette alternation. Raw PNGs retain the game's own visible HUD frame.

Baseline executable SHA-256:
`403b0dc6697e754eee0e7a0ec52f649dc8b776fa8379ff6781288e989869d8d5`.
Candidate executable SHA-256:
`fcb07ac9632b5b71da3b17974893582f105f0fe4e01224fccdea3ce73dd9cf02`.
Both local EXE image paths used by the setup configs hash to
`dae43d44a9f3176208830ddbba0607bb627ebcd7faf9b36fc03dd0ffda34d2e8`.

## Repeatable probe and qualification limits

`tools/probe_mmx4_hitreg.py` records each trial, counters, native HP write
traces, body/action snapshots, active attack classes and composed captures.
It uses a new private directory and rejects an existing output path.
`--basic-saber` selects the ordinary aerial move; `--vulnerable` stops
refreshing setup protection and puts idle X in contact range. `--source-inputs`
transplants the source P2 buttons; it does not replay the source RNG/state.

Example, from the repository root with an already prepared private runtime:

```powershell
python tools/probe_mmx4_hitreg.py `
  --runtime-dir build-pr23/feedback34-hitreg/source33-zero-p2 `
  --executable build-feedback34-debug/MegaManX4Recomp.exe `
  --output build-pr23/feedback34-hitreg/review-repeat `
  --bios build-capsule33-debug/bios/openbios.bin `
  --disc 'mmx4/Mega Man X4.cue' --cases single combo air overlap
```

The balance regression executable covers private recovery visibility,
both-seat timing, current health-bank selection, expiry, snapshot restoration,
forget/reset behavior, local-view digest stability and inactive actor guards.
The native probe and Python syntax check completed. The parent task completed
the shared full-suite and real two-peer Split qualification described below.

The local visual trials use one process and a unified view. They do not
establish cross-peer identity or qualify every boss and phase. A partial P1
aerial trial ended after a debug socket timeout; two older runs completed
their recorded trials but exceeded a15-second process-cleanup wait. Those
conditions are retained in the raw reports. The later qualification helper
permitted60 seconds for cleanup. The committed helper first requests a graceful
quit so diagnostics can flush, with bounded termination as a fallback. All
investigation-owned game processes exited.
No release, merge or production executable update was performed.

### Final two-peer Split qualification

Private receipt: `build-pr23/feedback34-lan-hitreg-positioned/`. Both hidden,
muted LAN peers use the final development executable, actual separate views,
Both doors and per-player scaling. Setup travels through synchronized developer
packets; there are no direct netplay RAM writes. Native Frost remains free
to attack and move during pad-driven attempts.

A fresh close-range mixed attack trial accepts damage while the other seat's
cooldown is active,48 to45HP. Its14 core/mod CRC samples match through6271.
The following eight ground-input/capture observations produce290 scoped flash
draws on each peer. Both rendered views were inspected: ordinary palettes,
white recovery (including capture5), saber poses, scenery and independent
camera placement remain coherent. Six additional CRCs match through6668;
the temporary rendering flag has not left shared simulation state divergent.
These are observations at selected presentations, not pixel-identical views.

An earlier multi-case run separately completed held-swing48 to47, ground48
to26 and aerial48 to41 phases, with4/15/19 matching CRC samples respectively,
through7405. Its next setup teleport was rejected before the mixed trial. A
second fresh mixed trial produced no accepted overlap because the participants
had separated; it is retained as a failed probe. The final positioned trial
restores contact range between attempts rather than forcing boss hits or
changing collision results. No failed full script is relabeled as a pass.

All35 Windows checks pass across the full suite and focused reruns, including
new callback-emission coverage. Linux GCC balance/combat/lifecycle tests pass.
The larger missed-damage report remains open: the source excerpt is a cadence
transplant into fresh native fights, not an exact replay of the original
netplay session or a qualification of every boss and phase.
