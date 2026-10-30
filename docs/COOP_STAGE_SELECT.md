# Co-op stage selection and victory music

Co-op stage selection shows X, Zero, Iris and Double together. Both companions
leave at Player 1's campaign's original companion-departure point. The
original interstitial exclusions remain; Player 2 has no separate campaign.

In local Unified, the campaign hero keeps the original main position. Each
local Split pane and each netplay peer instead puts its own hero in that
left slot, with the partner next to them. This also applies to Unified-camera
netplay: each peer has its own display. Player 1 still chooses the stage and
owns campaign progress. Both heroes play their original selection animation.
Double takes the front companion position on X's screen; Iris takes it on
Zero's screen. The other companion stands nearby, slightly behind. Iris
plays her native selection animation in either campaign.

When a player delivers the defeating hit to a boss, the existing victory-music
request selects that hero's theme. This does not add another clear, reward,
warp or music request. Authored scene and ending music keep their own flow.

## Implementation

`mmx4_coop_frontend.inc` extends native board initializer `8002E698` with an
additional hero and, when the native companion is admitted, its counterpart.
All four use native `800C9EAC` with their own animation tables and assemblies.
Its graphics and palette come from the original disc's `ST0D_0X/Z.ARC` and
`COL0D_0X/Z.ARC`, retained in immutable host texture banks. No original assets
are redistributed. Native character depth keeps the portrait/map frames above
the cast. The private pass calls the complete native renderer `80023D68`,
including its backdrop, rather than the sprite-only `80023DB8` routine.

Board admission uses native minors1..7 and9 (board dialogue), not stage13:
the selected stage changes before selection animation finishes, and the later
boss introduction can reuse board particle addresses. Both actor type and
screen ownership are checked. On leaving the board, cached private pair
images are retired before the introduction/loading frame.

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
| Early and late board story states | Both companions appear only while the campaign's native companion is admitted; original disappearance rules own that boundary. |
| Current-build board save/load | Both board actors and native animation state restore. |
| Two loopback peers, Unified-camera netplay, corrected four-character implementation | Each screen puts its own hero and companion first; 56 matching core/mod CRC samples through simulation1837, no desync, zero render verification mismatches. This checks the board and selection, not a complete network campaign. |
| P2 Zero defeats Web Spider in X's campaign, scaling enabled | One native victory request, `21/72` from `80034FF0`. |
| P2 X defeats Web Spider in Zero's campaign, scaling disabled | One native victory request, `22/75` from `80034FF0`. |
| Owner review corrections | Restored backdrop and portrait depth, both companions, original selection animations, matching nonblack Storm Owl intro panes, and no board animation table in recorded gameplay spawn states. |

`tools/test_mmx4_coop_regressions.py --cases stage_select --card-seed ...`
adds a reusable native board scenario to the existing hidden QA runner.
It checks both actors, companion admission, Player 1's stage-choice ownership,
both animation tracks, a visible Storm Owl introduction and actual entry
into gameplay. Spawn records are retained in `spawn-frames.json`. Screenshots require
visual inspection; actor counts alone do not establish correct artwork.
