# MegaManX4Recomp Rules

Static recompilation of **Mega Man X4 (USA)** — serial **SLUS-00561** — to
native code, built with the shared **psxrecomp** framework. The end goal is a
binary that plays without an emulator behind it, exactly like TombaRecomp,
MegaManX5Recomp and MegaManX6Recomp. X4, X5 and X6 share the same Capcom 2D
engine lineage, so much of the MMX5/MMX6 bring-up (DualShock-required input,
streamed engine overlays, 2D widescreen) is expected to port across at
different addresses.

## Inheritance

This project inherits, in order:

1. `F:/Projects/recomp-template/PRINCIPLES.md` — system-agnostic recomp/debug
   discipline (ground truth = original EXE + emulator oracle; generated C is
   evidence, not authority; first-divergence; no guessing). Enhancement layer:
   `F:/Projects/recomp-template/ENHANCEMENTS.md` (default = byte-identical with
   the option off).
2. The framework constitution at `psxrecomp-v4/CLAUDE.md` — a junction →
   `F:/Projects/psxrecomp/psxrecomp` (the shared `master` checkout). Read it
   first: no MIPS interpreter, no HLE BIOS shims, no stubs, recompiled-BIOS-first,
   fix the framework/runtime/config and **regenerate** — never hand-edit
   `generated/`.

## Project rules

- Game binaries (disc image, extracted boot EXE, the headerless Ghidra dump),
  Ghidra databases, memory cards, and build outputs are **local only** and must
  not be committed. See `.gitignore`.
- Tracked: `game.toml`, `seeds/`, `annotations/`, `ghidra/instructions.txt`,
  `ghidra/scripts/`, `ghidra/annotations/`, `CMakeLists.txt`, `tools/`, docs.
- Codegen/runtime fixes belong in the framework (`psxrecomp-v4/`) or in
  per-game `game.toml` config — never in `generated/*.c`. A fix that only this
  game needs is a smell; prefer a class fix that the next title inherits.
- After every run, resolve all dispatch misses before any other debugging.
- The framework version this project builds against is pinned in
  `psxrecomp-v4.pin`. This repo builds against the shared `master` checkout
  (junction `psxrecomp-v4` → `F:/Projects/psxrecomp/psxrecomp`), same as
  TombaRecomp / MMX5 / MMX6.
- The source disc is the Redump USA dump — hashes recorded in `DISC.md`; verify
  a future dump is byte-identical before blaming a regression.
