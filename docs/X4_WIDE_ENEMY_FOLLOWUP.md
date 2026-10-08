X4 wide enemy follow-up
======================
Original intro placements include ordinary enemy type 0x31; the old supplemental
filter rejected every type >=0x20. All authored types in ordinary allocation
categories 0..2 now use their original factories; scripted categories stay native.
Only newly exposed strips are scanned. Successful placement visits remain latched
through the extended activation area, preventing kills from reallocating there.
The bitmap and previous view are allocated in snapshotted mod memory; original
placement reset 0x80028DB4 clears them on stage/death reset. Original successful
latch instruction 0x80029284 (0xA2620000) supplies the guarded visit observation.
Original actor lifetimes/drawing use the existing widened native bounds.
Shared rectangle exposure helper lives in psxrecomp, not a copied game utility.
Latest merged shared cycle-scale mechanism uses guest_cycle_scale=2 during main
state6 gameplay only; CPU work is half charged while VBlank/CD/SPU keep time.
1080p Adaptive and resident ARC loader remain enabled; interpolation stays hidden.
Owner wide-view enemy/kill/retry/audio/performance playtest remains required.
