# Complete Story

An optional, disabled-by-default mod for local solo and X + Zero co-op. Enable
**Complete Story** in the launcher's Mods menu before starting the session.

- After four Mavericks, Zero enters the playable Memorial Hall Colonel duel.
- After that duel, either campaign plays Zero and Colonel's original movie.
- Both Double and Iris are fought, in the selected campaign's original order
  followed by the other encounter. The existing co-op option composes with this
  setting; it does not add a third encounter.
- X sees Iris's aftermath after the optional Iris encounter and the original
  Zero/Sigma flashback before Sigma.

Movies use the original CD/MDEC player, including normal skip controls and the
selected Japanese movie/subtitle replacement. No movies or other disc data are
distributed in the package. The selected hero, permanent progress, rewards and
native memory-card format retain the original campaign's identity. Enabling
the mod does not replay an event that an existing save has already completed.
The regular online co-op profile retains its own host settings; this local mod
is not an additional online session setting.

## Native route

Original SLUS-00561 disassembly identifies two Zero-specific Memorial Hall
bypasses: the four-clear route selector at `8002E54C`, then the frontend fade
completion at `8002EE84`. The mod changes their route registers, never PLAY+43
or the player's character. Both instructions remain original with the package
off. Each instruction hook requires the exact original word in live memory.

The existing Double/Iris loader selects the other encounter's banks, scripts,
portraits and voices for its second round. The standalone package reuses that
state machine and its serialized round/defeat/reload fields. Co-op remains the
sole owner of that state when the packages are active together, avoiding
duplicate defeats, reloads or controller updates.

The original optional fights lack some attacks belonging to the other hero.
`mmx4_story_damage.h` shares the existing co-op admission rules with solo play:
Iris retains her eight-hit armor exposure and authored core damage units;
Double admits Zero's missing first standing slash and aerial spin using his
existing saber damage. Real attack identity, native collision, hit tokens and
cooldowns are preserved. This adapter is scoped to the original stage11
encounter, exact actor pool and verified damage-table pointers.

Completed Memorial Hall enters the existing mode10 fade/movie gateway. Its
original movie8 call selects movie7 in this room, then returns to native mode9
and the stage board, rather than loading a nonexistent second section.
The same gateway admits movie8 for X after the second story boss and movie9
for X before Sigma. Native code owns playback, completion, stage loading,
dialogue and control recovery. No host callback recursively runs a movie or
CD loader.

## Repeating native checks

`tools/test_mmx4_complete_story.py` runs isolated, muted original-disc profiles
against a development executable. `--campaign 0` selects X and `1` selects Zero;
`--coop` adds two-player Split. The `memorial`, `fortress` and `sigma` cases
bypass earlier campaign prerequisites and prepare authored approaches, then
require native boss damage, movies and continuations. `--stock-story` provides
the original Memorial control. `--natural 7` observes the full Colonel movie;
the normal check exercises native Start after MDEC/XA progression.

Solo position setup uses native Pause so byte writes cannot race physics.
The Memorial fight uses ordinary movement and attacks without repositioning.
HP fixtures shorten a fight but never supply a victory, movie-complete flag or
the result being validated. Failed preliminary QA receipts remain distinct from
qualified runs. Movies are captured through the packed RGB888 scanout path;
the high-resolution gameplay capture does not support that format.

Validation status and final evidence are recorded on `beads-eio.9.283`.

## Qualified preview

The preview is based on stable 1.0.17 (`ce00634`) without framework or UI pin
changes. All 44 Windows CTests passed; the four affected checks passed again
after sharing the cross-character damage adapter. The package resolves alone,
defaults off and changes the session compatibility identity.

Bounded original-disc runs passed Memorial Hall, both fortress encounters and
the pre-Sigma movie transition in both solo campaigns and both two-player Split
campaigns. Native attacks delivered the boss victories, including P2/Zero's
Colonel finish in X's Split campaign. Save/load between the two story encounters
preserved the second round. Canonical campaign, lives and clear/progress fields
were checked at the continuations. Native playable/control gates recovered;
these checks do not establish a complete campaign playthrough.

Each added movie started and returned once, with MDEC frames and XA audio
progression. X's Split Colonel movie also completed naturally at the original
descriptor's full 1,216 frames. Other movie checks exercised native Start skip
after decoding began. The development co-op observer re-enters the movie entry
hook, so plugin-entry counters count twice; native movie-start/return counters,
not that wrapper count, establish single playback.

Evidence and raw failed preliminary receipts are retained outside the source
tree under `G:/Temp/Codex/psxrecomp/x4-complete-story`. Those profiles reported
generic OpenBIOS low-RAM service compilation falling back to the interpreter;
the statically recompiled game story path still ran. These are correctness
checks, not performance measurements or proof of fully native BIOS execution.
The repeatable helper's explicit compiler option now uses the pinned compiler
tool's supported `--gcc` argument.

Human playtesting, online qualification and Linux packaging have not been
performed for this feature. It remains a local preview, separate from stable
1.0.17, the X1 sprites and the Four Player branch.

## Prepared owner review

`tools/review_complete_story.py` prepares six current-build paused approaches
and provides an encounter selector with per-case Pass/Fail/Deferred notes.
Prepare each campaign with `--prepare 0` / `--prepare 1`; `--verify-states`
checks that cross-campaign restoration preserves both heroes and the campaign.
The profile must use the same executable and committed mod choices throughout.

The local handoff is in
`G:/Temp/Codex/psxrecomp/x4-complete-story/owner-review`; its
`Try Complete Story.cmd` opens a normal-window, normal-audio preview and the
checklist. It enables Complete Story, two-player Split, the existing x255 damage
multiplier and QA player invulnerability. Boss HP and victory flags are not
patched. Both heroes' native attacks were checked against full-health Colonel:
one hit changed 48 HP to zero. Iris's authored shield/invulnerable phases remain.

P1 is keyboard and P2 is the owner's DualSense. The panel records physical
input ownership only after separate real Right presses are observed with both
debug input overrides cleared. A configured device ID alone is not marked as
verified. Notes, private cards/states, setup receipts and diagnostics remain
inside that disposable profile; ordinary saves/settings are untouched.

### Memorial dialogue correction and selector responsiveness

The new Zero Memorial fight uses the original X/Colonel encounter dialogue and
portraits. The same numeric script in Zero's bank is an unrelated Sigma
conversation whose portrait graphics are absent from this room. The correction
selects the encounter's text, portraits and voice together, preserving Zero's
body and canonical campaign. Co-op reuses X's resident text bank; solo reads
member15 of the verified original `PL00_U.ARC` into private enhancement memory.
That immutable bank is refilled at dialogue admission so a host cache cannot
assume guest bytes survived an execution-state restore. The subsequent Colonel
movie and native board/save continuation remain intact.

The QA panel now performs all game socket operations on one worker, with bounded
timeouts, and delivers results to Tk through queues. Live pad polling can no
longer block the UI thread. A fault-injection check held requests unresponsive
while the hidden real Tk panel continued processing selection/restart events;
the largest observed heartbeat gap was 47ms. A separate native check switched
from the reported post-Colonel dialogue to X's fortress approach through that
same panel, returning paused with the correct campaign and roster. All44
Windows CTests passed, and both solo and Split dialogue captures were inspected.
These checks do not replace the owner's review; their original failed report
and per-case notes are retained.
