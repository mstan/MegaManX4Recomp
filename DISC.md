# Disc identity — Mega Man X4 (USA)

Format: **bin/cue, single track, MODE2/2352, NTSC-U**. Do **not** convert to
ISO — a 2048-byte "cooked" ISO discards the Mode-2 Form-2 XA sectors PSX uses
for streaming FMV/audio (X4 streams STR/*.STR MDEC movies + XA/*.XA BGM/voice).

| Field | Value |
|-------|-------|
| Title | Mega Man X4 (USA) |
| Serial | SLUS-00561 (file `SLUS_005.61`) |
| Volume ID | MEGAMAN_X4 |
| Publisher | CAPCOM CO.,LTD. |
| Track | 01, MODE2/2352, data |
| Size (.bin) | 592,217,136 bytes |
| MD5 | `1C425B49BB25C45B3964B2D565DD0EC0` |
| SHA-1 | `26FFE24A79384B24AF7571674251EE575E889B38` |

Verified 2026-07-05: locally computed MD5 + SHA-1 + .bin size + serial. Source archive
`Mega Man X4.7z` (kept bin/cue; never converted to ISO).

Boot EXE: `SLUS_005.61` — PS-EXE header (little-endian):
- entry (pc0):   `0x800DAE8C`
- initial $gp:   `0x00000000` (not preset — X4 sets $gp at runtime, as in X5/X6)
- load address:  `0x80010000`
- text size:     `0x0011F800` (1,177,600 bytes) → end of text `0x8012F800`
- data/bss:      addr 0, size 0 (all zero in header)
- initial $sp:   `0x801FFFF0` (PS-EXE header stack_base; SYSTEM.CNF STACK = 801FFF00)
- EXE file size: 1,179,648 = 2048-byte header + text

On-disc layout of note:
- `SLUS_005.61` — boot EXE (the static recomp target)
- `ARC/*.ARC` (138 archives: `CAPCOM.ARC`, `COLxx_*.ARC`, …) — streamed
  stage/engine code + assets loaded into RAM and executed. NOTE: unlike X5/X6
  (which use a single `ROCK_Xn.BIN`/`.DAT` overlay pair), X4 packs its streamed
  engine/stage code into these ARC archives. The overlay-cache pipeline captures
  the resulting dirty-RAM regions generically, so the same mechanism applies.
- `STR/*.STR` — MDEC movies (`CAPCOM20.STR`, `OP_U.STR`, `X1_U..X4_U.STR`,
  `Z1_U..Z5_U.STR`)
- `XA/*.XA` — streamed audio (`BGM1_U..BGM5_U.XA`, `BOSINT_U.XA`,
  `VOICE1_U..VOICE5_U.XA`)
- `ZNULL.DAT` — 37 MB null-padding file (disc layout filler)

Disc image and extracted EXE are local-only (gitignored); recreate from the
source dump if missing.
