# X4 extended-view enemy placement

Original intro placements include ordinary enemy type 0x31; the old supplemental
filter rejected every type >=0x20. Authored types in ordinary allocation
categories 0..2 use their original factories; scripted categories stay native.
Only newly exposed strips are scanned. Successful placement visits remain latched
through the extended activation area, preventing kills from reallocating there.
The bitmap and previous view are allocated in snapshotted mod memory; original
placement reset 0x80028DB4 clears them on stage/death reset. Original successful
latch instruction 0x80029284 (0xA2620000) supplies the guarded visit observation.
Actor lifetimes and drawing use the widened native bounds. Shared rectangle
exposure lives in psxrecomp.

Category4/type4 handler800C081C installs the destructible doorway hitbox and HP.
Only that verified prop type joins the extended placement/visit scan; scripted
encounters remain on the native scanner.

The category3/type7 intro controller800B6C9C supplements scanner800B6EB4 over each
layer's adaptive rectangle. This separate twelve-record table retains native
latches, allocation failure handling and category6 beam actors. Beam visibility
800D4024 widens only its original X tests; Y, animation and geometry stay native.
Stationary aspect changes need no guest bookkeeping or snapshot layout change.
Both hooks are inert at4:3.
