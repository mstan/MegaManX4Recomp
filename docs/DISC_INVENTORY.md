# Original-disc code inventory

The bounded September 11, 2026 inspection found **no separate game executable
overlay** on Mega Man X4 USA (SLUS-00561). The game already recompiles its resident
`SLUS_005.61` executable. Earlier repository statements that X4's ARC archives
contain streamed engine/stage code were assumptions and are corrected here.
This result does not warrant a new overlay release or version bump.

This is an original-disc and loader investigation, not a claim of complete
decompilation or proof that every execution path has been classified. Historical
RAM captures and native caches were not discovery inputs or coverage criteria.

## Reproducible structural inventory

The [declarative profile](../aot/disc_inventory.json) pins disc SHA-1
`26ffe24a79384b24af7571674251ee575e889b38` and checks selected original loader
instructions. It uses the shared psxrecomp
[inspector introduced in 070c92c6](https://github.com/mstan/psxrecomp/commit/070c92c647e1c7066a7af7329306eba7b56df178).
With a framework checkout containing that tool:

```sh
python3 psxrecomp-v4/tools/inspect_disc_inventory.py \
  --profile aot/disc_inventory.json --cue "mmx4/Mega Man X4.cue" \
  --output build/disc-inventory.json
```

The inspector reports metadata and hashes without writing game bytes. It
verifies all 163 ISO files, including every entry of the executable's 138-entry
archive file table. A missing classification, changed loader word, mismatched
ISO extent, malformed archive, or changed member count fails the inspection.

| Inventory | Result |
| --- | --- |
| Resident PS-X executable | One; load `0x80010000`, size `0x11F800`, entry `0x800DAE8C` |
| Typed ARC containers | 130 files, 612 members |
| Other ARC files | Eight raw data files, described below |
| Other files | STR video, XA audio, SYSTEM.CNF, ZNULL.DAT filler |
| Additional game code images recovered by the generic extractor | Zero |

The generic AOT extractor separately recovered the known SCPH-1001
boot-installed RAM helper. That is BIOS code, not evidence of a game overlay.
No new cache or release is being produced from that one helper.

## Archive loader evidence

`0x80013614` indexes fixed 12-byte records at `0x800F0E18`. Their first two
words are the file's absolute LBA and byte size; the third matches its first
on-disc word. All 138 records exactly match the ISO ARC files.

The archive callback `0x80013E68` reads a 2048-byte header, then consumes a
member count and total byte size followed by `{type, byte-size}` descriptors.
Members begin on successive 2048-byte boundaries. Descriptor sizes and sector
rounding account for every byte of all 130 typed containers. The header's unused
bytes are opaque, not assumed zero; repeated member types are valid.

At `0x80014008`, the upper half of the type selects a callback from the fixed
table copied from `0x80010014`. These are resident EXE addresses, not function
pointers loaded from the archive:

| Type upper half | Members | Callback | Observed handling |
| --- | ---: | --- | --- |
| 0 | 419 | `0x80014140` | Verbatim RAM data copy; publishes data pointers through `0x800F15BC` |
| 1 | 114 | `0x800142BC` | Graphics upload queue, consumed by `0x800148EC` |
| 2 | 79 | `0x80014514` | Sound upload queue, consumed by `0x80014968` |
| 3 | 0 | `0x800141BC` | Sequence loading callback, unused by these typed members |

The RAM copy callback forwards to the sector reader `0x800136B0`. It contains
no decompression or transfer of control into the copied payload. Its published
pointers occupy scratchpad data slots and resident globals. This alone cannot
prove how every later consumer uses those pointers; combined with the payload
inspection, it provides no positive executable-image evidence.

All 612 members contain zero aligned `jr ra` instructions. Instruction shapes
alone are not a reliable code classifier, especially for compressed data. The
inspected decompressor at `0x80016FF4` has a direct caller at `0x80015F54`:
it writes into texture buffers at `0x8016DEA8` or `0x8016EEA8 + slot * 0x1000`.
That caller constructs graphics rectangles; `0x80015E54` submits them through
the image-upload routine. No executable installation was established in this
path. This is a bounded examination of the identified loader/decompressor, not
an exhaustive survey of every transform in the game.

## The eight other ARC files

| Files | Original evidence |
| --- | --- |
| `MOJIPAT.ARC` | File index 65 is copied to `0x801F3000` at `0x80012E58`; text-pattern data |
| `PL00SEP.ARC`, `PL01SEP.ARC` | Selector bytes at `0x800F1654` are 74/76; copied to `0x801459C8` at `0x80015C90`, then passed to sequence-open `0x800DC9F4`; both contain `pQES` sequence headers |
| `PLDEMO00.ARC` through `PLDEMO03.ARC` | Selector bytes at `0x800F2180` are 80-83; copied to `0x801F6000` at `0x8001D314`; `0x80021E3C`/`0x80021E74` consume character/stage and replay-state fields |
| `PLDEMO.ARC` | Same 8192-byte replay-shaped layout; an active selection was not established |

None of these eight files contains an aligned `jr ra` or negative stack-adjust
instruction. The remaining non-ARC namespace contains the resident executable,
boot configuration, movie/audio streams and disc-layout filler; no second
executable or comparable code archive was identified.

## Validation and limits

The shared inspector passed nine synthetic tests, covering malformed sizes,
counts, extents, unclaimed payload, duplicate types, opaque header padding,
ISO-table mismatches, invalid record fields and missing file classifications.
The original-disc run passed with 163 files and 612 members. The generic extractor independently
reported zero additional game producers and one BIOS helper.

No runtime code, configuration values, framework pin or package version changes
as a result of this inventory. Interpreter and runtime compilation fallback
remain available. No new boot/gameplay claim accompanies these documentation
changes. Future positive evidence of relocated, generated, or compressed code
can extend this inventory; a historical dirty-RAM capture by itself is
insufficient.
