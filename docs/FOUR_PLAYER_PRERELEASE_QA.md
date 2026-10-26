# Four Player Co-op Preview 1 package checks

Version: `1.0.6-4p-spike.1`, Windows x64 only. The approved Release ENHANCED
runtime is reused unchanged: SHA-256
`8fd255da5480077857cd82604736603c27209cf636e679344c384f001a00d5fe`.
Runtime source: game `1b1f40b5ba0fbdaf929feb6e32bdc5b632b172cf`, framework
`a0738464afe8bdf8cb2e9e1eb939699e37027ac4`, recomp-ui
`b964eabe5a3438b8ed121064354ba27c1accef3f`. Later branch commits change release
configuration, documentation and a source config check; no runtime is rebuilt.
`BUILD_INFO.json` records the exact release commit. The release asset checksum
is published in `SHA256SUMS.txt`.

The archive is staged through the framework mod, execution-manifest and pinned
toolchain gates. It contains OpenBIOS and its MIT notice, launcher/runtime
notices, the subtitle importer and public documentation. The current-framework
emitters are rebuilt for the bundled native fallback toolchain. Original-disc
inventory found no separate X4 executable overlay, so no unqualified cache or
private code captures are shipped.

The archive has 15 verified mod packages. CD Speed and Fast Loading
are absent. Four Player Co-op, free cutscenes and diagnostic recording start
off. The clean source-controlled preview settings enable OpenGL, native
internal resolution and the render worker, windowed with normal audio. There
are no personal saves, state files, bindings, media packs or disc paths.

A fresh extraction opens recomp-ui without a disc and does not approve a
missing disc for netplay. Dashboard, Settings, Mods and all four Controls
slots render. Native resolution, render worker, windowed presentation and
normal-audio preferences persist when the launcher saves its settings.

The exact packaged game configuration, settings, catalog and binary pass a
separate hidden, muted native smoke using the owner's external original USA
disc. Only private test copies receive keyboard assignments and the four-player
mod selection. No game config addresses or runtime settings are substituted.
The game loads without a loose `mmx4/SLUS_005.61` file in the package. The render
worker starts, all four players enroll and move from ordinary shared keyboard
input, attacks run, the full 6 MiB authored background arena is active, four
private pause menus initialize and gameplay resumes. The test uses no RAM
writes or developer gameplay cheats; observations are read-only.

The busy-area measurements were 42.83 FPS idle and 37.04 FPS charging/firing.
They establish that performance remains an open limitation. They are not a
controlled comparison with previous smoke runs on different host/route loads.
The prior same-binary three/four-player HUD, background and four-peer LAN
qualification remains documented in [HUD QA](FOUR_PLAYER_HUD_QA.md) and
[layer QA](FOUR_PLAYER_LAYER_QA.md). Full campaign, Internet, vehicles and
cross-platform gameplay remain unqualified. Four-player save states/rewind
are blocked; native memory-card saves retain existing isolation.

Private retained evidence: `prerelease-qa-defaults/report.json`, launcher
screenshots, `labels-production-qa-prerelease/report.json`, its runtime log,
package report and upload verification receipt. Disposable extraction, test
runtime data, emitter build and download cache are cleaned after publication.
