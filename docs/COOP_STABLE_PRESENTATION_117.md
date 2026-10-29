# Stable presentation follow-up

Tracking: central Beads `beads-eio.9.280`. Experimental X1 characters and
Four Player Co-op remain separate from this stable cleanup.

## Unified Widescreen pause menus (`beads-eio.9.233`)

The stable runtime reproduced black paired pause menus with Unified local
cameras and Widescreen enabled. The same native menu sequence without
Widescreen displayed normally. The personal menus were live but their GPU
draws used the native-wide world surface, while local-pair presentation
captured the empty native rectangles.

Widescreen now identifies personal menus and their shared Options/Quit
frontend through the existing native-scene predicate. This classification
remains valid through nested GPU submission and presentation. It does not
change menu input, inventory, simulation clocks or world fades; gameplay
returns to its usual widescreen classification on resume.

Current original-disc native validation, hidden/muted with isolated cards:

- Inspected composed X/Zero menus: both visible, with their correct portraits
  and blue/red backgrounds. Baseline visibility ratios were `[0,0]`;
  candidate ratios were `[0.9977,0.9984]`.
- Full simultaneous-menu sequence passed independent weapon/tank management,
  healing while the partner navigates, held-Start rejection, either-player
  resume, paused save/load, P2 Quit confirmation, shared Options return,
  surviving-P2 management and native firing after resume.
- 2,736 render transaction restoration checks: no mismatch, abort, watchdog
  or VRAM leak.

Private evidence and exact capacity/source/binary receipts:
`G:/Temp/Codex/psxrecomp/x4-train-pillar116/`, particularly
`baseline-menu-unified`, `baseline-menu-unified-stock`,
`candidate-menu-unified-r2` and `candidate-unified-management`.

## Remaining qualification

The train foreground pillar report at
<https://youtu.be/ucIsvq2lM3k?t=2032> is still being qualified under
`beads-eio.9.279`. Teleporting ahead skips moving train collision progression;
those failed fixture routes are not passing evidence. Native deferred-pillar
ownership work is separate from the validated menu correction.

The stable presentation tracker also retains the older Frost enemy/dialogue,
armor/death, train-background and Quit/Continue reports, with their individual
evidence and caveats. Cosmetic severity alone is not a reason to defer a bug.
