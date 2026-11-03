# Mega Man X4 issue review, 2026-11-03

Reviewed all 22 open issue bodies and their complete comment threads against
game master `ae903a8f603fadaea43fd2bea2c8db88918d4636` (1.0.28), current source,
release notes and retained qualification records. These are review dispositions,
not 22 new runtime reproductions. No public comments or issue state changes
were made. Central tracking: `beads-eio.9.293`.

| Issue | Disposition | Evidence and next action |
| --- | --- | --- |
| [4: launcher does not open](https://github.com/mstan/MegaManX4Recomp/issues/4) | Needs reporter evidence | Three existing follow-ups have no response. Obtain current-build startup log, OS/GPU and whether any process/window appears; do not infer a cause from successful X6/Tomba launches. |
| [9: Switch Pro controller](https://github.com/mstan/MegaManX4Recomp/issues/9) | Partially fixed; hardware confirmation pending | SDL instance-ID-zero rebinding shipped in Playtest31. Switch Pro drift remains unconfirmed; saved deadzones and USB/Bluetooth/input preview matter. Keep distinct from29's binding-only report. |
| [25: intro WARNING loop after death](https://github.com/mstan/MegaManX4Recomp/issues/25) | Unreproduced historical report | Existing original-disc Unified survivor test completes WARNING and initializes Eregion at48HP. Keep open for the exact death timing/build/recording; a different successful approach is not a fix. |
| [26: online white screen](https://github.com/mstan/MegaManX4Recomp/issues/26) | Keep open for original connection path | LAN/Direct IP succeeded; shared-installation startup failure was separately confirmed. Existing follow-up asks for separate installations and both logs. This does not establish Internet/Discord startup correctness. |
| [27: music missing after state load](https://github.com/mstan/MegaManX4Recomp/issues/27) | Unreproduced; separate from boss-theme defect | Prior warm/cold and actual F7 restore checks retain music. Need a failing current-build state and matching recording. Wrong track during an added boss encounter has a different cause. |
| [28: state rejects changed mods/settings](https://github.com/mstan/MegaManX4Recomp/issues/28) | Narrow supported cases shipped; broad request conflicts with current owner policy | HUD and native character-code preference changes work since31. Arbitrary simulation/layout changes need not accept old snapshots. No cross-build state compatibility or migrations are required. Current-build restore correctness and native cards remain required. |
| [29: controller rebinding](https://github.com/mstan/MegaManX4Recomp/issues/29) | Shipped-fix closure candidate | ID-zero capture, Map All release handling and identical-controller selection shipped in31, with Windows/Linux focused checks. Several existing requests for device confirmation remain unanswered. Reopen on a concrete current failure. |
| [30: arbitrary FMV replacements](https://github.com/mstan/MegaManX4Recomp/issues/30) | Keep open as feature request | Japanese-disc movies and documented subtitle imports ship; arbitrary per-movie user files and higher-resolution playback remain broader unimplemented requests. |
| [32: hover jets, train scrolling, death effects, diagnostics](https://github.com/mstan/MegaManX4Recomp/issues/32) | Split into independent acceptance items | Hover jets shipped in32; later train-wrap scrolling shipped in42. Exact peer death-effect/survivor-history report is not qualified by those fixes. Current public diagnostic recording is optional/off; old mandatory-playtest policy no longer describes public defaults. See `COOP_GITHUB_REVIEW38.md` and checklist149/189. |
| [49: charged Aiming Laser and split slowdowns](https://github.com/mstan/MegaManX4Recomp/issues/49) | Keep separate graphics and performance items | The earlier uncharged/deferred beam fix for39 is not proof for this charged reticle/scan-line case. Split rendering improvements and stable performance qualification exist, but the exact Intro/Jet/Web recordings still need comparison. |
| [53: shared release packaging](https://github.com/mstan/MegaManX4Recomp/issues/53) | Implemented; closure candidate | `packaging/release.toml` exists; title Windows/shell entrypoints delegate to the shared builder.1.0.20 and later releases use that pipeline. Compatibility wrappers contain no forked packaging implementation. |
| [55: crash at character select](https://github.com/mstan/MegaManX4Recomp/issues/55) | Needs crash evidence | Existing attachment review found no fatal cause. Existing follow-up requests last-run report, startup log, OS/GPU/mods and host crash event. Avoid attributing this to dispatch, graphics or mods without those records. |
| [71: co-op under30FPS](https://github.com/mstan/MegaManX4Recomp/issues/71) | Needs matched current workload | Distinguish rendering cost from network stalls; request both peers' host/settings/camera/transport and diagnostics. Related to49/82 but insufficient evidence to mark duplicate. |
| [79: Japanese disc rejected by picker](https://github.com/mstan/MegaManX4Recomp/issues/79) | Fixed in this branch | Both Japanese-media manifests used semicolon-separated patterns; the current UI splits resource patterns at commas. Corrected both manifests; the parsed-package regression passes. Native picker interaction itself was not exercised. |
| [82: frame drops](https://github.com/mstan/MegaManX4Recomp/issues/82) | Strong closure candidate | A commenter explicitly confirms full-speed performance in1.0.24. This does not qualify the original author's unknown hardware or every netplay workload; retain separate71/49 reports. |
| [87: intro widescreen and boss walk](https://github.com/mstan/MegaManX4Recomp/issues/87) | Partially covered; keep movement remainder |1.0.17 preserves wide scenery during the scripted dragon door lock. That fix does not establish the separate final-dragon involuntary-left movement report; obtain a current recording of that sequence. |
| [90: multiple widescreen layers/rooms](https://github.com/mstan/MegaManX4Recomp/issues/90) | Keep open, five independent visual items | Review parallax, Mushroom stairways, Cyber whiteout coverage, Sigma borders and intermittent Slash corruption separately. Later Storm Owl background-range repair covers a narrower proven fault; do not claim all five fixed. |
| [91: impact shake](https://github.com/mstan/MegaManX4Recomp/issues/91) | Keep open for exact location/history | Earlier Magma/Intro shake fixes do not prove the reported Slash right-side impact case. Compare left/right positions under the same current renderer/aspect and impact. |
| [92: leave uncleared stages](https://github.com/mstan/MegaManX4Recomp/issues/92) | Elective feature/design decision | Define return destination, retained pickups/progress, lives and co-op agreement before implementation. It should remain optional. |
| [93: skip Sigma boss rush](https://github.com/mstan/MegaManX4Recomp/issues/93) | Elective feature/design decision | Choose immediate final teleport versus opening Sigma's existing teleporter. Preserve normal campaign progression with the option off; a QA checkpoint fixture is not a shipped feature. |
| [94: No Music package](https://github.com/mstan/MegaManX4Recomp/issues/94) | Track through open PR99 | Owner already invited first-party inclusion and author supplied the PR. Validate its audio scope/defaults separately; do not close before integration. |
| [102: premature Gunner Sigma kill](https://github.com/mstan/MegaManX4Recomp/issues/102) | Highest-priority remaining gameplay reproduction | New1.0.24 report includes a video and both heroes. Test the dialogue-to-teleport HP/invulnerability boundary with damage modifiers off and compare stock behavior. No fix or current-build reproduction is claimed here. |

The owner-reported Iris/Double music defect is additional to these22 issues.
The native stage11 boss selector at `8001658C` branches on PLAY+43. During the
optional second encounter, assets/dialogue use the other story while this
branch still sees the original campaign. A guarded instruction hook now uses
the existing encounter selector for both co-op and solo Complete Story.
Double retains the native track/volume pair `0F/75`; Iris retains `0D/7F`.
The campaign byte, player identity, card data and other sections stay native.

Focused tests cover both orders, first/second rounds, snapshot restoration,
co-op and solo package ownership, local presentation exclusion and other
sections. The hook word `14400004` is checked against the original executable.
Native qualification and final retained evidence are recorded on the central
issue; source/unit results alone do not constitute a released game fix.

## Music qualification

Fixes are proposed in [PR108](https://github.com/mstan/MegaManX4Recomp/pull/108).
Music commit: `d7b077e`; Japanese-disc filter commit: `479b30d`.

| Native profile | Observed result |
| --- | --- |
| X campaign, two-player Split, Complete Story | Pass: Double uses BGM2 channel7/volume117, then Iris uses channel5/volume127. Native first-boss defeat and a disk save/load occur between encounters; campaign stays0. |
| Zero campaign, two-player Split, legacy both-story-bosses option, Complete Story disabled | Pass: Iris uses channel5/volume127, then Double uses channel7/volume117. Native first-boss defeat and a disk save/load occur between encounters; campaign stays1. |
| X campaign, solo Complete Story | First Double track correct. Existing attack fixture timed out with Double at1HP before a native defeat; second encounter remains unverified by this run. Failed receipt retained. |
| Zero campaign, solo Complete Story | Pass: Iris uses channel5/volume127, then Double uses channel7/volume117. Native first-boss defeat and a disk save/load occur between encounters; campaign stays1. |

The runtime checks read the game's selected XA file/channel and native volume;
they are not a human listening test. Audio emulation stayed active with a dummy
output device, and all windows were hidden. Fixtures bypass earlier campaign
clears and shorten the first boss's HP, but native attacks must deliver defeat.
No victory or music-selection result is set by the fixture. Native cards and
settings are confined to disposable profiles.

The development candidate reuses retained runtime objects from framework
`09ba00ed7ad9d5e7d88ca428e2c7dc23822a2ded`. The current title-pinned emitter
`97e7b7f0e465e23384e2efbf43153c6a297d3fd2` regenerated exactly one changed game
shard (`full_01`); its sole behavioral delta is the guarded music instruction
callback. The updated story module and development fixture modules were rebuilt.
This is bounded game correctness evidence, not a new production package,
whole-campaign pass, Internet-netplay check or performance measurement.

Focused story/snapshot and original-executable instruction checks, three
hook-registration tests, seven release-configuration tests and the parsed
preloaded-package test pass. Retained binary SHA-256:
`74493f43395e1bfda5d7521d5cbf3a0bb02adbfb1bb0ba37b16d56c251e7519f`.
Receipts, captures, isolated states and build attribution are under
`G:/Temp/Codex/psxrecomp/x4-issue-review-20261103`; redundant regenerated
sources were removed. The candidate remains unmerged and unreleased.
