# Co-op stage selection and victory music

Co-op stage selection shows X and Zero together. Double remains beside them
in X's campaign; Iris remains beside them in Zero's campaign. The original
story conditions still remove that companion when appropriate.

In local Unified, the campaign hero keeps the original main position. Each
local Split pane and each netplay peer instead puts its own hero in that
position, with the partner next to them. This also applies to Unified-camera
netplay: each peer has its own display. Player 1 still chooses the stage and
owns campaign progress. Both heroes play their original selection animation.

When a player delivers the defeating hit to a boss, the existing victory-music
request selects that hero's theme. This does not add another clear, reward,
warp or music request. Authored scene and ending music keep their own flow.

## Implementation

`mmx4_coop_frontend.inc` extends native board initializer `8002E698` with one
additional hero particle. The native companion-admission logic is unchanged.
The added hero runs native `800C9EAC` with its own animation table and assembly.
Its graphics and palette come from the original disc's `ST0D_0X/Z.ARC` and
`COL0D_0X/Z.ARC`, retained in immutable host texture banks. No original assets
are redistributed. A reserved board bucket puts both heroes above the map
and its frame, below the native fade, including final-fortress selection.

Independent views swap only draw positions inside the existing presentation
transaction. They do not change campaign identity, cursor state or animation
updates. Board actor references and the recorded finisher participate in
snapshots and simulation digests.

The combat hook records an accepted native hit that reduces the current boss
from positive HP to zero, using the actual attack owner. Instruction
`80034FE8` changes only the existing victory request's arguments: X `22/75`,
Zero `21/72`. Ordinary enemy kills and nonlethal contacts cannot claim it.
Fresh stage/board entry clears that record.

## Validation

The candidate builds against title master `fd445a98`, framework `9868d30c`
and UI `b964eabe`. The 42-test suite passes, including lethal-hit ownership
for both heroes in either seat, nonlethal contacts, unrelated actors and
fresh-stage reset. Native original-disc captures, snapshot receipts and
simulation evidence are retained privately in
`G:/Temp/Codex/psxrecomp/x4-stage-select-20261030`.

| Native check | Result |
| --- | --- |
| X and Zero campaigns, local Unified and local Split | Both heroes use their own artwork and original selection animations. |
| Early and late board story states | Original companion appears early and is absent late. Final-fortress map/frame and native fade retain correct layering. |
| Current-build board save/load | Both board actors and native animation state restore. |
| Two loopback peers, Unified-camera netplay | Each screen puts its own hero first; 55 matching core/mod CRC samples, zero mismatches. |
| P2 Zero defeats Web Spider in X's campaign, scaling enabled | One native victory request, `21/72` from `80034FF0`. |
| P2 X defeats Web Spider in Zero's campaign, scaling disabled | One native victory request, `22/75` from `80034FF0`. |

`tools/test_mmx4_coop_regressions.py --cases stage_select --card-seed ...`
adds a reusable native board scenario to the existing hidden QA runner.
It checks both actors, companion admission, Player 1's stage-choice ownership,
both animation tracks and actual entry into gameplay. Its screenshots require
visual inspection; actor counts alone do not establish correct artwork.
