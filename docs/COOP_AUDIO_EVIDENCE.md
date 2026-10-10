# X4 private character audio

Original source: owner-provided USA SLUS-00561 executable and disc. The owner
reported wrong P2 voices in both rosters: X/P2 sounded like Zero, and Zero/P2
sounded like X. Changing the campaign flag cannot change the loaded samples.

The SNES Mega Man X boss-rush `src/mmx_render_assets.c` preloads the owner's
resources and routes actors to private residency. This adapter applies that
pattern to audio. Assets are prepared from the selected disc before use and
retained in memory, never distributed. ADPCM remains encoded until the native
voice decoder reads it, preserving pitch, interpolation, history and ADSR.

## Original boundaries

- `8001540C` selects a game sound group and indexes record/end pointers at
  `80141F50`/`80141EE8`. Direct records use their low six bits as a VAB handle;
  upper bits select direct/sequence routing and are preserved.
- `PL00_U.ARC`/`PL01_U.ARC` group-1 header/body members are 10/11; group-3
  members are 12/13. Headers have type 6 and body types `00020101`/`00020301`.
  The header's low-24-bit offset locates `pBAV` after its sound records.
- `800E4398` parses VAB metadata at an explicit sample base without copying
  samples. `800E479C..800E47A8` leaves status 2, awaiting a body; original body
  completion at `800E4BC0..800E4BD4` publishes status 1. `800E1A60(0)` releases
  the transfer gate. Private completion occurs after registering the body.
- `800154E8` calls `800DFCFC` with explicit voice, VAB, program and tone.
  It queues parameters/KEYON, so private bank selection must survive until
  physical KEYON occurs later.

## Ownership and state

P2 uses private VAB handles 12/13 for X and 14/15 for Zero. The original parser
installs character metadata at logical sample base zero. Scoped sound-record
pointer swaps end before P1 resumes. Other groups retain their normal path.

The SPU has optional immutable banks 1..31; bank zero remains hardware RAM.
KEYON consumes a pending binding, retaining it through playback/KEYOFF. A new
ordinary KEYON returns to RAM. DMA, reverb, capture buffers and RAM IRQ checks
continue using hardware RAM. Private fetches use bounded bank offsets.
Active/pending bank IDs are serialized for rollback. Old SPU section sizes are
rejected. The co-op compatibility digest covers the adapter and SPU sources.

## Validation limits

The audio boundary test covers both rosters/groups, pointer restoration,
bindings, reopened metadata and transfer-gate release. Library calls are models.
The actual SPU decoder test covers concurrent banks/RAM at identical addresses,
DMA replacement, looping, KEYOFF, ordinary reuse and snapshot replay. Existing
end-block, pitch and Gaussian tests pass. Native eye/ear checks remain pending.
# Simultaneous sounds: subsequent co-op regression

The earlier private sample banks preserve character identity, but both seats
still selected the same original hardware voice. Direct sound records in
both PL archives use channel 20 for character voices, with two-tone records
using 20/21; movement/weapon effects also share fixed channels. A later jump
or hurt can therefore replace the other seat's ongoing sound.

The new adapter lets the original `800DFCFC` driver compute volume, pitch,
start offset and ADSR from the original VAB. For retained P2 handles it starts
a private voice instead of publishing the queued hardware KEYON. It restores
the hardware register mirror at `80166D90 + voice*10`, the voice record at
`8013DCA8 + voice*34`, dirty byte at `8013E1D0 + voice`, history/KEYOFF block
`80175EF0..80175F38` and KEYON/reverb/noise masks at `8013BC08..8013BC14`.
The finite native calculation runs uncharged to prevent an intervening driver
flush. P2 priorities reside in serialized private guest memory. Sound-status
and character sound-stop callbacks address the private slots; global silence
still silences both sets of voices.

An isolated MIPS run of the original `800E4398` VAB parser and `800DFCFC`
key-on routine succeeded for X bank 13 and Zero bank 15 on channel 20. Both
produced pitch `800`, ADSR words `80FF/1FC0`, and no hardware MMIO writes;
stereo gains differed as expected from their original program data. This
checks original driver calculations, not full gameplay audio qualification.
The actual SPU unit test now mixes hardware channel 20 with private slot 20,
checks independent KEYOFF/volume, unchanged hardware registers, and byte-exact
PCM replay after restoring the extended snapshot. Ordinary snapshots keep
their original size when private voices have never been used.
