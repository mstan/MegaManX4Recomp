# Magma Dragoon first-slope rumble after 1.0.6

## Reproduction and native contract

In that exact route, P2 has no active horizontal shake in 17 observations at
X400..699 or 48 observations at X700..999. P1 does shake. Native write traces
identify caller `800A0CB8`, issuing duration8/amplitude4/period2. `800A0C6C`
checks the native camera's quake bit, then `800A0CB0` issues this command on
grounded rolling ticks. This recurring rumble is not a collision at the ball's
current position. Original `BC9F8/BCA88` gates the rightward rolling emitter
against its authored X+385..X+1216 window.

The existing local-impact filter discarded this command for a hero ahead of
the remaining balls. Earlier balls collide with the waiting hero and retire,
so the reported fast leader has no nearby ball for most of the descent.
The earlier protected walking check kept those balls alive; it qualified
nearby impact timers and missed this route.

## Change

For this exact part1/rightward rolling caller and native type33/phase4 body,
also publish the native rumble to living views inside the active type33,
subtype0x10 emitter's original window. Use each view's own world center and
preserve an already-running shake's timer and phase. Keep local impact
eligibility for views near a ball. Native actors, collision, allocation and
emission still update once. No new persistent state or hook entry is added.

Part2, falling/pillar impacts, leftward rolling, inactive or different emitters,
views outside the window, dead heroes, private redraws and Unified retain their
existing paths. Model checks cover these exclusions, independent timer/phase,
the other shake axis, origins and room bounds.

## Qualification

The corrected bounded candidate verifies all 7,082 unchanged input intervals
and player-state hashes through both stage sections. P2 now receives the native
rumble during the reported first-slope descent, with timer phases0..7 and the
native4px horizontal camera displacement. Composed captures retain both views.
At the recorded stationary X790.375/Y805, the original camera stays at X630;
the candidate alternates X630/634 with native phase4/132. The candidate also
alternates X806/810 while P2 stands at X966.9375/Y893.
The complete comparison is `report-replay-development/` versus
`report-replay-candidate-bounded/` in the private evidence root
`G:/Temp/Codex/psxrecomp/x4-slope-106`.

All39 Windows development CTests pass. Both campaigns pass both-leader native
slope descents with damage enabled: the waiting hero takes real boulder damage,
the native timers progress1..7, and both heroes remain healthy (minimum30/42HP
for waiting/leading heroes). Both campaigns also pass either-leader part2
pillar emission and16 actual terrain edits, nearby quake timer/X0/4 controls,
and the real P2-led Dragoon arena admission. Pillar setup uses position/damage
protection, qualifying the native event rather than ordinary traversal survival.
Results live in `after-slope-campaign0/` and `after-slope-campaign1/`.

No new public release, Linux gameplay or online session is claimed here. The
broader end-of-part1 audit in feedback194 remains separate.
