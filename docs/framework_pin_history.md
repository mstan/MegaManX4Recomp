# Framework pin history (historical)

The `psxrecomp` framework used to be pinned via this hand-maintained
`psxrecomp-v4.pin` file. That mechanism has been **replaced by a real git
submodule**: the framework commit this repo builds against is now recorded as
the `psxrecomp-v4` submodule pointer (see `.gitmodules`). Bump it the normal
way:

    git -C psxrecomp-v4 fetch && git -C psxrecomp-v4 checkout <new-sha>
    git add psxrecomp-v4 && git commit -m "bump psxrecomp-v4 to <new-sha>"

The notes below are kept only as a historical changelog of which framework
build each MegaManX4Recomp release was cut against.

---

# 2026-07-13 UPDATE (v0.0.2) — repinned to master dde268d: a large batch of
#   framework engine improvements accumulated since 2ba81d5 (full-rate OpenGL
#   presentation, audio reserve/transient hardening, byte-verified static+overlay
#   dispatch, the widescreen framework work — 2D-scene pillarbox, present ring,
#   curved-backdrop scoping — and the analog-stick/D-pad decouple fix, inert for
#   digital-only X4). This release also UN-HIDES X4's own widescreen: the opt-in
#   true-16:9 work merged after v0.0.1 (game.toml [widescreen] offer=true,
#   bg2d/cull/HUD-arena hooks) is now shipped as an EXPERIMENTAL launcher toggle
#   (default 4:3). BIOS + game C regenerated against this pin. Verified by
#   playtest before release.
# 2026-07-10 UPDATE — repinned to master 2ba81d5 (merge of
#   feat/audio-mmx4-attract): MMX4 attract-demo audio fix, two framework
#   defects, user-ear-validated on the ice-stage demo. (1) cdrom read stream
#   froze disc time while a controller INT was unacked -> XA sectors ~1.5%
#   late -> SPU cd_ring starved 5-6x/s (6 Hz music dropout); now the disc
#   clock never pauses and data-ready INT1s pend one deep (Beetle
#   SetAIP/CheckAIP model). (2) SPU END-without-REPEAT blocks kept the
#   envelope alive and decoded past the terminator; X4 parks finished SFX
#   voices on +28672-DC filler with ~infinite release -> mix railed at
#   +32767 ("static" during ice-block hits, 38-41% samples clipped). Now
#   jumps to repeat + forces env=0 (Beetle spu.cpp:333,341-352) -> 0.00%
#   clipped. Runtime-only: NO regen required (codegen untouched);
#   build-master carries the validated exe.
# 2026-07-05 UPDATE 2 — repinned to master 0b95879: the data-as-code recompiler
#   fix landed on master as 85987f2 (byte-identical recompiler tree to
#   fix/data-as-code a0ee2ae — verified `git diff master fix/data-as-code --
#   recompiler` empty), so the branch pin is obsolete. Same codegen hash
#   0x0f5548cf. ALSO RESOLVED: the "keyboard input not reaching the game"
#   follow-up below was never a keyboard/runtime bug — X4 predates the
#   DualShock and its libpad rejects pad id 0x73 (analog); with
#   [controller] default_analog=true NO input source (keyboard, DualSense,
#   debug override) got past the title. game.toml now presents a digital pad
#   (default_mode="digital", id 0x41); input verified working (FMV skip +
#   title respond to keyboard START). SIO-ring evidence: poll answers
#   01/42/00.. -> FF/73/5A/F7/FF with START held — buttons delivered, game
#   discarded them under id 0x73.
# 2026-07-05 UPDATE — MMX4 now REQUIRES the data-as-code recompiler fix to boot.
#   a0ee2ae (branch fix/data-as-code, off master 80f4ed6): control-flow-aware
#   function boundaries — recovers ~4600 real funcs the old heuristic stubbed as
#   data (incl. func_800EE3A4). Without it MMX4 FAIL-FASTs at frame 277. WITH it,
#   MMX4 boots HLE -> X4 entry -> plays the intro FMV (frame 6000+). New codegen
#   hash 0x0f5548cf (was 0x7ea4ba61) -> regen MMX4 (psxrecomp-game built from this
#   fix) before building. The fix is PARKED on fix/data-as-code, NOT yet on master
#   (lands in the coordinated cascade). Known: ~2804 stubs remain (may hit a later
#   gameplay stub); MMX4 full.c bloated 76->225MB from a separate parked alias
#   issue (task #13); KEYBOARD INPUT not reaching the game (attract+DualSense pad
#   fine) — open follow-up. To reproduce: build psxrecomp-game from fix/data-as-code,
#   regen, build build-master; boots to intro FMV.
# Created 2026-07-05: Scaffold MegaManX4Recomp (Mega Man X4, USA, SLUS-00561).
#   Pinned at psxrecomp master tip 80f4ed6 (facelifted framework: OpenGL+Vulkan
#   renderers, HLE boot tier, TCC overlay tier; overlay codegen hash 0x7ea4ba61).
#   Mirrors the MegaManX5Recomp setup — same Capcom X-series 2D engine family
#   at different addresses (entry 0x800DAE8C, load 0x80010000, text 0x11F800).
#   X4 streams stage/engine code from ARC/*.ARC archives (vs X5/X6's single
#   ROCK overlay); the overlay-cache pipeline captures the dirty-RAM regions
#   generically. First recompile + build + smoke-boot done at scaffold time.
