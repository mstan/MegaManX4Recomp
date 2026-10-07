Mega Man X4 Fast Loading
=======================

This feature is disabled by default.

Host-pacing modes accelerate real-world loading while preserving every guest
frame, CD deadline, interrupt, callback, and game-logic step. They stop on the
first frame where the sustained-load predicate clears.

Experimental CD-timing modes deliver emulated CD interrupts earlier. They can
change guest-visible timing and may break loads, audio, or game logic. Use 2x
first and disable the mod if anything stalls.
