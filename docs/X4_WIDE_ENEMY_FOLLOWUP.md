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
Owner accepted the wide-view enemy/performance follow-up.

Owner accepted enemy/performance follow-up. One remaining prop repair: original
category4/type4 handler800C081C installs the destructible doorway hitbox and HP.
Only that verified prop type joins the extended placement/visit scan; scripted
encounters remain on the native scanner. OpenBIOS is the current review default.

Owner subsequently accepted X4 gameplay, with searchlights as the final repair.
The category3/type7 intro controller800B6C9C now supplements its original
scanner800B6EB4 over each layer's full adaptive rectangle. This small separate
twelve-record table uses its native latches, allocation failure handling and
category6 beam actors. Beam visibility800D4024 widens only its original X tests;
Y, animation and geometry stay native. Stationary aspect changes are handled
without guest bookkeeping or a new save-state layout. Both hooks are inert at
4:3. Focused OpenBIOS intro validation proved two beams visible beyond the old
4:3 bounds and correct 4:3/wide switching. Owner authorizes agent validation
for this final repair and merging the complete accepted campaign to master.
