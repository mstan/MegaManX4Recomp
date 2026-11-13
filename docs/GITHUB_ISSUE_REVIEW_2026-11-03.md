# Mega Man X4 issue review, 2026-11-03

Reviewed all 22 open issue bodies and their complete comment threads against
game master `ae903a8f603fadaea43fd2bea2c8db88918d4636` (1.0.28), current source,
release notes and retained qualification records. These are review dispositions,
not 22 new runtime reproductions. No public comments or issue state changes
were made. Central tracking: `beads-eio.9.293`.

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
