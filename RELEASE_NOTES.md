# Prepared 1.0.3 - portable generated texture-pack defaults

- Rebuild against the accepted framework backport (#747) for portable generated defaults.
  Generated texture-pack directories follow the current installation and remain
  ephemeral, including while HD textures are disabled.
- Explicit resource selections retain their meaning. Legacy absolute selections
  are not migrated because the saved format cannot distinguish a generated value
  from an intentional player choice.

This is a source contribution, not a published or qualified package.
See [the rebuild and two-artifact checklist](docs/PORTABLE_GENERATED_DEFAULTS.md).

# Mega Man X4 Recompiled - 1.0.2 Stable Public Beta

- Fixed Player 2 falling to their death when both players dismount at the end of Marine Base's bike section. Both heroes now complete their original dismounts and land normally.
- Fixed Jet Stingray spawning too early in widened co-op views and disappearing before the fight. Supplemental placement waits for the actual boss arena.
- Fixed Split Mushroom's lift camera drifting when Player 2 jumps high, which made platform events appear late in their view.
- Rechecked the existing fixes for the opening-stage part 2 object dropping when Player 2 leads, and Mushroom's rainbow copies after Player 1 dies, with both character rosters.
- Diagnostic recording remains optional and **off by default**. Public packages omit CD Speed, Fast Loading and developer cheats.

**Windows x64 ZIP. The 1.0.2 Linux AppImage is pending. Both netplay peers must update.**
Native memory-card saves carry forward; emulator save states are specific to their build.

Local native checks cover the dismount, actual boss-door transition and active Jet
fight, bounded lift high-jump windows, falling object and rainbow-copy patterns.
All 38 Windows production checks pass. Full Mushroom ceiling-panel clearance,
fresh online gameplay and Linux gameplay are not newly qualified by this release.
[Evidence and limits](docs/COOP_MARINE_MUSHROOM_RELEASE_GATE.md).
[Known issues](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v1.0.1 Stable Public Beta

- Fixed the surviving Player 2 entering the yellow teleporter but failing to trigger Sigma's entrance scene after Player 1 dies.
- The original campaign dialogue and normal transfer into the next area are preserved. Both death orders and the both-living control pass in both campaigns with local Split/Both doors; the P2-survivor route also passes with Unified/Either doors in X's campaign.
- Diagnostic recording remains optional and **off by default**.

This release includes the Windows x64 ZIP. A Linux AppImage is pending.
Both netplay peers must update. Native memory-card saves carry forward;
emulator save states are specific to their build.

The bounded qualification covers the yellow pad, entrance scene, forward
transfer and restored controls. It does not clear every older Sigma
death/Continue report or establish a full Sigma victory.
[Evidence and limits](docs/COOP_SIGMA_SURVIVOR_ENTRANCE.md).

## Previous v1.0.0 Stable Public Beta

- Optional Japanese disc media: choose Japanese FMVs, voices and the Rockman X4 title screen independently using your own Japanese disc.
- Spanish and Brazilian Portuguese dialogue catalogs, rendered with the original game's font and native glyph styling. Both translations remain drafts awaiting native-speaker review; translated audio and artwork are outside their current scope.
- Corrected the clipped Japanese title, loading-logo colors and unnecessary disc loading during Game Start with language mods enabled.
- Diagnostic recording remains available and is **off by default**. Enable **Playtest Diagnostics → Input Display and Recording** for local play, or **X + Zero Co-op → Netplay diagnostic recording** as host for both netplay peers. Existing explicit preferences are preserved.

Unified remains the default co-op camera; independent Split and local 8:3 remain
experimental. Both netplay peers must use the same release. Campaign memory-card
saves carry forward; older emulator save states may be incompatible.
Public packages disable development cheats and omit CD Speed and Fast Loading mods.

This is a public beta with bounded validation. Full campaign coverage, native-speaker
translation review and unresolved reports on the [known-issues checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md)
remain open. [Language setup, evidence and limits](docs/LOCALIZATION.md).

## Previous v0.1.1-coop-playtest.43

**Two heroes, two pause menus.**

- Either player opens both character menus. X and Zero can independently select weapons and manage their supplies; either player resumes the team.
- Independent pause menus are on by default in local Unified, local 8:3 and netplay. They require separate E-tank energy for each player.
- Tank use blocks unpause while the other player can keep navigating and selecting equipment. Gameplay resumes only after healing finishes and a player presses Start again.
- Menus fade smoothly back into gameplay, with the stage restored while the screen is black.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified remains the default; local 8:3 split remains experimental. Campaign
saves carry forward; emulator save states from older builds may be incompatible.
Public packages disable development cheats and omit loading-speed mods.

The nearby intro explosion shake and missing bee groups remain under
investigation. Single-slash timing report157 remains **unconfirmed**. Other
unchecked reports stay on the numbered checklist.

Keep **Input Display and Recording** enabled. After a bug, quit normally and
send the matching `saves/diagnostics/session-...` folder beside the executable
or AppImage, plus a video timestamp if available. For netplay, include both
computers' folders.

[Known issues and numbered checklist](https://github.com/mstan/MegaManX4Recomp/blob/master/docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.42

**Reliable P2 pause menus, Dragoon cameras and moving train scenery.**

- Fixed P2 opening P1's pause menu and losing controls after quitting and starting another game.
- Fixed Magma Dragoon's refight locking one or both split cameras, whichever player starts the fight.
- Fixed Slash Beast's scenery freezing in P2's view after the background loops. Both stage sections keep scrolling.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified remains the default; local 8:3 split remains experimental. Campaign
saves carry forward; emulator save states from older builds may be incompatible.
Public packages disable development cheats and omit loading-speed mods.

The nearby intro explosion shake and missing bee groups remain under
investigation. Single-slash timing report157 remains **unconfirmed**. Other
unchecked reports stay on the numbered checklist.

Keep **Input Display and Recording** enabled. After a bug, quit normally and
send the matching `saves/diagnostics/session-...` folder beside the executable
or AppImage, plus a video timestamp if available. For netplay, include both
computers' folders.

[Known issues and numbered checklist](https://github.com/mstan/MegaManX4Recomp/blob/master/docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.41

**Survivor rainbow effects, leading-player stage objects and moving train scenery.**

- Fixed Split Mushroom's black rainbow trails when P1 dies before P2 enters the boss room.
- Fixed the lower piece of the intro's first falling object ignoring a leading P2.
- Fixed a camera handoff that could erase Slash Beast's scrolling scenery.
- With boss scaling enabled, further Ground Hunter hits damage Slash Beast without endlessly restarting his current weakness stun. A new stun can still begin after he recovers.
- Diagnostic sessions now record the release, effective local/netplay camera mode and co-op options. Mode changes are recorded automatically.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified remains the default; local8:3 split remains experimental. Campaign saves
carry forward; emulator save states from older builds may be incompatible.
Public packages disable development cheats and omit loading-speed mods.

Dragoon's reported refight camera lock remains unconfirmed. Remaining elevator, stage-effect, background and performance
reports stay on the numbered checklist. Single-slash timing157 remains
**unconfirmed**; this release does not claim to fix it or every split-screen issue.

Keep **Input Display and Recording** enabled. After a bug, quit normally and
send the entire matching `saves/diagnostics/session-...` folder beside the
executable or AppImage, plus a video timestamp if available. For netplay,
include both computers' folders.

[Known issues and numbered checklist](https://github.com/mstan/MegaManX4Recomp/blob/master/docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.40

**More reliable P2 progression, with optional separate E-tanks.**

- Added **Separate E-tanks** to Co-op settings, off by default. Each player gets their own stored health energy when enabled; tank acquisition and campaign saves remain shared.
- Reduced Zero's scaled boss damage by20% compared with the previous co-op scaling balance. X's damage and unscaled play are unchanged.
- Split Mushroom's climbing latches wait for a living partner below them in the same shaft.
- P2's Split Mushroom midboss entrance starts from the correct camera.
- Either progressing player triggers Slash Beast's bomb scenes, including a surviving P2.
- P2 falls normally from the opening stage's dropping slab, instead of staying attached to it.
- The opening stage's closing walls activate when P2 descends ahead of a living P1.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults.
Separate E-tanks, boss scaling, Fight Double and Iris, and local8:3 split remain
optional. Memory-card campaign saves carry forward; older emulator save states
may be incompatible with this build. Public packages disable development cheats.

Local8:3 split remains experimental. Remaining elevator, stage-effect, background
and enemy-spawn reports are still under investigation. The latest single-slash
timing report157 and severe local split lag remain **unconfirmed**, not declared
fixed in this release. The unfinished Dragoon and elevator candidates are excluded.

Keep **Input Display and Recording** enabled. After a bug, quit normally and send
the entire matching `saves/diagnostics/session-...` folder beside the executable
or AppImage, the release version and a video timestamp. For netplay, include both
computers' folders.

[Known issues and numbered checklist](https://github.com/mstan/MegaManX4Recomp/blob/master/docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.39

**Steadier split cameras and a persistent local split-screen layout.**

- Fixed the intro rescue getting stuck with endless explosions when Zero/P2 enters the gate first. Both heroes can continue into part 2.
- Local 8:3 keeps two panes through menus, movies, shared scenes and unavailable camera-pair frames.
- Local split automatically loads its required authored-map renderer, preventing distant terrain corruption when Custom Renderer was disabled.
- Reduced redundant background drawing in experimental offline side-by-side co-op.
- Fixed the later Frost Walrus rising-platform camera and landing limit for P2, including when P1 is dead.
- Scaled Zero attacks can connect again as their native attack pose changes after recovery.

Local **8:3 split** remains experimental. The optimization reduces rendering work,
but the reported severe lag is **not confirmed fixed**. Unified remains the default.
Residual Jungle background banding remains under investigation; 32:9 is still deferred.
The latest single-slash timing report157 remains **unconfirmed**; the new saber
correction addresses a separately traced extra rejection rule.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Stacked lifebars and Either-player doors remain the defaults. Boss scaling,
Fight Double and Iris, and local split remain optional. Campaign memory-card
saves carry forward. Development cheats are disabled in public packages.

Keep **Input Display and Recording** enabled. After a bug, quit normally and
send the entire matching `saves/diagnostics/session-...` folder beside the
executable or AppImage, the release version and a video timestamp. For netplay,
include both computers' folders.

[Known issues and numbered checklist](https://github.com/mstan/MegaManX4Recomp/blob/master/docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.38

**Two local cameras, one co-op game.**

- New **Local co-op display** dropdown: **Unified** (default) or **Side by side 8:3 (Experimental)**. Each local player gets a 4:3 camera and their own HUD. Works offline with two controllers; online camera selection remains separate.
- Menus and shared story scenes use one view, then return to the two local cameras.
- Fixed X/P2's missing Aiming Laser beam in Zero's campaign.
- Fixed the frozen Space Port and Final Weapon backgrounds in independent views.

Local split is experimental and renders both views, so it can cost more performance.
Jungle background artifacts remain under investigation. A genuine 32:9 mode with
two 16:9 views is tracked for later; it is not included in this release.

Original-disc offline movement, independent HUDs, pause/resume, resizing and
protected render-state checks pass. The weapon and background fixes also pass
actual two-peer Split netplay tests. [Evidence](docs/COOP_LOCAL_SPLIT38.md).

GitHub #41's intro freeze remains unconfirmed. Current native tests pass through
the gate, explosions, Dragoon's dialogue/rescue and healthy arrivals in part 2;
the reporter has been asked for a fresh full diagnostic session. Other unchecked
historical reports remain on the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults.
Boss scaling, Fight Double and Iris, and local split remain optional.
Campaign memory-card saves carry forward. Public packages disable development cheats.

Keep **Input Display and Recording** enabled. After a bug, quit normally and
send the matching entire `saves/diagnostics/session-...` folder beside the
executable or AppImage, the release version and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.37

**More reliable P2 progression and independent boss hits.**

- Cyber Peacock's course assessment now finishes when P2 enters the portal.
- Either living hero can trigger Storm Owl's section exit, including an
  interrupted partner-death sequence. Both Split views leave the midboss room.
- Intro-stage falling platforms respond to the leading P2.
- Cyber's course analysis graphic now appears on both heroes.
- A ready partner can damage a boss during the last shared recovery tick.
  Scaling retains doubled effective durability and separate, shorter cooldowns.

The owner compared Zero's Frost Walrus combos with scaling OFF and ON and
accepted the shorter ON recovery. This is a bounded human playtest, not proof
that every historical missed-hit sequence has been reproduced. Native local
and two-peer Split checks cover the progression corrections and adjacent
X/Zero hits. [Evidence and limits](docs/COOP_PLAYTEST37_INVESTIGATION.md).

Still unconfirmed: the later Intro boss-approach recurrence, Owl's
upgrade/death stranding, Web's missing-enemy recurrences, and Frost's reported
survivor camera, post-death graphics and ice-bird slowdown. Those reports stay
on the checklist; this release does not claim they are fixed. Local 8:3 split
screen remains deferred.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.
Production packages disable debug tools and development cheats.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).
Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.36

**Boss-scaling slash recovery corrected.** Zero's fresh ground chains,
finishers and aerial slashes now use short recovery with Separate cooldowns.
Each native blade still damages a boss once; a held spin cannot repeatedly hit.
Both players retain independent cooldowns and doubled effective boss health.

- Leading P2 now starts the native collapse of Frost Walrus's later ice path.
- Split cameras retain their own view and HUD during ice-bird field freezes.
- Added a synchronized native field-effect fixture to development netplay.
  It is excluded from public builds.

The Frost enemy-sprite corruption and reported first-entry second-door warp
remain unconfirmed. Completed ice-bird events retain the original shared-stage
retirement behavior; they are not duplicated for the trailing player.

Windows title tests, both-seat native Zero combat, held-spin and mixed-attack
controls, and real Split LAN collapse/freeze checks pass. Input excerpts are
transplanted into fresh native fights; this is not exact playback of a netplay
recording. [Evidence and limits](docs/COOP_FEEDBACK35_FROST.md).

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).
Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.35

**Frost ice paths, isolated split-screen flashes, and a saber-contact correction.**

- A leading Player 2 can activate Frost Walrus's later outdoor ice path while
  Player 1 remains behind.
- Distant split-camera views keep their normal colors during intro-stage red
  flashes. The nearby view retains the effect.
- Fixed stale saber-contact history during per-player boss cooldowns, allowing
  a fresh swing to reuse its native contact key after recovery. A held swing
  still counts as one hit. Cooldown lengths are unchanged.

**Issue 157 remains unconfirmed.** The contact-cleanup correction and independent
X/Zero hits have been validated, but the tester's exact missed follow-up slash
sequence has not been reproduced or confirmed resolved. This release does not
claim that every reported missed hit is fixed.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.34

**Safer boss fights and clearer co-op hit feedback.**

- Fixed a reset to character selection when charged X fire overlaps Zero's fire
  attack. The delayed flame effect now follows the correct player's shot.
- Storm Owl releases both heroes after catching them together, instead of
  leaving one permanently grabbed.
- Boss-refight returns check the partner's landing before the warp-in, avoiding
  the wall crush beside Slash Beast's return pad.
- With separate player cooldowns, bosses keep flashing during the remaining
  recovery window. Damage and cooldown lengths are unchanged from Playtest33.

The broader report of missed Zero follow-up hits remains open. Fresh native
fights reproduced complete damaging combos, aerial hits and simultaneous
attacks; the flashing correction is not presented as a fix for every miss.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.33

**Dr. Light's capsule conversations and faster co-op boss combat.**

- X's armor capsules show the correct Dr. Light dialogue and hologram in Zero's
  campaign. X receives his armor while P1 keeps ownership of campaign progress.
- Boss scaling now halves boss-hit cooldowns with either cooldown method.
  Separate player timers remain independent: a native 64-tick cooldown becomes
  32 ticks, so Zero's aerial slash and combo finisher no longer impose the longer
  wait from the previous separate-player setting.
- Zero's opening ground slashes keep their short combo timing. The Mods tooltip
  explains the same half-length cooldown policy for both choices.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.32

**Safer Mushroom traversal, Ride Armor arrivals, and X's hover effects.**

- Mushroom's hatch before the first miniboss waits for a nearby living partner,
  including the brief animation frame where their collision box is unavailable.
- The Mushroom part2 lift starts for a surviving P2 when P1 is dead.
- Cyber Peacock's course portals finish an interrupted partner warp before
  advancing, preventing a P2-led transition from locking the checkpoint loader.
- A partner can finish warping back beside a grounded, occupied Ride Armor.
  Magma Dragoon's entrance no longer requires destroying the armor to restore them.
- X's upgraded boots show their native blue hover jets in Zero's campaign.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

The [release dispositions](docs/COOP_PLAYTEST32_DISPOSITIONS.md) distinguish
these five validated fixes from reports that did not reproduce in bounded
checks. Those reports remain tracked; they are not advertised as fixed.

Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders.

## Previous v0.1.1-coop-playtest.31

**Controller rebinding, Windows borderless mode, and more flexible offline saves.**

- Fixed controller rebinding when SDL assigns the selected controller ID zero,
  including Map All release handling and two identical controllers.
- Windows borderless mode uses an ordinary desktop-sized window with SDL2 as
  well as SDL3, and restores the previous window geometry when leaving it.
- Mod selections save when edited, including when closing the launcher without
  pressing Play.
- Save states allow HUD layout and native character-code option changes.
  Existing constant cheat-provider records migrate without weakening checks
  for mutable co-op state, schemas or enhancement memory layout.
- Documented the retirement of generic CD Speed and Fast Loading mods. Verified
  resident disc loading is built in; native music and movie timing is retained.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Campaign memory-card saves carry forward. Changes to snapshot memory layout or
ongoing co-op/boss protocols still require compatible settings. Online startup
and intermittent music/intro-boss reports remain under investigation.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.30

**Fortress co-op fixes, chainable Zero combos, and lighter diagnostic recording.**

- Either hero can activate the fortress boss-refight teleporters. The existing
  Both-player door option now also lets you require both living heroes on a pad.
- Corrected boss weakness reactions to use the hero who actually landed the hit.
  Native checks cover Storm Owl, Jet Stingray, Frost Walrus and Sigma's second form.
- Zero can chain his opening ground slashes with separate per-player boss
  cooldowns. The finisher keeps its full cooldown; the earlier shared method remains available.
- Nearby living partners stay present through stationary fortress boss scenes,
  including Sigma's phase changes. Actual room changes still transport the party.
- Either player can pause while moving freely through an open Both-player door.
- Diagnostic checksums and input-file writes run on a bounded background worker.
  Recording locations and replay format are unchanged; failures remain visible.

**Windows ZIP and Linux x86_64 AppImage. Both netplay peers must update.**
Unified cameras, Stacked lifebars and Either-player doors remain the defaults;
Split cameras are experimental. Boss scaling and Fight Double and Iris remain
optional and default off. Campaign memory-card saves carry forward.

The numbered checklist distinguishes validated fixes, current native checks,
and reports still awaiting a matching reproduction. Unconfirmed death-sprite,
conditional weakness, targeting and older stage reports remain tracked; this
release does not claim every historical report is fixed.

Keep **Input Display and Recording** enabled and look for **REC**. After a bug,
quit normally and send the complete `saves/diagnostics` folder beside the
executable or AppImage, the release version, and a video timestamp. For netplay,
include both computers' folders. An abrupt process kill can lose the unflushed tail.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.29

**Storm Owl's boss arena now stays in place when the players arrive apart.**

Fixed a trailing player reopening the arena's camera boundary just before
the boss introduction. The fight now uses the original arena limits, avoiding
the large wall scenery that could scroll into view. Validated in local Unified
co-op and Split-camera netplay with either player leading.

Includes all Playtest 28 fixes. **Windows ZIP and Linux x86_64 AppImage.**
**Both netplay peers must update.** Unified cameras, Stacked lifebars and
Either-player boss doors remain the defaults; Split is experimental.
Boss scaling and Fight Double and Iris remain optional and default off.

Campaign memory-card saves carry forward. Reports107,109,112,119 and121 remain
open after bounded checks: missing Spider enemies, permanent Frost ice, Sigma
enemy facing, and the reported Slash Beast survivor background freeze.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers and the stage area and camera mode.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.28

**More reliable paired doors, independent cameras, and X versus Iris.**

- **Both-player boss doors:** Keep each player's own camera and HUD while
  crossing together. Fixed Frost Walrus's doorway trap and unwanted teleport.
- **Web Spider:** Fixed broken-door exit scenery in Split, distant P2 water
  ripples, and the waterfall animation freezing after P1 dies.
- **Independent health pickups:** Full-health capsules now replenish their
  collector without switching to the other player's camera or freezing them.
  Unified cameras retain the original shared replenishment pause.
- **Frost Walrus:** Opening ice platforms activate for a leading P2, and
  both independent views keep their snowfall animated.
- **Iris:** X can trigger her armor reaction, expose the crystal, and damage
  it. Her native armor sequence and the selected boss-scaling rules remain.

Includes all Playtest 27 fixes. **Windows ZIP and Linux x86_64 AppImage.**
**Both netplay peers must update.** Unified cameras, Stacked lifebars and
Either-player boss doors remain the defaults; Split is experimental.
Boss scaling and Fight Double and Iris remain optional and default off.
Scaling's cooldown method defaults to Separate for each player, with Reduced
shared cooldowns available.

Campaign memory-card saves carry forward. Use those when updating between
playtests. Reports107,109,112,119 and121 remain open after bounded checks:
missing Spider enemies, permanent Frost ice, Sigma enemy facing, and the
reported Slash Beast survivor background freeze. They are not claimed fixed.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers. Include the stage area and camera mode.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.27

**Ice physics, elevator cameras, and teammates returning at the next area.**

- **Frost Walrus:** Player 2 now slides on ice with their own momentum, for
  both X and Zero. Their movement no longer depends on Player 1's floor controller.
- **Split Mushroom:** Fixed Player 2's Split camera staying behind as the
  second-section elevator rises. Each view follows its rider.
- **Area transitions:** A dead teammate returns when the survivor advances
  to the next stage section. The survivor's resources and shared lives carry
  forward. Same-area checkpoints and voluntary drop-out keep their existing rules.

Includes all Playtest 26 fixes. **Windows ZIP and Linux x86_64 AppImage.**
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. Boss scaling and Fight Double and Iris
remain optional and default off. Scaling's cooldown method defaults to Separate
for each player, with Reduced shared cooldowns available.

Campaign memory-card saves carry forward. Use those when updating between
playtests. The earlier unconfirmed intro-door crash (item53) and Storm Owl arena
wall/camera report (item89) remain open.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.26

**Safer teleport recovery and correct scenery when Player 2 continues alone.**

- **Teleport safety:** If a player is teleporting when their partner dies,
  the living player now recovers near their departure point with working
  controls. Landing positions are checked against terrain. An arrival that
  already committed to the next room stays in that room; shared lives and
  genuine deaths are preserved.
- **Web Spider:** Fixed incorrect scenery in the second section during local
  Unified co-op when Player 1 is dead and Player 2 continues alone.

Includes all Playtest 25 fixes. **Windows ZIP and Linux x86_64 AppImage.**
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. Boss scaling and Fight Double and Iris
remain optional and default off. Scaling's cooldown method defaults to Separate
for each player, with Reduced shared cooldowns available.

Campaign memory-card saves carry forward. Use those when updating between
playtests. The earlier unconfirmed intro-door crash (item53) and Storm Owl arena
wall/camera report (item89) remain open.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.25

**Boss doors restored in Split netplay, and both heroes return for Sigma stage 3.**

- **Boss doors:** Fixed missing inner boss doors in Split-camera netplay when
  using the Both-player door rule. Both players can cross the paired doors
  together without a partner teleport.
- **Sigma stage 3:** If Player 2 dies during Double and Player 1 wins, Player 2
  now returns at the next level with health and working controls. Shared lives
  are preserved. The fix also covers the reverse survivor case.

Includes all Playtest 24 fixes. **Windows ZIP and Linux x86_64 AppImage.**
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. Boss scaling and Fight Double and Iris
remain optional and default off. Scaling's cooldown method defaults to Separate
for each player, with Reduced shared cooldowns available.

Campaign memory-card saves carry forward. Use those when updating between
playtests. The earlier unconfirmed intro-door crash (item53) and Storm Owl arena
wall/camera report (item89) remain open.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.24

**Safer co-op transitions and separate boss hit cooldowns for X and Zero.**

- **Cyber Peacock:** Flipping the room no longer crushes the nearby partner
  through stale Split-camera limits. Both players' views turn with the room.
- **Storm Owl:** Both-player midboss doors wait for the crossing and camera
  movement to finish, keeping the partner in the room without another teleport.
- **Ride Armor:** The second rider draws at the correct priority. After a
  forced dismount, returning players check the terrain before appearing and
  wait if there is no safe nearby landing.
- **Boss scaling:** A new **Boss hit cooldowns** dropdown defaults to
  **Separate for each player**. X's hit cooldown no longer blocks Zero's
  damage, and vice versa. **Reduced shared cooldowns** remains available.
  Boss scaling itself remains optional and off by default.

Includes all Playtest 23 fixes. Windows ZIP and Linux x86_64 AppImage.
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. **Fight Double and Iris** stays optional
and default off. Campaign memory-card saves carry forward. Use those when
updating: this build changes co-op savestate data, so older playtest savestates
are incompatible.

The Storm Owl arena wall/camera report (item89), earlier unconfirmed intro-door
crash (item53), and Sigma stage3 P2 disappearance (item86) remain open.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.23

**Ride Armor for Player 2, smoother bike entrances, and restored backgrounds.**

- **P2 Ride Armor:** X or Zero can board the stage's armor, move and attack,
  then eject with Up + Jump. Doors and partner teleport sequences dismount
  the rider safely. Waiting at a Both-player door keeps the rider mounted
  until the door actually opens.
- **Jet Stingray:** P2 keeps pace during the native bike entrance, with the
  correct rider animation and one shared READY sequence.
- **Storm Owl:** P2's sky keeps scrolling in Split mode. Missing background
  columns are restored.
- **Slash Beast:** Mountain and sky regions no longer disappear into black
  space as the background scrolls.

Includes all Playtest 22 fixes. Windows ZIP and Linux x86_64 AppImage.
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
our defaults; Split is experimental. **Fight Double and Iris** stays optional
and default off. Campaign memory-card saves carry forward; use those instead
of older playtest savestates when updating.

The separate Storm Owl arena wall/camera report (item89), earlier unconfirmed
intro-door crash (item53), and Sigma stage3 P2 disappearance (item86) remain open.

Leave **Input Display and Recording** enabled and look for **REC**. If a bug
recurs, note its video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.22

**Intro-stage progression fixes and shared victory poses.**

- **Intro part 2:** A surviving P2 now starts the crusher panels when P1 is
  dead. The original moving panels and wall-jump route remain intact.
- **Intro boss door:** With Both-player doors selected, the act1 exit waits
  for both players to finish crossing before starting its cutscene. This fixes
  the reproduced stall with P1 walking against the wall and P2 hidden.
- **Celebrate together:** Both living, present heroes now perform their own
  victory poses after normal Maverick clears, then warp out. This is automatic,
  with one campaign victory theme and reward. Survivor-only clears and story
  movie sequences retain their existing behavior.

Includes all Playtest 21 fixes. Windows ZIP and Linux x86_64 AppImage.
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. **Fight Double and Iris** stays optional
and default off. Normal campaign memory-card saves carry forward. Older
playtest savestates may not load; use memory-card saves between versions.

The Storm Owl arena wall/camera report (item89) is reproduced and remains
under investigation; this build does not claim a fix. The earlier unconfirmed
intro-door crash (item53) and Sigma stage3 P2 disappearance (item86) remain open.

If a problem recurs, leave **Input Display and Recording** enabled, look for
**REC**, note the video timestamp, quit normally, and send the complete
`saves/diagnostics` folder beside the executable or AppImage. For netplay,
include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.21

**Smoother partner teleports and corrected Jet Stingray bike layering.**

- **Partner teleports:** P2's arrival now finishes while P1 jumps or uses Nova
  Strike. Walking and dashing also leave the arrival intact, so P1 can keep
  fighting while their partner returns.
- **Jet Stingray:** Each rider, bike and exhaust now stays together when the
  players overlap. This fixes the bike drawing over the other bike but behind
  its rider, for both X and Zero.

Verified in local co-op and split-camera netplay. Double's appearance in Zero's
story was also checked through the optional Iris-to-Double sequence; the
Playtest 20 sprite fix already covers it.

Includes all Playtest 20 fixes. Windows ZIP and Linux x86_64 AppImage.
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. **Fight Double and Iris** remains optional
and default off. Campaign memory-card saves carry forward.

The unconfirmed Sigma stage 3 P2 disappearance remains open. If it recurs,
keep **Input Display and Recording** enabled, look for **REC**, note the video
timestamp, quit normally, and send the complete `saves/diagnostics` folder
beside the executable or AppImage. For netplay, include folders from both peers.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.20

**Fixes for co-op boss doors, Zero versus Double, and the optional Iris fight.**

- **Colonel:** With Both-player doors enabled, the fight waits until the pair
  finishes crossing. This fixes the door interruption that could kill P1 and
  leave P2 trapped.
- **Double:** Zero's spinning jump attack now deals damage, including as P2.
- **Iris in X's story:** The optional encounter loads the correct sprites and
  can start after both players cross her boss door.

Includes all Playtest 19 fixes. Windows ZIP and Linux x86_64 AppImage.
**Both netplay peers must update.** Unified cameras and Stacked lifebars remain
the defaults; Split is experimental. **Fight Double and Iris** remains optional
and default off. Campaign memory-card saves carry forward.

**Still under investigation:** P2 disappearing at Sigma stage 3 entry. A new
hidden two-peer Split netplay attempt completed the transition with P1 dead and
P2 visible and controllable on both screens. This release does not claim a fix
for that report.

Diagnostics default on in fresh installs. Keep **Input Display and Recording**
enabled and look for **REC**. If the disappearance recurs, note the video
timestamp, quit normally, and send the complete `saves/diagnostics` folder
beside the executable or AppImage from **both peers**.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.19

**Player 2 rides into Jet Stingray's stage with the proper animation.**

X and Zero now use their native leaning bike entrance instead of standing
upright on the accelerating bike. Both riders still enter together, share
one READY animation, and regain normal controls afterward.

Verified with both character rosters in split-camera netplay and with Zero
as Player 2 in local co-op. Includes all Playtest 18 fixes.

Windows ZIP and Linux x86_64 AppImage. **Both netplay peers must update.**
Unified cameras and Stacked lifebars remain the defaults; Split remains
experimental. **Fight Double and Iris** stays optional and default off.
Campaign memory-card saves carry forward.

Diagnostics default on in fresh installs. Keep **Input Display and Recording**
enabled and look for **REC**. After reproducing a bug, quit normally and zip the
complete `saves/diagnostics` folder beside the executable or AppImage.

[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.18

**Smoother co-op progression, independent cameras, and boss fights.**

- **Walk through boss doors together.** With **Both players** selected, both
  living, present players can move and attack through the open door without
  the partner teleporting out and back in.
- **Opening stage:** destroying the breakable miniboss door correctly releases
  Player 2's independent camera.
- **Web Spider:** Player 2's scenery updates correctly in part 2 instead of
  disappearing or freezing as the players separate.
- **Sigma stage 1:** enemies spawn when Player 2 advances ahead of Player 1.
- **Colonel:** lightning no longer leaves the screen white after Player 1 dies.
- **Double:** Zero's attacks with native damage values now hurt him during
  his normal vulnerable phases.

Windows ZIP and Linux x86_64 AppImage. **Both netplay peers must update.**
Unified cameras and Stacked lifebars remain the defaults; Split cameras remain
experimental. Existing gameplay option defaults are unchanged.

**Save compatibility:** normal campaign memory-card saves carry forward.
Older co-op save states are incompatible; create new states with Playtest 18.

Diagnostics default on in fresh installs. Keep **Input Display and Recording**
enabled and look for **REC**. After reproducing a bug, quit normally and zip the
complete `saves/diagnostics` folder beside the executable or AppImage.

Jet Stingray's reported P2 entrance presentation remains open.
[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.17

**Both players now survive Web Spider's part 1-to-2 transition when P2 leads far ahead.**
The outgoing fade could incorrectly kill the hidden P1, leaving only P2 in
the next area. This is fixed; a player who was already dead stays dead.

**Local co-op settings are easier to find.** In Mods, enabling **X + Zero Co-op**
now opens its settings panel: boss-door policy, boss scaling, Double and Iris,
Ultimate Armor X Code, and Black Zero. These options work locally and online;
their existing defaults are unchanged.

Windows ZIP and Linux x86_64 AppImage. **Both netplay peers must update.**
Includes the earlier Playtest 16 fixes. Unified cameras and Stacked lifebars
remain the defaults; Split cameras are experimental. Jet Stingray's reported
P2 entrance presentation remains an open issue.

Diagnostics default on in fresh installs. Keep **Input Display and Recording**
enabled and look for **REC**. After reproducing a bug, quit normally and zip the
complete `saves/diagnostics` folder beside the executable or AppImage.

Keep normal memory-card saves for campaign progress. Use save states made with
this release. [Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.16

**Fixes Playtest 15's checkpoint restart when Player 1 teleports to catch up.**
When P1 goes off-camera, both players now stay in the current stage and P1
returns beside P2, preserving their health and lives. Genuine stage exits,
including Storm Owl with only P2 alive, still advance normally.

Windows ZIP and Linux AppImage. **Both netplay peers must update.**
Includes Playtest 15's armor palette and Web Spider capture fixes.
Unified cameras and Stacked lifebars remain the defaults; Split is experimental.

Diagnostics remain enabled by default for fresh installs. Keep **Playtest
Diagnostics → Input Display and Recording** enabled and look for **REC**.
After reproducing a bug, quit normally and zip the complete `saves/diagnostics`
folder beside the executable or AppImage.

Keep normal memory-card saves for campaign progress; use save states made with
this release. The reported failure was reproduced from a tester recording;
the corrected return and both Storm Owl exit paths were checked in native
gameplay. Windows and Linux production checks pass.
[Known issues and numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.15

Two-player X + Zero co-op for Windows and Linux. Both netplay peers must update.

- **Storm Owl progression:** P2 can now reach the second part of the stage while
  P1 is dead. The normal transition with both players alive also works.
- **X's armor colors:** collecting armor in Zero's campaign keeps X's correct
  palette, including after opening and closing the pause menu.
- **Web Spider capture:** a web that kills one player now releases instead of
  pulling in and damaging the surviving partner.

Unified cameras and Stacked lifebars remain the defaults; Split is experimental.
The co-op door options and optional doubled boss health remain available in Mods.

Diagnostics remain enabled by default for fresh installs. If you previously
disabled them, enable **Mods → Playtest Diagnostics → Input Display and Recording**.
Look for **REC** while playing. After reproducing a bug, quit normally and zip
the complete `saves/diagnostics` folder beside the executable or AppImage.

Keep normal memory-card saves for campaign progress; use save states made with
this release. Windows and Linux production checks pass. These fixes were
reproduced and checked in native gameplay; outstanding reports retain their
status in the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.14

Two-player X + Zero co-op for Windows and Linux. Both netplay peers must update.
Unified cameras and Stacked lifebars remain the defaults; Split is experimental.

- **Magma Dragoon progression fixed:** if P1 dies and P2 wins, P1 can now
  advance the weapon reward, save, and return to stage selection. Both character
  orders were tested; boss clears and saves stay with P1's campaign.
- **Diagnostics enabled by default:** the input display and recording start
  automatically on a fresh install. Loading a state with Diagnostics disabled
  no longer creates misleading, incomplete recording folders.
- Includes the new co-op showcase, screenshots and player guide. Project code
  is MIT; bundled dependencies and game imagery retain their separate terms.

If you carry over settings where recording was turned off, re-enable
**Mods → Playtest Diagnostics → Input Display and Recording** before testing.
Look for **REC** on the input display. After reproducing a bug, quit normally
and zip the complete `saves/diagnostics` folder beside the executable or AppImage.

Keep normal memory-card saves for campaign progress; use save states made with
this release. Earlier unresolved reports retain their status in the
[numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.13

Two-player X + Zero co-op for Windows and Linux. Both netplay peers must update.
Unified cameras and Stacked lifebars remain the defaults; independent Split
cameras remain experimental.

### Fixes

- Distant players, enemies and weapon effects no longer wrap onto the other
  player's independent screen. The intro dragon and its muzzle effect stay
  with the progressing player's view.
- Both independent views animate Split Mushroom's rotating staircases.
- Nova Strike keeps the partner's camera steady. Independent HUDs appear
  during ordinary simultaneous arrivals.
- The camera stays put during a team wipe, and latches account for a dead
  partner. Weapon sprites no longer show the reported horizontal seams.
- P2 joins Jet Stingray's bike entrance with its native animation, slightly
  behind P1, with one shared READY announcement.

### Optional co-op settings

- **Boss doors: Both players** — wait for both living, present players to
  touch a boss door. **Either player** remains the default.
- **Fight Double and Iris** — after the campaign's usual fight, return to
  the inner boss door for the other encounter, then continue P1's story.
  Off by default.
- **Co-op boss scaling** — double effective boss health and shorten shared
  hit cooldowns. This uses shorter shared cooldowns, not separate timers per
  player. Off by default.

The fixes have targeted original-disc gameplay checks, including paired LAN
camera tests. The new boss options have native fight/defeat checks and save,
load and rewind coverage. This release does not claim fixes for the parked
online startup reports or every previously reported staircase issue.

For a bug report, quit normally and zip the complete `saves/diagnostics` folder
beside the executable or AppImage. Keep Playtest Diagnostics enabled and include
a video when possible. Existing memory-card saves remain your campaign saves;
use save states made with this release.

See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.12

Windows ZIP and Linux x86_64 AppImage. Both peers must use this release.
Unified cameras and Stacked lifebars remain defaults; Split remains experimental.

- Fixes Zero getting stuck in an animation loop while gripping X's Lightning
  Web. Zero can slide and wall-jump from either side, with either player order.
- Fixes Zero staying suspended or sliding across the top of Lightning Web.
  Vertical contact now collapses the web through its original behavior, as
  it does for X. Zero's double-jump contact is also preserved.

Both reported problems were reproduced on Playtest 11. The fixed build passes
eight native gameplay cases covering both player orders, with the supplied
recording also verified. Earlier unresolved reports retain their documented
status; this release does not claim to fix every intro-door or staircase issue.

For bug reports, quit normally and zip the complete `saves/diagnostics` folder
beside the executable or AppImage. Keep Playtest Diagnostics enabled and include
video when possible. Startup logging fixes from Playtest 9 are included.
See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.11

Windows ZIP and Linux x86_64 AppImage. Both peers must use this release.
Unified cameras and Stacked lifebars remain defaults; Split remains experimental.

- Fixes the intro's post-dragon rescue scene locking when Zero/P1 is dead and
  X/P2 wins in Zero's campaign. The scene follows its surviving owner while
  preserving Player 1's campaign. Reproduced from a branch of a tester recording
  and verified through Mission Completed.
- Fixes X/P2's acquired armor artwork in Zero's campaign. Boots, helmet, body,
  both arm variants and full armor render through the private character
  textures. Permanent upgrades still belong to the single shared campaign.

The separate P2 intro-door crash report remains unconfirmed; this release does
not claim to fix every entry failure. Earlier unresolved staircase/input and
experimental Split reports retain their documented status.

For bug reports, quit normally and zip the complete `saves/diagnostics` folder
beside the executable or AppImage. Keep Playtest Diagnostics enabled and include
video when possible. Startup logging fixes from Playtest 9 are included.
See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.10

Windows ZIP and Linux x86_64 AppImage. Both peers must use this release.
Unified cameras and Stacked lifebars remain defaults; Split remains experimental.

- Fixes private special-weapon graphics sampling the wrong part of their texture.
- Fixes P2 and the co-op HUD disappearing after exhausting private texture slots.
  The host texture cache now safely reuses slots that neither player's queued
  drawing commands reference, including after saving and loading.
- Fixes P2 disappearing during an upgrade capsule's shared palette effects.
- Fixes Web Spider's waterfall exit stalling when P1 is dead and P2 reaches it.
  The surviving player can complete the exit and arrive in the next section.

The reported Frozen Zero problem on Split Mushroom's second rotating staircase
remains unresolved. Its supplied recording does not reproduce a character-state
lock; movement inputs are absent during the reported stall. This release does
not claim to fix the underlying input-delivery problem or every disappearance.

For bug reports, quit normally and zip the complete `saves/diagnostics` folder
beside the executable or AppImage. Keep Playtest Diagnostics enabled and include
video when possible. Startup logging fixes from Playtest 9 are included.
See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.9

Windows ZIP and Linux x86_64 AppImage. Both peers must use this release.
Unified cameras and Stacked lifebars remain defaults; Split remains experimental.

- Fixes co-op shutting down and P2's character/HUD disappearing after cycling
  weapons and firing X's charged buster. Reused projectile slots can retain a
  Gravity Well sprite pointer; the private renderer now follows the native
  static-artwork flags instead of decoding that stale pointer.
- Fixes empty startup logs on Windows launches without a console, including
  normal Explorer launches. Capture now attaches the actual stderr stream,
  verifies its first write, and reports failures to create the log or pointer.
- Adds explicit co-op failure reasons and partner removal/return context to
  the automatic startup log, even with gameplay input recording disabled.

For a report, quit normally and zip `saves/diagnostics` beside the executable
or AppImage. `startup-log-location.txt` points to the newest startup log;
`diagnostics-location.txt` points to the gameplay session. Keep Playtest
Diagnostics enabled and include the complete session folder, including its
inputs, settings and starting cards, plus any video of the problem.

The missing-shot report on Mushroom's staircase remains unconfirmed. Bounded
checks show ordinary and charged shots travelling on both staircase sections
with X/P1 and on the second section with X/P2. This release does not claim a
separate fix for that report or every earlier disappearance report.
Discord/WAN investigations and the other unresolved staircase/elevator reports
remain parked. See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.8

Windows ZIP and Linux x86_64 AppImage. Both peers must use this release.
Unified cameras and Stacked lifebars remain defaults; Split remains experimental.

- Fixes P2-led Split Mushroom room progression and portal-platform collision
  after a retry. Incoming players can land on the portal platform, and the
  return waits for a safe partner landing.
- Keeps each independent player view during catch-up instead of switching
  P2 to P1's camera and the unified HUD under load.
- Applies X5's aligned state-copy optimization to player and camera contexts.
  A camera copy now uses63 rather than252 guest-memory accesses. This reduces
  copy overhead; broader Split performance remains a playtest item.
- Records startup and public account failure messages automatically under
  `saves/diagnostics` beside the executable or AppImage, before gameplay starts.
  `startup-log-location.txt` identifies the latest file. Send both peers'
  newest `startup-*.log` files if sign-in or game startup fails.

The second/purple staircase was checked after the native preceding-section
handoff using development skips; the reported floating and brief arrival
glitch remain unreproduced. The elevator ascent and exit pass with its overhead
panels removed through their native destruction states. Destroy those panels
when playing: ceiling crush is authentic.

Discord authorization and real WAN startup have not been requalified in this
release. Intermittent co-op removal remains on the backlog. The older
intro-act2 WARNING report still needs a matching current diagnostic recording.
See the [numbered checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md).

## Previous v0.1.1-coop-playtest.7

Windows ZIP and Linux x86_64 AppImage. Unified cameras and Stacked lifebars
remain defaults; online Split cameras remain experimental.

- Fixes the Online/Discord START-to-game ICE handoff that left the host on a
  white screen and the guest waiting. Complete mod settings now survive
  lobby messages; invalid or truncated plans are rejected before starting.
- Fixes Split Mushroom's premature elevator death plane and delays its latch
  while both visible players are passing through. The original overhead-panel
  crush remains: destroy those panels while ascending.
- Preserves the partner while new room bounds initialize. Both players retain
  their native simultaneous arrival animations and safe placement.
- Offline co-op supports mapped F7 save/load and F8 rewind with both players'
  contexts and resources. State jumps remain unavailable during netplay.
- Playtest Diagnostics records beside the executable or AppImage under
  `saves/diagnostics`. `diagnostics-location.txt` identifies the latest session
  or error. Offline recordings that begin after a load or rewind include the
  exact state baseline needed for replay. Send the complete session folder.
- Adds separate default-off Ultimate Armor X code and Black Zero mods, usable
  together. X still receives the armor from a capsule. Co-op hosts choose the
  corresponding shared options for both seats; P1 owns campaign progress.
- Resets private texture registrations and enhancement allocations between
  sessions so lobby rematches reuse their resources.

Native hidden Windows checks qualify both rosters, solo character codes,
offline warm/cold states and rewind, diagnostic baseline replay and two-peer
LAN controls with matching state digests. Original character-code instruction
contracts and complete lobby-plan tests also pass on Linux. Full Discord
authorization and real WAN/NAT play remain additional human qualification.
The exact rotating-staircase floating and brief teleporter reports remain
unreproduced; intermittent co-op disappearance remains on the backlog.

## Previous v0.1.1-coop-playtest.6

Windows ZIP and Linux x86_64 AppImage with the first online sign-in fix and
the dependency updates from PR #23. Unified cameras and Stacked lifebars
remain defaults; online Split cameras remain experimental.

- The account client initializes its socket transport before its first HTTP
  request. This fixes the Windows first-sign-in failure that reported an
  unreachable lobby server before sending any request.
- Integrates the updated framework renderer and guest-clocked audio while
  retaining X4's co-op graphics banks, paired arrivals, independent audio,
  two-player launcher/controller UI and release toolchain support.
- Includes the updated shared lobby client's direct room latency probes.
  Both peers must use this release and the same original disc.

The sign-in transport regression is checked in a fresh process without a
browser or credentials. Full Discord authorization and real WAN gameplay
remain pending playtester confirmation. Split Mushroom's elevator, intermittent
retry/menu netplay disconnects and broader Split performance remain open.
Previous gameplay qualification is recorded below; this dependency release
does not claim a new complete campaign playthrough.

## Previous v0.1.1-coop-playtest.5

Windows and Linux packages from the approved co-op gameplay source. This
updates Windows with the fixes previously released in the Linux-only playtest4;
it introduces no further gameplay changes. The owner confirmed the paired
opening-stage spawn looks good. Unified and Stacked remain defaults; online
Split cameras remain experimental.

- Both players warp in together, slightly side-by-side, at suitable stage,
  section, retry and course-portal arrivals. Native terrain checks preserve
  safe landings; authored doorway walking and special story spawns retain
  their native lifecycle.
- Includes the qualified boss-entry, survivor handoff, dash/effect ownership,
  pause/dialogue, campaign-save, bike rendering, Lightning Web wall-jump and
  Split train-background fixes described under playtest4 below.
- Both platforms provide the two-player netplay lobby, separate local
  controller assignments, optional Side by side HUD and default-on input
  diagnostics. Private development controls are disabled in public packages.

The original 28 regression entries are qualified. The later Split Mushroom
elevator report is tracked as open item29, without an elevator-specific fix or
qualification. Real WAN play, intermittent retry/menu netplay disconnects and
broader Split performance remain open testing/investigation areas. Use the
same release and original disc on both peers, with OpenGL and a fresh boot.

## Previous v0.1.1-coop-playtest.4

Linux AppImage co-op regression release, with the gameplay fixes validated
on Windows. Unified remains the default, and Split cameras remain experimental.

- Netplay hosting starts the game with the selected BIOS, fixing the white
  screen after Play. Shared launcher and network dependencies are updated.
- Both players use paired native arrivals for stage entry, forward sections
  and team retry. Cyber Peacock course portals return living players to safe
  native landings; boss approaches preserve the surviving player's controls.
- Dash exhaust, trails and delayed attack effects use their allocating
  character and position. Ride Chaser riders, bikes and exhaust stay together;
  Zero's fire/ice fragments and X's special effects retain their own graphics.
- Shared dialogue setup preserves the native palette. Pause fades retain the
  partner body, effects and HUD, with character textures preserved across scenes.
- P1 owns the campaign and permanent upgrades. Continue, native cards and
  cleared portraits retain that progress through Quit and Escape Device returns.
- A surviving P2 can continue native stage transitions after P1 dies.
- Zero can use the partner's Lightning Web as a native wall-jump anchor.
- Both Split views keep the train scenery moving during Slash Beast's fight.

The stable [October 10 checklist](docs/COOP_PLAYTEST_FEEDBACK_20261010.md)
tracks the original 28 reports and their individual evidence; those entries are checked.
All 18 title checks pass. Native two-peer Split tests cover either player
leading into Slash Beast's fight, with matching state checks through tick8672.
The Linux production build also passes all 18 checks, AppImage layout/seeding,
and hidden launcher TCP smoke tests of the real Netplay button and two-player
controls. Private development controls are disabled. These tests cover the
reported cases rather than complete campaigns or production FPS.

The Windows/Linux launcher retains its two-player lobby and two independent
local controller assignments. Stacked HUD remains the default, with Side by
side available. Playtest Diagnostics remains enabled by default and records
accepted inputs plus starting cards/mod choices under `saves/diagnostics`.
Use matching builds and OpenGL on both peers, with a fresh boot after mod changes.

## Previous v0.1.1-coop-playtest.3

Two-player co-op regression update for Windows and Linux. Unified remains
the default; online Split cameras remain experimental.

- P2 owns its pause menu and can resume it. Native world drawing, charge
  glow/projectiles, saber hits, dash trails and water effects survive pause.
- Both players' jump/hurt sounds can overlap through independent voices.
- P2 can activate enemies and Web Spider's secret-hole camera.
- Intro, Jet Stingray and Cyber Peacock handoffs retain living players at
  native landing heights. Cyber orbs prevent withdrawal while captured.
  Jet uses one READY actor, and a dead P2 stays dead across its sections.
- Victory has one campaign reward and departure. P1 owns persistent progress
  and upgrades; boss clears agree in Continue, native cards and grey portraits.
  Fresh stage selection resets carry state so re-entry cannot inherit a corpse.
- Playtest Diagnostics defaults on, displaying both accepted input streams
  and recording inputs plus starting memory cards/mod choices under
  `saves/diagnostics`. Offline replay requires the matching build and settings.

The numbered [regression checklist](docs/COOP_PLAYTEST_REGRESSIONS.md) has all
26 outcomes checked. Validation includes all 16 title CTests, hidden original
disc scenarios, a two-boss native save/cold-load loop, and matching two-peer
development netplay state through tick6253. These checks cover the reported
cases; they do not establish complete campaign coverage. Split performance
and an intermittent netplay watchdog stall remain open investigations.

The Windows/Linux launcher lobby, two local controller assignments, stacked
default HUD and Side by side option remain available. Use the same release
on both peers. Co-op requires OpenGL and a fresh boot after changing mods.

## Previous v0.1.1-coop-playtest.2

Windows and Linux two-player co-op playtest. Unified cameras and vertically
stacked lifebars are the defaults; Split cameras remain experimental.

- The launcher exposes Netplay and its two-player lobby on both platforms.
- Local play offers separate Player 1/Player 2 controller assignments,
  including two controllers of the same model.
- Player 2 uses their own character's attack, hurt and death audio. Private
  audio banks prevent the campaign character's samples from replacing them.
- Player 2's death particles start at their own position, with red particles
  for Zero and blue particles for X, while Player 1 stays alive.
- Both lifebars share the left anchor in widescreen. Mods offers **Stacked**
  (default) and **Side by side** HUD layouts.
- Player 2 no longer appears on the Quit Game screen.

The owner approved the exact Windows build after checking P2 damage following
pause/resume, death-particle ownership/color, both character voice orders,
HUD alignment/layout and Quit Game drawing. The candidate passed all 14 title
checks; Linux also passed them and the native private-audio-bank regression.
The Windows download packages that tested executable without rebuilding it.

Enable **X + Zero Co-op** in Mods for local play and assign both players under
Controls. For online play, select **Netplay** and the **X + Zero Co-op** profile.
The host chooses cameras/HUD layout for both peers. Use the same release on
both computers. Offline play uses Unified. Co-op requires OpenGL and a fresh
game launch after changing mods. Controllers use Start/Options to pause;
keyboard pause follows each player's Controls binding.

The Linux x86_64 AppImage requires glibc 2.38 or newer. Make it executable
with `chmod +x`, then run it; `--appimage-extract-and-run` works without FUSE.
Writable data lives in `~/.local/share/MegaManX4Recomp`.

Split can still run below full speed, including on LAN. Split refill/section
handoffs, vehicles, chained scenes and retry/menu disconnects need additional
playtesting. This is a playtest release with limited stage coverage.

## Previous v0.1.1-coop-playtest.1

An early Windows and Linux co-op playtest. Unified is the default camera mode;
Split (Experimental) is available for online co-op. The current main release
remains available separately.

The Linux x86_64 AppImage requires glibc 2.38 or newer. Make it executable
with `chmod +x`, then run it; use `--appimage-extract-and-run` on systems
without FUSE. It keeps writable data in `~/.local/share/MegaManX4Recomp`.
Linux and Windows use the same declared co-op gameplay and execution contracts.

- Simultaneous native X and Zero; vertically stacked lifebars in the shared view.
- Independent online views of one shared world, with local player HUDs.
- Distant Split backgrounds render from the authored map, avoiding shared
  tile-ring corruption when players separate.
- Split health refills stay with their collector, without the shared world
  pause or partner-camera snap. Unified retains the native refill pause.
- Living outgoing teleport passengers survive native section transfers.
  Actual deaths stay dead until a team retry, and scene handoffs retain their
  native teleports.

Enable X + Zero Co-op in Mods for local play. Online rooms use the X + Zero
Co-op profile; the host chooses the camera mode for both peers. Use the same
build and OpenGL on both computers. Offline play always uses Unified. Co-op
is opt-in outside its netplay profile and takes effect on a fresh boot.

Validation includes focused ownership, lifecycle, camera-boundary and authored
background regression checks; original-disc loopback netplay separation; and
owner playtesting in Web Spider. The owner confirmed the boss door worked and
distant background corruption was substantially improved. This does not
establish full stage/campaign/door coverage. The prior run's logs lacked health
history, so the reported area-2 death cannot be conclusively attributed to the
transition regression found in the source.

The exact production binary passes all 13 configured CTests, native room-offer
checks for both camera choices, shared catalog/execution staging, and system
DLL dependency checks. Two input-neutral production peer pairs agree on their
logged core/mod state during boot. The timed attract-gameplay probe did not
reach gameplay; it remains failed and provides no stage coverage. The latest
asynchronous refill and section-transfer changes still need native playtesting.

Known limits: Split may run below full speed even on LAN. Vehicle stages,
chained scenes, surviving-player handoffs, and retry/menu paths need more
coverage. An intermittent post-retry menu disconnect is still under
investigation. This release is intended for playtesting, with isolated co-op
saves; it is not a finished co-op port.

## Previous v0.1.0-alpha

- Adaptive authored-map renderer replaces the retired fixed 16:9 tile-ring path.
- OpenGL and 1080p internal resolution are enabled by default, with pixel-art sampling.
- Title-specific resident ARC loading defaults on; generic CD/host timing options are retired.
- Wider enemy activation, persistent kills at wide edges, distant destructible door hitboxes, and adaptive intro searchlights.
- Gameplay uses the approved 2x guest CPU budget while device clocks retain their timing.
- Bundled OpenBIOS, normal damage by default, and interpolation unavailable.
- Windows x64 ZIP and native Linux x86_64 AppImage use the shared release staging and execution-contract checks.

The owner approved the Windows gameplay candidate. Package checks cover startup,
version, dependencies and defaults; they do not establish complete stage coverage
or a measured correction to the historical sound-delay concern.

Release packaging uses `tools/package_release.sh` for the Windows ZIP and
`tools/package_native_appimage.sh` for the Linux AppImage after a production
build. Both delegate catalog, toolchain and executable-contract gates to the
pinned shared framework; the older standalone packagers are historical.

## Historical v0.0.2-alpha notes

Historical coverage correction (2026-09-11): the ARC-overlay claims below were
assumptions. Original-disc inspection found no separate game overlay; see
[the inventory and its limits](docs/DISC_INVENTORY.md). No new release or version
bump accompanies this correction.

## 🆕 New in v0.0.2

- **Experimental Widescreen (true 16:9).** X4's opt-in wide field of view is now
  available from the launcher's **Widescreen (EXPERIMENTAL)** toggle — it widens
  the three-layer scrolling background and re-anchors the health/weapon HUD to
  the true 16:9 edges, without disturbing world sprites. **4:3 remains the
  default**; flip the toggle to try 16:9.
- **OpenGL is now the default renderer** (full-rate hardware presentation).
  Software is still one click away in the launcher (Settings → Renderer).
- **A large batch of framework engine improvements** since v0.0.1 — full-rate
  OpenGL presentation, audio reserve/transient hardening (the MMX4 attract-demo
  audio fix and beyond), and byte-verified static + overlay dispatch. X4 was
  re-recompiled and re-validated against the updated framework.

---

The first public cut, made days after first boot. Mega Man X4 boots and
**plays** as a native Windows program — no emulator behind it — on the
[PSXRecomp](https://github.com/mstan/psxrecomp) framework, the same one behind
[TombaRecomp](https://github.com/mstan/TombaRecomp),
[MegaManX5Recomp](https://github.com/mstan/MegaManX5Recomp) and
[MegaManX6Recomp](https://github.com/mstan/MegaManX6Recomp). The game's MIPS
code is machine-translated ahead of time into native C and compiled into a real
Windows executable that runs on a faithful simulation of the PS1 hardware plus
the recompiled PS1 BIOS.

## ✨ Highlights — what works

- **Boots and plays.** BIOS → disc detect → engine load → intro cinematics →
  title → attract demos → into the game, with no known crashes on the covered
  path.
- **The intro cinematics decode and play** (the X vs. Zero opening), riding the
  faithful CD read/seek-timing work that unlocked MMX5's FMV.
- **Correct-by-hardware controller handling.** X4 shipped *before* the
  DualShock existed and its pad driver rejects analog pads outright — exactly
  like the real console, where a DualShock with the analog LED lit is ignored
  by this game. The runtime presents the plain digital pad X4 expects, and the
  launcher offers no pad-mode selector (there is exactly one valid mode).
  Keyboard and SDL gamepads both work.
- **A recompiler milestone made X4 possible:** X4's engine keeps inline data
  tables inside its code, which tripped the old function-extent heuristic into
  classifying ~261 KB of real engine code (7,373 functions) as data. The new
  control-flow-aware extent analysis recovers them — this fix benefits every
  PSXRecomp title.
- **Fast loading (turbo loads).** Loads fast-forward the whole machine at full
  host speed while keeping authentic 1× guest CD timing (and audio) intact.
- **Supersampling + anti-aliasing.** Internal-resolution SSAA (1×–4×) with
  optional linear present filtering.
- **Graphical launcher.** Pick BIOS / disc / memory cards, verify the disc, and
  configure renderer, supersampling, and controller — choices persist between
  launches.
- **Self-contained overlay toolchain.** As you explore new areas the runtime
  converts the game's streamed ARC overlay code to native code in the
  background — no developer tools needed; the release bundles a fully
  self-contained toolchain (embedded Python + TinyCC).

## ⚠️ Known issues

- **Early preview — not verified deep into stages.** The covered path (intro,
  title, menus, attract, starting a game) works with no known crashes, but X4
  has ~2,800 not-yet-recompiled code regions that an unvisited area may hit. By
  design that halts the program loudly instead of silently misbehaving — if it
  happens to you, please report where you were.
- **Memory-card save/load not yet verified end-to-end** in this build (the
  card hardware layer is exercised and healthy).
- **Widescreen is experimental and opt-in.** Authentic 4:3 remains the default;
  enabling the launcher toggle widens X4's background, actor activation/despawn,
  and draw-cull bounds for a true 16:9 field of view. Player health/weapon HUD
  pieces move to the 16:9 left boundary and enemy/boss health pieces move to the
  16:9 right boundary without changing their original layout.

## 📝 Setup

- **Bring your own** PlayStation BIOS (`SCPH1001.BIN`) and Mega Man X4 (USA,
  SLUS-00561) disc image — the launcher asks for each. Verify your disc against
  `DISC.md` before reporting regressions. Use `.cue` + `.bin` (or `.bin`); do
  **not** convert to a 2048-byte "cooked" `.iso` — that discards the XA sectors
  the movies and audio stream from.
- Options live in the launcher's **Settings** and are remembered between
  launches.
- The overlay cache grows as you play; please keep `overlay_captures.json`
  private — it contains game code read from your disc (see README).

# Mega Man X4 Recompiled — v0.0.3-alpha

This release replaces the old in-tree launcher with the shared Dear ImGui
`recomp-ui` launcher. BIOS/disc selection, settings, controls, memory cards,
and the Launch button now use the DPI-independent shared layout, addressing
the inaccessible/broken launcher reported in issue #3.

## Launcher and packaging

- Uses the current shared `recomp-ui` PSX profile and game-specific launcher
  art.
- Bundles the matching fonts and assets beside the executable.
- Preserves the existing renderer, widescreen, controller, and runtime choices
  while removing the fixed old launcher layout.

All existing alpha caveats and game-compatibility notes above still apply.

# Mega Man X4 Recompiled — v0.0.4 audio latency test 1

This prerelease reduces X4's host audio cushion from 180 ms to 60 ms. The
smaller cushion should make sound effects respond much sooner. It is an X4-only
choice: the framework retains the safer 180 ms default for games that need it
to cover long audio-production stalls.

Please report whether the delay is improved and whether you hear any new
crackle, especially during stage transitions, cutscenes, or loading.
