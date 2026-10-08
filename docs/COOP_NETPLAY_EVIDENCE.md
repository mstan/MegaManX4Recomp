# X4 co-op private netplay evidence

Validated on Windows/NVIDIA OpenGL on 2026-10-08 with the original local X4
disc and `openbios.bin`. The script is
`tools/test_mmx4_coop_netplay.py`; it creates private loopback peers, copied
executables, an empty offline mod catalog, independent configuration/saves,
and owned process handles. It uses host-layer controller inputs, not native
RAM writes, snapshots or simulation-layer overrides. No public rooms are used.

## Observed matrix

| Campaign | Transport | Result | Private report |
| --- | --- | --- | --- |
| X | Delay-sync, input delay 2 | Passed | `.cache/mmx4-coop-netplay/20261008T213148Z-0ad2fe80/report.json` |
| Zero | Delay-sync, input delay 2 | Passed | `.cache/mmx4-coop-netplay/20261008T213240Z-3c58770f/report.json`, `delay-zero` case |
| X | Delay-sync plus 25ms receive delay and ±10ms jitter per peer | Passed | `.cache/mmx4-coop-netplay/20261008T213756Z-e3f20748/report.json`, `jitter-x` case |
| Zero | Delay-sync plus 25ms receive delay and ±10ms jitter per peer | Passed | Same report, `jitter-zero` case |

The delay cases used executable SHA-256
`79a656a886df8c8a8084f7ace8121372d1b1edd4d47342c3a9996607fa287c2e`.
Both final jitter cases used the later executable
`d9951fb143db8a046bfdb3f70f6b8237b72323a4297e7cb17b361d6fe5684de9`,
which includes lifecycle capture before native section clearing. The reports
retain launch arguments, ports, executable/configuration/BIOS hashes, states,
and owned PIDs. Private screenshots and logs are excluded from commits.

All passing cases reached native intro gameplay (`PLAY=6`), enrolled both
characters with the expected campaign ownership, and observed independent
movement on both peers. Square was held during movement and normal session
inputs were compared afterward; this script does not prove enemy damage.
Each seat opened its own native pause menu, froze the second gameplay pass,
and closed that menu. The final jitter reports record native MENU main state
1 (interactive), 2 (closing), then normal gameplay. Core and host gameplay
CRCs agree at every common logged simulation tick, with fresh matched samples
after each movement and menu phase. Final matched checkpoints reached tick
3359 for X and 3132 for Zero. Both surviving peers reported the original
runtime's disconnect reason after the script stopped only its owned guest.
All owned processes have exited.

## Profile and presentation policy

Actual software and Vulkan netplay launches returned failure before guest
boot, explaining that this profile requires OpenGL retained textures. Each
pair deliberately used different offline preferences: host 16:9/software/
1080p and guest 21:9/Vulkan/720p. Explicit CLI OpenGL selects the supported
backend; trusted activation selects fixed 4:3. The final runs verify actual
OpenGL scales 5x and 3x, while software at 1x remains authoritative for VRAM
and CRCs. Both jitter logs confirm the recomp-net link simulator engaged and
held packets; environment configuration alone is not accepted as evidence.

Retained banks affect the GL image. Ordinary guest VRAM is still sampled by
software authority, following the X6 approach. This proves state agreement
across these presentation settings, not equality of retained-bank graphics
and authoritative VRAM. Captured menus need separate visual review.

Framework validation passed 11 selected policy/regression CTests. Real
OpenGL readback checks passed 312 assertions at 1x and 321 at 4x, including
explicit active-profile permission for banks in dual-raster mode and original
VRAM sampling by software with a retained bank selected. Actual build flags
also passed syntax checks for `main.cpp` and `gpu_gl_renderer.c`.

## Failures retained during validation

The first private run caught host-layer virtual devices being ignored by
headless netplay sampling; framework commit `1e33761d` routes them through
normal local capture. The next reached gameplay but refused bank enrollment
under dual-raster rendering. Framework commit `dfd29f76` adds explicit trusted
OpenGL/bank policy, retaining software authority. Failed reports remain in
the private evidence directory.

An earlier Zero jitter run remained in the native menu after a six-simulation-
tick close pulse. Both peers stayed synchronized. The driver now waits for
MENU main 1 and holds Start until native main 2 or gameplay is observed;
both campaigns passed with recorded native transitions. It does not force
menu state or waive failure checks. The original failure is retained in the
`20261008T213240Z-3c58770f` report's `jitter-zero` case.

## Limits and further qualification

The two-peer script qualifies intro inputs, menus, profile policy, state
agreement and disconnect. Stage loading, combat, tanks, death/respawn, bikes,
and story progression have separate native or fixture evidence. It is not a
two-peer stage-clear matrix or an exhaustive network soak test.

The title host digest includes canonical hidden seat snapshots and the
pending projection/lifecycle state. Native nested calls can yield at VBlank.
Their host-stack CPU/PAD restoration continuations are not fully serialized
into the digest, so this is not proof of every possible future machine state.
The profile uses delay-sync and refuses rollback, savestates and rewind.
Compatibility revision changes, or execution-contract source hashes, must
identify incompatible compiled co-op logic; profile policy hashing alone
does not hash the implementation of a callback.
