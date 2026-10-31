# Co-op stage selection and victory music

The separate **Co-op Stage Select Cast (Experimental)** mod shows X, Zero,
Iris and Double together on the stage menu and its conversations. It is off by default.
Netplay hosts use the corresponding experimental co-op profile option, also
off by default; ordinary offline mod preferences do not configure a match.
Both companions
leave at Player 1's campaign's original companion-departure point. The
original interstitial exclusions remain; Player 2 has no separate campaign.

On both screens, Player 1's campaign hero keeps the main position and Player 2
stands behind. Player 1's support character also stays in front of Player 2's
support on both screens. Player 1 chooses the stage and owns campaign progress.
Both heroes play their original selection animation. Iris
plays her native selection animation in either campaign.

All four characters remain visible during scripted stage-menu conversations
while the campaign still admits its support character. Only the original
speakers receive dialogue animations; the partner hero and support keep their
own idle animations. Both heroes animate when a stage is selected.

When a player delivers the defeating hit to a boss, the existing victory-music
request selects that hero's theme. This does not add another clear, reward,
warp or music request. Authored scene and ending music keep their own flow.

## Implementation

`mmx4_coop_frontend.inc` extends native initializer `8002E698` to add an
additional hero and, when the native companion is admitted, its counterpart.
All four use native `800C9EAC` with their own animation tables and assemblies.
Its graphics and palette come from the original disc's `ST0D_0X/Z.ARC` and
`COL0D_0X/Z.ARC`, retained in immutable host texture banks. No original assets
are redistributed. Native character depth keeps the portrait/map frames above
the cast. The private pass calls the complete native renderer `80023D68`,
including its backdrop, rather than the sprite-only `80023DB8` routine.

Board admission and custom drawing use native minors1..7 and9, not stage13:
the selected stage changes before selection animation finishes, and the later
boss introduction can reuse board particle addresses. Both actor type and
screen ownership are checked. Initializer `8002E698` clears references before
admitting the added actors; `8002E994` also checks admission. On leaving the board, cached private pair
images are retired before the introduction/loading frame.

Independent views use the same cast order inside the existing presentation
transaction. They do not change campaign identity, cursor state or animation
updates. Added actors use the retail animation and visibility helpers with
their own tables. They never project the campaign byte or shared dialogue
control byte. Only idle and the authored stage-confirm sequence1 are admitted;
talk commands such as sequence8 cannot index Iris's shorter table.
Private board drawing also rejects retired personal pause menus after
Quit/Continue: those menus may draw only during the actual gameplay pause.
The enabled setting, actor references and recorded finisher participate in
snapshots and simulation digests.

The combat hook records an accepted native hit that reduces the current boss
from positive HP to zero, using the actual attack owner. Instruction
`80034FE8` changes only the existing victory request's arguments: X `22/75`,
Zero `21/72`. Ordinary enemy kills and nonlethal contacts cannot claim it.
Fresh stage/board entry clears that record.

## Validation

The visibility follow-up builds against title master `4be40a0`, framework
`09ba00ed` and UI `3f1d6f1`. All 44 Windows title tests pass, including elective
mod defaults, required co-op activation and session fingerprint separation.
The suite also covers lethal-hit ownership
for both heroes in either seat, nonlethal contacts, unrelated actors and
fresh-stage reset. Native original-disc captures, snapshot receipts and
simulation evidence are retained privately in
`G:/Temp/Codex/psxrecomp/x4-cast-followup121`. Earlier release qualification
remains under `G:/Temp/Codex/psxrecomp/x4-stage-select-followup-20261030`.

Tester evidence: `Select screen bugs 1.0.19.zip` and
<https://youtu.be/jH2kzTL_b1o>. Loading the card's third record reproduces
the Double conversation. In1.0.19, the added Iris receives sequence8 and an
animation pointer into executable code (`800CA030`), outside her animation
table. Version1.0.21 overcorrected this by hiding the partner cast throughout
minor9. `Secret files.zip` confirms that the standalone cast feature was on;
the public1.0.21 AppImage reproduces two actors during dialogue and four only
on the interactive menu. The visibility follow-up keeps the added cast present
and idle instead. The tester clarified that both heroes
"talking" meant their mouth animations during text dialogue, not audio.

| Native check | Result |
| --- | --- |
| Released1.0.21, tester's native save and exact enabled mod state | Reproduced missing partner cast: only X/Double in dialogue despite the feature being active; four actors appear once the interactive menu opens. |
| Mod disabled, same save and local8:3 | Original two-character menu and dialogue; current-build save/load and gameplay entry pass. |
| Zero campaign, private early-story native save, local8:3 | Four actors throughout Iris's conversation; X/Double remain idle. Matching pane layouts, save/load, selection animation and gameplay entry pass. |
| X campaign, Unified camera, tester's native save | Four actors through Double's conversation and interactive menu; save/load and gameplay entry pass. |
| Earlier release: early and late board story states | Both companions appear only while the campaign's native companion is admitted; original disappearance rules own that boundary. |
| Current-build board save/load | Cast actors and native animation state restore. |
| Final candidate, two consecutive native Quit/Continue returns, local8:3 | Four actors after each return, preserved P1 campaign/clear flags and gray portraits, no stale pause UI, zero render-transaction verification mismatches. |
| Earlier1.0.19 loopback qualification |56 matching CRC samples did not cover the scripted Double conversation; that missing case is now reproduced above. The old view-local ordering is superseded. |
| Earlier release: two-peer loopback, Split-camera netplay, experimental cast enabled |55 identical core/mod CRC samples through simulation1825; no render verification mismatches. Both peers admit the added hero. Direct peers with unequal cast options reject the match before frame0. This does not qualify the new dialogue follow-up over netplay. |
| Earlier release: P2 Zero defeats Web Spider in X's campaign, scaling enabled | One native victory request, `21/72` from `80034FF0`. |
| Earlier release: P2 X defeats Web Spider in Zero's campaign, scaling disabled | One native victory request, `22/75` from `80034FF0`. |
| Earlier owner review corrections | Restored backdrop and portrait depth, both companions, original selection animations, matching nonblack Storm Owl intro panes, and no board animation table in recorded gameplay spawn states. |

`tools/test_mmx4_coop_regressions.py --cases stage_select --card-seed ...`
adds a reusable native board scenario to the existing hidden QA runner.
It checks both actors, companion admission, Player 1's stage-choice ownership,
both animation tracks, a visible Storm Owl introduction and actual entry
into gameplay. Spawn records are retained in `spawn-frames.json`. Screenshots require
visual inspection; actor counts alone do not establish correct artwork.
The follow-up's `visual-validation.png` collects the public reproduction,
both campaign layouts, both native returns, Unified and disabled-mod captures.
The final extra change only restricts retired pause-menu drawing; the affected
Quit/Continue sequence and the 44-test suite were rerun after that change.
