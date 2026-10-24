# October 10 co-op feedback

Current status, October22: **214 numbered items; 178 checked and 36 unchecked.**
The Marine Base211 and renewed opening/Mushroom199/203/185
prerequisites were qualified for1.0.2. [Evidence and limits](COOP_MARINE_MUSHROOM_RELEASE_GATE.md).
Five checked fixes are qualified for [Playtest32](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.32).
Capsule dialogue154 and scaling cooldowns152/153 are qualified for
[Playtest33](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.33).
Items126/155/156 are qualified for [Playtest34](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.34). Saber report157 is reopened after the38 follow-up. The37 manual comparison was bounded acceptance; the latest report remains unconfirmed. A newly traced saber-pose veto is corrected in the39 candidate. The earlier automated acceptance remains withdrawn. Distant intro flashes158
and the later Frost path159 are qualified for [Playtest35](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.35).
[Playtest34 follow-up evidence](COOP_FEEDBACK34_FOLLOWUP.md).
Unchecked reports include
older bounded investigations, the parked online-startup cases and an elective
story-scene enhancement. They are not 30 confirmed current defects.
[Release dispositions](COOP_PLAYTEST32_DISPOSITIONS.md)
record the checks and limits without relabeling unreproduced reports as fixes.
The full numbered table remains the source for evidence and validation limits.
Mushroom fixes are in `6e422ee`, hover jets in `3ac1021`, and mounted partner
arrivals in `3b471b9`; the Playtest32 integration also fixes overlapping transports.

| Status | Items | Current work |
|---|---|---|
| Fixed and qualified for Playtest34 | 126, 155, 156 | Charged X fire follows its own projected parent shot; hub returns validate the landing; Owl's native release clears both captured bodies. Native reproductions and post-fix control checks pass; [release scope and limits](COOP_PLAYTEST34_DISPOSITIONS.md). |
| Fixes qualified for Playtest35 | 158, 159 | Distant intro views retain normal colors while the nearby view keeps the red pulse. P2 can also arm and cross the later Frost ice path while P1 stays at spawn. Actual Split LAN and render-state checks pass. [Evidence](COOP_FEEDBACK34_FOLLOWUP.md). |
| Fixes qualified for Playtest36 | 160, 162 | Native P2 ice-collapse contact and retained independent views during native field freezes. Report163 is qualified native retirement behavior;161/164 remain unconfirmed. [Evidence and limits](COOP_FEEDBACK35_FROST.md). |
| Fixed and validated for Playtest32 | 145, 146, 147, 148, 149 | Mushroom hatch/lift admission, interrupted Cyber portal arrivals, grounded occupied Armor return anchors, and owner-bound X hover jets. |
| Fixed and validated for Playtest33 | 152, 153, 154 | Scaling now halves boss hit cooldowns in either method, including Zero's aerial/finisher in separate mode. X's armor capsules use Dr Light's conversation in Zero's campaign. Native before/after checks and regression suites pass; [qualification limits](COOP_PLAYTEST33_DISPOSITIONS.md) remain explicit. |
| Current recurrences; exact cause still unqualified | 2, 107, 109, 167, 169-171 | Later Intro/Owl/Web/Frost symptoms remain unconfirmed after bounded checks. [Review](COOP_FEEDBACK36_REVIEW.md). |
| Qualified for Playtest37 | 25, 157, 165, 166, 168, 172 | Native progression corrections, owner-accepted Zero combo timing and P2 course analysis. [Evidence and limits](COOP_PLAYTEST37_INVESTIGATION.md). |
| Bounded checks; failure not reproduced | 144, 150 | Slash HUD/scenery corruption and X's stage-colored weapon icons remain tracked with the actual capture limits. |
| Enhancement tracked separately | 151 | Both heroes together in additional suitable story scenes. |
| Older unchecked cases retained | 30, 32, 34, 36, 41, 43, 46, 49, 51, 53, 107, 112, 119, 121, 130, 133, 135, 136, 140 | See each original row for bounded checks, missing evidence and owner-requested deferrals. Discord/startup30/32 remain parked. |

Owner approved items1-18 after all ten unique clips were reviewed, then added items19-28 through further tester feedback. Their original qualifications remain recorded; item25 is reopened for newer Playtest13 feedback. Split Mushroom elevator item29 is qualified in both rosters; the owner confirmed its later overhead-panel crush is authentic. Numbers remain stable. A checkmark means the fix is validated; reported and observed symptoms remain distinguished below. The diagnostic review is beads-eio.9.41; implementation lives in the linked central issues.

| # | Done | Symptom | Evidence and owning issue |
|---|---|---|---|
| 1 | [x] | Netplay host remains on a white screen after Play | Reproduced through real hidden LAN lobby Create/Join/Play. Fixed empty settled BIOS in START caps; both peers reached native co-op with 95 matching core/mod digests, tick 3098. Latest UI/network plus explicit ICE and host ICE handoffs integrated; focused ICE and launcher tests pass. WAN play remains a human playtest qualification. `beads-eio.9.42` |
| 2 | [ ] | P2-first intro-stage boss entry still softlocks | Reopened for Playtest36: P2 can reach the boss after missing platform triggers, followed by corruption, disappearing P1 and left-walking lock. Earlier bounded both-campaign door tests remain historical evidence; this report is not cleared by them. Upstream platform admission168 may be related. [Review](COOP_FEEDBACK36_REVIEW.md). `beads-eio.9.10`, `beads-eio.9.188` |
| 3 | [x] | P2-first Volcano boss approach kills Zero during the drop | Reproduced from native Volcano checkpoint3: viewport-based co-op death line kills Zero at Y610 while the camera trails his fall. Fixed death line to the authored room maximum; qa-volcano-entry-fixed-16 lands Zero at Y650 with HP32 while X stays above. Boss activation/handoff remains item5. `beads-eio.9.10` |
| 4 | [x] | Death during boss approach produces an excessive particle burst and reported slowdown | Reproduced fatal C4 entrant softlock. Transfer the abandoned native doorway to the survivor, initialize its auto-walk, and prevent doorway crush only during recovered auto-walk. Preserve the actual global scene lock during partner death. qa-arrival-death-train-41 passes both death orders: exactly one native death sequence each, survivor HP32, no spent shared life, both gates released. No separate frame-rate improvement claim. `beads-eio.9.43` |
| 5 | [x] | Boss activation and partner arrival depend on which seat advances | Reproduced in Volcano: the partner left above the committed shared room kept the leader from progressing. Fixed native partner handoff when a new shared camera restriction excludes the other living seat. qa-volcano-handoff-final-18 passes both leading seats, boss65 HP48, both players at floor Y650/651, normal action2, clear scene/door gates and HP32. Split ordinary camera areas stay independent. `beads-eio.9.10` |
| 6 | [x] | Moving the leader cancels or restarts the partner warp-in | Earlier Cyber walking fix retained. Reopened and fixed after Playtest20: native Double fight reproduces P1 jump and Nova canceling P2's committed return. Personal +67 and Nova's PLAY+1C were treated as shared scenes. Local A/B and hidden Split netplay now pass uninterrupted walking/dashing/jumping/Nova arrivals; peer CRCs match through6753. Both-seat regression checks preserve real scene gates. Included in Playtest21. `beads-eio.9.10` |
| 7 | [x] | Cyber boss-room partner arrival passes below the platform and P2 dies or disappears | Healthy outer/inner Cyber entries and both moving leaders pass in both campaigns. Commit the arrival anchor to the new grounded room, so a leader dying during arrival cannot return the passenger to the old doorway. qa-arrival-death-train-41 verifies the passenger completes at X729/Y2506 with HP32 after leader death; both ordinary and recovered entries retain native terrain after the handoff. `beads-eio.9.10` |
| 8 | [x] | Boss introduction leaves one player missing when the other stays behind | Slash Beast's entrance checks X9472 while its actor waits at X9312; nearest-player AI selected the lagging partner. Choose the leading living seat only at that entrance, then retain native script ownership and nearest fight AI. QA49/50 pass both leading seats, with partner left at X9360: native C0 intro, both healthy and visible at X9473/Y394-395 when the fight starts. `beads-eio.9.10` |
| 9 | [x] | Boss dialogue box uses a rainbow palette | Canonical shared textbox setup in50a6df6 preserves the campaign's panel resources. QA56 qualifies the P1 native panel; QA78 P2-owned Slash entrance captures the actual blue gradient panel with white lettering and matching native RAM/VRAM palette. Inspected owner1-panel3 contains no rainbow bands. Native phase6 no-panel scenes remain distinct from visible dialogue. `beads-eio.9.26` |
| 10 | [x] | Solid purple graphics artifacts appear after leaving pause | Immutable body textures, canonical shared textbox setup and native pause world drawing qualified through QA51/52/54/56. Both rosters, each leading seat and both pause owners run repeated P2 attacks with12 captures over5 seconds after each resume; inspected captures stay healthy. The reported rectangles are not reproduced on this branch. `beads-eio.9.26` |
| 11 | [x] | After the train death sequence, a red warp beam persists with no visible active player | Native qa-slash-pause-airdash-48 manual train_deaths passes sequential P2/P1 fatalities at checkpoints0 and2, including later platform X6464/Y346-347. Each team retry spends one shared life, finishes both original arrivals, and restores both visible, grounded, normal bodies with HP32. Native train withdrawal/rejoin also passes QA41. The clip does not establish a permanent softlock. `beads-eio.9.10` |
| 12 | [x] | P2 dash afterimage colors belong to the other character | Reproduced: P2 inherits P1 trail CLUT13-15. Initialize private colors from the original counterpart COL stage archive. Native qa-trail-colors-zero-26 / qa-trail-colors-x-27 confirm red Zero and blue X, with three visible trails before and after withdrawal/rejoin; screenshots inspected. `beads-eio.9.27` |
| 13 | [x] | P2 dash afterimages disappear after Jet Stingray victory | Warp cleanup erased complete native trail structures. Preserve initialized resources while cancelling visible images. QA49 now follows a real P2 Jet boss kill, native reward and Save UI directly into the train: three correctly red Zero trails appear both before and after withdrawal/rejoin. `beads-eio.9.27` |
| 14 | [x] | P2 dashing removes P1 body, especially after a P2-owned boss door/event | Immutable body textures (87b87ec) preserve both native characters. QA83/84 drive P2's original Intro boss event in both rosters, observe actual P2 Dash during P1's incoming beam, then Dash with either seat after arrival. Separate bodies on the observed arena floor for clear captures: P1's complete blue X/red Zero body remains intact, with no corrupt rectangles. Native Cyber P2-door Dash and open-air Dash qualifications also pass. `beads-eio.9.27`, `beads-eio.9.10` |
| 15 | [x] | P2 Ride Chaser dash animation is missing in Jet Stingray | QA53/54 execute original bike Dash in both character arrangements: ground phase5, changing rider frames and bike frame3. Inspected captures show both riders' dash poses. Missing animation is not reproduced on this branch; layering remains item16. `beads-eio.9.44` |
| 16 | [x] | P2 draws over P1 Ride Chaser bike and exhaust | Reopened after Playtest20: the old +4 offset spilled P2 bike/exhaust into another layer. Reserve four native buckets per rider in the same layer, including X armor and wind/exhaust. Local Unified A/B and composed Split netplay frames with both character arrangements now keep the complete P1 bundle in front; peer CRCs and priority invariants pass. Included in Playtest21; see evidence below. `beads-eio.9.45` |
| 17 | [x] | Defeated portraits revert to color after returning from a stage | Qualified the actual return paths missing from the earlier tests. QA49 preserves clear16 through native Quit/Yes/current-data Continue and Escape Device/Save UI. QA50 adds a real Web Spider clear (16->17), matching Continue/card records, then repeats both return paths: both portraits remain grey in inspected screenshots and palette reads. Restrict gameplay scene ownership to gameplay/transition modes, preserving P1's frontend resources and input. Recoloring is not reproduced on this branch; no persistent data loss is demonstrated. `beads-eio.9.37` |
| 18 | [x] | P2 character and HUD pop out/in during pause transitions | Include the partner body/effects/HUD whenever the original world is rendered during native pause fades. Native QA51/52/54 perform four pause/resume sequences per run; both pause-world draw counters advance on every fade (288 each), with healthy inspected resumed frames. Quit still excludes gameplay drawing. `beads-eio.9.26` |
| 19 | [x] | P2 ordinary dash exhaust appears beside P1 | Deferred effects0/1 read canonical PLAYER/Double and assembly. Project their allocating owner for initialization, position, facing and cleanup; dust3 stays shared. QA74/75 execute28 Dash captures per roster: exhaust follows X/P2 and Zero/P2 with native matching colors, while the idle P1 body remains intact. Both-owner/Double/reuse tests pass. `beads-eio.9.46` |
| 20 | [x] | Both players should warp in together, slightly next to each other at every suitable spawn point | Natural stage/section/retry arrivals preserve native collision delay, each character's animation and P1's gates, with a24px horizontal gap. QA78/82 neutral arrivals stand safely in Jungle and both Cyber sections; QA76 observes actual Dash during the partner's beam. Genuine direct checkpoint/portal returns now initialize each body's native floor geometry and start both beams together after safe bounds/terrain checks. QA83/84 both rosters drive real Cyber hub1â†’checkpoint2 and hub5â†’section1 transfers: both return healthy, grounded, with clear controls and24px spacing. Dead/withdrawn carries, manual rejoins, authored door walk and stage12 story spawns keep their appropriate lifecycle. All17 title checks plus8 offline netplay-adapter checks pass. `beads-eio.9.47` |
| 21 | [x] | P2 health is not refilled entering stage part2 | Fixed unconditional old-HP restoration: living forward-section arrivals follow native spawn0/-2 refill; -1, same-section checkpoints, deaths and withdrawals keep their state. QA55 Web Spider section initialization restores P2 HP8->32 with two lives. Both-roster ownership/policy/death tests and all16 title CTests pass. Native route through the section-exit event remains an additional transition check. `beads-eio.9.48` |
| 22 | [x] | Zero should wall jump from X's Lightning Web shots | Allow the other Zero body to use the original contact helpers with an isolated body mirror and per-owner/shot history; private shot pools and original web updates stay intact. Native QA76/77 fire real X webs in each roster, then Zero contacts them and enters original wall-jump action10 with upward/away motion. Native jump/fall/slide controls, fields, slot reuse and all17 title tests pass. Boss damaging webs retain their original behavior. `beads-eio.9.49` |
| 23 | [x] | P1 dashing causes severe slowdown and graphics corruption after P2 opens a boss door | QA83/84 qualify P2-owned Intro entry in both rosters and native Dash by either seat after the return, with actual P2 Dash during the partner beam. Inspected separated-body images stay healthy. Bounded idle/Dash comparisons show no severe frame collapse or stall; QA83 measured75â†’70 for P1 and82â†’79 for P2 debug frames/s. These are diagnostic windows rather than production/display FPS, and broader Split performance remains experimental. `beads-eio.9.50`, `beads-eio.9.27` |
| 24 | [x] | Cyber Peacock part2 flip switches ignore P2 and hide P2 when used by P1 | Native General20 checks only PLAYER and reflects only canonical bodies. Allow either living seat to activate the switch; mirror P2 body/active shots and both saved Split cameras once with the native world, preserving fractional coordinates and arrival anchor. QA60 P2/Zero and QA61 either activator with Zero/P1-X/P2 finish visible, healthy and grounded after a real flip. Shared-world/eligibility and double-reflection tests pass. `beads-eio.9.51` |
| 25 | [x] | P2 cannot trigger Storm Owl part1-to-part2 exit | Reproduced36 leading-P2 exclusion and death/fade race: P2 finishes its outgoing warp while P1 remains HP0/state1, so native completion never advances. Either living hero now satisfies the original6352 predicate; the guarded native completion branch accepts a healthy completed P2 during a real forward fade. Local Unified and real Split LAN, both rosters, reach part2 with both heroes visible/healthy/controllable and unchanged lives; matching peers through6400/6113. [Evidence](COOP_PLAYTEST37_INVESTIGATION.md). Owl166 is separately qualified;167 remains open. `beads-eio.9.52`, `beads-eio.9.188` |
| 26 | [x] | P2 weakness weapons do not trigger boss weakness reaction or damage | Native QA74/75: X/P2 Soul Body and Zero/P2 Ryuuenjin each reduce Cyber HP48->44 and enter the original weakness phase5. The native accepted-hit wrapper retains attack identity and original damage. Also fixed a reproduced X/P2 Ground Hunter selection freeze: deferred graphics loader27 must use its allocating X resources instead of Zero/P1. Protocol and ownership tests pass. `beads-eio.9.53` |
| 27 | [x] | Zero fire and ice techniques corrupt graphics | Reproduced malformed fragments: deferred particle47/48 initialization bound X/P1's assembly for Zero/P2. Initialize them with the allocating character and origin, preserving ordinary shared update order. QA68 replaces brown/yellow rectangles with blue ice shards; flame/ice images in both rosters QA68/69 match healthy native Zero/P1 rendering. Both original techniques execute (actions57/54). Allocation/reuse/dead-or-moved-owner tests and all16 title CTests pass. `beads-eio.9.54` |
| 28 | [x] | Slash Beast boss background remains stuck | Native QA87 identifies the actual four-slot scenery controller at80169CB8 and its post-camera8/4px writes. Propagate those original writes once to both saved Split views. QA88 real two-peer netplay passes each leading seat: both panes advance equally (496/248 and464/232px) while foreground X stays fixed; both peer images show moving scenery and core/mod CRCs match through tick8672. Exact native dispatch/allocator, callback order and boundary/parallax models plus all18 title checks pass. `beads-eio.9.55` |
| 29 | [x] | Split Mushroom elevator immediately kills both characters, including after retry | Fixed future destination720 being used as the death plane while the camera remains2047 and ridersY2200. QA35 passes the limit change, one-life team retry and continued ascent in both rosters: both retainHP32 through camera1746/1748. Owner watched QA34 and confirms later ceiling crush is authentic: players must destroy the overhead panels. Preserve that behavior. Fix11da504; validation stops before those obstacles. `beads-eio.9.57` |
| 30 | [ ] | Windows online Discord sign-in says the lobby server cannot be reached | **Parked at the owner's October11 scope check; collect future failures with startup logs instead of repeating sign-in investigations.** The first-HTTP socket fix is merged in SDK e5305930 and shipped in Playtest6. Fresh-process/native-request checks pass; full Discord authorization remains unconfirmed. Shared `beads-3qv.1.13`; logging item48. |

| 31 | [x] | P2 and HUD disappear after X cycles special weapons and returns to the buster | **Reproduced weapon-reuse failure fixed and qualified for Playtest9.** QA111/112 reproduce removal at Jungle's opening waterfall with both HP32 and failure8; native R1-only QA115 excludes the selection fixture as its cause. The reused buster retains Gravity Well's compressed pointer while native flags21 select static artwork; rendering now follows original flags20/40. QA116 passes two P1 cycles. Strengthened QA117/X-P2 and QA118/X-P1 each pass18 attacks, including visible full charged busters, healthy bodies, enrollment and failure0 throughout. QA118 retains the stale pointer yet its projectile and both HUDs stay visible; all exit normally0. Fix1903e75 is on master. This check qualifies that reproduced case; earlier boss-door enemy-fire, Mushroom midboss and intro disappearance reports are not independently qualified. `beads-eio.9.61` |
| 32 | [ ] | Online/Discord host enters white screen and guest remains in the lobby | **Parked at the owner's October11 scope check.** Earlier START/ICE fixes are included in Playtest7; private WS/ICE QA56 boots both peers with92 matching core/mod samples through3020. This does not qualify the reported WAN failure. Future reports should include both portable startup logs and their embedded build identities; input recordings start after title input. Logging is item48. `beads-eio.9.42` |
| 33 | [x] | F7 Square-to-save does nothing; save/load/rewind mappings and behavior need qualification | Offline co-op now captures both native contexts, ownership and private resources. QA39 X/P1 and QA40 Zero/P1 pass mapped saves, repeated loads, restoring P2 after death, P2 pause ownership, rewind accept/cancel and fresh-process loading followed by attacks. QA44 repeats the final safe-context implementation with X/P1. Diagnostic QA41/42 require the exact loaded baseline and verify152/48 hashes after cold load/rewind. All19 Windows title checks and focused Linux serializer, arena, RAM-map and diagnostic tests pass. Network state jumps remain explicitly unavailable. Published in Playtest7. `beads-eio.9.62`, shared `beads-eio.3.332`, `beads-eio.3.333` |
| 34 | [ ] | Split Mushroom section2 teleporter glitches briefly near second rotating staircase | Exact staircase-arrival footage is missing among available0740â€“0757 clips. IMG0757 lift retry at5.77â€“6.47s has separate blue/red beams, overlapping circular arrival effects, then both normal bodies; no clear palette corruption or stuck beam. Keep the exact report unqualified. Related room-initialization fix171d2ea does not prove it resolved this visual report. `beads-eio.9.63` |
| 35 | [x] | Split Mushroom latch closes while the on-screen partner remains below it | Reproduced native environment kind13 closing on P1 while P2 remains visible below. Defer original800C36E8 until P2's collision bottom clears the platform top when both living bodies share the view. QA9 P1-first and QA10 P2-first retain state1/action0 with one below, then complete state2 after both clear. QA10 offscreen closes normally. Real two-peer QA14 passes waiting and closure with identical core/mod CRCs through tick5066. Netplay uses agreed aspect and canonical camera bounds so peer views cannot change the outcome. All18 title checks pass; native hook included in development and shipped codegen. Commits ca569ca/c79366b on fix/mmx4-web-door-and-netplay-start, published in Playtest7. `beads-eio.9.64` |
| 36 | [ ] | X, Zero and items float above the rotating staircase | IMG0755 shows a real visible offset above the tread/railing. QA35 walks the second spiral in both rosters atX400/600/800/1000/1200, comparing4:3 and16:9: feet align, native slope equations hold, and each configured ring/map check has1113 tiles and0 differences. Disabled-renderer native4:3 images also align. QA36 at1080p adds161px player spacing and both pause owners, with unchanged correct heights. Current offset and matching pickup remain unreproduced; keep open. `beads-eio.9.65` |
| 37 | [x] | Automatic Ultimate Armor X and Black Zero mods, usable together | Separate default-off Mods entries and host-agreed co-op options enable each original character code. X still receives Ultimate Armor from a capsule. QA46/47 pass both rosters with both enabled, including states, pause, death restore and rewind; baseline QA49 verifies158 hashes. QA51 solo bodies/attacks and Windows/Linux original EXE capsule/palette/Nova contracts pass. QA54 publishes six distinct room offers, adopts either host code choice despite opposite offline settings and rejects mismatches. Real two-peer LAN QA53 matches100 core/mod digests through3250. All21 Windows title tests pass. Published in Playtest7. `beads-eio.9.66` |
| 38 | [x] | Tester cannot find diagnostic recordings; qualify several recordings on the next build | Diagnostics now always use executable/saves/diagnostics, or the AppImage file's folder, independent of memory-card settings/cwd. Location pointer beside application names current session or failure. Bundles include starting cards, mods, settings and session mode/build metadata. Native nextQA30 X/P1 produces2896 input records and offline replay verifies2895 hashes; QA31 Zero/P1 produces2551 and replay verifies2550. Both hidden launcher Play and no-launcher paths pass with cards stored separately. Actual two-peer LAN QA32 produces2521 input records per seat, metadata marks network traces, and95 core/mod CRCs match through3166. Full netplay playback remains unsupported and the helper rejects it explicitly. All18 Windows tests and Linux recorder/replay/path/error tests pass. Published in Playtest7. `beads-eio.9.67` |
| 39 | [x] | Lobby rematches exhaust co-op private allocations and retain old texture/packet registrations | Source review found allocation cursors surviving session teardown. Reset owned texture banks, packet registrations and enhancement arenas before each activation. Focused tests cover20 packet/texture resets, five large stable CPU/GPU allocations, stale-journal retirement, active-pass refusal and unchanged native RAM. Final native co-op first boot/state suite and two-peer LAN controls pass; repeated full lobby UI rematches remain additional coverage. `beads-eio.3.334` |
| 40 | [x] | Additional mod options truncate the lobby START capability plan | Native room query exposed a fixed640-byte nested START buffer. Use declared package/field bounds throughout nested codecs, queue and incoming frames; reject incomplete settings before enqueueing or publishing local caps. Actual X4 metadata and all12 maximum packages round-trip, small buffers/invalid plans refuse cleanly. Focused tests pass Windows with/without ICE and Linux with ICE. QA54 real room offers and recorded launcher adoption pass; QA53 real LAN gameplay agrees. Shared fix5a2332b6a, published in Playtest7. `beads-eio.3.335` |
| 41 | [ ] | GitHub25: intro-act2 WARNING loops after a player dies; Eregion never appears | Reported on Playtest6 local co-op. Current Playtest7-source hidden native tests pass checkpoint entry with either corpse in both campaigns, plus first approach, near-entry death and death during the actual warning. Inspected first-approach captures with X/P1 dead show WARNING clearing and Eregion/HP bar appearing; General24 progresses0/1/3/4 and retires. Reusable warning-death test also proves survivor movement after the fight starts. No historical failure or specific6-to7 fix established: keep open for a current-release retry and complete diagnostic session. `beads-enkq`, [GitHub25](https://github.com/mstan/MegaManX4Recomp/issues/25) |

| 42 | [x] | P2 independent display switches to P1's camera and the unified HUD under load | IMG0782 shows the switch and return in Playtest7. Framework catch-up policy deliberately refused the own-view redraw at4 frames behind for at least60 ticks. Remove that automatic fallback while preserving sandbox/session/backend safety. Windows/Linux sandbox regression fails before and passes after. All21 X4 checks pass. Hidden QA59 keeps separate X/P1 and Zero/P2 cameras/HUDs with25ms receive delay andÂ±10ms jitter,84 matching intro CRCs plus3 later samples through3068, over120 new local redraws per seat and no additional gameplay refusals or state leaks. Framework b292bfdc6; [PR634](https://github.com/RetroPortingToolKit/psxrecomp/pull/634), assigned TechnicallyComputers. This is the identified automatic fallback fix, included in Playtest8; performance is tracked separately in item43. `beads-g4xe`, shared `beads-685x` |
| 43 | [ ] | Residual slowdown with independent screens | **X5 comparison/optimization implemented and validated.** Ported c021463's aligned word copies to player/camera contexts, preserving host-store journals, native updates and separate views. One camera uses63 rather than252 accesses each way. Healthy distant windows measured53.5-53.6Hz before versus59.5-60Hz after; the candidate's later enemy death invalidates that full run. Stable synchronized-invulnerability controls92/93 both finish near60Hz with matching core/mod state through4693/4678. Separate verification94 passes restoration/rejoining through3797 with no device/VRAM/state leaks; diagnostic hashes cost15-18ms per redraw. All22 Windows and Linux memory/camera checks pass. Keep wider residual performance open for current playtest feedback; the optimization is not a universal speedup claim. `beads-802e` |
| 44 | [x] | Mushroom P2-led transition walks off the floor off camera and dies | Reproduced in QA65: P2 walks past the horizontal exit, falls atX3619/Y267 and dies while checkpoint1 remains active. Native Mushroom camera/controller reads only P1. Supply the progressing seat position for its one shared update and restore P1. Regenerated development/release hook. QA67 and opposite-roster QA73 reach checkpoint2 with both HP32 and no life spent by the transition. Included in Playtest8. `beads-kmsf` |
| 45 | [x] | After both players die and retry, P1 loses collision with the Mushroom teleporter platform | Reproduced QA61: P1 lands16px below the portal while P2 stands on it. The P2-owned portal dispatch bypassed dual-player solid collision. Restore canonical contact histories before its one shared solid pass. QA64 and permanent helper QA73 pass landing, jump/landing and native team retry in both rosters, with one life spent per retry. All21 Windows checks and Linux collision/lifecycle/controller checks pass. Included in Playtest8. `beads-kmsf` |
| 46 | [ ] | Mushroom elevator still fails for P2 in the newer playtest | Earlier arrival/retry/destination checks pass both rosters. QA88/95 now complete ascent, native upper wall climb and checkpoint3 exit with both HP32/lives2. Explicit private cheat advances only the authored ceiling panels' native destruction states; player, lift and collision rules stay native. This qualifies the later exit after those obstacles, not natural panel combat. The supplied video ends near boarding; no P2-specific defect is reproduced. At the owner's October11 scope check, further speculative elevator tests stop pending specific current failure evidence. Second staircase remains item36. `beads-kmsf`, previous `beads-eio.9.57` |
| 47 | [x] | Green Mushroom portal initially returns P2 below its entrance platform | Stronger native test reproduces P2 atY186 rather thanZero's platform Y170. Incoming warp excluded P2 from world-solid collision along with combat. Allow native floor collision for a healthy initialized incoming body; retain combat exclusion. Wait for completed P1 landing on negative portal returns and align character box bottoms when sharing a surface. QA84/85 pass actual P2 portal entry/return, jump/landing, team retry and subsequent P2 exit in both rosters. Native cases require the original disc and remain repeatable with the helper's `--portal-return` case. Included in Playtest8. `beads-kmsf` |
| 48 | [x] | Startup/sign-in failures need logs before gameplay recording starts | Portable startup capture shipped in Playtest8, but October12 evidence exposed an ordinary Windows GUI launch regression missed by the earlier closed-descriptor/redirected-stderr fixtures: the unattached stderr FILE retained descriptor -2 and produced empty logs. **Corrected and production-qualified for Playtest9; published Playtest8 remains unchanged.** Reopen the actual FILE, attach raw/native stderr, check header/location writes and report capture failure. The actual public8 executable produces0-byte detached/unredirected logs; the final production candidate records655/658 unredirected bytes and659 redirected bytes including the real pre-boot rejection, preserves redirected output, and earlier live qualification captures a later launcher failure before normal exit. Nine focused Windows launch/error cases, all31 Windows and25 Linux title checks plus four focused Linux logging cases pass. `beads-eio.9.71`; original `beads-eio.9.68`, shared `beads-eio.3.336` |
| 49 | [ ] | X's charge/release shows no travelling shot on the Mushroom staircase | **Bounded native attempt complete; exact report unreproduced and deferred.** IMG_0786.mov shows a charge effect ending around00:07.02 and a right-facing buster pose without a clear travelling shot; both bodies/HUD groups remain. QA113 visibly produces ordinary and three charged X/P1 shots on green and purple stairs. QA114 produces ordinary and three charged X/P2 shots on purple second-section stairs; its development-induced first-section hurt lock is excluded. No damage claim or exact-recording replay is possible from the empty ZIPs. Keep separate from floating stairs/elevator reports and leave unchecked. `beads-nbnc` |
| 50 | [x] | Special weapons still have glitched graphics in Playtest 9 | **Published in Playtest 10.** `Boss weapons glitched.zip` reproduces invisible/malformed special-weapon sprites. Compressed private textures were sampled using the native VRAM allocation's row offset. Emitting those sprites with a temporary local tile origin fixes their appearance, preserving all 5,755 original input/state checks. The P2 native weapon-cycle regression passes18 attacks; all31 Windows title checks pass. Remaining charge-glow, Aiming Laser and charged wind/gravity shapes also occur with co-op disabled; do not classify them as this co-op regression. Item31's prior fix covered the reused buster failure. `beads-eio.9.72` |
| 51 | [ ] | Zero stops moving near the top of Mushroom's second rotating staircase | **Gameplay lock not reproduced; input delivery remains open.** `Frozen Zero.zip` replays all 14,704 input/state checks on the published executable. The [video](https://youtu.be/y5omzTrBHLE) and trace agree: the final 1,135 boundaries contain 1,129 neutral P2 inputs and six Start inputs, with no movement buttons reaching the recorded controller state. In a separate development replay retaining every original hash check, Zero ends alive with31HP and immediately walks/jumps when P2 inputs are injected. This does not establish why the tester's controller stopped delivering movement. Separate from item36's reported stair-height issue. `beads-eio.9.73` |
| 52 | [x] | Supplemental Playtest8 intro/capsule/waterfall disappearance reports | **All three reproduced, fixed and published in Playtest 10.** Intro exhausted the 4,096-entry sprite cache; capsule effects used a rejected shared palette. Safe texture-ID reuse and shared-palette support preserve P2 through both failures, with follow-up movement and save/load checks passing (`c5ba8de`). The waterfall exit commanded surviving P2 but checked dead P1's position. Binding both phases to the same player lets P2 enter area2 at32HP and move/jump, while P1 stays dead. All30 Windows title checks pass. `beads-eio.9.74` |
| 53 | [ ] | Playtest 10: entering the intro door as P2 reportedly crashes | Unreproduced in bounded checks. `Metadata 2.zip` has three local recordings and two nonempty startup logs with no explicit fatal error. Its Zero-campaign segment replays18,693 original checks with both players alive. Current native P2-led X-campaign entry reaches the real dragon fight with both visible/healthy; X/P2 survivor entry in Zero's campaign also passes. This does not establish the tester's exact trigger or cover a separate process crash. Keep open; item54 is the reproduced post-victory lock. `beads-eio.9.76` |
| 54 | [x] | Playtest 10: X alone defeating the intro dragon in Zero's campaign reportedly crashes | Reproduced a post-victory softlock on unchanged Playtest10: native X/P2 kills HP1 Eregion while Zero/P1 remains dead, then action20/gate1 persists for over4,900 frames. Rescue actors44/46 used the corpse position, setting an unreachable camera target. Bind their init/update to the committed scene owner. The fixed native sequence reaches MISSION COMPLETED with Zero's campaign unchanged and P1 still dead; both-campaign ownership regressions and all32 title checks pass. Separate from the earlier WARNING-only report. Published in [Playtest 11](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.11). `beads-eio.9.76`; related deferred `beads-enkq` |
| 55 | [x] | Playtest 10: X's armor artwork does not update after a capsule in Zero's campaign | The private body renderer omitted native800257BC's acquired-armor pass. Prior Zero/P1-X/P2 capsule evidence retained boot flag8 across save/load but showed unarmored artwork. Include the native armor pass in private texture conversion. Seven armor configurations pass for both X/P1 and X/P2, with inspected armor captures; all32 title checks pass. One P1-owned campaign remains canonical. Published in [Playtest 11](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.11). `beads-eio.9.77` |
| 56 | [x] | Zero/P2 loops against Lightning Web and slides when touching its top | **Both reproduced, fixed and published in [Playtest 12](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.12).** The supplied Playtest10 [video](https://youtu.be/fSi_JH0r6HM) and recording agree; all6,092 retained replay checks pass. The contact gate rejected native WallGrip9, repeatedly restarting Fall7; preserve grip contact and Zero's double-jump state. Partner top/bottom contact also never reached the web's native expiration check, leaving Zero suspended; include it in the original once-only update. Unchanged Playtest11 fails both native regressions; fixed left/right slide-then-jump and X/Zero top-collapse cases pass in both rosters, eight cases total with HP32. All32 Windows title checks pass. `beads-eio.9.79` |
| 57 | [x] | With P1 dead, Mushroom latch remains open after P2 passes | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Native closing predicate now uses the living P2 when P1 is dead. Recorded-checkpoint A/B leaves the latch open on Playtest12 and closes it on the fix; P2 lands safely and P1 remains dead. Both-campaign ownership tests pass. `beads-eio.9.81` |
| 58 | [x] | Camera pans toward dead P1 during P2's subsequent death | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Damage still precedes death in the original recording. The unwanted pan afterward is a separate defect: with neither player alive, hold the current camera through native death/termination instead of targeting P1's corpse. Identical-checkpoint native death A/B: old camera Y2091..2251; corrected Y1963 throughout the death sequence; both reach native respawn. `beads-eio.9.82` |
| 59 | [x] | Horizontal lines in special-weapon sprites | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Private origin zero exposed native page-edge clipping, shortening flipped tiles from 16 to 15 pixels. Private tiles now retain all 16 rows/columns and reflect their pixels. Frost Tower captures are continuous and all 349 excerpt gameplay hashes match; 1,024 tile-origin/reflection cases pass. `beads-eio.9.83` |
| 60 | [x] | Jet Stingray intro: enroll P2 during the native bike entrance | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** P2 joins during the native bike entrance, starting 32 pixels behind P1. Both rosters pass the native entrance/control check with one shared READY actor, and preserve a dead partner across the section transfer. The remaining standing rider pose is fixed separately in item75. `beads-eio.9.84` |
| 61 | [x] | Optional boss doors require both living, present players | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Default remains Either player. Native checks verify either seat waits alone at the door, both then enter, and either sole survivor can enter. Unit checks cover withdrawn partners. Eight native room offers include the host's choice in the agreed settings identity. `beads-eio.9.85` |
| 62 | [x] | Optional sequential Double and Iris fights | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Default off. Both native encounter orders complete and X's default-off control runs only Double. The second encounter reloads at the inner boss-door checkpoint, preserving lives and P1's campaign. Includes snapshot/digest state and encounter-specific dialogue resources. All33 checks pass. `beads-eio.9.86` |
| 63 | [x] | Optional double boss HP and shorter shared hit cooldowns | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Default off. Uses the owner-approved shorter shared cooldown fallback. Fractional damage doubles effective HP while retaining native health ranges, phase tables and separate Sigma pools. Native Jet fights pass in both rosters, Web Spider passes, and default-off Jet retains its original damage/cooldowns. Both players deal damage and native deaths complete. Includes Web's cut-support damage and train miniboss sections; targeted unit coverage plus all34 Windows checks pass. Native save/load, pause restore, rewind and fresh-process load pass with both new boss options enabled. `beads-eio.9.87` |
| 64 | [x] | LAN independent-camera Mushroom stairs stop animating for P2 | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Both supplied peer recordings match, including233 full component digests through sim8704. The original controller drives one stair animation from one camera. Each view now keeps its own native animation state; the shared checkpoint controller still runs once. Native two-peer checks pass both seats on both green/purple staircases, with peer state matching through sim9061. Separate from item51's missing controller input and item36's reported floating feet. `beads-eio.9.88` |
| 65 | [x] | Independent netplay shows unified HUD during arrival; Nova Strike disturbs cameras | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Supplied `Nova strike netplay individual camera bugs.zip` and [video](https://youtu.be/GXLj0-7lkZw). Native attack34 uses PLAY+1C, also used by scene locks. Distinguish Nova from shared scenes and render private views during ordinary concurrent arrivals. Native LAN checks pass with X in either seat: idle partner camera stays fixed, both private renderers keep advancing, and peers retain matching state through sim5154/4672. `beads-eio.9.89` |
| 66 | [x] | Intro dragon appears on both distant independent views; shots only in progressing player's view | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Native intro actor0 switches to screen-relative coordinates during its background attack. Rebase it and attached type10 effects through the canonical world camera only inside private rendering. Actual composed before/after captures show the boss and muzzle effect remaining with X at3237 while Zero at224 no longer sees a duplicate. Original state stays screen-relative, both peers match through sim5444, render isolation and all32 checks pass. `beads-eio.9.90` |
| 67 | [x] | Product-focused co-op README, fresh screenshots and MIT license | **Completed after Playtest 13 publication**, per owner. Co-op-first page with seven actual gameplay/launcher screenshots, current downloads/defaults and short setup. Detailed player/developer guides moved out of the landing page. Project code is MIT; dependency and Capcom notices stay distinct. Hidden captures, GitHub Markdown render and local-link checks pass. Future packagers retain guides/images and independent license notices; a private Windows ZIP verifies exact notice/image bytes and an unchanged executable. `beads-eio.9.91` |
| 68 | [x] | Distant player wraps onto the other Web Spider view | **Published in [Playtest 13](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.13).** Paired Jungle logs from Opening stage dragon camera and web spider camera wrap around ZIP match. Native GPU sprite positions wrap every2048px. Per-view bounds now reject distant world sprites and armor before emission without changing actor visibility/lifetime. Actual composed before/after captures verify both views with either player ahead; all32 checks pass, peer state matches through sim5891, and private rendering preserves RAM/VRAM/devices. `beads-eio.9.92` |
| 69 | [x] | P1 cannot continue after P2 defeats Magma Dragoon | Reproduced from the reported setup on Playtest13 source. Mode7 incorrectly retains P2 gameplay text/actor ownership. The fix keeps this reward frontend under P1. Hidden native tests pass both campaigns: dead P1, P2's actual boss kill, P1 reward/save input, correct campaign and cleared boss in the native card/Continue cache, grey portrait on the selector. Published in [Playtest 14](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.14); separate from intro rescue item54. `beads-eio.9.94` |
| 70 | [x] | Loading a state with Diagnostics disabled creates incomplete recording folders | Supplied gameplay sessions contain only C rows at sequence0, with Diagnostics explicitly disabled. The co-op restore observer incorrectly starts a recorder whose native input hook is absent. Offline restore now respects the committed Diagnostics package; enabled baseline recording and network behavior remain covered. Published in [Playtest 14](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.14). Fresh Windows/Linux packages each capture181 native inputs with no Diagnostics override. `beads-eio.9.95` |
| 71 | [x] | X/P2's acquired armor uses stage colors in Zero's campaign | Reproduced and fixed after Playtest14. Private CLUT lookup used VRAM's 1024-pixel stride for a packed 256-wide palette: armor7843 read stage colors at860 instead of armor colors at260. Sprite and UI paths now share the correct mapping. Supplied Zero-campaign Cyber state shows correct armor before/after pause; boundary tests pass. `beads-eio.9.98` |
| 72 | [x] | Web Spider's lethal web transfers the capture and damage to the surviving partner | Reproduced and fixed after Playtest14. Native hostile web22 transferred its ongoing capture to the nearest survivor after death. It now retains the actual victim, keeps contact and its continuation in that seat, and runs native cleanup on death. Both victim roles pass native capture/death checks with the distant partner unmoved, uncaptured and at unchanged HP. `beads-eio.9.99` |
| 73 | [x] | P1's off-camera catch-up teleport restarts both players at the checkpoint | **Fixed after Playtest15.** Reproduced on unchanged15 with all4,177 retained input/state checks matching. Its stage-exit fix let a healthy P1 passenger's finished beam terminate the stage. Keep the living partner's world running during that temporary departure. The corrected continuation preserves all3,076 original prefix checks and returns both players at19/32HP, unchanged lives/checkpoint. Actual Storm Owl exits still pass with both alive and with only P2 alive. All34 Windows tests and the Linux lifecycle check pass. `beads-eio.9.101` |

| 74 | [x] | Only P2 arrives in Web Spider part 2 after a distant P2-led exit | **Fixed after Playtest16.** Archive peers match all10,520 shared native rows. Reproduced hidden P1 being killed by the higher exit camera during the outgoing fade. Retain the committed scene's camera owner through that fade and the leader's finished walk. Native two-peer reproduction now delivers both players alive with32HP and unchanged lives, with matching core/mod CRCs through6466. A deliberately dead P1 stays dead in the separate control; P2 still arrives safely (matching CRCs through6377). The [user's video at2:55](https://youtu.be/E601p79mDoQ?t=175) shows X's HUD emptying during the fade and only Zero arriving, matching the log failure. All34 Windows checks pass. `beads-eio.9.103` |
| 75 | [x] | P2's Jet Stingray entrance animation is wrong in netplay | **Included in [Playtest19](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.19).** Native boarding initialization consumed the dash-pose request before the rider could apply it, leaving pose0 on the accelerating bike. Finish boarding before the first entrance tick. Actual composed before/after frames show Zero leaning on the bike; the reversed roster also shows the proper X entrance. Split peer CRCs match through4064/3716, local Unified entrance passes, one READY remains, and both riders regain controls. The native regression now checks rider poses through acceleration/deceleration, not just bike state. All34 Windows checks pass. `beads-eio.9.104` |
| 76 | [x] | Local Mods page makes co-op boss/door and character settings hard to find | Enabling Co-op now opens its settings panel. Descriptions and player instructions clarify local/online support. Hidden local UI selects Both boss doors, both story bosses, scaling and both character codes; actual PLAY commit persists every choice. Existing defaults unchanged. Shared launcher commit248c347, all34 Windows checks pass. `beads-eio.9.105` |
| 77 | [x] | P2 passes the opening stage's breakable miniboss door and the camera sticks | Reproduced and fixed after Playtest17. Destroying the native barrier released only P1's saved camera bounds; P2 remained clamped at X3288. Publish the finalizer's two bounds writes to living views. The fixed two-peer probe advances P2 to X3411 while P1 stays at X1367, both alive, with matching peer CRCs. `beads-eio.9.107` |
| 78 | [x] | Web Spider part 2 scenery disappears or freezes for P2 | Reproduced and fixed after Playtest17. The section's background controller never advanced P2's region state, leaving its opening forest over later terrain. Keep per-view controller state while preserving shared script ownership. Actual composed before/after frames at P2 X1094/Y954 show restored terrain; 17 recorded-position probes complete with matching peer CRCs. `beads-eio.9.108` |
| 79 | [x] | Both-player boss doors still teleport the partner | Fixed after Playtest17. Both-player admission now retains independent controls through the open doorway, waits for physical clearance and suppresses the ensuing conversation's partner warp. Native two-peer test waits for the second contact, walks/slashes P2 while P1 stands still, then crosses and finishes the boss conversation with both players at32HP and no departure. Peer CRCs match through5794. Either-player mode and dead/withdrawn exemptions remain covered. `beads-eio.9.109` |
| 80 | [x] | Sigma stage 1 enemies wait for P1 when P2 advances ahead | Reproduced and fixed after Playtest17. Accept the actual placement table's valid4-byte base alignment while retaining8-byte records and native directional flags. The fixed two-peer probe spawns the enemies at X944,1108 and1223 as P2 advances while P1 stays at X683; peer CRCs match through4672. `beads-eio.9.110` |
| 81 | [x] | Colonel's lightning whites out the screen after P1 dies | Reproduced and fixed after Playtest17. Preserve the shared palette rebuild request across P2 survivor projection. The unfixed native lightning leaves1,920 white palette entries after its RGB additions clear; the fixed probe restores19, and the inspected P2 frame shows the normal stage and HUD. Peer CRCs match through10338. `beads-eio.9.111` |
| 82 | [x] | Zero cannot damage Double, including jumping spin | Ground-combo fix retained. Kuuenzan34 has FF/0 damage; reuse Double's authored two-point saber damage for that collision, preserving attack type, tokens, cooldown and scaling. Six original attempts leave48HP; fixed attempts reduce48 to42 with boss scaling on. Both-seat/survivor cooldown and lethal-hit tests pass. `beads-eio.9.112` |
| 83 | [x] | Sigma stage 1 second boss door kills P1 and traps P2 in local Unified | Reproduced from the Playtest19 local recording with Both-player doors. Colonel starts WARNING before the physical crossing completes, cancelling the aperture exemption and crushing P1. Defer that admission until the doorway and camera finish. Native continuation reaches the fight with both visible, clear controls and unchanged40/31HP; no partner departure. Windows34 checks and Linux lifecycle check pass. `beads-eio.9.116` |
| 84 | [x] | Iris graphics are corrupt in X campaign | Reproduced in exact Playtest19 local replay (17,303 verified input hashes). Story hook selected stage13's sprite slot instead of stage11's, leaving Double's graphics loaded. Correct the selector; native Iris introduction and armor/core combat render correctly. `beads-eio.9.86` |
| 85 | [x] | Iris sometimes fails to load properly | Reproduced a stalled intro after a Both-player door. Door minimum2048 conflicts with Iris waiting for camera2032. After physical clearance, admit her intro with the camera inside the authored arena. Fresh entry and recovery from the captured stall reach her fight with both players visible and no partner warp. `beads-eio.9.86` |
| 86 | [x] | P2 disappears at Sigma stage 3 entry | Playtest24 archive/video captures P2 dying before P1 defeats Double; slot11's section-carry rule wrongly preserves that corpse into the next fortress level. Treat11/0-to11/1 as a fresh level. Fixed Split/Either two-peer run follows the native Double defeat into Sigma3: both seats return at32 HP, both move, shared lives stay2, and recorded core/mod checks match through7488. Ordinary mid-stage corpse carry is retained. `beads-eio.9.117` |
| 87 | [x] | Double might use incorrect sprites in Zero's story | Not reproduced after Playtest20's shared sprite-selector correction. Fresh native Zero campaign with the optional sequence on defeats Iris, loads Double with correct inspected sprites, defeats him and returns to Zero's normal movie route. Campaign1 and two lives are retained; one sequel/two boss defeats are recorded. No additional graphics patch. `beads-eio.9.86` |

| 88 | [x] | P2 alone gets stuck in intro part2's crusher shaft | Metadata6 local Unified Playtest21 recording verifies all9,313 retained native checks. Dead P1 remains above; crusher C0558 tests only P1 Y, so panels never close/open the onward route. Use living P2 Y for that predicate while retaining native panel movement/map edits/contact. The verified recorded-state continuation escapes using wall jumps to X4175/Y443, HP23 unchanged and P1 still dead at X863/Y394. Both-campaign and negative predicate checks pass. `beads-eio.9.120` |
| 89 | [x] | Storm Owl has an extra wall inside the boss arena | Reproduced on current source with the rear player at the original2680/683 release area: that hero overwrites the leader's arena minimum with0, then warps past its entry trigger. Commit the authored2240 minimum only when Owl's native introduction starts. Fixed local Unified reachesX2248 with both players healthy and the camera locked; two-peer Split passes both leading seats with matching core/mod state through8938 and inspected arena captures. Regression fails before the fix; all34 Windows checks and Linux lifecycle pass. Included in Playtest29. `beads-eio.9.121` |
| 90 | [x] | Both living players perform their victory poses together | Default for normal Maverick clears: retain both native bodies/animations without partner withdrawal or a second theme/reward. Actual native Jet defeats pass in both campaigns; X fist raise and Zero salute are visible together, one clear/Continue cache update is retained. A save made during both poses reloads and finishes correctly. Midboss, survivor-only and story/movie paths keep native flow. Lifecycle snapshot/digest tests and all34 Windows title checks pass. `beads-eio.9.122` |
| 91 | [x] | Intro act1 exit cutscene starts before Both-player door finishes | Metadata6 first recording reproduces a nonfatal stall. Defer D0580 admission until physical clearance: old replay branch leaves camera min4368/P1 stuckX4872/P2 hidden; fixed branch commits min4848, ends the native walk atX4912, and enters intro part2. Separate from older unconfirmed crash item53. `beads-eio.9.123` |
| 92 | [x] | P2 Ride Armor ownership and safe dismounts | Corrected native mount mode; original Raiden boards, moves, attacks and ejects for either P2 character. Native partner arena transport ejects on foot; world armor retains HP. Both-door waiting/admission, death, occupied world slot and stage cleanup covered. `beads-eio.9.128` |
| 93 | [x] | P2 Jet Stingray intro looks slower than Playtest19 | Shared entrance camera restores missing autoscroll displacement. Both rosters regain controls 6/7 native frames apart in Split netplay; Unified 5 frames. Native poses and one READY retained. `beads-eio.9.129` |
| 94 | [x] | Storm Owl background graphics corrupt in Split netplay | Confirmed P2 sky froze while P1 sky advanced. Publish native kind5 aircraft/parallax movement into both independent views without repeating stage events; restore native panorama wrapping to remove black columns. Native before/after and peer CRCs pass. `beads-eio.9.130` |
| 95 | [x] | Slash Beast background graphics are glitched | Reproduced large missing mountain/sky regions around recorded input2730. Independent drawing discarded native wrapped panorama columns. Title drawing policy restores the authored background; strict private replay and screenshots validate it. `beads-eio.9.131` |
| 96 | [x] | Cyber Peacock flip switch kills the nearby partner | Reproduced in two-peer Split with P2 at X624/Y1126. Reflecting private camera positions without C5210's new vertical limits crushed P2. Both saved views now receive those authored limits and their parallax-origin shifts. Both switch activators pass with both players retaining32HP. `beads-eio.9.133` |
| 97 | [x] | Storm Owl midboss door teleports P2 after a Both-player crossing | General began action14 while the door was still in phase3, aborting the crossing. Defer his native admission until physical clearance and camera arrival. Local Unified and two-peer Split show one completed crossing and no partner departure. `beads-eio.9.134` |
| 98 | [x] | P2 Ride Armor draws its rider at the wrong sprite priority | Ride Armor and Ride Chaser share C5=FF; only Chaser kind0 gets the bike ordering now. Native Ride Armor retains rider bucket2/vehicle bucket3. Local and Split captures validate the rider; native mounting, attacks and ejection pass. Related X return safety is tracked separately as100. `beads-eio.9.135` |
| 99 | [x] | Boss-scaling cooldown dropdown with independent player cooldowns | Default method is per-player full native cooldowns; reduced shared cooldowns remain selectable and scaling itself stays optional. Native Jet/Spider damage, overlapping attacker windows and death pass; Split Jet combat also passes with matching peer state. Preserve one native expiry tick for Spider's temporary hit-table cleanup. Timers reset on slot/stage reuse and persist in X4P8 snapshots/digests. `beads-eio.9.136` |
| 100 | [x] | A returning P2 X is crushed after forced Ride Armor dismount | Native HP80 store80033C14 proves ceiling+floor crush at5558/683 after the scripted leader position5558/682. Preventing an extra8px descent alone was insufficient. Probe the returning body's terrain, try bounded same-height offsets, and defer if no safe landing exists. The native Zero-campaign case keeps X alive, returns him on foot when the leader reaches clear ground, and passes next-stage cleanup. `beads-eio.9.137` |
| 101 | [x] | Second boss doors are missing in Split netplay with Both-player doors | Reproduced two-peer Cyber Peacock: General8 at992/2496 never spawned after the outer door. The supplemental scan now admits unflagged doors while preserving native eligibility and other scripted activation. Fixed native run crosses both doors without teleporting either player, restores both controls, and matches peer core/mod state through5892. `beads-eio.9.139` |
| 102 | [x] | Web Spider part2 scenery breaks when P2 progresses alone in local Unified co-op | Metadata7 is Playtest25. Its scenery controller selected the dead P1's old region directly from the singleton body. Run that one canonical callback with the surviving P2; native recorded-position probes change the stale opening mode5/offset40 to the correct mode2/offset512 without repeating stage events. `beads-eio.9.141` |
| 103 | [x] | A living player can remain stuck after teleporting as their partner dies | The hidden waiting phase restored position/visibility but retained outgoing action0. Recover to a terrain-checked departure point and initialize the native controllable pose for either seat; keep a committed arrival in its new room. Preserve genuine deaths, shared lives, voluntary withdrawal and victory/section transitions. Focused old-fail/new-pass cases and native door-death recovery pass; the full original Storm Owl replay was not completed. `beads-eio.9.142` |
| 104 | [x] | P2 lacks slippery flooring in Frost Walrus | Native kind5/B60BC owns momentum outside PLAYER and previously updated P1 only. P2 now has private native controller state, included in snapshots/digests; the surviving P2's shared script pass cannot apply it twice. Original-disc before/after checks reproduce the abrupt stop and verify sliding for P2 X and Zero. `beads-eio.9.144` |
| 105 | [x] | P2's Split camera stays behind on Mushroom part2's elevator | Reproduced with two connected peers at stage3/section1/checkpoint2. The lift released only the canonical camera's vertical targets. Publish its native limit changes to views in the same shaft while retaining separate origins; do not repeat the platform or panels. Both views now follow the ascent, with matching peer core/mod state. `beads-eio.9.145` |
| 106 | [x] | A dead teammate does not return at the next stage section | Forward sections now revive either dead seat with initialized native HP while retaining the survivor's resources and shared lives. Same-section checkpoint deaths, whole-team retries and voluntary withdrawal retain their rules. Native Mushroom area-load checks revive either seat without spending a shared life; both-seat boundary fixtures and snapshot checks pass. The special Double/Iris-to-Sigma3 new-level policy remains. `beads-eio.9.146` |
| 107 | [ ] | Web Spider part1 spiked platform enemy does not spawn for leading P2 | Still open, with new Playtest36 report that it is missing even when both players traverse. Prior native probes did spawn kind76 at1976/901 and the later enemy3520/1384; those bounded successes do not clear the recurrence. New archives and mode identities indexed in [review](COOP_FEEDBACK36_REVIEW.md). `beads-eio.9.149`, `beads-eio.9.188` |
| 108 | [x] | Web Spider part1 broken-door exit corrupts P2's background | Reproduced during a native P2-led General6 exit: the shared camera moves to Zero while native4:3 draws P1's stale tile ring as wrong cliff terrain. Use Split's authored map renderer in canonical shared passes too. Inspected fixed composed frame restores the waterfall/floor; both players reach part2 healthy and controllable, no local-view state leak, peer checks agree through7203. Old-fail/new-pass stale-ring fixture, Windows34 and Linux background checks pass. Included in Playtest28. `beads-eio.9.150` |
| 109 | [ ] | Web Spider part2 first bee enemies do not spawn for leading P2 | Re-reported on Playtest31 in `31 bugs.zip` and [video](https://youtu.be/tS316c1P3sE); both peers'29325 overlapping recorded state hashes agree. New current-source Split/Both LAN natural approaches cover P1 and separately P2 leading, partner left at the start. Actual bee parent29 and flying offspring28 spawn in both cases, with an inspected composed P2 wasp image and matching core/mod checks through8002. This is not the earlier unrelated ground-enemy candidate. The supplied recurrence remains tracked; no missing spawn reproduced or new spawn fix claimed in this bounded route. `beads-eio.9.149` |
| 110 | [x] | Large health pickup shifts the displayed camera to P1 | Archive A identifies the full-health capsule, native kind5, omitted from Split's private refill handling. Native baseline heals P2 while freezing the world and presenting P1's unified view. Include kind5: the real capsule now heals8 to32 with no shared freeze, P1 moves29.5px during replenishment, and P2's own redraw/HUD continues. Peer state matches through5987; all34 Windows checks and Linux lifecycle/split units pass. Unified retains the native shared pause. Included in Playtest28. `beads-eio.9.11` |
| 111 | [x] | Frost Walrus part1 ice platforms do not spawn for leading P2 | Video samples locate the opening timed ice, distinct from the rising shaft. Native baseline keeps the cycle unarmed with P1 at144/347 and P2 at705/426; bringing P1 over creates ice. Allow either living hero to arm the original cycle and use both Split camera ranges for its tile sequences. Corrected native P2-ahead and distant-camera cases pass with visible ice, healthy players and matching peer CRCs through7398. Windows34 and Linux lifecycle/split checks pass. Included in Playtest28. `beads-eio.9.151` |
| 112 | [ ] | Frost ice platforms remain indefinitely after P1 spawns them and dies | Opening timed-ice and rising-shaft controls continue changing phases, masks or positions after P1 dies; the permanent-platform symptom remains unreproduced. Keep this separate from the corrected P2 activation111 and personal ice momentum104. Archive B. `beads-eio.9.151` |
| 113 | [x] | P2 snowflakes animate only after P1 dies | Reproduced with settled Split views: P1 stays144/347 while P2 waits3281/378; P2 snow origin and visible flake positions freeze until P1 dies. Native B75C8/type9 updates the shared weather atlas, outside ordinary mode5 camera follow. Publish its exact layer2 origins/changed metadata to both saved views without repeating its clock. Fixed native snowfall moves before and after P1 death; composed flake-position comparisons pass and peers agree through5416. Windows34, Linux combat/split and entry/release wiring checks pass. Included in Playtest28. `beads-eio.9.152` |
| 114 | [x] | Frost Both-player boss door teleports P2 and can trap P1 walking left | Included in Playtest28. Reproduced in native Split/Both at checkpoint4. Midboss52218 starts its scene during pair phase2; defer its whole admission until physical clearance. Also fix the permanent camera6080/minimum6032 equality wait by accepting the authored room interval. Both leading arrangements now cross, keep both players healthy/visible and restore controls without transport; peer state matches through8514. The supplied pair2 trace confirms Both was active despite saved Either settings. `beads-eio.9.109` |
| 115 | [x] | P2 water ripples appear only when P1 is nearby | Reproduced in current native Split: P2's active water effects at432/394 are hidden with P1 at56/139, appear when P1 approaches400/395, then hide again when P1 leaves. Their owner projection excluded Split's two-view visibility query. Admit attached-effect context to that existing query. Fixed far/near/far native checks retain P2's pair in all15 samples; inspected composed images and matching peer state through4964. Windows34 and Linux combat/split checks pass. Included in Playtest28. `beads-eio.9.28` |
| 116 | [x] | Both-player first/second door opening focuses the camera on P1 until both pass | Included in Playtest28. Keep independent saved cameras, view selection and actual local presentation through paired phases1–3, sharing the authored door limits. Real boss introductions retain shared scenes. Composed Frost frames show separate X/Zero screens and HUDs with232px separation; peer state matches through8036. Cyber and Spider each cross both native doors, restore controls and retain both players with no transport or local-view state leaks. Windows34 checks and Linux lifecycle/split tests pass. `beads-eio.9.109` |
| 117 | [x] | X attacks do not knock back Iris | Retail Iris rejects every X attack. Admit X through her existing native armor reaction without changing lasting armor HP or the eight-hit exposure sequence. Native X/P2 buster attacks alone now reduce the remaining armor hits from8 to0 and expose the real crystal; peers agree through7844. Both X seats and survivor cases pass combat units. Included in Playtest28. `beads-eio.9.153` |
| 118 | [x] | X attacks do not damage Iris's crystal | Give X collision-specific damage entries using Iris's own native damage units, retaining projectile identity, tokens, cooldowns and boss scaling. Native X/P2 buster hits now reduce the actual crystal48 to47 with scaling on; read-only trace identifies attack0 and the original HP store. Both players remain32HP and controllable, with matching peer state through31713. All34 Windows checks and Linux combat/split checks pass. Included in Playtest28. `beads-eio.9.153` |
| 119 | [ ] | Sigma stage2 P2-triggered enemy respawns facing nearby off-camera P1 | Bounded Split/Both check: with healthy P1 ahead at1000 and P2 at560/458, newly spawned kind51 at702/410 faces the closer P2 left despite P1 being right. Original kinds51/58 choose facing during initialization; existing kind58 can retain its earlier right-facing pose. Peers agree through9248. This does not identify the reported enemy/respawn or establish a facing defect; keep open for a timestamp and diagnostics. `beads-eio.9.154` |
| 120 | [x] | Web Spider background animation stops after P1 dies and P2 continues | Reproduced the opening waterfall freezing after native P1 death while P2 remains healthy. Shared palette controllers keep advancing, but survivor projection discards their upload bit1. Preserve stage palette uploads/rebuilds and canonical palette binding for that one stage pass. Fixed native composed water and VRAM colors animate before/after death; peers agree through8897. Windows34 and Linux lifecycle/split checks pass. Part2 survivor camera follow also passes separately. Included in Playtest28. `beads-eio.9.108` |
| 121 | [ ] | Slash Beast background animation stops after P1 dies and P2 continues | Bounded current-source Split/Both probes do not reproduce this: at the second-section train floor and after real paired boss entry, saved scenery advances8px/tick and each composed background region changes in all4 alive and4 survivor captures. P2 stays32HP; peer checks match through16614 and33925. Exact reported mode/location/recording segment remains unbound; no Slash scrolling fix is claimed from the separate Spider palette fix120. `beads-eio.9.55` |

| 122 | [x] | P2 cannot activate Sigma3 boss-refight teleporters | Admit the native General27 actor in P2's Split view and retain the actual entering hero through its state machine. Split/Either P2 X enters alone and brings both healthy heroes to Cyber's refight; genuine P1/P2 survivor cases also pass. `beads-eio.9.160` |
| 123 | [x] | Offer Both-player activation for boss-refight teleporters | Existing door selector now also controls refight pads. Native Split/Both holds P2 alone, admits the pair together, and exempts a genuinely dead partner; Either remains the default. Both campaigns and both survivor seats pass. `beads-eio.9.160` |
| 124 | [x] | Magma Dragoon needs an extra walk forward before the refight triggers | Checked through the actual P2-led hub pad: both arrive healthy, P2 advances into the authored admission, and Dragoon's fight starts without P1 having to lead. Peer state matches through27968. The retail walk-in remains; no additional seat gate reproduced. `beads-eio.9.157` |
| 125 | [x] | P2 weakness does not affect Storm Owl | Reproduced on current29: native Zero/P2 Rakuhouha (R2, action58/type36) damages Owl but never reaches weakness phase11. Select the accepted hit's character at the original reaction branch instead of P1's campaign. Patched Split/Both native peers enter phase11;722 core/mod samples match through23136. Windows34 and Linux combat checks pass. Fortress-specific admission/grab/transport reports remain separate. `beads-eio.9.158` |
| 126 | [x] | Storm Owl leaves P1 grabbed until P2 dies | Reproduced in the fortress refight: the native release clears one body while the other remains captured through later attack cycles. Mirror Owl's native release/abort events to the other body only for capture code1. The post-fix actual double grab releases both healthy heroes, and both move afterward. Both-roster/target unit checks preserve pending damage commands and one AI update; [qualified scope](COOP_PLAYTEST34_INVESTIGATION.md). `beads-eio.9.157`, `beads-eio.9.176` |
| 127 | [x] | P2 weakness does not affect Jet Stingray | Actual P2 Zero ice attack type33 enters Jet's native rootphase3 after the attacker-selector correction;48 to46HP, paired core/mod state agrees through30016. `beads-eio.9.158` |
| 128 | [x] | P1 teleports away after the final hit on Split Mushroom | Qualified through the actual hub pad in local Unified: P1 X's Lightning Web supplies the final hit, both authored beams complete, and both heroes return healthy/visible/controllable at checkpoint13. A teleport-fixture gravity error was excluded from this result. `beads-eio.9.163` |
| 129 | [x] | P2 weakness may not affect Slash Beast | Current local Unified, Zero campaign: actual X/P2 Ground Hunter damages the fortress refight48 to46 and enters native reaction phase11. No additional Slash-specific failure reproduced after the attacker correction. `beads-eio.9.157` |
| 130 | [ ] | Web Spider weakness may have no effect | Current local Unified, Zero campaign: X/P2 Twin Slasher damages the actual fortress Spider48 to47. This establishes damage; the separately authored supporting-web cut/reaction was not matched, so that part remains unconfirmed. `beads-eio.9.157` |
| 131 | [x] | P2 weakness does not affect Frost Walrus | Actual P2 Zero fire attack type35 enters native weakness phase1/10 after the attacker-selector correction;48 to46HP, paired core/mod state agrees through27138. Combo cooldowns remain separate. `beads-eio.9.158` |
| 132 | [x] | P2 Zero cannot combo Frost Walrus because immunity occurs between hits | Per-player scaling now gives the first two accepted ground-saber strikes4-tick timers, retaining the finisher's full timer. Native Split/Either emits all three classes24/25/26, damages actual Frost48 to45, and keeps both heroes healthy; peers match through6946. Shared mode is unchanged. `beads-eio.9.164` |
| 133 | [ ] | Death animations lose sprites or disappear | The report does not identify whether this is a hero or boss; actor and exact event need binding to the video/recording before a fix is claimed. `beads-eio.9.157` |
| 134 | [x] | P2 teleports away after the final hit on Jet Stingray | Qualified through the actual hub pad in local Unified: P2 Zero's native ice attack supplies the final hit, the authored outgoing beams complete, and both heroes return healthy/visible/controllable at checkpoint14. No premature missing partner remains in that case. `beads-eio.9.163` |
| 135 | [ ] | Lightning Web sometimes fails to trigger wall-mounted Split Mushroom's weakness | Native P1 X Lightning Web hits the wall-mounted actual refight,48 to46 with a return from phase7 to phase2. The reported intermittent failed-hit condition has not been matched; no additional weakness fix claimed. `beads-eio.9.157` |
| 136 | [ ] | Room before Sigma lacks P1's intro after death or Continue | Actual final kind74 fight admits both healthy heroes. A genuine native team death/retry restores both controllable arrivals; matching peers through21634. The specific surviving-P2 yellow-pad scene is now reproduced and fixed under210; the broader older death/Continue variants remain unqualified. `beads-eio.9.157` |
| 137 | [x] | Pause does not open while a Both-player boss door is open | Permit each seat's native pause during free door traversal. Opening/admission/intro locks remain enforced. Native Split/Both Frost door: both owners pause/resume then complete crossing, with matching peers through11109. `beads-eio.9.159` |
| 138 | [x] | Lightning Web may not trigger Sigma phase2 weakness | Checked current native second form (kind68/variant1): actual Lightning Web reduces48HP to46 and creates the authored weakness effect type32/variant6. Peer state matches through15809. No additional Web-specific fix needed. `beads-eio.9.157` |
| 139 | [x] | P2 Zero cannot combo Sigma phase2 because immunity occurs between hits | Same opening-strike timer correction as132. Actual P2 Zero generates classes24/25/26 against the native second form, its combo counter advances, and HP46 to44; peers match through15809. `beads-eio.9.164` |
| 140 | [ ] | Sigma phase1 targets P2 except during the scythe attack | Shared native AI uses the nearest living hero, with P1 winning ties; both-seat/Unified/Split targeting tests pass. Phase-specific cached targets and the reported positions have not been matched, so no Sigma AI rewrite is claimed. `beads-eio.9.157` |
| 141 | [x] | P2 teleports out and back during Sigma phase2 | Reproduced native85CA8 presentation withdrawing nearby P2 with32HP. Nearby living heroes now remain for stationary fortress boss scenes. Fixed native complete first-to-second form transition retains both visible bodies and controls; peers match through9920. Actual transport commands remain enabled. `beads-eio.9.163` |
| 142 | [x] | P2 teleports out and back during Sigma phase3 | Same-arena boss presentations use141's correction. Native second-form defeat still carries both heroes to the lower room; both arrive healthy, enter the actual kind74 fight, and survive its admission without a missing partner. Genuine upper-to-lower-room transport is preserved. `beads-eio.9.163` |
| 143 | [x] | Adopt the SNES Mega Man X diagnostic file optimization before release | Bounded background formatting, hashes and writes preserve the v2 format and remove disk waits from steady-state input recording. Windows/Linux writer and replay checks pass; a real co-op recording replays all2542 complete native inputs. `beads-eio.9.161` |
| 144 | [ ] | Slash Beast lifebars and scenery boxes become corrupted | Metadata8 report; distinguish HUD/actor artwork from the previously fixed panorama and the unconfirmed frozen background. No matching composed frame qualified yet. `beads-eio.9.170` |
| 145 | [x] | Mushroom doors before the first miniboss trap P2 when P1 crosses first | Reproduced as a native kind13 hatch at6088/792, rather than a C4 boss door. Original prefix verifies15908 inputs and closes with P2 below atY843. P2's movement briefly clears its contact-box pointer; the earlier hatch guard treated missing geometry as clearance. Wait for that living on-screen partner's ordinary box instead. Fixed replay retains checks until the intentional frame15786 fork; the hatch stays open with P2 below, and subsequent native controls move P2 without transport. Both-roster missing-box/ordinary-clearance tests pass. A complete natural ascent is not claimed. `beads-eio.9.170` |
| 146 | [x] | Mushroom part2 lift does not work with only P2 alive | Fixed after Playtest31. Native C3A7C waits on the dead P1's zero action. Use the surviving P2 for that one shared admission update, composed through the existing lift-camera dispatcher. A supplied restored-state segment retains all953 complete input/state checks; its subsequent native checkpoint2 control stalls atY2216 on the original and rises toY2059 with the fix, P2HP36 and P1 still dead. Ceiling panels and later ascent retain their native rules; this does not qualify every older elevator report. `beads-eio.9.170` |
| 147 | [x] | Cyber portal entry during the other hero's withdrawal softlocks | Reproduced in current Split/Both LAN: P2-led course transport overtakes P1's earlier incoming beam, hides state1, and leaves the native checkpoint loader waiting forever for canonical state3. When a committed real transport overtakes that passenger, finish its native outgoing beam while preserving cached health/visibility/origin. The previously failing native P2 entry now reaches checkpoint2 with both healthy, visible and controllable. Both-owner/phase unit checks reject dead passengers; ordinary scene locks and completed outgoing bodies remain unchanged. The supplied6763-input prefix ending in pause is not claimed as this reproduction. `beads-eio.9.170` |
| 148 | [x] | Magma Dragoon Ride Armor boss entry hides the partner until armor destruction | Reproduced the return gate rejecting native action18/personal1/no-foot-box indefinitely. Use only an occupied grounded Armor's native collision box and floor as the return anchor, retaining scene locks and terrain checks. Native room-handoff A/B leaves the healthy partner hidden under the old gate and returns them safely with the fix, preserving the occupied Armor. Extended actual Dragoon admission completes with both heroes healthy/visible and the fight unlocked, Armor intact. Both-owner unit checks reject airborne, unoccupied, Chaser and scene-locked anchors. The supplied unchanged31 prefix36550 verifies but has an on-foot leader, so it is not claimed as mounted-leader reproduction. `beads-eio.9.170` |
| 149 | [x] | X's upgraded boots lack their blue hover flames in Zero's campaign | Reproduced and fixed after31. Native effect33/800B2AD0 reads the singleton player's pose229/230 and deletes itself when it sees idle Zero/P1. Run this personal effect with its allocating X, preserving its assembly, position and native expiry. Fresh Zero-campaign native check shows five healthy P2-hover samples with visible effect33 and blue/white foot jets matching the initialized X/P1 control; the actor expires after landing. Later authored hazard deaths are excluded from that visual qualification. Owner/following/expiry/reuse tests, all35 Windows checks and Linux combat checks pass. The earlier unrelated effect4/9 candidate remains removed. `beads-eio.9.170` |
| 150 | [ ] | X's special-weapon icons inherit stage colors in Zero's campaign | Separate from acquired armor body palettes. Native Zero-campaign live-HUD captures now cover X/P2 weapons1/3/7 in Spider, Mushroom and Slash, in addition to the earlier pause-menu check. The same icons retain their distinct colors across these stage palettes; the reported failure is not reproduced in this bounded check. No palette rewrite claimed. `beads-eio.9.170` |
| 151 | [ ] | Show both heroes together in suitable story scenes | Elective Metadata8 request for scenes such as Magma Dragoon and Storm Owl. Assess scene gates, rider dismounts and safe placement; existing simultaneous arrivals and victory poses remain separate behavior. Tracked separately from the correctness release gate in `beads-eio.9.173`. |
| 152 | [x] | Zero/P2 aerial attacks leave longer boss immunity with separate player cooldowns | Owner confirms scaling/separate mode and requests half-length boss iframes generally. Separate mode retained native64 ticks while shared mode used32. Both scaling methods now divide boss hit cooldowns by2, rounding up; separate mode retains independent owners. Native P2 Frost repeat aerial observations improve from69-74-frame accepted gaps to many35-40-frame gaps, with10 instead of6 accepted damage observations in12 attempted short hops. Reverse-roster Zero/P1 also passes. Earlier64-tick expiry checks did not qualify the desired scaling cadence and are superseded. All35 Windows checks and Linux balance checks pass. Qualified for Playtest33. `31 bugs (1).zip`, [video](https://youtu.be/tS316c1P3sE), `beads-eio.9.171` |
| 153 | [x] | Zero/P2 grounded-combo finisher delays the next combo with separate player cooldowns | Same full64-versus-half32 discrepancy after finisher26. Separate mode now halves that timer while retaining the short first/second grounded-strike timing, distinct player timers, original hit tokens and doubled effective health. Native P2 ground mash emits24/25/26 and damages Frost48 to25 after the fix versus48 to34 before, with earlier post-finisher reconnects; Zero/P1 also passes. Both-owner tests release finisher/aerial on tick32, preserve the other seat's eligibility, round odd durations up and retain snapshots/native unscaled timing. All35 Windows and Linux balance checks pass. Qualified for Playtest33. `beads-eio.9.171` |
| 154 | [x] | X's armor capsule shows Iris/Colonel story text in Zero's campaign | Reproduced with the real Spider boots capsule: Zero's campaign opened HUNTER H.Q./Mavericks emerged, while X's campaign opened DR. LIGHT/This battle. Capsule text, portrait and cue selectors now use X's bank while the shared textbox retains P1's campaign and palette. Native after-check shows Dr Light's hologram and correct later pages, completes the conversation and restores both players' control. All35 Windows checks and focused Linux lifecycle/story-selector checks pass. Qualified for Playtest33. `beads-eio.9.174` |
| 155 | [x] | Simultaneous fire-weakness hits reset the game to character selection | Reproduced on current source with charged X/P2 Rising Fire plus Zero/P1 Ryuuenjin: deferred effect26 copied the wrong shot's graphics and decompression overwrote PLAY. Bind that native update to its allocating seat and emit the required callback. Both rosters now defeat native Frost and return healthy after repeated charged overlaps; fire graphics inspected. Sigma's arena setup was not matched, so this is qualification of the shared parent-ownership fix and exact Frost reproduction. `beads-eio.9.176` |
| 156 | [x] | P2 returns inside a wall and dies during fortress refights | Bound to three Metadata9 Slash returns, then reproduced: X/P2 at1960 is crushed beside P1's safe1936 pad. Gate hub arrivals on a grounded partner and native terrain validation. Actual Slash defeat/return passes both rosters, with both heroes controllable at1936 and their native foot heights; also passes when P1 died before P2's final hit. `beads-eio.9.176` |
| 157 | [ ] | Boss scaling suppresses fresh Zero follow-up slashes | Reopened after38. Native overlap tracing found a lifetime-wide veto rejecting changed saber poses after both timers expired; the39 candidate delegates existing-blade contacts to native pose/token checks. Latest historical single-slash report remains unconfirmed; do not claim all bosses or modes from one Frost comparison. [Renewed audit](COOP_CAMERA_HITREG39.md). `beads-eio.9.177` |
| 158 | [x] | Opening-stage explosions flash red on the distant Split partner's screen | Remove intro red pulses from distant private view palettes within the render transaction. Nearby views keep the pulse; global fades and Unified play stay native. Actual Split LAN shows normal distant colors and210 scoped corrections, matching peers through5179,907 committed private views per peer and zero render-state faults. The stronger explosion-controller path has native-source and unit coverage; the actual capture qualifies the small red pulse. Qualified for Playtest35. [Evidence](COOP_FEEDBACK34_FOLLOWUP.md). `beads-eio.9.179` |
| 159 | [x] | Leading P2 does not trigger the later outdoor Frost ice path | Distinct from opening-cycle111. Native type21/subtype3 checks only P1.x>=3168. Baseline waits with P2 ahead; P1 crossing starts it. Allow an eligible living P2 to satisfy that initial predicate, preserving one native path sequence. Actual Split LAN creates six platforms and P2 walks onto the path while P1 remains at144; inspected presentation and matching core/mod CRCs through5081. Qualified for Playtest35. [Evidence](COOP_FEEDBACK34_FOLLOWUP.md). `beads-eio.9.180` |
| 160 | [x] | Later Frost ice path remains indefinitely for leading P2 | Qualified for Playtest36: the exact C41C8 type17 overlap query includes P2, starting the original collapse sequence. Actual Split LAN P1 remainsX144 while healthy P2 starts segments8/9 and native retirement8; matching peers through4773. This extends contact, without adding an expiry timer. [Evidence](COOP_FEEDBACK35_FROST.md). `beads-eio.9.183` |
| 161 | [ ] | Frost part2 enemy sprites corrupt with P2 leading | Paired35 logs retained. A native-clock cadence excerpt drives P2 fromX120 to693 while P1 staysX96; inspected early enemies, ice and player effects render normally. This bounded section does not reproduce the later-stage report; unsafe teleport experiments are explicitly discarded. A precise video timestamp/affected enemy is still useful. [Limits](COOP_FEEDBACK35_FROST.md). `beads-eio.9.184` |
| 162 | [x] | Frost ice-bird field effect focuses independent views on P1 | Qualified for Playtest36: a native world freeze no longer declines the private viewport. Synchronized original type17 field fixture retains both distinct cameras/HUDs;7/8 own-view passes observed while the freeze remains set, no render leaks, healthy players and matching core/mod CRCs through5059. This is native event qualification, not exact replay of the bird encounter. [Evidence](COOP_FEEDBACK35_FROST.md). `beads-eio.9.185` |
| 163 | [x] | Ice birds do not respawn for trailing P1 | Qualified native behavior: completed/killed birds can retire via8002B13C while retaining placement flags; the separate offscreen-return branch clears them with8002B0C8. Do not duplicate an already completed shared environmental encounter for the trailing seat. No respawn-policy change shipped. [Native evidence](COOP_FEEDBACK35_FROST.md). `beads-eio.9.186` |
| 164 | [ ] | P2 warps after Frost second Both-player boss door | First-entry dialogue remains a possible explanation. Direct teleports skipped authored camera/terrain regions and did not qualify this report. Ordinary paired traversal and a real shared scene remain separate; no scene-policy change claimed. [Limits](COOP_FEEDBACK35_FROST.md). `beads-eio.9.187` |
| 165 | [x] | Cyber Peacock P2 course portal softlocks | Reproduced: native type34 assessment follows P1 and is culled during P2 admission before signaling the kind38 course controller. Retain one native assessment update with the actual owner and protect its completion through the camera handoff inside the existing bounds dispatcher. Both local Unified rosters and real Split peers complete the original S-rank result, checkpoint loading and subsequent input movement with both heroes healthy/visible; matching peers through4578/4323. Native grading/timers/self-deletion remain intact. [Evidence](COOP_PLAYTEST37_INVESTIGATION.md). Targeting feature172 is separately qualified. `beads-eio.9.188` |
| 166 | [x] | Storm Owl part2 P2 stuck after miniboss | Reproduced in real Split/Both: Generaid Core death releases canonical maxX to2560 while P2 retains2240 and stops at2552.5. Publish only the native current/target max-X writes to saved views still in that enclosure. Both finishing seats, both character assignments, native corpse retirement and matching peers through8197/7972 pass; local Unified both-finisher controls and all35 Windows checks pass. Item167 remains separate. [Evidence](COOP_PLAYTEST37_INVESTIGATION.md). `beads-eio.9.188` |
| 167 | [ ] | Storm Owl Buster Upgrade followed by X death strands Zero above | Additional secondhand description; do not automatically attribute it to a specific supplied session or claim earlier camera/arrival fixes cover it. [Review](COOP_FEEDBACK36_REVIEW.md). `beads-eio.9.188` |
| 168 | [x] | Intro falling platforms do not trigger for leading P2 | Native walking controls reproduce the original P1-only2000 threshold: P1 drops subtype5 from288 to408; P2 leaves it suspended. Extend only BEED4's loaded coordinate to either living leader. Both local and actual Split rosters pass both leading seats, native falling/healthy bodies and matching peer state through6628/6212; all36 Windows checks pass. Boss approach2 remains separate. [Evidence](COOP_PLAYTEST37_INVESTIGATION.md). `beads-eio.9.188` |
| 169 | [x] | Frost rising-ice shaft P2 camera stays behind after P1 dies | Reproduced later shaft stale5312 landing limit and192px vertical follow.39 corrects the native5728 unlock and each view's phase-specific112px follow. Actual Split LAN checks pass with either leader and P1 dead; P2 remains healthy and both peers agree afterward. [Evidence and limits](COOP_CAMERA_HITREG39.md). `beads-eio.9.188` |
| 170 | [ ] | Frost P2 camera graphics corrupt after the first death | New current36 report; distinguish saved camera limits/origin, local presentation and map/VRAM faults. No claimed fix yet. [Review](COOP_FEEDBACK36_REVIEW.md). `beads-eio.9.188` |
| 171 | [ ] | Frost part2 slows down after ice-bird freezing effect | New performance report, separate from the fixed camera-selection gate162. Measure actual native effect and rendering work rather than infer the cause from a retained camera. [Review](COOP_FEEDBACK36_REVIEW.md). `beads-eio.9.188` |
| 172 | [x] | Cyber course character-analysis targeting graphic also follows P2 | Draw a translated copy through the native emitter while retaining one assessment/grade/completion update. Both local Unified rosters and actual two-peer Split near/distant views pass, including original course completion, healthy movement, restored scratch bytes and matching peers through9057/8611. All36 Windows checks and Linux helper cases pass. [Evidence](COOP_PLAYTEST37_INVESTIGATION.md). Separate from boss missile targeting. `beads-eio.9.189` |
| 173 | [x] | Optional local side-by-side4:3 views in an8:3 window | Local-only co-op mod dropdown; Unified remains default. Offline controller-driven separation, HUDs, pause/resume, resize, P2 survival after P1 death and shared-script flat/paired handoff pass.880 additional verified draws with no rollback faults. Owner authorizes Playtest38 as an opt-in experimental mode. Human feel/performance checks and the Jungle rendering observation remain documented limits. [Evidence](COOP_LOCAL_SPLIT38.md). `beads-eio.9.190` |
| 174 | [x] | GitHub39: X/P2 Aiming Laser beam animation missing in Zero campaign | Bind its deferred type9 beam update to the actual X owner. Healthy target-lock baseline fails; native X/P1 control and corrected two-peer Split X/P2 beam pass, with matching peer state. Locally qualified, not released. [Evidence](COOP_GITHUB_REVIEW38.md). `beads-eio.9.192` |
| 175 | [x] | GitHub39: Space Port and Final Weapon backgrounds frozen for P2 | Publish native-once controller7 scroll and controller6 atlas animation to both saved views, retaining each view's parallax motion. Baseline reproduces both; native two-peer Split and Windows/Linux helper checks pass. Locally qualified, not released. [Evidence](COOP_GITHUB_REVIEW38.md). `beads-eio.9.192` |
| 176 | [x] | GitHub41: first intro gate explosions reportedly continue forever | Reproduced Zero/P2-first rescue handoff; corrected in merged PR45, commit23b72d8. Native original-disc traversal completes the rescue and reaches part2 with both players present. Included in39. `beads-nvej` |
| 177 | [x] | Selected offline8:3 canvas falls back to one4:3 image |39 candidate duplicates shared/native presents into both4:3 panes for the session. Native gameplay, pair refusal, pause/resume and resize checks pass. [Evidence](COOP_LOCAL_SPLIT39.md). `beads-68fs` |
| 178 | [x] | Distant offline split view shows fragmented terrain | Reproduced with the retail shared tile ring. Local split now requires the authored-map renderer; stationary partner terrain remains intact as the other advances.624 protected-view checks pass. [Evidence](COOP_LOCAL_SPLIT39.md). `beads-68fs` |
| 179 | [x] | Each hero opens their character's own pause menu in either campaign | Native projection already exists; both menus visually checked in both campaigns. Controller QA covers ownership, guest Quit blocking, weapon selection and resume in local8:3 and Split/Unified LAN. [Evidence](COOP_PERSONAL_MENUS40.md). `beads-or94` |
| 180 | [x] | Optional separate E-tank contents under co-op | Default-off option implemented. Shared acquisition/W-tanks and P1 saves preserved; private energy survives native reloads and snapshot/view transactions. Native local and real LAN healing/default-shared controls pass with matching peer state. [Evidence](COOP_PERSONAL_MENUS40.md). `beads-or94` |
| 181 | [x] | Reduce Zero's boss damage by20% with co-op scaling | Exact carry for either seat/cooldown method; X/unscaled damage and native hit admission preserved. Matched native Frost controller trials measure36 raw Zero damage to14 HP with fractional carry; unit and Linux checks pass. [Evidence](COOP_PERSONAL_MENUS40.md). `beads-or94` |
| 182 | [x] | Storm Owl lasers and early part2 enemy beams appear in P1's view when P2 leads | Renewed in1.0.2 and traced to singleton-camera clamps in native secondary20/36. Supply the actual emitter's nearest living view to the loaded clamp/culling origins. Native192-sample qualification observes both facings with original offsets, both heroesHP32 and separate composed views. Included in1.0.3; no full-stage or online claim. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.217` |
| 183 | [x] | Mushroom climb latches close behind P1 and trap P2 | Native39 closes with P2 below. Candidate holds the latch for a living partner in the shaft, then closes after both clear it. Unified visibility and dead/withdrawn exclusions retained. [Evidence](COOP_STAGE_CONTROLLERS40.md). `beads-t56p` |
| 184 | [x] | P2-led Mushroom midboss camera initially offset | Native entry used distant P1's camera. Initialize the one midboss scene in the entering P2 view. Native entry, actual final hit and corpse cleanup pass; post-fight traversal remains item187. [Evidence](COOP_STAGE_CONTROLLERS40.md). `beads-t56p` |
| 185 | [x] | Mushroom lift P2 camera offset and follows high jumps | Reproduced with native high jumps: P2's view drifts101px from the shared lift scroll. Keep the shared vertical rail during native lift phase1, preserving horizontal views, rider physics and phase2 release. Both rosters pass both-alive and P2-survivor windows;1538 samples retain zero cameraY difference, living riders32HP and unchanged lives. Full ceiling-panel clearance remains unclaimed. Included in1.0.2. [Evidence](COOP_MARINE_MUSHROOM_RELEASE_GATE.md). `beads-t56p` |
| 186 | [ ] | Mushroom lift blue-block destruction stops with P1 dead | Surviving P2 must activate the native panel mechanics. `beads-t56p` |
| 187 | [x] | P2 invisible wall after Mushroom midboss | Reproduced after native Tentoroid death in1.0.2: P1's left limit opens4352 while P2 retains4608 and stops4616. Publish the original death-limit stores to matching saved views. Native P2 final hit, original retirement, controller-driven climb and actual part2 transition pass with living P1, bothHP32; survivor control crosses the former wall with P1HP0/P2HP32. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.218` |
| 188 | [ ] | P2 Ride Chaser dash reportedly speeds P1 and leaves P2 behind | Initial native control accelerates the correct vehicle; exact report not yet reproduced. `beads-t56p` |
| 189 | [x] | Slash Beast backgrounds stop animating for both players | Playtest41 fixes discarded canonical scrolling. The new P2 recurrence is independently reproduced: private drawing discards the native panorama wrap, so the saved origin grows and every later draw snaps back to the same scenery. Preserve the native wrap in saved planes and the owner's writeback. Native local8:3 checks in both sections now retain animated near/far scenery with separated, living heroes; paired images inspected. This qualifies the scrolling defect, not every later train event. Published in Playtest42. [Evidence and limits](COOP_FEEDBACK42.md). `beads-t56p` |
| 190 | [x] | P2 does not trigger Slash Beast bomb scenes | Bind the shared train controller to the progressing hero and retain that seat through commands. All three actual native bomb admissions/endings pass at2256,3376,6736; final scene also passes with P1 dead. The5056 phase is intentionally not another bomb admission; do not use that as a failure. Other train reports remain open. [Evidence](COOP_STAGE_CONTROLLERS40.md). `beads-t56p` |
| 191 | [x] | P2 does not trigger Slash Beast midboss | Renewed in1.0.2 and reproduced natively: P2 crosses7009 without allocation while P1 waits6464. Route the separate category7/type3 controller through the progressing hero scene transaction. Native admission, initialized living midboss, restored idle state and controller movement pass with either leader and with P1 dead. The survivor retains corpse HP0. Included in1.0.3; no native midboss defeat or full-stage clearance is claimed. `beads-eio.9.216` |
| 192 | [x] | P2 does not trigger Slash Beast train stop | Owner's post-Playtest40 follow-up confirms Slash Beast triggers fixed. Tester confirmation of the part2 scene, distinct from background189. `beads-t56p` |
| 193 | [x] | P2 does not trigger the other Slash Beast train starting | Owner's post-Playtest40 follow-up confirms Slash Beast triggers fixed. Tester confirmation of the part2 scene, distinct from background189. `beads-t56p` |
| 194 | [ ] | P2 does not trigger Dragoon part1 fireballs and shaking | Owner confirms ordinary fireballs work in1.0.5. The post1.0.6 first-slope recording exposes a separate recurring rolling-rumble locality bug. The bounded candidate restores that native command inside the emitter's authored window and verifies all7,082 unchanged input/state hashes. [First-slope evidence](COOP_106_SLOPE_RUMBLE.md), `beads-eio.9.225`. Earlier [placed-fireball fix](COOP_105_STAGE_FOLLOWUP.md), `beads-eio.9.221`; the fresh end-of-section audit remains open under `beads-t56p`. |
| 195 | [x] | P2 does not trigger Dragoon part2 platform fireballs | The first pillar sequence is a distinct type34 controller checking only P1 X>576. Allow living P2 to arm its original timers. Native A/B: old P2 leaves the pillar intact; either corrected leader triggers the authored X192/320/672 fireballs and removes the same16 pillar tiles. Both rosters retain32HP. Position protection isolates this event from traversal survival. Prepared for the next release. [Evidence](COOP_105_STAGE_FOLLOWUP.md). `beads-eio.9.221`. |
| 196 | [x] | Intro part1 P2-led explosives leave P1's entrance-slope camera lower | Reproduced in1.0.2: a distant P2 event shakes the wrong canonical camera and accelerates P1's downward follow when slope bounds open. Keep the one explosion controller in its progressing hero's view and publish only nearby native shake commands. Original/candidate slope observations and both native leader controls pass. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.215` |
| 197 | [x] | P1-led intro explosions/shake absent in P2's pane | Native corridor controls pass with either leader. Both nearby saved views receive native commands; distant views retain locality. A temporary P2-origin controller preserves newly published P1 shake when restoring the canonical camera. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.215` |
| 198 | [x] | P2 near intro boss door sees explosions without screen shake | Native P2-led corridor now retains original explosions and vertical shake in the correct view (109 positive observations), with both heroes healthy. Both-leader and final authored360px camera limits pass. This covers part1; new part2 wall shake remains separate item212. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.215` |
| 199 | [x] | P2 does not trigger the first part2 breakable-object drop | Native local8:3 reproduction: P1 waits at144 while P2 climbs past592; the lower piece14 hangs atY304 until P1 arrives. The native table has15 valid subtypes, but the P2 predicate excluded14. Include it: the same P2-only approach now drops the piece to392 with both HP32. No object state, timer or fall completion is forced. [Evidence](COOP_FEEDBACK41.md). Published in41; tester now confirms fixed. `beads-eio.9.197` |
| 200 | [ ] | Intro part2 P2 does not spawn bees | Flying groups are emitted by the falling-object sequence (BF3B0), separately from the static type7 placements. The earlier static-enemy spawn check does not validate these groups. `beads-eio.9.197` |
| 201 | [x] | P2 stays carried by a falling intro platform | Reproduced in matched native movement: P1 falls while P2 stays attached. Preserve BED90's native negative carry instruction across P2's collision pass. Native retest gives both action7/airborne with exactly1px foot-height difference throughout the drop, then both land healthy. Windows36/36 and Linux boundary checks pass. [Evidence](COOP_STAGE_CONTROLLERS40.md). `beads-eio.9.197` |
| 202 | [x] | P2 does not trigger intro part2 closing walls | Extend the native loaded-Y predicate to the leading living partner. P2 drops into the shaft, panels enter their original closing states and both retainHP32 while living P1 waits behind. Camera-settling setup required for debug teleports. [Evidence](COOP_STAGE_CONTROLLERS40.md). `beads-eio.9.197` |
| 203 | [x] | Split Mushroom's rainbow copies turn black after P1 dies | Reproduced when P1 dies before boss initialization; later deaths miss it. Preserve canonical stage palette bindings and the native upload request during projected Mushroom initialization. Matched native local8:3 retest shows colored trails and all128 palette bytes matching the original source. Windows/Linux boundary checks cover both rosters and both stage/fortress offsets. [Evidence and limits](COOP_FEEDBACK41.md). `beads-eio.9.199` |
| 204 | [ ] | Web Spider waterfall/log platforms slow down with both players present | Local8:3 report, video vtSGsKW2dhk around9:00-10:10. Bounded native movement from432 reaches726: both together59.98fps, P1 away60.18fps, all healthy. No slowdown reproduced in that stretch; this does not clear the full moving-log section. [Current limits](COOP_FEEDBACK41.md). `beads-eio.9.200` |
| 205 | [x] | Diagnostics must identify the effective play/camera mode and release without asking testers | Native session records the actual release and local8:3 despite saved netplay preference Unified. Session metadata includes committed co-op options; asynchronous mode-changes.csv retains initialization and runtime transitions. Windows/Linux recorder and writer checks pass, covering all five mode labels without changing input-v2. [Evidence](COOP_FEEDBACK41.md). Published in Playtest41. `beads-eio.9.201` |
| 206 | [x] | Both cameras lock in Magma Dragoon's refight, resembling Mushroom's arena | Reproduced both initiating owners in the exact Playtest41 replay and controlled native admissions. Preserve the target bounds written after the shared intro ends: update saved views still in that arena without replacing their origins, current limits or native easing. Fixed local8:3 checks let both views pan during the active fight, with either player initiating and both healthy. Published in Playtest42. [Evidence](COOP_FEEDBACK42.md). `beads-eio.9.203` |
| 207 | [x] | Slash Beast can be repeatedly stunned by his weakness | Fixed in c2b74fa and Playtest41. Native matched saved-state tests with X/P1 and X/P2 retain accepted damage (52 raw/26 applied), suppress four repeated stun restarts each, and recover into phase6 with264px of actual boss movement. Windows/Linux boundary checks cover both scaling methods and excluded contexts; native coverage is local8:3. [Evidence](COOP_FEEDBACK41.md). `beads-eio.9.204` |
| 208 | [x] | P2 pause opens P1's menu; P1 must resume and P2 stays frozen | Reproduced after native two-confirmation Quit and re-entry. Clear the retired menu callback guard on stage initialization and the menu latch on ordinary gameplay entry. Fixed native local8:3 test opens Zero's menu, permits only P2 resume, blocks guest Quit and moves P2 by61.5px afterward, P1 stationary and both32HP. Combined Windows36 and Linux30 production checks pass. Published in Playtest42. [Evidence](COOP_FEEDBACK42.md). `beads-eio.9.205` |
| 209 | [x] | Default-on independent co-op menus opened together, with either player resuming both | Included in Playtest 43. Both native character menus and required private E-tank contents are implemented. Tank use blocks unpause while the partner can navigate and select equipment. Local Unified/8:3 and both LAN camera modes pass native controller management, shared confirmations, dead-owner handling and render isolation checks; offline paused snapshots and post-menu weapon fire also pass. Playtest43 retained P1 campaign authority. The1.0.3 release allows either player to initiate Quit/Escape while P1 retains Options. Native menu fade-out and world fade-in restore palettes and switch Unified layout while black. [Contract and validation limits](COOP_SIMULTANEOUS_MENUS.md). `beads-eio.9.207` |
| 210 | [x] | Surviving P2 enters the yellow teleporter but cannot trigger Sigma's door scene | Reproduced in 1.0.0: the original NPC position predicate still reads P1's corpse. Register and emit the complete entrance controller through existing scene-owner projection in stage12/section0. Native Split/Both checks pass both campaigns, both death orders and the both-living control through the actual yellow pad, scene, forward transfer and restored controls. X/P2 survivor also passes Unified/Either. Campaign, corpse health during the corridor and shared lives are preserved. Included in1.0.1. [Evidence and limits](COOP_SIGMA_SURVIVOR_ENTRANCE.md). `beads-eio.9.210` |
| 211 | [x] | P2 dies at Marine Base's final bike dismount | Reproduced in1.0.1: shared-scene carry delays P2 ejection until below the landing floor. Keep both native Chaser poses personal; native landing/control checks pass Unified and both Split rosters at32HP with unchanged lives. Also defer supplemental Jet placement until its arena is ready, fixing the widened Unified/Either continuation; actual door entry and sustained live boss pass. Windows38/38 checks pass. Included in1.0.2. [Evidence](COOP_MARINE_MUSHROOM_RELEASE_GATE.md). `beads-eio.9.212` |
| 212 | [x] | Intro part2 P2 lacks shake from falling structures and closing walls | Native first-fall and actual closing-wall commands now reach the nearby saved P2 camera. The top wall emitter covers its full432px footprint, including the shaft floor. Native H/V commands and wall retirement pass with distant P1 unchanged and both HP32; position protection isolates shake from crush survival. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.215` |
| 213 | [x] | Slash Beast cannon lasers follow P1 while P2 leads | Native emitters1658/4458 reproduce singleton-camera clamp; corrected beams retain original-119/-375 offsets. Both native emitter checks and shared-family boundary regressions pass. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.216` |
| 214 | [x] | P2-first Eregion admission pans from the beginning and softlocks | Commit the admitted hero's saved camera at the real native scene command despite WARNING freeze. Native distant-living-P1 control reaches initialized48HP boss, both heroes healthy and P2 movement restored. Included in1.0.3. [Evidence](COOP_102_STAGE_FOLLOWUP.md). `beads-eio.9.215` |
| 215 | [x] | Sigma1 P1 death locks waiting P2's camera and makes the ladder fatal | Native1.0.5 reproduces P2 death at3928/1000.5 with cameraY736. Bind and retain P2's camera during the one survivor stage-script pass. Both character rosters now release the native target bottom to2048 and land safely on the next platform at1018/1019 with32HP, preserving the dead partner and lives. Prepared for the next release. [Evidence](COOP_105_STAGE_FOLLOWUP.md). `beads-eio.9.222`. |

### October18 Playtest36 recurrence and new requests

The owner reports157 unchanged in36. Its prior acceptance is withdrawn.
New current36 archives, local Cyber video and two public video references
are indexed in [the ground-up review](COOP_FEEDBACK36_REVIEW.md). Items2/25
are reopened,107 remains open, and165-173 track the additional bugs/features.
No new gameplay fix or release is claimed by this intake.

### October18 Playtest35 Frost follow-up

Two new paired archives confirm build `c873d56d`, Split cameras, Both-player
doors and boss scaling with the default independent cooldowns. The larger ZIP
also repeats the shorter recording; it is not an independent reproduction of
that same run. Report157 persists, and the tester confirms disabling scaling
restores their expected slash behavior. Track that as an active recurrence,
not resolved by prior visual/contact-cleanup corrections. Source and validation:
[Frost follow-up](COOP_FEEDBACK35_FROST.md).

### October17 scaling cooldown clarification

The tester confirms boss scaling with separate player cooldowns. The replacement
`31 bugs (1).zip` is byte-identical to the previously inspected archive (all20
CRC entries pass). The owner requests boss hit cooldowns divided by2 generally,
including the separate mode. The old separate implementation intentionally kept
the full native timer for aerials and finishers while the shared alternative
halved it. Proving that the64-tick timer expired did not qualify that intended
scaling behavior. Both methods now use half-length boss hit cooldowns, rounding
odd durations up; Zero's first two grounded strikes retain their shorter chain
timing. Native AI/scene timers are outside this change.

Fresh hidden, muted original-game Frost refight checks compare the old and new
separate mode with X/P1-Zero/P2. Repeated short-hop aerial34 hits reconnect
earlier after the change, and native ground attacks24/25/26 inflict more damage
in the same scripted input sequence. A fresh Zero/P1-X/P2 check also passes
ground and aerial cadence. These asynchronous observations support the timing
change; they do not claim every attempted slash physically overlapped the boss.
Both-owner unit checks prove exact32-tick expiry for native64, independent
overlapping timers, odd-duration rounding, unchanged doubled survival and
snapshot restoration. Original balance remains native when scaling is off.
All35 Windows title checks pass in the ordinary and development-netplay
profiles; the Linux GCC balance suite also passes. The Mods tooltip now states
the same half-length policy for both choices.

Three completed Split LAN phases also qualify one held slash damaging once
(48->47), repeated P2 combos (48->24), and consecutive aerial damage (47->45
with47 observed frames between hits). Their25 core/mod CRC samples agree,
through matched tick6980. These are individual completed phases, not a claim
that the full four-case script passed: its later mixed-player setup rejected
a teleport for a dead, edge-parked P1. A fresh interior-position run kept both
heroes healthy but did not produce accepted hits inside the overlap window.
That extra native simultaneous-hit probe remains unqualified; independent
partner eligibility is covered by the both-owner unit cases above.

The additional [comparison video](https://youtu.be/F4QwpLa-Ofo) was supplied
during this follow-up. Its download failed with HTTP403 after the intro; it is
not claimed as a full retail comparison. The native A/B checks above provide
the gameplay evidence. Private logs/reports are under
`build-pr23/feedback33-iframe-cadence/`. These fixes are included in Playtest33.

### October17 capsule dialogue follow-up

The new report needed no replay. Hidden, muted fresh native sessions compared
the authored Web Spider boots capsule in both campaigns. A private teleport
and camera placement exposed the upper-route capsule to the ordinary placement
scanner; the test did not fabricate an NPC, dialogue index, script payload or
textbox state. Before the fix, X/P2 in Zero's campaign used the opening
HUNTER H.Q. text instead of the same capsule's DR. LIGHT text in X's campaign.
After the fix, the hologram and successive pages match the X conversation.
The full native conversation closes, X jumps into the capsule and receives
the boots (campaign armor flag08); both heroes resume normal control with
HP32, scene gates cleared and P1's Zero campaign byte still01. Earlier pickup
setup attempts stopped at the capsule base and are excluded from award proof;
the final `zero-campaign-final/report.json` and captures qualify acquisition.

The shared textbox initializer still runs in P1's canonical context, preserving
its palette refresh and campaign ownership. Only the existing text, portrait
and cue selectors use X's resources during actual capsule initialization or
while a native capsule waits on its open X-bank conversation. Continuation is
derived from the live actor, textbox and cursor, including after restoration;
there is no additional saved flag. Closed textboxes, other text banks, ordinary
Zero story scenes, non-text boss selectors and local presentation passes do
not acquire the capsule override. The optional Iris/Double setting remains
independent and can stay off.

All35 Windows title checks pass; the affected lifecycle and story-selector
suites also compile and pass under Linux GCC. Native proof covers the Spider
boots conversation in local co-op; it is not a new full-game, every-capsule or
two-peer netplay qualification. Private captures and reports are under
`build-pr23/feedback33-capsules/`. This is a post-Playtest32 fix, not a claim
that the already published package contains it.

### October17 Playtest31 paired feedback

`31 bugs.zip` passes CRC checks for all20 entries, SHA256
`2b5e73cedf171b34a9f84f07c4827a68af98b38eddbeb44a4587bbe0701297d4`.
It contains both network sessions, initial cards, settings, mod choices and
nonempty startup logs. Both identify Playtest31 profile
`a2ed2a044309d2cf51c67fbed296bd947b076bb2`, Split cameras, Both-player doors,
boss scaling and both story bosses. These network recordings are not treated
as successful offline replays. All29325 overlapping native-boundary state
hashes agree between the supplied peers; that comparison is not a full-core
or cross-build compatibility qualification.

The509-second video shows the P2-led Spider route and fortress refights.
Spider part2 begins around3:20; P1 stays at112/443 while P2 advances.
The recurrence stays on109, and new moves152/153 stay separate from the
previous opening-strike fix. No additional weapon timing change is justified
by this intake alone. The tester's report that the rest of Spider looks good
is retained as feedback, without closing unrelated unqualified cases.

### October17 Metadata8 intake

`Metadata 8.zip` passes CRC checks for all91 entries (SHA256
`3f7cc646977b490facbd04fe8fb8e99898f3a9c9a006297a1070b17091c3107c`).
It contains11 local recordings, five complete restored-state baselines,
starting cards/settings/mod choices and eight nonempty startup logs.
Although the tester calls the build30, the captured profile is
`a2ed2a044309d2cf51c67fbed296bd947b076bb2` and the startup installation path
explicitly names Playtest31. The settings select Unified cameras, Both-player
doors, boss scaling and both story bosses; the campaign is Zero's in the
inspected gameplay intervals. Items144–151 preserve all reports separately.
Bounded native comparisons qualify the hatch and survivor-lift fixes145/146.
The hover-flame defect149 is now fixed through its actual native effect33;
the earlier effect4/9 candidate did not restore it and remains removed. Other reports
remain reported or awaiting a matching trigger; the two-character story-scene
request151 is an enhancement, not a confirmed failure.

### October16 fortress feedback intake

The archive `Sigma stage 2 and 3 bugs.zip` passes CRC validation for all184
files (SHA256 `7f7839ebeaf3372943e1e699329c0340f31d6fa78d224ba5e5eb1c4a26d3d8d4`).
Its newest paired input sessions contain388233/388236 frames and identify
Playtest26, co-op profile `c342aa784feabf6c0417ab57ba5424bd8a2693f9`,
execution `93f56a5e8f8d35fdee77384cb5e3772b5dfbd9b973dcbcd76206bba14f883dc5`.
Both recordings enter stage12/section0 at frame45426 and section1 at238598.
The saved mod snapshots select Split cameras, scaling and the optional
Double/Iris sequence; saved Either doors do not establish the negotiated
door policy during every encounter. The guest startup log confirms LAN.
Items122–142 preserve each report and the elective teleporter request.
They are intake items, not claims of21 reproduced current-build defects.
Video reference: https://youtu.be/gm8QSDn5Uqg . Central intake: `beads-eio.9.157`.

The newest peer recordings match byte-for-byte across all388233 common rows;
the guest contains three additional tail rows. This does not indicate a
divergence in those samples. A coarse visual review covers all6503 seconds
of the supplied video through five timestamped contact sheets; it is not a
frame-by-frame qualification of every reported symptom.

### October16 attacker-owned boss reactions

Retail post-hit branches at80050994,80071DE0,80072510,800753F8,80082640,
800850D8 and80091340 load the campaign byte PLAY+43. The shared co-op actor
keeps that byte canonical even when its collision wrapper accepts P2's shot.
Retain only that hit's character during the current actor callback and
override the loaded reaction selector at the exact guarded sites. The
campaign, enemy AI target, attack ID, damage table and native update count
remain unchanged. A miss or rejected overlap clears the selector; the actor
boundary clears it before snapshots and the next enemy update.

Native qualification for125 compares actual R2 Rakuhouha (type36/action58):
the old build accepts a damaging hit without Owl's phase11; the patched
Split peers enter that original weakness reaction. The initial Triangle
probes produced Raijingeki and are explicitly excluded as weakness evidence.
All34 Windows checks and native Linux combat checks pass, including both
rosters, both AI seats, both attackers, misses, rejected hits and stale actor
contexts. The other reported bosses remain unchecked pending their exact
native attacks/reactions; this common correction does not prove all cases.
Additional fortress qualification uses the exact native Zero attacks: fire
type 35 triggers Frost's 1/10 reaction and ice type 33 triggers Jet's root phase 3.
With the reversed roster, P2 X's AimLaser type 6 triggers Owl's weakness and
reduces 48 HP to 44. Both rosters now have native weakness evidence; other
boss-specific reports and Zero's combo rules remain independently open.
Private evidence: `build-pr23/feedback30-owl-old-r2`,
`build-pr23/feedback30-native-after/owl-attack-controls-result.json`,
`build-pr23/feedback30-sigma23-20261016`. Tracking: `beads-eio.9.158`.

### October16 fortress admission and open-door pause

General 27 refight pads now use the entering hero throughout the original
state machine and receive the same P2-view supplemental admission as doors,
only in the fortress hub. The existing Boss doors setting also governs pads.
Both waits for both living, present heroes on the same pad; Either remains
unchanged by default. One canonical solid pass is retained. Native paired
checks cover X-campaign Both (pair and P2 survivor) and Zero-campaign Either
(P2 first and P1 survivor), reaching the actual Cyber refight checkpoint3
with matching peer state through 19013, 21637, 5317 and 7968 respectively.

Open Both-player doors permit native pause during free traversal only.
Both owners' menus, resume, and completion of the real Frost door pass with
matching peer state through 11109. Phase opening, admission, stale stage and
other native scene guards are covered separately. Windows title checks and
Linux lifecycle/split/combat tests pass. Private receipts:
`build-pr23/feedback30-final-both`, `build-pr23/feedback30-final-either-zero`.

### October16 diagnostic writer optimization

Follow the bounded background methodology from SNES Mega Man X commit 235d40bc,
shutdown follow-up 55cd11d5 and snesrecomp's a6be975 diagnostic writers.
X4 retains its v2 replay format, session directories and initial sidecars.
A portable C++ worker owns formatting, checksums and periodic input-file
writes; only copies captured at the existing input boundary cross its fixed
4096-slot queue. No worker reads live guest memory. Normal shutdown drains
accepted inputs; a full queue or write error stops capture visibly, and
wall-clock flushing also handles paused or hung gameplay.

The 10,000-frame formatting/hash/write-submission microbenchmark produces
byte-identical old/new output on each platform. Windows producer mean drops
from 1.229 us to 0.141 us (p99: 7.3 us to 0.4 us); Linux mean drops from 1.116 us to 0.084 us.
This excludes guest-memory sampling and does not establish an FPS gain.
Its explicit asynchronous barriers increase total bulk-test elapsed time;
the benefit is removing disk latency and this work from simulation updates.
All 35 Windows checks plus Linux writer/replay-boundary checks pass, including
queue wrap, owned-state mutation, stalled-worker saturation, idle flushing,
ordered controller reads, failed paths, recovery and baseline segments.
A real muted, hidden Zero-campaign co-op run records 2543 native inputs and
2256 controller reads, exits normally, and replays all 2542 complete native
intervals with matching state hashes. The standard replay helper omits the
last interval, as it did before this change. Private receipts:
`build-pr23/diagnostic30-native-capture`, `build-pr23/diagnostic30-native-replay`.
Tracking: `beads-eio.9.161`.

### Playtest30 fortress scenes and Zero combos

The native stationary/facing scene command14 previously withdrew the other
hero for every fortress presentation. Keep both nearby, living, visible
heroes on the same layer for those boss calls. Actual command15 transports,
portal callers, distant/absent/dead partners and active door/warp gates keep
the ordinary carry path. No new persistent snapshot state is introduced.
The native baseline withdraws P2 during85CA8's refill between Sigma forms;
the fixed full transition retains both bodies and controls at32HP, with
matching peer state through9920. The actual final kind74 room also admits
both healthy heroes (matching through21281). A genuine team death/retry
restores both native arrivals at528/459 and552/458 (matching through21634).
This checks arrival/retry and does not establish what the older report136
meant by missing intro dialogue or animation.

Additional local Unified checks follow the actual hub pads into Mushroom and
Jet Stingray. P1 X's Lightning Web final hit and P2 Zero's ice final hit each
complete the authored outgoing beams and restore both heroes at their native
hub checkpoints13/14, with32HP and normal action2. These real room changes
still transport the party. An earlier airborne-X stall came from the private
teleport fixture clearing acceleration at PLAYER+2C along with velocity;
native8002B694 requires that acceleration for falling. Restoring the observed
native jump acceleration before firing clears the stall. That setup artifact
is excluded from game-defect claims, as are health mutations during an
already-running weakness response.

Separate per-player boss cooldowns cap Zero's first two accepted grounded
saber strikes (native classes24/25) at4 ticks. The third strike, other moves,
X, and the legacy shared method keep their previous durations. Native P2
Zero produces the complete24/25/26 chain against Frost and Sigma's second
form, damages both, and advances the new combo counter. Peer checks match
through6946 and15809. Actual Lightning Web also produces Sigma's authored
type32/variant6 weakness effect and reduces48HP to46.

Windows35 title checks and Linux lifecycle/balance checks pass. Private
evidence: `build-pr23/feedback30-native-final`; baseline:
`build-pr23/feedback30-finish-split`. Some exploratory checkpoint/teleport
setups failed before their intended observation (including placing a hero
inside Mushroom's wall); those are excluded from gameplay defect claims.
Actual refight-return receipts: `build-pr23/feedback30-jet-native-final` and
`build-pr23/feedback30-mushroom-native-gravity`.
Tracking: `beads-eio.9.163`, `beads-eio.9.164`, release `beads-eio.9.162`.

Playtest27 native evidence: the original-disc Frost comparisons cover both
character assignments. The paired Split/Either Mushroom lift probe starts at
the authored stage3/section1/checkpoint2 and confirms both cameras follow the
ascent with matching peer core/mod state through5283. The reverse fortress
survivor case also passes: P1 X dies, P2 Zero defeats Double, and both players
return healthy and controllable at Sigma3, then move, with matching peer
core/mod state through9952. Mushroom revival checks use the native area loader
after a forced seat death; they do not claim a complete midboss playthrough.
The earlier authentic ceiling crush from undestroyed panels is unchanged.

### October16 P2-ahead and survivor feedback intake

Tracking: `beads-eio.9.148`. Items107–121 were queued reports at intake; the rows above now record
subsequent qualification and fixes. Existing item104 remains checked: the supplied
Frost ice-physics recording predates its Playtest27 fix. If it persists in27,
that would be a new qualification; no such current-build evidence is established here.

The three archives below all identify Playtest26 via co-op build
`mmx4-coop-delay-v3:c342aa784feabf6c0417ab57ba5424bd8a2693f9`, with netplay
enabled. Their saved mod snapshots select Split cameras, Either-player doors,
boss scaling and the optional Double/Iris sequence. Widescreen and HD textures
are disabled. The tester explicitly reports Both-player doors for114/116;
the saved settings cannot establish which negotiated option was active in
those moments. Preserve that discrepancy for reproduction rather than silently
changing the report. The later Sigma-facing and survivor-animation messages
are not yet tied to a specific session or video timestamp.

| Evidence | Supplied archive | Video | Archive SHA256 |
| --- | --- | --- | --- |
| A | `Minor Web Spider stage bugs.zip` | [Web Spider feedback](https://youtu.be/TuA3Kkfzbuk) | `0404cec463a03ed5cecc295a100cc099f0fc15caf83e79810eb6307098a51161` |
| B | `Forst Walrus stage bugs.zip` | [Frost Walrus feedback](https://youtu.be/UYdwjNHgfGc) | `c1dd9d67479d9324b03d4a19c70ad0bf844322c5d57dff59072e081cab4f387f` |
| C | `Web Spider water ripple effects boss door focused on player 1 camera X attack Iris no knock back or cyrstal damage.zip` | [Water effects, doors and Iris feedback](https://youtu.be/leHSU7f0uKU) | `593bace0fcc03a8b07950cae7b1ec0f277966e3bfaa458cd27aea7cf7d98bb5e` |

Archive metadata was inspected for the initial intake. Videos are linked as
supplied; no frame-by-frame review or gameplay reproduction was claimed then.
Subsequent native qualification is recorded in the individual rows. Prioritize
blocked progression at the Frost doors/platforms, then missing combat/spawns
and transition/camera faults, then cosmetic animation. Keep all individual
acceptance cases visible even where they share an owning issue.

Tester-confirmed improvements: Cyber yellow-ball capture freeze, course portal animation, Web Spider stage. Future enhancement: Cyber bullseye on P2. The clips alone do not prove code causes, infinite softlocks or persistent save-data loss.

### Playtest25 Metadata7 survivor follow-up (2026-10-15)

Metadata7 SHA256 `3633e429ea805c5e1bd2c347d93395cf2dcd2c9509d315eff1ecf07d1694caf7`
contains a137491-input local co-op recording from Playtest25, with Both-player
doors, boss scaling and the optional story-boss sequence enabled. During Web
Spider1/1, P1 is dead at112/442 while P2 traverses the stage. The old Unified
background callback continues selecting P1's opening-region mode5/offset40.
Native before/after probes at the recorded upper/middle/lower positions show
the survivor selecting mode2/offset512 instead. Split's existing per-view
controller state and ordinary P1-owned Unified behavior remain covered.

The same recording ends in Storm Owl7/1: P1 dies at2022/931 while P2 is hidden
in warp phase1. Recovery exposes P2 at its own1947/647 departure point with40HP
but action0. That phase omitted the native idle initialization already used
by the outgoing/incoming rescue paths. All three now share terrain-checked
origin recovery and initialize control; committed arrivals retain their new
room's saved point. Genuine passenger deaths are left to native death, and a
new diagnostic event identifies successful recovery.

Lifecycle regression cases fail before and pass after for both passenger
seats, all three warp phases, unsafe terrain, committed-room recovery and a
genuine double death. A native Cyber door test exercises the waiting-phase
recovery with actual outgoing animation, fatal owner health, preserved
shared lives and subsequent survivor movement. Strict follow-up checks for
both passenger seats require native idle/action2 and cleared door/script
locks before sending movement input; automatic door walking does not count
as player control. These use Either-player doors to exercise the real warp
path; Both-player door policy is unchanged. The old build also proceeds
through the separate scene-owner handoff, so that check qualifies integration
and does not claim to reproduce Storm Owl's complete softlock. The attempted
full original replay timed out before Web Spider after a bounded480 seconds;
no full replay completion is claimed. An initial far-room teleport crossed
an uninitialized camera boundary and is excluded from validation.

All34 Windows CTests and native Linux lifecycle/split fixtures pass. Private
evidence: `build-pr23/feedback25-20261015/qualification.json`, native before/after
captures, runtime logs and regression receipts. No release is cut by this
follow-up, and no new live netplay qualification is claimed.

October11's bounded follow-up for item36 now includes the preceding native
section ending. QA100 uses checkpoint skips/invulnerability, lowers the actual
Tentoroid actor33 fromHP48 to1, defeats it with original attacks, then follows
the game's normal transition into section1. Both arrive withHP32/lives2 and
walk the second/purple staircase atX400/800/1200; inspected feet and native
slope heights remain aligned. QA97 independently loads a native card with
co-op disabled and reaches that section through the original selector, also
without the reported offset. Neither run reproduces the floating pickup;
retain item36 for a matching current recording rather than inventing a fix.

Item48 additionally passes QA102's hidden sign-in page against a deliberately
unavailable loopback endpoint. The visible error's public text is present in
both the original stderr and portable startup log. No account/browser/public
lobby is used. All24 Windows checks pass; Linux console/no-console logging
fixtures pass. This checks failure capture, not the parked authentication bug.

Playtest8 packages the Mushroom fixes44/45/47, independent-view fix42,
X5 aligned-copy optimization43 and startup logs48. Production builds pass24
Windows /23 Linux checks; packaged launchers, room offers, execution identity
and co-op profile match. Both packaged applications produce portable startup
logs. The remaining open reports stay open:34/36/41/43 need current evidence;
30/31/32/46 are parked/backlog. This release adds no new WAN or broad gameplay
qualification. Release tracking: beads-eio.9.69.

Owner's subsequent October11 scope update parks further staircase reproduction
(items34/36) pending a future matching diagnostic recording. The supplied ZIP
passes its CRC check and contains39013 sequential native input rows, but only
Web Spider stage1 sections0/1, with netplay=true. Its captured local diagnostic
feature is disabled. Netplay records through the co-op profile regardless of
that local toggle; a later local run with diagnostics disabled would not record.
This is consistent with an older netplay session being sent, not proof of
which later run the tester played.

For the next local Mushroom run, enable Playtest Diagnostics and confirm REC
on the input display. After reproducing the second/purple staircase issue,
quit normally and send the complete session folder identified by
diagnostics-location.txt, plus the matching video and approximate problem time.
Do not attempt offline replay of the older network trace as a Mushroom test.

The remaining evidence audit leaves all eight unchecked reports open:

| Items | Next action |
| --- | --- |
| 30/32 | Discord/WAN investigation parked at owner request; collect startup logs if it fails again. |
| 49 | Missing staircase shots remain unreproduced after bounded native checks; retain for a matching recording. No damage qualification. |
| 34/36 | Future matching Mushroom recording; no further speculative staircase runs. |
| 41 | Current diagnostic session if the intro-act2 WARNING failure recurs. Stored native results show warning retirement, Eregion and survivor controls, without establishing the older Playtest6 cause. |
| 43 | Current slowdown scene, settings and hardware alongside diagnostics. The X5 optimization is shipped; controlled runs finish near60Hz, without proving every performance report resolved. |
| 46 | Later elevator report parked pending a specific current failure; earlier elevator fix/ascent/exit qualified. |

No gameplay tests are running. PR17 is closed as superseded and PR23 is merged.
The requested X6 counterparts exist: stacked-HUD PR41 and voice issue42,
with central voice tracker beads-blqg. These are separate X6 follow-up artifacts,
not claims that X6 gameplay bugs were tested or fixed here.

New independent reporter feedback extends item31's local disappearance family:
P2 vanishes at the intro dragon boss and Web Spider waterfall in Playtest7,
and still does so in Playtest8, while playing local co-op with Parsec.
Online behavior is unknown. The version, mode and repeated location hints
make targeted reproduction actionable, but the exact waterfall, player
characters, HUD visibility and whether P2 returns or dies are still unspecified.
Request a matching video and complete Playtest8 diagnostic session with REC
visible. Treat shared cause with the earlier reports as unproven; the backlog
scope remains unchanged until active reproduction is requested.

The owner could not readily obtain those details, so bounded hidden local
probes resumed for item31. QA105 qualifies P2-led dragon entry and a short
native combat sequence with both HP32, active bodies and HUDs, failed0.
Its midair-teleport water samples leave Zero floating and are excluded from
the disappearance conclusion. QA106 instead walks from native starts and
checkpoints without teleporting: both cross the first water and descend the
secret waterfall, retaining bodies/HUDs and failed0. P2 takes a real hit32->29
in the first route. Inspected captures confirm both sprites and HUD groups.
This does not cover an unspecified waterfall or the reporter's full prior route.

New source logging records co-op disable reason codes and partner-presence
transitions with scene/withdrawal/warp state, health and positions. These use
the existing automatic portable startup log even when input diagnostics is off.
The changes are for a subsequent build, not the published Playtest8 assets.
The report remains unchecked; logging is not a disappearance fix.
Commit c97a16e is on master. All24 Windows checks pass. QA109 performs native
Select withdrawal/rejoin and records all four complete presence transitions
in the portable log, preserving HP32/lives2/failure0 and exiting normally.
The fatal-failure message is compiled but no renderer fault was forced in
these bounded cases. All owned hidden processes are stopped. Further exact
disappearance qualification awaits a matching occurrence.

## October 12 supplied-video and archive evidence

Full timestamped assessment and next actions:
[COOP_FEEDBACK_20261012.md](COOP_FEEDBACK_20261012.md), `beads-eio.9.70`.
Existing item numbers remain stable; new missing-shot item49 extends the
checklist to49 rows, now41 checked after qualifying item31's reproduced
weapon-reuse case. Other unresolved gameplay reports remain open.

Both newly supplied ZIPs are CRC-valid and contain only zero-byte startup
logs: 3 in `New Compressed (zipped) Folder.zip`, 15 in `Diagnostics.zip`, with
the first three names shared. Neither contains inputs, session metadata,
cards or mod/settings state. No REC indicator is visible in the reviewed
gameplay, so the missing replay cannot establish whether input diagnostics
was enabled. Automatic startup capture did fail to provide evidence. Its
Windows no-console FILE/descriptor failure is independently reproduced and
corrected for Playtest9 under `beads-eio.9.71`; the earlier item48 fixtures
did not qualify these ordinary GUI launch conditions. Production no-console
launches now capture both startup and subsequent application errors, including
a live launcher error. All31 Windows title checks and four Linux logging
cases pass. The separate adversarial review found no blocking issue and led
to a retained assertion of the detached pointer-failure error text. Published
Playtest8 binaries remain unchanged. This is not a gameplay fix.

YouTube YTcB2JQEFdM shows a Playtest8 Windows package folder at 00:02, native
Continue/slot1/Data1, then Jungle. At its opening waterfall, repeated P1
weapon cycling/firing precedes the removal of P2's body and lower HUD:
present at 01:48.43, absent by 01:48.47 in sampled frames. P2's health bar
looks full immediately before removal; P1 stays active in the same scene
through the remaining visible gameplay. No immediate death or room change
is apparent. Input-driven withdrawal and co-op failure cannot be distinguished
from this video alone; the independent item31 reproduction and fix are below.
The large green cone around 01:26–01:28 may be a native aiming effect and
is not classified as a new graphics bug.

IMG_0786.mov supports the owner's separate missing-shot stairwell report:
an X-like charge effect ends around 00:07.02, followed by an apparent
right-facing buster pose with no clear travelling shot in the visible area.
Both bodies/HUD groups remain present. New `beads-nbnc` tracks the visual
symptom; actual input release, shot allocation/rendering and damage remain
unqualified by the supplied clip. This does not qualify the older
floating-staircase, teleporter or elevator reports. Native follow-up after the
initial media assessment is recorded below.

The owner authorized focused reproduction of items31/49, then Playtest9 after
the disappearance fix is validated. QA111/112 reproduce the Jungle removal
after weapon cycling with both HP32 and failure8 (`actor-texture`), at271–281
sprite banks. QA115 uses only native R1 cycling and reproduces the same failure
after returning to the buster, excluding the weapon-selection fixture as its
cause. The rejected reused P1 slot holds Gravity Well's compressed family9
pointer (PL00 offset2BE04; native and immutable packed entries agree), while
buster flags21 select static artwork in the original renderer. The private
renderer incorrectly decoded that stale pointer. Its source fix now follows
flags20/40 and uses the existing raw-artwork/owning-palette path. Cache
exhaustion and the green aiming cone are not this diagnosed failure.

QA116 passes two P1 weapon cycles after the fix. Strengthened native R1-only
QA117/X-P2 and QA118/X-P1 each pass18 attacks, including visible full charged
buster type0x12 with native static-artwork flags; both bodies remain healthy
and active, enrollment persists and failure0 is asserted throughout. QA118's
second-cycle P1 buster retains stale table801A47A4 yet the projectile and both
HUDs remain visible. All three runs exit normally0. Item31 is now checked for
this reproduced weapon-reuse case. Fix1903e75 is on master and qualified for
Playtest9; other original disappearance locations are not independently
qualified by these runs.

The item49 attempt stays unresolved and deferred. QA113 has visible ordinary
and three charged travelling X/P1 shots on green and purple stairs; QA114 has
the same X/P2 shots on purple second-section stairs, including inspected
`stairs-1-400-1-True-2.png`. QA114's first-section development-induced hurt lock
is excluded. No projectile damage was qualified, and the exact supplied
missing-shot state remains unreproduced. Other parked items remain parked.

Both final production builds pass 31 Windows and 25 Linux title checks; hidden
packaged launcher/room-offer checks pass. Final Windows unredirected logs
contain 655/658 bytes and the redirected case 659 bytes, including the real
application error while preserving redirected stderr. [Playtest 9](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.9)
is published with Windows ZIP and Linux AppImage; uploaded asset checksums are
verified. This does not expand the parked WAN/Discord qualifications.

October12 follow-up recordings (`Boss weapons glitched.zip`, `Frozen Zero.zip`)
are complete Playtest9 local sessions, including initial cards/settings and
nonempty startup logs. Their published-executable replays verify5,755 and14,704
input/state boundaries respectively. Private development inspection changes
only the copied recording's build label and retains all original state checks;
it is additional visual/control inspection, not the production replay result.

Item50 exposes a different defect from item31: the native sprite emitter adds
an actor's VRAM tile allocation (for example0520/0530/0540), while its private
decompressed texture starts at zero. The renderer now supplies a local tile
origin during packet emission and restores the original allocation immediately
afterward. This restores the missing web and ice sprites and malformed Ground
Hunter artwork in the recording without changing any of its5,755 state checks.
The native P2 R1 weapon-cycle regression passes18 attacks with both players
retained, and all31 Windows title checks pass. A separate single-player run with
the co-op package disabled cycles through all nine weapons: the large circular
charge glow, Aiming Laser reticle/cone and charged wind/gravity shapes also
appear there. Those appearances alone do not establish a co-op regression.
The corrected co-op sprites are published in [Playtest 10](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.10).

For item51, both original and development replay reach the recorded end with
Zero alive at31HP. The last1,135 boundaries contain no P2 directional, jump or
attack buttons; six contain Start and the remainder are neutral. In development
inspection, resuming normally and injecting P2 Right moves Zero from
X2096.8125/Y189 toX2162.3125/Y173; Jump then reachesY153.43. No collision or
staircase workaround is warranted by this evidence. The cause of the missing
controller input remains open.

Item52's Playtest8 material was initially deferred, then qualified after the
owner requested continuation. The original executable verifies all7,876 intro,
7,379 capsule and6,720 waterfall input/state checks. None of these recordings
contains P2 Select withdrawal input.

| Clip | Observation | Assessment |
| --- | --- | --- |
| `2026-10-12 17-31-15.mp4` (43.37s) | X/P2 and its HUD disappear around23-24s during the intro Eregion fight; Zero/P1 remains through the post-fight dialogue. | Reproduced cache exhaustion:4,096 sprite banks, failure8 at frame11796. Fixed in source; P2 remains active and moves after this boundary. |
| `2026-10-12 17-34-40.mp4` (32.43s) | Jungle capsule sequence in Zero's campaign leaves only Zero/P1 and its HUD after the room restarts. | Reproduced rejected shared palette78C0 at2,911 banks, independently of exhaustion. Fixed in source; both sprites remain after the upgrade sequence. |
| `2026-10-12 17-39-37.mp4` (16.79s) | At the waterfall passage, neither body remains visible while both HUD groups persist; P1's health is empty. | Reproduced: P1 HP0, P2 HP11, enrollment1/failure0. P2 stalls atX6360.5/Y427 because the exit checks dead P1's position. Fixed native replay advances to area2; an immediate survivor-control probe passes. |

`metadata.zip` passes all CRC checks and contains15 local sessions with cards,
settings and input traces, including12 save-state baselines. Its three empty
startup logs are the known Playtest8 logging defect corrected in9 (item48).
The two new rendering fixes are distinct from item31's stale buster pointer.
Textures already live in host memory; the exhausted resource was a4,096-ID
lifetime cache. Full pools now recycle only IDs absent from both world and HUD
packet buffers. Pending GL work drains before a recycled ID is redefined.
Victim selection uses saved guest memory, preserving the snapshot layout and
deterministic reconstruction. Capsule effects outside the private character
palette use their authored native VRAM palette.

Fixed-source inspection retains the recordings' original checks until the
expected first differences at intro input5124/frame11796 and capsule
input5785/frame5413. These are changed gameplay outcomes, not claims that the
entire buggy recording still matches. Separate follow-up control tests keep
both bodies, exercise P2 movement/jump, and successfully save/load. Intro
observes404 cache reuses; inspected before/jump/restored captures retain both
sprites. All30 title checks in this debug configuration pass, including20,000
safe replacements across the four texture pools and restored-packet retention.
All three fixes are published in [Playtest 10](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.10).

The waterfall is a separate scene-ownership defect. At the recorded end,
destructible exit actor80165F1C (kind6/variant0) is in native state4. C165C
redirects its outgoing-walk command to surviving P2, but C169C still tests
the singleton P1's X against6336. Dead P1 is atX5039; P2 has already reached
X6360. The new filters bind those two phases to the same scene owner while
leaving the intact obstacle's collision/update path unchanged. Both game
configurations include the emitted callbacks; regeneration changes one shard.

Fixed native replay first differs at input6052/frame9333 when the exit now
completes, and reaches section1. The generic two-living-player probe initially
waited through the survivor's later death before testing a team respawn, so
that run does not qualify immediate survivor control. A separate hidden probe
restores the previously verified stuck state, crosses the unchanged native
exit, and immediately checks X/P2 arriving at32HP, moving and jumping in area2.
Zero/P1 remains dead throughout; no camera-clamp bypass or forced resurrection
is applied. Inspected arrival/jump captures show the surviving X. All30 Windows
title checks pass, including the new exit ownership/commitment regression.

## Metadata 2: intro rescue and acquired armor

The new archive contains three local Playtest10 sessions and two nonempty
startup logs, with no explicit fatal error. The supplied Zero-campaign baseline
replays all18,693 retained original input/state checks on the unchanged build.
Both players survive its intro fight, so that recording does not exercise the
reported X-only victory. The owner cannot distinguish desktop exit from freeze
from the secondhand report; those outcomes remain separate.

A bounded native fixture reproduces item54. Zero/P1 dies through the original
death path; X/P2 crosses the actual intro event and WARNING, then defeats
HP1 Eregion with native attacks. Unchanged Playtest10 remains in gameplay with
the scene gate set for over4,900 frames. Sprite-pool rescue actors44/46 read
the dead P1 body outside the existing enemy/script projections: D12C0 sets
the camera limit to P1.x-48=3888 while the arena minimum is4444, and D14F8
waits for that unreachable position. Filtering their D1248/D1284 updates
through the committed scene owner preserves the canonical campaign while
using the surviving body. The fixed run reaches mode9/MISSION COMPLETED,
with Zero's campaign unchanged and P1 still at0HP. Its screenshot was inspected.

Private evidence lives under `build-pr23/qa-playtest10-metadata2/`:
`inspect-zero-baseline-original`, `zero-alone-victory-2` (old behavior),
`zero-alone-victory-3/live-pools.json`, and `zero-alone-victory-fixed-1`.
The private runner disables frequent captures and allows30 seconds without a
heartbeat because an earlier capture-heavy run hit the host's four-second
watchdog before the door. That earlier timeout is not evidence of item53,
and production watchdog behavior is unchanged.

At the owner's request, a second reproduction branches from the tester's
actual recording: all4,735 original input/state checks pass through the
pre-door segment, then a saved state preserves both bodies atX3728. The branch
kills Zero/P1 through native death without repositioning the players and drives
X/P2 through the event. After X's native boss kill, unchanged Playtest10 remains
locked for3,666 more frames. Its camera target3680 is below the arena minimum4444,
confirming the same cause in the tester's state. This is a deliberate change
after a verified replay prefix, not a claim that the altered continuation
matches the original recording. Evidence: `recorded-pre-door-branch-1` and
`recorded-p1-death-old-1`.
The fixed branch also defeats Eregion. A debug-connection timeout interrupts
the helper immediately after the kill; its captured post-kill state resumes
on the final build and reaches MISSION COMPLETED with campaign1 and P1 HP0.
The final capture was inspected. Evidence: `recorded-p1-death-fixed-1` and
`recorded-victory-final-1`. Final executable SHA256:
`467f47498faab53fc58eca42983366bbff3e6a2b53b76a1bba89a18d78db7ce7`.

Item55 has a separate rendering cause. Shared permanent armor flags already
reach X/P2, including after the previously captured Zero-campaign capsule and
save/load. The co-op renderer called the native body emitter80024334 but
omitted acquired-armor emitter800257BC. Include its packets in the same private
texture conversion, retaining the local compressed tile origin until both
passes finish. Only the projected P2 body gets this added call; P1's original
caller already emits armor after its body filter returns. No P2 campaign/save
record is introduced. The retained
`x_armor` native scenario covers unarmored X, boots, helmet, body, both arm
variants and full armor, with captures for visual inspection.
All seven configurations pass in both rosters. P1's control case uses native
stage initialization because a direct PLAYER write can land during a temporary
P2 projection. Inspected full-armor and arm-variant captures show the expected
armor; the final X-campaign run also passes P2-led native intro entry with
both players visible and healthy. Evidence: `armor-fixed-1/zero-campaign` and
`armor-p1-and-door-fixed-2` beneath the same private evidence directory.
The final build's `armor-p1-final-1` repeats all seven P1 configurations and
confirms the added private armor pass never runs for P1; its full-armor capture
was also inspected.

All32 title checks passed together before the final P1 armor guard. The final
build passes31/32 on its first serial run, with the unchanged redirected-log
fixture passing its isolated rerun. An earlier parallel run also exceeded the
startup-log fixture's three-second live-observation deadline before passing
isolated and full serial reruns. Record these timing failures rather than
claiming every run was clean. Hook registration and Python syntax checks pass.
These fixes are published in Playtest11.

The owner's final bounded item53 probe (`build-pr23/qa-playtest11-entry`)
passes three native Zero-campaign P2-owned entry cases: both players alive,
P1 killed during the actual WARNING actor's display, and P2 killed during
that display. Each reaches the live Eregion fight with scene/door gates clear
and the surviving body healthy and controllable; the dead seat remains dead.
No desktop crash or entry softlock is reproduced. Item53 stays unchecked,
and the owner authorized proceeding with Playtest11 on that result.

Playtest11 publication is complete: Windows ZIP, Linux x86_64 AppImage and
SHA256SUMS are uploaded, with GitHub asset digests verified. Compiled source
`c6d8ea8` includes gameplay fix `ef34a19` and is integrated into `master`.
Production checks pass32/32 on Windows and26/26 on Linux. Both packaged
hidden launchers expose two players and Netplay; default/code-enabled room
offers, execution identity and co-op compatibility profile match across OSes.
Portable startup logs pass, including three Windows no-console/redirected
launch modes. Developer controls are off. Item53 remains open; publication
does not turn its bounded non-reproduction into a claimed fix. Private release
receipts: `build-pr23/release11-qa/release-receipt-final.json` and
`build-pr23/release11-qa/publication.json`.

Playtest12 publication is complete: Windows ZIP, Linux x86_64 AppImage and
SHA256SUMS are uploaded, with GitHub asset digests verified. Compiled source
`c0402c2` includes Lightning Web fix `8e0be3f` and is integrated into `master`.
Production checks pass32/32 on Windows and26/26 on Linux. Both packaged
hidden launchers expose two players and Netplay; default/code-enabled room
offers, execution identity and co-op compatibility profile match across OSes.
Portable startup logs pass, including three Windows no-console/redirected
launch modes. Developer controls are off. Item56 is complete; unrelated
unresolved reports retain their prior status. Private release receipts:
`build-pr23/release12-qa/release-receipt-final.json` and
`build-pr23/release12-qa/publication.json`. Release tracking: `beads-eio.9.80`.

October 13 follow-up (items 57-60): `Kill cam.zip` contains a complete local
Playtest12 recording and both populated startup logs. The unmodified compatible
debug build verifies all 43,365 original input/state hashes. Its saved checkpoints
allow short, explicit branches rather than another full-stage playthrough.

Item57's native latch predicate 800C36E8 reads the canonical body through a2.
The existing both-living-player wait guard skipped dead-P1 cases. Projecting the
living P2 through this one native predicate retains the canonical campaign and
original shared gate/closing animation. In the checkpoint A/B, P1 stays dead at
y2507. P2 is explicitly placed above the actual latch and made invulnerable to
isolate closure: the old latch stays open; the fixed latch closes and P2 lands
at y2202 with unchanged HP. This is a deliberate branch, not an unmodified
continuation of the recording.

For item58, the exact saved-state excerpt retains 249 original input/state hashes.
At global frame38861, native collision routine 8002D9BC writes HP2 -> FE at
8002DC1C (subtract 4), then marks death 80 at8002DC3C. The camera is still at y1923
before the fatal hit and moves toward dead P1 only afterward. Nearby enemies
are visible in the raw frame capture. The reported ordering is not reproduced;
this recording shows damage causing death followed by a camera drop.

For item59, original GPU packets show Frost Tower assembly rows starting 16
pixels apart but flipped quads ending after 15 pixels. Native 80024754/800247A8
clip UV endpoints at texture-page edges; the private origin introduced for
item50 makes the first-row seam repeat. Convert compressed assembly quads to
complete 16x16 private tiles and reflect the pixels within each tile. Palette,
blend flags and actor positions remain native. Existing bank keys use aligned
origins, leaving two low bits for reflection; old snapshots retain their
unreflected tile/full-frame banks. All 256 tile origins and 4 reflections are
checked, including the captured clipped Frost Tower packet. The candidate
retains all 349 original gameplay hashes in the visual excerpt after an explicit
private build-header translation; it is not described as an unchanged-build
replay. The old and corrected raw captures were inspected.

Private evidence: `build-pr23/feedback-killcam-20261013/`, including
`inspect-original`, `latch-old`, `latch-fixed`, `death-excerpt-2`,
`weapon-excerpt-2`, `weapon-fixed-2`, `jet-before`, and `jet-after-1`.

Item60 now enrolls P2 during the authored Marine Base acceleration, starting
32 pixels behind P1. Each bike runs its native entry animation and shares the
existing READY actor; ordinary checkpoint/story locks still cannot enroll P2
early. Carried deaths and withdrawn players retain their lifecycle rules.
The retained `jet` scenario checks both native entrances before control, one
READY actor, control release, and a dead P2 staying dead in section2. Both
X and Zero campaigns pass (`jet-final-x`, `jet-weapons-final-zero`).
The latter also passes all18 attacks in the native P2-X weapon cycle without
losing either player or disabling the renderer. All32 Windows title checks
pass after the final changes. Final hidden development executable SHA256:
`76a261b6cc74ef92c57924d2d85f45d8465028c6681574c34154c44aed6b942e`.
These source fixes have not been published as another playtest release.

2026-10-13 camera/QoL follow-up: item58 now follows the clarified report: the
unwanted camera movement **after** fatal damage is itself a bug. The corrected
camera gate holds through both gameplay and the native termination minor mode.
It still permits shared scene pans and the loader's normal respawn camera.

For item64, the native green/purple staircase controllers (800B7078/800BA57C)
advance their shared checkpoint once. Their animation helpers
(800B7140/800BA644) now run with separate per-view animation records and camera
layers. The `--stairs-only` hidden LAN regression exercises either moving seat
on each staircase. All three animation phases cycle and full peer state stays
identical through sim9061. The original native baseline leaves P2 phase0
unchanged as P2 walks up the stairs. The supplied paired logs also match233
component digests through sim8704; this was a camera presentation defect, not
evidence of simulation desynchronization.

For item65, X's native Nova Strike (action34, entered by800375B4) sets PLAY+1C.
That flag had incorrectly selected shared cameras and suppressed private
rendering. Recognize the personal attack separately while retaining actual
scene ownership. Ordinary simultaneous arrivals also render their private HUD
before the first camera tick, using the authored starting camera read-only.
The valid pre-fix `nova-before-2` run has both players alive: cameras collapse
to367/367,412/412,548/548 and private rendering stops during Nova. Corrected
`nova-fixed-2` and `nova-fixed-zero` runs perform the native attack with X in
either seat; the idle partner camera remains0, both view counters advance,
and peer CRCs match through5154/4672. Both runs pass the existing view isolation
probe (RAM/VRAM/device preservation). The earlier `nova-before`/`nova-fixed`
fixtures put the partner off safe ground and are excluded from conclusions.
`screenshot_hires` reads the canonical pre-present buffer in this runtime, so
those captures are not presented as visual proof of the displayed private HUD.

For item61, native Jungle door runs cover either waiting seat and either sole
survivor, under both default Either and optional Both policies. Waiting stops
only the door admission query; the native door still renders and updates.
The existing unit suite covers withdrawn-partner exemptions. Eight native
room-offer queries verify distinct agreed settings and preserve offline
preferences. All32 Windows checks pass. Private receipts are under
`build-pr23/feedback-qol-20261013/`. None of this follow-up is released yet;
items62/63 remain open.

2026-10-13 wraparound follow-up (item68): the supplied paired Jungle inputs
are byte-identical (SHA256
`6ce411a9a44db635b23db33f537a56aca76a1f0f4c24a57e5763b89327be79fa`).
At F6236 the players are exactly2048px apart: X1917,1467 and Zero3965,1333.
The old renderer keeps actors drawable if either view sees them, then submits
their far screen coordinates to the GPU's signed11-bit position decoder.
Thus the distant body, weapon effects and enemies can wrap into the other view.

`wrap-before-3` uses native Jungle checkpoint2 and synchronized development
teleports to observed safe positions1917,1467 and3965,1338. Real composed
`present_shot` captures show the duplicate characters on both panes.
`wrap-fixed` repeats those positions and swaps the leading seat: each pane
shows its own player and nearby enemies without the distant duplicates.
Both characters retain their world positions and health; peer CRCs match
through sim5889 before and5891 after, and render isolation checks pass.
All32 Windows checks pass, including signed offsets, vertical wrapping,
per-layer cameras, the armor filter, projection and screen-relative exclusions.
The failed earlier checkpoint0 setup clamped P2 before the lower route loaded
and is excluded. Hidden-window QA now disables host controller shortcuts only
in its private settings because development packets can otherwise open F7.
No player preferences changed. Receipts remain under
`build-pr23/feedback-qol-20261013/`. The intro dragon remains item66; this
player-wrap reproduction alone does not qualify that separate scripted encounter.

Item66 is now independently qualified by `dragon-native-route` and
`dragon-projected-attack`. Entering before the first encounter atX1200 and
walking normally preserves the original native background phases. Before,
the same boss appears at the left edge of both composed panes (sample66).
After, samples77-79 show the native background attack at screen38,180 only
in the progressing player's view, including its attached muzzle effect;
the partner stays at224,426 with its own nearby enemies. The shared actor
still reports layerFF and unchanged native coordinates outside each draw.
Both peers match state through sim5444, and local rendering verification
reports no RAM, VRAM or device leaks. Thirty-two checks pass; targeted tests
also retain unrelated screen effects and avoid fractional camera rounding.

Item62 is qualified by the `story_bosses` original-disc scenario in
`tools/coop_story_scenarios.py`. `story-both-x-final` completes Double then
Iris and enters stage11/1 at frame14136; `story-both-zero-final` completes
Iris then Double and enters Zero's normal movie route at frame14963.
Each records exactly two native defeats and one handoff, with lives2 and
P1's campaign identity preserved. `story-default-x-final` leaves the toggle
off, defeats only Double and enters stage11/1 at frame11089 without a handoff.
The fixture lowers boss HP, then actual charged-buster/saber hits perform
the kills; Iris's separate vulnerable core must first be exposed normally.
Captured fights show both native boss sprites. All33 Windows checks pass,
including option defaults, lifecycle resets, snapshot state and hook contracts.

The optional second encounter uses the native inner-door checkpoint, so
players walk through that door again. It does not introduce a P2 campaign
or modify the canonical character byte to select the extra fight. Stage
banks, admission, text and portrait/voice indices use scoped register hooks.
The original loader is scheduled normally rather than called recursively
inside a host callback, since disc loading can suspend. The apparent final
Double stall was a40-second test timeout during his388-character speech;
allowing the native text to finish completes the sequence. Private receipts
remain in `build-pr23/feedback-qol-20261013/`.

Playtest 13 published Windows ZIP and Linux AppImage from `f459bf0`.
Production builds pass 34 Windows and 28 Linux checks; packaged launchers,
eight complete option offers, execution identities, AppImage layout and
portable startup logs pass. Uploaded SHA256 digests match the tested assets.
Release receipts: `build-pr23/release13-qa/`. Parked reports retain their status.

## Metadata 3: Magma Dragoon reward input

The 41.12-second clip shows actual Magma Dragoon's defeat in Zero's campaign,
with X/P2 surviving and Zero/P1 dead. Dialogue and the victory warp complete;
the clip ends on RYUENJIN's instruction text. It does not show a process crash
or the intro dragon. Current Playtest13 gameplay source reproduces the input
lock: after native P2 attacks defeat HP1 Dragoon, P1 advances the reward to
mode7/1/5 but repeated P1 confirmation cannot dismiss its text.

The lifecycle's gameplay-scene predicate incorrectly includes mode7, the
weapon/technique reward frontend. The retained P2 scene owner replaces its
text input with the last gameplay pad snapshot, which no longer refreshes.
It can also project the survivor over the frontend's stage-actor pass.
Exclude this mode from gameplay ownership so P1 retains the campaign's live
input and presentation. Both-campaign unit cases cover a dead P1, living P2,
stale P2 script/input and unchanged canonical campaign/body state.

The ZIP contains one 31-row startup recording and three restored-state
segments, with 48,399/25,471/16,002 co-op reads but no native frame/hash rows.
Their captured mod choices explicitly disable Input Display and Recording.
An observer registered for co-op nonetheless opens new offline recording
folders on restore. These segments cannot be verified as input replays;
the native reproduction uses a documented fixture, not an invented replay.
Guard that observer with the committed Diagnostics selection. The new test
verifies disabled restore creates neither a recording nor a session folder,
while existing enabled-baseline replay checks still pass. Future testers
should enable Playtest Diagnostics before reproducing and include the full
session folder.

Private evidence is under `build-pr23/feedback-metadata3-20261013/`.
`current-native-2` records the pre-fix input lock. The first fixed native run
passed the reward and saved, then the helper's rapid confirmation entered
Web Spider before it sampled the selector. A separate initial reverse-roster
run hit a screenshot request during a native display-disabled fade. These
are test-harness failures; the retained scenario now stops confirming while
the selector builds and avoids capturing that reward-screen fade.

Final native runs `final-zero-campaign` and `final-x-campaign` both pass.
P2 defeats the native boss while P1 stays dead; P1 alone advances the reward
and memory-card UI. The selector shows Dragoon grey, with clear bit8 retained
in the card and Continue cache and campaign1/0 respectively. Both selector
captures were inspected. The reusable `dragoon_survivor_reward` scenario
records the complete sequence. All34 Windows checks and Python syntax checks
pass. These fixes ship in Playtest14, not Playtest13.

Playtest14 production packages compile source `badd2f6`, including the fixes
in `758902a`. All34 Windows and28 Linux checks pass. Both packaged launchers,
eight native room offers, execution identities and portable startup logs pass.
Fresh installs on both platforms each capture181 native input records with
Diagnostics left at its enabled default. An older explicit disabled preference
is preserved; re-enable Input Display and Recording before sending new reports.
Linux's writable player guide includes the screenshots and dependency notices.
Release receipts: `build-pr23/release14-qa/`; tracking: `beads-eio.9.96`.

## Metadata 4: Playtest13 follow-up

The tester reports a surviving P2 unable to leave Storm Owl part1, acquired
X armor taking stage colors in Zero's campaign, and Web Spider's lethal web
moving the surviving player into the trap and damaging them. Items25,71,72
track these separately. Playtest14 changes reward-frontend ownership and the
disabled-diagnostics restore observer; it does not establish a fix for any
of these three reports. The earlier generic death-lock fix and missing-armor fix
remain valid for their recorded cases, with this newer evidence kept open.

The supplied ZIP contains seven local-co-op Playtest13 session folders,
each with a complete baseline snapshot, two starting cards, settings and
mod choices. Diagnostics is explicitly disabled in all seven. Across the
input files there are475,112 co-op reads but zero native frame/hash rows.
They cannot be verified as input replays. This is the incomplete-segment
problem fixed as item70 in Playtest14, not evidence that logging regressed
in Playtest14. The ZIP contains no videos or startup logs.

Read-only snapshot inspection finds Zero's campaign in all seven states:
one at Jungle stage1/section1/checkpoint5, four at Storm Owl stage7/section0
(three restore the same checkpoint2 state), and two at Cyber Peacock
stage6/section0. The stage7
states have dead P1 and living P2; the Jungle and stage6 states have both
players alive. Armor flags are8 in five states and
14 with arm variant2 in the stage6 pair.
These snapshots remain useful starting points even though the intervening
play is not recorded. Their contents and the original archive are unchanged.

The saved co-op choices enable Both players boss doors, boss scaling and
sequential Double/Iris fights. The tester confirms the new door policy and
double boss health work well; record that positive feedback for items61/63.
They did not explicitly confirm the sequential story-boss option.

The old `storm_warp` helper was found to load Snow Base/Frost Walrus's native
stage2, not Storm Owl's stage7. Its earlier qualification was therefore
incorrectly attributed. The helper is now named `snow_base_survivor`, and
the combat evidence document is corrected. The two new runs of that old
fixture are not Storm Owl evidence; both lose P2 during scripted miniboss
combat and retry. Neither establishes the reported living-survivor lock.
The initial supplied-state load attempt failed because the development build
enabled `MMX4_COOP_NETPLAY_DEBUG`, which adds snapshot fields. Rebuilding with
ordinary TCP debug tools enabled and that netplay-debug option disabled loads
the supplied states successfully through the save-state menu command after
boot. Subsequent probes start from these snapshots and use explicit offline
health/position fixtures; they are not replays of the missing original inputs.

Item25's `storm-exit-ground` restores the actual Owl checkpoint and moves the
surviving P2 to the final floor at6310/380 (settles427). Walking right passes
the exit and falls out of bounds; P1 remains dead at438/520. Native background
controller800DA440/44C compares canonical P1 X with6352 outside the projected
stage-script pool. A guarded instruction hook substitutes living P2's X only
when P1 is unavailable. A P1 control run additionally exposed completed warp
state3/positive HP being held in the corpse-survivor loop. That exception now
requires zero HP, allowing the native stage-termination branch to run.
`storm-exit-fixed` crosses the same threshold and finishes at stage7/section1,
with dead P1 and playable X/P2 refilled from14 to34HP. The arrival screenshot
was inspected. `storm-control-fixed` also completes a P1-led exit with both
players alive and playable in part2. Upper-floor teleport probes and the earlier turret obstruction
were setup limitations, not evidence of a controls lock. The retained native
scenario is `storm_survivor_exit`; `snow_base_survivor` remains separately named.

Item71's native palette upload rectangle at800F1658 is `(0,480,256,8)`.
PSX CLUT encoding uses a1024-pixel row, so CLUT7843 addresses packed offset260,
not860. Native800362F8 installs X's armor colors at260/280. Both private sprite
and private UI conversion now map the first two character rows correctly;
shared animated-effect CLUTs still use live VRAM. `armor-fixed` restores the
supplied Cyber snapshot with armor14/arms2 and verifies the visible result
before and after P2's pause menu. Both captures were inspected.

Item72's `spider-native-2` enters the actual boss from the supplied Jungle
checkpoint, then sets P1 HP2. The native web captures P1, kills them, transfers
the same actor to P2, and drops P2 HP32->28. Native8009DA7C pins and damages
the singleton body without a death check; nearest-living projection had changed
that body. The fix retains the victim in the existing serialized secondary
actor owner record, uses one consistent contact context, and runs native
8009DCF4 cleanup when that victim dies. No snapshot format expansion is needed.
`spider-fixed-p1` and `spider-fixed-p2` use native captures and lethal web ticks;
the partner waits204 pixels away and remains HP32, uncaptured and unmoved.
The waiting partner has the offline invulnerability fixture enabled to exclude
unrelated boss damage; the capture flag, world position, and original web's
native removal independently verify that the capture does not transfer.
The captured web disappears and clears the dead victim's capture flag in both
cases. `spider_capture` retains this native scenario in the regression runner.

All34 Windows CTests pass after these changes, including asset palette mapping,
web victim ownership/native cleanup, completed-warp termination and the emitted
instruction-hook audit. Python syntax checks pass. These fixes are newer than
Playtest14; this investigation does not publish another release.

Private intake and bounded native-test receipts are retained under
`build-pr23/feedback-metadata4-20261013/`. Triage: `beads-eio.9.97`.

### Playtest 15 release qualification

Playtest15 packages the validated fixes for items25,71,72 from `7ac01d6`.
Production source is `e465fa1`; debug controls are disabled. All34 Windows and
28 Linux title checks pass. The Windows ZIP and Linux AppImage pass hidden
launcher checks, match all eight native room offers and share the same execution
identity and co-op profile. Both untouched packages record181 native input rows,
metadata and both memory cards with Diagnostics enabled by default. Windows
detached/no-console/redirected startup logs and Linux portable startup logs pass.

Release assets contain no game discs or player saves. Package qualification and
publication receipts are retained privately under `build-pr23/release15-qa/`.
Release tracking: `beads-eio.9.100`. Download:
[Playtest15](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.15).

### Metadata 5 follow-up: healthy catch-up reload (item73)

`Metadata 5.zip` and `Metadata 5 (1).zip` are byte-identical, SHA256
`c034c836fe265c086b188a8c4f0a3427e8e25e3969961d1e7dca8bca5d082dd5`.
They contain two genuine local Playtest15 recordings with11,463 and4,178 native
input boundaries, plus nonempty startup logs. Both use Zero's campaign. The
shorter recording has widescreen enabled; the longer has it disabled.

The unchanged published15 executable verifies4,177 checks from the short run
(the replay tool omits the incomplete final interval). At input3076/native
frame3034, mode6/0 becomes6/1 while Zero/P1 still has19HP and X/P2 has32HP.
It then reloads checkpoint1. This is stage termination, not two lethal hits.
The earlier Storm Owl correction in `7ac01d6` excluded all positive-HP state3
bodies from the survivor loop, including a P1 passenger temporarily warped out
by the co-op lifecycle. The corrected predicate includes hidden passengers
without preventing a genuine stage-owner departure.

The fixed development run preserves all3,076 original input/state checks up to
the failure boundary; only its private replay build header is translated. The
rest of the old recording is deliberately omitted because the corrected game
must diverge from the recorded checkpoint reload. Both original beams finish:
Zero/P1 returns at3102/426 with19HP, X/P2 remains at3102/427 with32HP, both are
visible, grounded and in normal idle, with the same three lives/checkpoint1.
The first observation attempt continued past that return and caught an ordinary
enemy hit on P1; it is retained separately and is not evidence of teleport damage.

Native `storm_team_exit` and `storm_survivor_exit` controls reach actual stage7
section1 without losing a life, with respectively both players alive and only
P2 alive. The lifecycle regression fails before the fix and passes afterward,
covering either roster, both finished-passenger phases, true owner departures
and existing team-death behavior. All34 Windows title tests, the focused Linux
lifecycle test and Python syntax checks pass. Private evidence:
`build-pr23/feedback-metadata5-20261013/`. This follow-up does not publish a release.

### Playtest 16 release qualification

[Playtest16](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.16)
packages item73's `6b3f7c2` fix for P1's catch-up teleport incorrectly restarting
the team at a checkpoint. Production source is `0799cd8`, with debug controls
disabled. All34 Windows and28 Linux title checks pass. Windows ZIP and Linux
AppImage hidden launchers pass, all eight native room offers match, and both
platforms share the same execution identity and co-op profile.

Both fresh packages record181 native inputs with metadata and both cards,
without overriding the default Diagnostics setting. Portable startup logs
pass, including Windows launches without a console. Package layout excludes
game discs and player saves. Uploaded-asset checksums and publication receipts
are retained privately under `build-pr23/release16-qa/`. Release issue:
`beads-eio.9.102`. The Gameplay qualification above remains unchanged.

### Playtest 17 release qualification

[Playtest17](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.17)
packages item74's `f054a9e` scene-camera fix and item76's `e784244` local Mods
improvement, including shared UI `248c347`. Production source is `baeef9a`.
All34 Windows and28 Linux title checks pass; packaged launchers, eight option
offers and cross-platform execution/co-op identities pass. Fresh packages
record real native inputs, session metadata and cards with default Diagnostics.
Windows detached/no-window startup logging and Linux portable logs pass.
Development controls are disabled; packages exclude game discs and player saves.
Jet Stingray entrance presentation remains open (item75). Private receipts:
`build-pr23/release17-qa/`. Release issue: `beads-eio.9.106`.

### Playtest 17 follow-up: independent scenery and paired doors (77–79)

The supplied opening-door/Web Spider ZIP contains paired native recordings,
metadata and nonempty startup logs, but no video. All shared input/state rows
match in its three netplay runs: 12,932 and 4,235 intro rows, and 19,043 Web
Spider rows. Settings are Split cameras and Both-player boss doors. Archive
SHA256: `e65f0e12b429102c532fe514a6570b72195b9562707819f252fbab9703827837`.

The hidden intro reproduction uses native Zero attacks to destroy the actual
barrier. Its HP falls to zero, its original explosion finishes, and the actor
retires. P1's saved maximum X becomes 4544, but P2's remains 2976; P2 is clamped
at X3288 despite continued movement. The barrier uses the original category4
pool's 32 slots of 0x8C bytes. Its finalizer releases two camera halfwords;
the fix propagates those exact writes to the living independent views.

The hidden Web Spider probe follows coordinates from the recording. At
P2 X1094/Y954, the actual composed frame shows the opening forest over missing
terrain while Zero remains grounded and healthy. P2's foreground camera is
915/762, but its background remains in mode5 at40/256. The section's original
kind1 background controller updates the canonical view without advancing
P2's saved region/scroll state. The probe preserves matching peer core/mod
CRCs through6594. This is rendering evidence, not a simulation desync claim.

Both-door admission already waited for both players, then ran the ordinary
partner departure. The new traversal needs to preserve both bodies and their
controls through the opening, defer closure until they physically clear it,
and avoid a delayed partner warp at the ensuing boss conversation. Independent
review checks native controller ordering, preemption, death and actor reuse;
unit checks alone do not qualify the visible crossing.

Fixed native probes qualify items77 and78. P2 passes the destroyed intro barrier
to X3411.5 while P1 remains at X1367, with32HP each. The Web Spider probe now
shows the correct platform and background at the previously corrupted position;
all17 positions complete. Peer CRCs match through4706 and8000 respectively.
Both-door validation caught the original sealed terrain still blocking the
players despite the open animation. The final correction makes only class38
terrain inside the fully open door's rectangle passable. Its floor boundary,
adjacent terrain and lethal classes remain native. The two-peer test now waits
for both contacts, lets P2 walk and slash while P1 remains stationary, then
physically crosses and completes the actual boss conversation with both at32HP,
unchanged lives and no partner departures. Peer CRCs match through5794. Windows
checks and focused Linux lifecycle/split checks pass; validation artifacts are
`doors-fixed5`, `all-fixed` and the retained failing baselines.

Native before/after evidence and review notes are retained privately under
`build-pr23/feedback17-20261014/`.

### Split netplay boss follow-up (80–82)

The Sigma/Colonel/Double ZIP adds two paired Playtest17 recordings to the
previous intake. Their45,315 and76,153 shared native input/state rows match
exactly. The shorter run includes Space Port Colonel with P1 dead and P2 alive
at sequences35510–40988. The longer run reaches Sigma stage1 at sequence61705;
its initial corridor shows P2 ahead of P1. These rows establish encounters,
positions and peer agreement, but do not record rendered flash duration or
enemy allocation times. No video is included in this archive.
The reporter later supplied [companion footage](https://youtu.be/vfe7GBgFGZg).
The investigation uses the paired recordings and separate native reproductions;
it does not claim a frame-by-frame review of that video.

Archive SHA256:
`05db2b34d51dd74895035a0f3faded9aa10f7efaf0c47edfa279fcf73d417c1a`.
Private intake: `build-pr23/feedback17-bosses-20261014/`.

Hidden native before/after runs qualify all three reports. Sigma's actual
placement table starts at800F848C: its records are8 bytes apart but its base
is only4-byte aligned. The former8-byte base check disabled P2 scans entirely.
The narrow fix accepts that table without changing native directional flags.
The unfixed run admits the missing enemies only when P1 follows. The fixed run
admits the X944 pair, X1108 enemy and X1223 group while P1 remains at X683;
both peer core/mod CRCs match through4672.

Colonel's native stage flash clears its shared RGB additions and requests a
palette rebuild. Returning from P2's survivor projection discarded that request,
leaving the bright working palette resident. Preserve only its shared rebuild
bit after restoring the campaign's palette source. The unfixed native run leaves
1,920 white entries after the flash, compared with18 beforehand. The fixed run
returns to19, and its composed P2 frame shows the ordinary stage, boss and HUD.
P1 remains dead and P2 remains alive; peer CRCs match through10338. Regression
checks cover both character rosters and retain private character-upload isolation.

Double's normal hit table contains damage for Zero's follow-up cuts but rejects
them as deflections. Eight unfixed native combo attempts reach attack classes25
and26 yet leave him at48HP. The fix admits only existing nonzero Zero entries
in that exact normal table, after native duplicate-hit rejection. The fixed
controller-driven combo reduces him from48 to45HP and enters his original
cooldown table; peer CRCs match through7554. His native zero-damage first cut,
invulnerable phases, X's hit behavior and native boss scaling are preserved.

The final combined build passes all34 Windows checks and focused Linux
lifecycle, combat and split regressions. Native evidence is in `bosses-fixed`,
with `sigma-before`, `colonel-before` and `double-combo-before` retained for
comparison. The combined co-op snapshot layout is now X4P6 for private scenery
and physical door state; older co-op save states are incompatible. Campaign
memory-card saves are unchanged. This follow-up does not publish a release.

### Playtest 18 release qualification

[Playtest18](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.18)
packages items77-82 from `bcf62c6`. Production source is `5853d58`, with
development controls disabled. All34 Windows and28 Linux title checks
pass. Packaged hidden launchers, all eight native room offers, cross-platform
execution/co-op identities, portable startup logs and fresh default input
recordings pass. Both packages exclude game discs and player saves. Unified
and Stacked defaults are unchanged, and Split remains experimental. X4P6
requires new co-op save states; campaign memory-card saves carry forward.
Private receipts: `build-pr23/release18-qa/`. Release issue: `beads-eio.9.113`.

### Jet Stingray rider entrance after Playtest 18

Item75 is reproduced in a fresh original-disc two-peer run. The bike initializer
8003B470 sets PLAYER+C5=1 to request boarding. P2's next bike update requests
rider pose47 and clears substate6; native boarding helper80033974 then sets
substate6=1 before the rider controller can consume that pose. The bike animates
normally while its rider remains in pose0. Finish that original boarding helper
inside enrollment, before the first acceleration update. No sprite frames,
character resources, entrance timing or shared READY actor are replaced.

Private evidence: `build-pr23/feedback-jet18-20261014/`. `before` captures Zero
standing throughout both entrance phases; `fixed-zero` captures the native
leaning pose throughout those phases with matching peer CRCs through4064.
`fixed-x` passes the reversed roster, all four entrance phases, a single READY
and matching CRCs through3716. `fixed-local-zero` passes local Unified through
normal ride control. The corrected native scenario is available as
`tools/test_mmx4_coop_netplay_debug.py --jet-only` or
`tools/test_mmx4_coop_regressions.py --cases jet_intro`, with the usual original
disc/development-build arguments. All34 Windows title checks pass. This fix is
included in Playtest19; it is absent from the older Playtest18 packages.

### Playtest 19 release qualification

[Playtest19](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.19)
packages item75 from `30b7b0a`. Production source is `646668c`, with
development controls disabled. All34 Windows and28 Linux title checks
pass. Packaged hidden launchers, all eight native room offers, cross-platform
execution/co-op identities, portable startup logs and fresh default input
recordings pass. Both packages exclude game discs and player saves. Unified
and Stacked defaults are unchanged, and Split remains experimental.
Double/Iris stays optional and default off; campaign saves carry forward.
Private receipts: `build-pr23/release19-qa/`. Release issue: `beads-eio.9.115`.

### Playtest 19 Sigma doorway recording (83)

`Stuck doors player 1 dies unified local.zip` contains three earlier sessions
and the relevant local `session-1792007983262131`. Archive SHA256:
`15a8e6cb97a8aa4cb128f7c10c72a0fd36a3f61f6497444549933b33bdd14ecd`.
The production replay verifies all12,838 complete native input/state checks.
Both-player doors are enabled. The saved Split preference does not activate
independent views offline; this is local Unified as reported.

At native frame10089, Colonel's80085F8C allocates WARNING and commands action15
while the paired door remains in physical-crossing phase2. That aborts its
terrain exemption. Native80033C14 then kills P1 through crush bits0x0C, changing
HP40 to0 at frame10091. Recovery transfers the introduction to P2 behind the
door, leaving the survivor stranded. The correction defers the entire native
admission until the paired crossing and camera arrival complete. It changes no
health, damage or collision classification and adds no new saved state.

The fixed hidden run restores a snapshot from the exact original replay,
verifies its275-input approach prefix with the current build header, then uses
controller input to walk both players through and advance the original scene.
Both remain at40/31HP, finish at X4528/4593 with clear C0/C4 and normal action2,
and enter the Colonel fight. No partner departure occurs. This is a checked
native continuation, not a claim that the corrected outcome matches the old
recording's hashes after the fix takes effect. Unit regressions cover both
projections, opening/crossing/camera waits, admission after closure and ordinary
non-paired admission. Windows34 checks and the Linux lifecycle check pass.
Private before/after evidence: `build-pr23/feedback19-sigma-door-20261014/`.

The same local recording also qualifies the P2 health-refill report: the large
pickup sets all eight native PLAY+10..17 freeze flags, raises P2 from23 to39HP,
and holds both players still during the refill. Its small pickup and P1's
pickup also hold both positions. No missing shared pause reproduces here.
Split netplay intentionally keeps health refills asynchronous, per the earlier
owner preference; the archive includes separate network sessions as well.
This update does not publish a release.

## Playtest19 Double / Iris / Sigma intake (2026-10-14)

Source: `Double Iris Sigma stage 3.zip`, SHA256
`c3827afbd4ddc35e9e131598b2e9cfef580944f63ab147af1e4a18aafc3a1fd7`.
Companion videos: https://youtu.be/M6oZGhGRx2k and
https://youtu.be/p6TYd_-tSD4. Public retrieval returned HTTP403; findings below
come from supplied diagnostics and hidden native runs, not video review.

Original Playtest19 local session1792009197507469 replays exactly through
17,303 verified native inputs. Iris has mismatched sprite pieces. Loader1302C
selects stage11 sprites through V1; the old130D4 hook only changes stage13.
The corrected selector uses the current encounter without changing P1's
campaign. The Double/Iris option remains optional and off by default.

A native continuation also reproduces Iris waiting in state1/2/1 with0HP after
both players cross her door. Her81250 camera wait requires2032, but physical
closure raises the minimum to2048. Wait through the paired crossing, then
accept the camera inside2032..2096. Ordinary admission and private render
passes stay untouched. Fresh entry and recovery from the captured stall both
reach the48HP fight with both visible, clear controls and no partner departure.
Subsequent native armor/core combat and defeat advance to11/1.

Zero's airborne spin is Kuuenzan34; Double has no retail damage for it. The
co-op collision borrows his authored saber26 damage2 through a lookup register.
The attack class, consumption, hit token, invulnerability, original RAM table,
damage multiplier and boss scaling are preserved. Six original native attempts
leave48HP; six fixed attempts reduce48 to42 with recorded boss scaling enabled.
Unit checks include both seats, survivors, cooldown and lethal hits.

Paired sessions1792009600374384 /1792009600376151 match all670 common gameplay
frames in11/1, including state digests. They show P1 dead and P2 alive. A native
continuation with P1 killed and P2 defeating Iris reaches11/1, refills P2 to48HP
and permits movement fromX384 to555.5 with visible body/HUD. Item86 remains open.
This is not an offline replay of the network session or a new two-peer graphical
qualification of these fixes.

All34 Windows checks and Linux combat, story-state and lifecycle checks pass.
Private evidence: `build-pr23/feedback19-double-iris-sigma3-20261014/`
(`local-inspect`, `double-air-original`, `double-air-fixed2`,
`iris-selector-fixed2`, `iris-camera-recovery`, `iris-full-entry-fixed`,
`sigma-survivor-entry`). No release is published by this update.

### Sigma Split netplay follow-up (2026-10-14)

The earlier survivor continuation used local Unified and the optional Iris
sequel. It did not qualify the reported Split netplay mode. On owner review,
ran two hidden SDL/OpenGL peers over loopback LAN using a separate development
build of2638bed with synchronized fixtures. Copied the recording's mod settings:
Split cameras, Either-player doors, boss scaling, Ultimate X, Black Zero and
the Double/Iris sequence off. The new scenario starts a fresh X campaign with
32HP capacity; the supplied recording has48HP. It does not replay the original
inputs, retry history, save progression or remote-network timing.

Native checkpoint setup reaches Double's room. After a synchronized fatal
request for P1, P2 defeats Double with a native attack; only boss HP and setup
positions are adjusted. The native ending advances11/0 to11/1. P1 stays dead,
P2 refills to32HP, completes arrival and walks fromX384 to486. Composed captures
show Black Zero and his HUD on both peers, including P1's spectator view.
Both peers commit private local views; renderer verification mismatches,
watchdogs, VRAM leaks and co-op view-state leaks stay zero. Core/mod CRCs agree
through the final checkpoint at tick8928. Both owned processes exit afterward.

Evidence: `sigma-split-survivor-1/report.json`, `scenario-events.json`, both
peer runtime logs and composed `arrival-ready-p1.png`, `arrival-ready-p2.png`,
`survivor-moved-3-p2.png` under the private intake directory above. Item86 stays
open: this unsuccessful reproduction attempt is not a confirmed fix. Request
both peers' diagnostics and a video timestamp if it recurs in the next build.

### Playtest 20 release qualification

[Playtest20](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.20)
packages the Colonel paired-door fix from `b527698`, Double aerial damage and
Iris assets/admission from `2638bed`, and the Split netplay evidence in `959e15a`.
Compiled production source is `dcabda9`; both development control modes are off.
All 34 Windows and 28 Linux title checks pass. Hidden packaged launchers,
eight complete native room offers, cross-platform execution/co-op identities,
AppImage layout, startup logs and fresh default input recording checks pass.
The Windows ZIP and Linux AppImage exclude discs and player saves. Unified and
Stacked remain defaults, Split remains experimental, and Double/Iris stays
optional and default off. Item86 remains open and is not claimed fixed.
Private receipts: `build-pr23/release20-qa/`. Release issue: `beads-eio.9.118`.

### Jet Stingray rider/bike layering follow-up (2026-10-14)

Owner confirms item75's intro animation is correct. Reopened item16 for the
remaining interleaved rider/bike composition. Native priority tracing proves
the old +4 offset overflowed the eight-bucket layer: P2 rider/bike/exhaust
became6/8/10. Assign four buckets per player within that same layer instead,
including wind and X armor, restoring native priorities after drawing.

Local Unified X-campaign A/B frames reproduce and correct the overlap, with
native body/bike priorities unchanged. Two hidden loopback Split netplay runs
pass the existing entrance-pose, shared READY and control-release checks.
Composed frames from both peers show complete P1-over-P2 layering in both
character arrangements. Final core/mod CRC checkpoints agree through4097
(X campaign) and3650 (Zero campaign). The priority regression test passes.
No visible windows or tester saves were used. This fix is not in Playtest20.

Private evidence: `build-pr23/feedback20-jet-layer-20261014/`, notably
`trace2-x`, `fixed-local-x`, `fixed-split-x` and `fixed-split-zero`.
See `COOP_COMBAT_VEHICLE_EVIDENCE.md` for the native bucket analysis.

### Boss-fight arrival cancellation and reverse story sprites (2026-10-14)

Reopened item6 after new feedback that P1 must stand still during P2's boss
return. The reporter cannot identify the boss or camera mode. A private
continuation of the earlier local Double fight reproduces two concrete cases:
jumping sets the leader's native +67 airborne phase and cancels P2's beam;
Nova sets that phase plus PLAY+1C and does the same. P2 becomes inactive,
waits for P1 to land, then starts a new beam. Walking and ground Dash already
complete normally. This identifies a reproducible cause without claiming an
exact replay of the new secondhand report.

Keep the personal +67 gate when choosing a safe, grounded landing point,
but ignore it after that point is committed. Reuse the existing native Nova
classification for PLAY+1C. Actual scene ownership, C0/C3/C4, room fades and
victory gates retain their behavior. The before-failing regression covers
both leading seats, jump/fall and Nova, preserving the leader's attack/lock;
negative cases retain real chained scenes and unsafe initial placement.
Lifecycle and story-state CTests pass.

Native local A/B uses the same saved active Double fight and normal Select
withdrawal/return controls. Original jumping and Nova take91/112 frames and
show37/47 hidden samples after the beam starts. Fixed idle/walk/dash/jump/Nova
complete in36-40 frames with zero hidden samples. P2 is controllable on the
original floor while P1 is still airborne (action7) or in Nova (action34).
The first probe timed out because its diagnostic-local frame count stops
while P2 is withdrawn; the corrected probe uses the native frame counter.

Item87 is a qualification request rather than a reproduced new graphics
failure. A fresh native Zero campaign, optional sequence enabled, completes
Iris66 then Double73, including actual boss defeats and the original movie
route. Inspected `story-fight-1.png` shows Double's correct sprites. Campaign1
and two lives remain; counters show one sequel and two boss defeats. The
Playtest20 stage11 sprite-selector fix already applies in either direction.
No further story graphics change is made.

Private evidence: `build-pr23/feedback20-return-story-20261014/` contains
`before-2`, `before-nova`, `fixed-local` and `zero-story-native`. The new
`--boss-return-only` native netplay scenario retains composed frames, verifies
that P2 stays active throughout the beam, and checks both peer state digests.

The two-peer Split follow-up `fixed-split-x-inward` passes walking, Dash,
jump and Nova in the live Double arena. Each return takes40-41 native frames,
P2 stays active at32HP, and composed captures show Zero standing normally
while X remains airborne or in Nova. Both peer core/mod digests agree through
tick6753. The first Split run passed movement/jump but rejected its Nova case
because X faced the solid arena wall: native800375B4 disallows that attack.
The final scenario resets only its setup position to the observed arena floor,
faces inward, and verifies actual leader action34 before accepting the result.
This arrival fix is for the next build; the Double selector correction is
already in Playtest20. No release was cut by this follow-up.

### Playtest21 package qualification (2026-10-14)

Items6 and16 are included in Playtest21, compiled from `d8153e5`. The full
release suite caught a duplicate native armor hook; rider ordering now composes
with existing split-camera culling through one registration. Dispatch regression
assertions and all34 Windows/all28 Linux title checks pass after rebuilding.
Packaged hidden launchers, eight matching option offers, execution/co-op
identities, AppImage layout, portable startup logs and fresh default recordings
pass on both platforms. Development controls are disabled. Item86 remains open;
item87's reverse story encounter already works with the Playtest20 selector fix.

Private receipt: `build-pr23/release21-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.119`.


### Playtest22 package qualification (2026-10-14)

Validated items88/90/91 ship in Playtest22 from source `90a8172`: P2's intro
crusher trigger, paired intro-door cutscene admission, and both heroes' native
Maverick victory poses. Both campaigns and saving/loading during the paired
poses pass native gameplay checks. Normal campaign memory cards are unchanged;
older playtest host savestates can be rejected by the X4P7 schema.

All34 Windows/all28 Linux title checks pass. Both production packages pass
hidden launcher/controller/netplay UI checks, eight matching native option
offers, strict execution/co-op identities, portable startup logs, and181fresh
native diagnostic inputs without an explicit diagnostics override. Both debug
control modes are off. The Linux AppImage passes layout checks, and package
runtime/archive hashes match the tested binaries. Items53/86/89 remain open.

Private qualification: `build-pr23/release22-qa/release-receipt-final.json`.
Release issue: `beads-eio.9.127`.

## October 15 follow-up: Ride Armor and moving backgrounds

Ride Armor now retries the native unoccupied mode0 for P2 after P1 gets first
choice. Ownership transfers the authored actor, preserving its HP and location.
Committed doors and partner departures use the native ejection path; waiting
for a Both door does not eject early. A forced departure leaves unoccupied
armor in the world, or retires the private actor if another world vehicle
already occupies the native slot. Native Raiden checks cover both rosters in
Split netplay, local mounted save/load and death, manual ejection, stage cleanup
and local Unified transport into Dragoon's arena. Eagle uses the same native
mount/ejection interface; its full flight/combat playtest is not claimed here.

Jet's saved Split camera omitted the foreground scrolling displacement used
by the authored bike entrance. The entrance now uses the canonical camera
until P2's native mode0 finishes. Both rosters retain their native poses and
share one READY actor. Measured control releases are 307/314 and 265/271 in
the two Split runs; local Unified is 267/272. The retained Playtest19 capture
also has the old delay, so a newly introduced regression is not established.

Storm Owl's kind5 controller runs after the camera snapshots. The baseline
shows P1 sky movement of 52px and P2 movement of 0px over 100 transport ticks.
The fixed two-peer run advances both by 52px, keeps shared stage events at one
pass, and agrees on core/mod state through tick 4260. Per-view foreground
contributions retain the native signed truncation for half/quarter parallax.
Both presentation probes have zero verification mismatches and VRAM leaks.

Initial Slash Beast assessment (superseded by the panorama follow-up below). Its new recording contains 5552/5550 native rows
with an identical shared prefix. A private instrumented two-peer inspection
verified 5,400 original native frame/state hashes and 184 peer core/mod samples.
Only the private copy's build/plan header labels were translated; state checks
were retained and production replay still refuses network sessions. Captures
through P1's death have not identified a clear corrupt background layer. An
image or timestamp is needed to distinguish the reported fault from intended
train/scene transitions. The earlier direct endpoint teleport was unsafe and
is excluded as reproduction evidence.

Windows: 34/34 CTests pass. Linux release-config combat, lifecycle, diagnostics
and Split boundary binaries compile/run. The lifecycle fixture also hits a
pre-existing assertion with NDEBUG disabled on unchanged HEAD; release-mode
CHECK assertions remain active and pass. Native QA is private under
`build-pr23/feedback22-20261015`, including `qualification-net-x-final`,
`qualification-net-zero`, `qualification-local-armor-handoff`,
`storm-native-before`, `storm-native-fixed` and
`slash-recorded-inspection-3`. No new release has been published by this work.


## October 15 panorama follow-up

Final visual review isolated the second graphics fault shared by Storm Owl
and Slash Beast. Native 273F8..274D4 wraps source columns within each background
plane's inclusive screen interval. The authored-map renderer instead rejected
those columns, leaving black strips or large missing regions while the scroll
crossed an interval edge. A title-specific read-only tile policy now reproduces
that horizontal wrap for layers 1/2. Foreground boundaries, camera positions,
collision and shared GPU memory stay untouched. Windows/Linux packet fixtures
cover both sides, negative coordinates, invalid intervals and foreground edges.

Storm's native two-peer capture now has continuous city/sky across the entire
view, and both skies still scroll with matching core/mod checks through 4291.
Slash's original recording reproduces the large black mountain/sky region near
input 2730. Comparing original-state captures at 2731/2734 shows the full panorama
restored. The replay retained all original native frame/state checks; header
translation is confined to the private inspection copy. See private evidence
`storm-panorama-fixed`, `slash-recorded-inspection-3` and
`slash-panorama-inspection-verified` under the same QA directory. The original
uncertainty for item 95 is resolved; no new screenshot is needed from the owner.

### Playtest 23 package qualification (2026-10-15)

Items 92-95 ship from compiled source `25af248`: P2 Ride Armor with safe native
dismounts, Jet Stingray entrance timing, Storm Owl sky motion, and the native
panorama wrapping shared with Slash Beast. Items 53, 86 and 89 remain open.

All 34 Windows and 28 Linux title checks pass. The Windows ZIP and Linux AppImage
pass hidden launcher/controller/netplay UI checks, eight matching native option
offers, execution/co-op identity checks, portable startup logs, and fresh default
recordings of 181 native inputs per platform. Development controls are disabled.
The AppImage layout and final package hashes pass. These checks do not claim
a new live Windows-to-Linux gameplay qualification.

Private receipt: `build-pr23/release23-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.132`.

### Next-playtest follow-up (2026-10-15)

Items96-99 address Cyber Peacock's grounded partner death, Storm Owl's paired
midboss door, Ride Armor rider priority and the requested boss-cooldown choice.
Scaling remains off by default. When enabled, its default cooldown method is
separate full-duration immunity for each player; reduced shared cooldowns remain
available. Ordinary enemies, contact damage and native weakness reactions retain
their existing handling. A one-tick native expiry preserves Web Spider's
temporary hit-table cleanup. The additional timers are included in snapshots
and deterministic state checks; the co-op snapshot schema is now X4P8.

Hidden local native cases cover the paired door and both bosses, including
overlapping player cooldowns and native death. A two-peer Split run completes
both Cyber switch activators, the Storm Owl door, Ride Armor and scaled Jet
Stingray combat, with 596 matching core/mod samples through tick19104. Its final
presentation debug query times out after those cases complete; this is recorded
as a harness limitation, not an overall successful harness exit. See private
`build-pr23/feedback23-20261015/completed-net-checks.json`,
`overlapping-cooldowns-cleanup-fixed/report.json` and
`cooldown-offers-final/report.json`. The latter confirms the host negotiates the
chosen cooldown method, including the omitted-option default.

All 34 Windows title checks and four focused Linux C fixtures pass. These checks
do not constitute a new Linux release build or cross-OS gameplay qualification.
The README now starts with the owner's requested playtester acknowledgment.
Items53,86 and89 remain open. Item100 is the additional grounded-return safety
case discovered during Ride Armor validation. Its native fatal writer was
captured in `x-return-writer-warm/return-writes.json`; the complete fixed case
passes in `x-return-safe-landing3/report.json` under the same private QA directory.
That case deliberately moves the leader from the unsafe scripted position onto
clear ground. If the local search has no valid landing, the passenger remains
hidden until one is available. It does not bypass terrain damage or disable
crushes. The bounded terrain-model tests cover both passenger seats, safe
offsets, deferral without health loss and completion after a valid site appears.

### Playtest 24 package qualification (2026-10-15)

Items96-100 ship with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment is above the title and present in both packages.
Compiled source `f973422`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows and 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `55af9dcf00fb5f2d17cc1dc2250cb3e3a106ce7dbe9a909ad46452216ea20554` |
| Linux x86_64 AppImage | `bb8a05a177af809c326a561295fef8f6ff7560773cb2c0fc4276d7c4f5dee16d` |

Private receipt: `build-pr23/release24-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.138`. Existing items53,86 and89 remain open.

### Playtest 24 second-door and fortress-arrival follow-up (2026-10-15)

Item101 reproduces in two native Split peers with Both-player doors. Cyber's
outer door crosses normally, but the stationary camera leaves the inner door
at992/2496 inside the viewport without admitting it through retail's moving-edge
placement strips. The supplemental placement scan excluded General8 doors.
The correction includes unflagged doors in that scan while retaining native
eligibility, the shared visit latch, directional exclusions and the exclusions
for other scripted General actors. The focused fixture fails with the old code
and passes with the correction, including allocation failure and duplicate-view
checks. The fixed two-peer Cyber run admits the inner door, completes both
crossings without partner departure, and restores both controls in the boss
room. Peer core/mod checks match through5892. Evidence:
`paired-doors-fixed6/report.json`, case `stage=6`, and its timestamped door
observations/screenshots. The overall harness exit is failed: an additional Web
Spider setup placed P2 in terrain before the door check, so that case is not
qualified. Earlier loading/transport and screenshot-helper failures are also
excluded from the native success claim.

The new item86 recording and accompanying Double video establish a different
survivor from the earlier test: P2 dies, then P1 wins. The archive records P2
HP0/state3 at Sigma stage3 initialization. Retail uses slot11 section0 for the
story boss and section1 for the next fortress level; the ordinary section-carry
policy incorrectly carries the corpse across that level boundary. The
correction enrolls both seats afresh there. Old-code failure and corrected
Windows/Linux fixtures cover both dead seats, unchanged shared lives and the
ordinary mid-stage corpse policy. The fixed two-peer Split run explicitly uses
Either-player doors (the owner-reported setting; the archive's saved setting is
Both). It kills P2, defeats native Double with charged X attacks, then follows
the native story transition. Both seats arrive in11/1 active at32 HP and move
under controller input; shared lives remain2. All234 common recorded core/mod
samples agree through7488. Evidence: `sigma-survivor-fixed/report.json`, captures
`double-p2-dead`, `double-defeated`, `sigma3-ready` and `sigma3-both-move`.
The rendering captures for this case are canonical headless buffers; the native
enrollment and control checks establish the corrected lifecycle behavior.

All 34 Windows title checks pass across the suite and a retry of the detached
startup-log child timing test. Four focused Linux fixtures pass; this is not a
new Linux release build or cross-OS gameplay qualification. Private evidence is
under `build-pr23/feedback24-20261015`; original archives remain unmodified.
The combined receipt is `completed-native-checks.json`. Items53 and89 remain
open; these fixes have not yet been packaged as a new release.

### Playtest 25 package qualification (2026-10-15)

Items101 and86 ship with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment and requested blog link are above the title and
present in both packages. The README packaging revision is recorded separately
from the compiled-code snapshot.
Compiled source `aa63e61`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows title checks pass across
the suite and an isolated startup-child timing retry; all 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `7e520ca068d2631892fe4b9129f68004af79c1092a0bde122e27ac2a06108219` |
| Linux x86_64 AppImage | `2107012b12f3363c690a40c326cb6225c608423d8058bef289a5e54b9cd2f39e` |

Private receipt: `build-pr23/release25-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.140`. Existing items53 and89 remain open.
The first Linux default-recording attempt timed out before emulation while
reading the image through WSL's Windows-drive bridge. A sampled retry confirmed
a blocked read of that BIN in `p9_client_rpc`. A private copy on Linux's native
filesystem was SHA256-verified against the original, and the unchanged AppImage
then recorded181 native inputs with fresh defaults. The failed attempts remain
in the private QA directory; they are not presented as passing runs.

### Playtest 26 package qualification (2026-10-15)

Items102 and103 ship with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment and requested blog link are above the title and
present in both packages. The README packaging revision matches the compiled-code snapshot.
Compiled source `51c86dd`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows title checks pass; all 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `2f13719202a6f28e295a4b19546fe3415ef485395ee314ab5175cb86dd5f4086` |
| Linux x86_64 AppImage | `1de7d454c49f15c9caceb52f6b7eebd8d6d13d194e1967b705a0a14118be9fa3` |

Private receipt: `build-pr23/release26-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.143`. Existing items53 and89 remain open.

### Playtest 27 package qualification (2026-10-15)

Items104,105 and106 ship with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment and requested blog link are above the title and
present in both packages. The README packaging revision matches the compiled-code snapshot.
Compiled source `602949f`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows title checks pass; all 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `debd41bcdc3379a8e56ac4d45ad8f3c87564083d5a6b66b31d0371f0442e8a6d` |
| Linux x86_64 AppImage | `e5a4c6837312ad176272b4b01468698cd56d6cb1e7c54199aae1ece9d543bde2` |

Private receipt: `build-pr23/release27-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.147`. Existing items53 and89 remain open.

### October16 paired-door follow-up

Items114 and116 are fixed in source after Playtest27. Frost checkpoint4's
native midboss admission now waits for both bodies and the camera to clear
the doorway before retiring other stage actors or starting action15. An
opener can freely move beyond the new scrolling minimum, so completion
accepts the authored camera interval rather than exact minimum equality.
Both-door Split presentation keeps each hero's own camera/HUD during the
physical crossing; ordinary scripted introductions still share their view.

Native before/after validation covers both leading arrangements in Frost,
plus complete two-door Cyber Peacock and Web Spider entries. Composed images
verify separate X/Zero views while one waits and the other advances. Both
players retain health and controls without a partner beam. Peer core/mod
state agrees; no private-render state leak is recorded. All34 Windows title
checks and the native Linux lifecycle/split units pass. Negative camera
interval, deferred admission, real-scene sharing and snapshot cases are covered.
The failed intermediate P2-first equality wait and the incomplete initial
presentation correction remain in private evidence, rather than being counted
as passing tests. These changes are not yet in a published playtest.

Private evidence: `build-pr23/feedback28-frost-20261016/doors-before`,
`doors-after`, `doors-camera-fixed`, and
`build-pr23/feedback28-session2-20261016`.

### October16 opening ice-platform qualification

Item111 is fixed in source after Playtest27. Samples from the supplied Frost
video and its on-screen notes identify the opening timed ice near the Heart
Tank route, not the later rising shaft. The native baseline leaves the cycle's
region, timer and allocation mask at zero while P2 reaches705/426 and P1 stays
at144/347. Bringing P1 to the same ledge starts the cycle and creates visible
ice. The post-load activation predicate now admits either living hero inside
the authored673..3167 interval. Its pure proximity query uses either living
Split view with retail's unsigned16 bounds. The original shared timer, mask,
tile-edit queue and allocation logic continue running once.

The corrected two-peer Split run shows ice with P1 still at spawn, then checks
P2 at3246/361 while P1 remains144/347. Both retain health, and recorded core/mod
checks agree through7398. P1-death continuation also advances the cycle.
All34 Windows title checks and Linux lifecycle/split units pass. Exact interval
edges, absent/dead players, real script ownership and alternate camera layers
are covered. A reviewed intermediate candidate hooked the pre-load instruction
and failed native activation; it was corrected to the post-load comparison and
is excluded from the success claim.

Item112 stays unchecked: native opening-cycle and rising-shaft controls keep
updating after P1 dies, so they do not establish the reported permanent ice.
The initial direct shaft/upper-route teleports were also excluded because they
did not follow a safe authored route. Native movement from checkpoint3 does
spawn and animate the rising platforms for P2, including after P1 death.

Private evidence: `build-pr23/feedback28-session3-20261016` (opening baseline),
`feedback28-session5-20261016` (corrected native checks), and
`feedback28-session2-20261016` (rising-shaft controls).
Tracking: `beads-eio.9.151`. This change has not yet been packaged as a release.

### October16 full-health capsule qualification

Item110 is fixed in source after Playtest27. The original recording places
P2 at3650/1449 and P1 at654/891 while a full-health capsule heals P2 from20
to48. Native pickup dispatch maps kind5 to a full refill; the existing Split
pause isolation covered small/large health and the prior large-weapon path,
but omitted this form. Adding kind5 preserves its collector and native HP
count while restoring only that pickup's shared freeze flags. Unified and
preexisting scene locks retain their established behavior.

The actual-disc baseline uses the authored Spider1/1 checkpoint4 and collects
General2/variant5 at3632/1456. P2 heals8 to32 while PLAY+10 stays1, its private
redraw counter stops, and the inspected composed image switches to P1's
unified camera/HUD. The fixed run collects the same capsule: no shared freeze,
P1 moves29.5px during the intermediate healing samples, and P2 commits30
additional own-view redraws. The inspected intermediate image retains Zero's
own camera and HUD. Both peers agree through5987. This proves the actual
pickup path; it does not claim to replay the original long traversal exactly.

The expanded small/large/full-health fixture fails on the old source and
passes after the change for both collectors and both camera modes. It also
covers owner death and preserved preexisting freeze flags. All34 Windows
title checks and native Linux lifecycle/split units pass.

Private evidence: `build-pr23/feedback28-pickup-session-20261016`
(old8336b0d baseline), `build-pr23/feedback28-pickup-fixed-20261016`
(fixed native run), and `build-pr23/pickup28-*.log`.
Tracking: `beads-eio.9.11`. This change has not yet been packaged as a release.

### October16 distant water-effect qualification

Item115 is fixed in source after Playtest27. The earlier allocation-owner
fix correctly keeps P2's type7 effects attached to P2, but their final native
B288 visibility query runs inside a personal effect projection. Split's
availability gate admitted shared actor projections and excluded this
effect context, so the query culled P2's ripples against only P1's camera.
Include the existing personal-effect scope in the actor-context predicate.
The same native visibility query then uses both living views, retaining
scene-owner restrictions, screen-relative exclusions and local-render guards.
There is still one native effect update, without a second allocation or clock.

Native two-peer before/after checks hold P2 in water at432/394. The baseline
keeps its two active ripple actors hidden with P1 at56/139, shows them when
P1 approaches400/395, then hides them when P1 returns to spawn. The fixed
run retains both ripples at P2's own X-8/X+8 and native Y in all15 sampled
far/near/far states. Both heroes stay healthy. Actual composed P2 images
were inspected, and peer core/mod checks agree through4964. The projected
water ownership fixture fails before and passes after; the bounds fixture
also checks visibility near either view, rejection outside both and camera
restoration. All34 Windows title checks and Linux combat/split units pass.

Private evidence: `build-pr23/feedback28-pickup-fixed-20261016/water-*`
(7f705af baseline), `build-pr23/feedback28-water-fixed-20261016`
(fixed native run), and `build-pr23/water28-*.log`.
Tracking: `beads-eio.9.28`. This change has not yet been packaged as a release.

### October16 P2-led Spider exit scenery qualification

Item108 is fixed in source after Playtest27. Archive A's video around native
frame6785 shows both windows sharing the exit walk with unrelated terrain
after the leading P2 breaks the final waterfall barrier. Current native
Split/Either reproduces this at General6/variant0 at6304/392: P1 stays below
at5680/1307, P2 breaks it with ordinary Zero attacks and starts the original
exit scene at floor426. The composed shared image instead shows old cliff
terrain around the grounded body. Both players still load part2 correctly.

At native4:3, the background callback previously rebuilt authored tiles only
inside the private redraw transaction. A canonical shared scene can abruptly
inherit P2's distant camera while the native32x32 ring contains P1's old room.
Use the same authored renderer for canonical Split passes as for private views,
including zero-margin room ends. Unified retains its existing retail path.
No extra scene, simulation update, actor allocation or camera switch is added.

The fixed native entry shows the correct waterfall and continuous floor in
the inspected composed scene. Both players then arrive active, visible,
normal and healthy in part2, with no local-view state leak. Peer core/mod
checks agree through7203. The background fixture fails before and passes
after with a deliberately stale ring, while also checking the Unified
exclusion; all34 Windows title checks and the native Linux background check
pass. The initial direct exit teleport was clamped to5896 and is excluded:
accepted probes enter the upper corridor at5730/426, move and attack normally.

Private evidence: `build-pr23/feedback28-exit-session-20261016`
(eafd1a5 baseline), `build-pr23/feedback28-exit-fixed-20261016`
(fixed native run), and `build-pr23/exit28-*.log`.
Tracking: `beads-eio.9.150`. This change has not yet been packaged as a release.

### October16 independent Frost snowfall qualification

Item113 is fixed in source after Playtest27. The settled native Split probe
holds P1 at144/347 and P2 at3281/378, with P2's foreground camera fixed at
3121/234. Its snow layer stays at4118.75/342 across four samples while P1
lives, then starts moving after a real native P1 death. A read-only write
trace identifies the stage pool's kind9 controller at B75C8: B78BC advances
fractional wind and B790C advances/wraps snow Y. The ordinary mode5 camera
helper does not publish this weather to the noncanonical view.

The controller still runs once. Publish its exact native layer2 origins and
changed metadata to both saved views, keeping foreground/terrain cameras
independent. These coordinates encode weather atlas phases, including
absolute resets, so they do not inherit a stale per-view position offset.
The initial delta candidate animated successfully, but the final source uses
the exact native phase and adds explicit stale-phase/reset regression cases.
The new function entry is present in both development and release configs;
fresh generation changes only shard54. Registration and release-contract
checks caught the intermediate missing wiring before native qualification.

The final actual-disc run shows P2's snow moving with P1 alive and continuing
after P1 dies. Both views keep the proper terrain, P2 remains healthy, no
private-render state leak is observed, and peer core/mod checks agree through
5416. Inspected composed frames and sky-region snowflake masks distinguish
position from unrelated sky-palette changes: baseline alive has one identical
mask across four captures; baseline survivor and both fixed phases each have
four distinct masks. The earlier unsettled teleport/camera catch-up samples
are excluded from this visual claim. All34 Windows title checks and Linux
combat/split units pass. Fractional movement, vertical wrap, absolute startup
reset, one clock tick, later camera snapshot and excluded contexts are covered.

Private evidence: `build-pr23/feedback28-exit-fixed-20261016`
(`frost-snow-trace-settled` baseline and native writes),
`build-pr23/feedback28-snow-atlas-20261016` (final native run),
`build-pr23/snow28-visual-result.json`, and `build-pr23/snow28-*.log`.
Tracking: `beads-eio.9.152`. This change has not yet been packaged as a release.

### October16 X versus Iris combat qualification

Items117/118 are fixed in source after Playtest27. Archive C records Playtest26
Split netplay with boss scaling and both story bosses enabled. The original
Iris armor table80107DFC and crystal table80107E84 reject every X entry with
FF/0. Native ordinary and charged X/P2 attempts reproduce rejection and leave
the armor's eight-hit counter unchanged; peers agree through7170. Native Zero
attacks can expose the real variant1 crystal, whose X entries also reject hits.

The existing DE84 counterpart admission now handles Iris too. Reuse Iris's
native Zero reaction/damage entries for this collision only: armor reaction5
with zero lasting HP damage, basic buster1, intermediate buster2, full buster3,
neutral weapons2, and Rising Fire/Nova4. These are co-op damage choices, not
retail authored X damage. The original shot type, consumption and duplicate
token remain intact. The native all-FF cooldown table stays closed, and the
existing damage multiplier, boss scaling and player cooldowns still apply.
The original shot dispatch table800F25D8 identifies buster0/9/18..21/23,
normal weapons1..8, charged weapons10..17 and Nova22; the final mapping uses
those identities. Existing Zero-versus-Double admission is retained.

In a hidden, muted original-disc two-peer Split/Either match, Zero/P1 stays
idle while X/P2's real buster shots advance all eight armor reactions and
expose the crystal. The crystal then takes real buster hits: scaling retains
the first half point and the following accepted hit changes its HP48 to47.
The read-only trace shows native8002DF0C, actual attack0, and adjusted lookup
base80107EC4, preserving that shot's identity. Both players retain32HP and
normal controls, the renderer stays enabled, and core/mod checks agree through
31713. The native buster path is unchanged by the final dispatch-table mapping
review; other attack identities are covered by the focused combat checks.

The combat regression fails on the original source and passes with the fix.
It covers all X types0..23, both player assignments, either surviving player,
native armor/core reactions, duplicate tokens, cooldown tables, unrelated
actors/contexts, unchanged Zero damage and lethal native return values. All34
Windows title checks and Linux combat/split checks pass. Native qualification
here covers X/P2 in Zero's campaign; the reverse optional-story encounter and
every special weapon were not individually replayed. Earlier air shots that
missed the moving crystal, and a test teleport inside the closed left arena
boundary, are excluded from the successful native qualification.

Private evidence: `build-pr23/feedback28-iris-before-20261016`
(`iris-native-before`), `build-pr23/feedback28-iris-final-20261016`
(`iris-armor-final` armor rows and `iris-crystal-level` native HP result),
`build-pr23/iris28-title-tests4.log`, `build-pr23/iris28-linux-units4.log`,
and the private native table/disassembly captures. Tracking:
`beads-eio.9.153`. This change has not yet been packaged as a release.

### October16 survivor background animation qualification

Item120 is fixed in source after Playtest27. In a hidden, muted two-peer
Split/Both match, X/P1 stays at56/139 and Zero/P2 waits at432/394 by Jungle's
opening waterfall. An isolated water strip changes across three sampled
frames while both players live, then retains exactly the last uploaded frame
across four captures after a native P1 death. P2 stays healthy and controllable;
peers agree through13351. Larger image regions also contain moving enemy
effects, so their changing hashes alone do not establish water animation.

The native stage pool's kind3 palette clocks continue advancing. B5DCC caches
their destination in the shared stage palette, and B5C60 copies the new colors
and requests palette upload bit1 at B5CB4. The survivor's stage projection
restores P1's pending palette flags and retained only flash-rebuild bit4,
discarding that upload request. Preserve shared stage bits1/4, and keep the
canonical stage palette bound during that single world stage pass so controllers
initialized later also get the proper destination. Other character projections
keep their private resources and upload ownership. No extra animation tick or
stage event is introduced.

The fixed native run keeps both the composed water and the actual VRAM CLUT
moving after P1 dies. Six settled captures contain four distinct water states
alive and five after death; the water CLUT at208/484 has four distinct states
in each phase. P2 retains32HP, the renderer stays enabled, no local-view state
leak is recorded, and peer core/mod checks agree through8897. The lifecycle
regression fails on the original source and passes with the fix. It covers
both campaigns, Unified/Split, cached/late palette destinations, repeated
world-color uploads, unchanged private counterpart colors, ordinary private
projection ownership and the retained Colonel white/clear sequence. All34
Windows checks and Linux lifecycle/split checks pass. Native after coverage is
Split with X/P1 and Zero/P2; other combinations have boundary-test coverage.

Separate bounded probes keep item121 open. In Slash Beast section1, the actual
settled checkpoint2 positions are P1 at7728/363 and P2 at7656/362. Then native
paired checkpoint3 boss entry reaches Slash kind54/48HP with both players
at9472 and normal controls. Both probes keep scrolling after P1 dies: camera
origins advance at the native8px/tick and inspected background regions change
across all four captures in each alive/dead window. P2 stays32HP; peer checks
agree through16614 and33925. The reported frozen Slash condition remains
unreproduced; this is not evidence that every mode/location is clear.
Web Spider part2 also retains survivor parallax: moving P2 from1094 to1076
changes the saved background from1446 to1428 without a renderer failure.

Private evidence: `build-pr23/feedback28-survivor-bg-20261016`
(`spider-survivor-before`, `spider-waterfall-retry`, `waterfall-native-trace`,
`slash-survivor-before`, `slash-boss-survivor-before`),
`build-pr23/feedback28-survivor-bg-fixed-20261016`
(`waterfall-survivor-fixed2`), `build-pr23/survivor28-tests2.log`, and
`build-pr23/survivor28-linux-units.log`. A reverse-section setup that retained
P1's death and an early screenshot-file timing failure are excluded from the
successful native comparisons. Tracking: `beads-eio.9.108` and
`beads-eio.9.55`. The palette fix has not yet been packaged as a release.

### October16 Sigma stage2 facing qualification

Item119 remains open after a bounded hidden, muted Split/Both probe. Original
placement table800F848C contains ordinary enemy kinds51 and58. Their native
initializers compare PLAYER.x with their own X at80069B78..94 and80074494..B0.
The shared co-op actor pass selects the nearest living hero for that original
initialization. A newly spawned kind51 at702/410 faces left toward P2 at560/458
while P1 is farther away on its right at1000. Both retain32HP, and peers agree
through9248. This excludes a blanket canonical-P1 facing bias for that case.

Existing kind58 actors can retain a right-facing initialization chosen when
P1 first revealed them; arrival of a different nearer hero does not rerun the
native initializer. That could explain the reported appearance, but the exact
enemy and respawn have not been matched to a recording. Do not change shared
world targeting to follow whichever peer's camera is being displayed. A new
complete diagnostic session and timestamp are needed to qualify a recurrence.
The first setup placed P1 below a raised platform at800/443 and killed them;
it is excluded. A genuine team retry and jump/arrival from above produced the
healthy qualified case. Private evidence:
`build-pr23/feedback28-sigma-facing-20261016/sigma-facing-safe-result.json`
and `build-pr23/sigma28-facing-native.txt`. Tracking: `beads-eio.9.154`.

### Playtest 28 package qualification (2026-10-16)

Items108,110,111,113-118 and120 ship with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment and requested blog link are above the title and
present in both packages. The README packaging revision matches the compiled-code snapshot.
Compiled source `cc12940`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows title checks pass; all 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `7b83b6e7953437c687944c325ab69d043826aaf9b8b3be54b54a8a735c24eabf` |
| Linux x86_64 AppImage | `7f62629763d19ae411bb3b779622db92422d67e432f18e3c0b7883e932c2373d` |

Private receipt: `build-pr23/release28-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.155`. Existing items53 and89 remain open.

### Playtest 29 package qualification (2026-10-16)

Item89 ships with production Windows ZIP and Linux x86_64 AppImage packages.
The README acknowledgment and requested blog link are above the title and
present in both packages. The README packaging revision matches the compiled-code snapshot.
Compiled source `764c4f5`, framework `9bd4c20f9`,
UI `248c347`. All 34 Windows title checks pass; all 28 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `dd88f257a5fdfc432e169b677a2e36c4db512c6a2b50f1c4f8d378d2c46c6438` |
| Linux x86_64 AppImage | `0ab54f96467132a51c8975f87a2e0330855ee9025700322ced13f40991781bdf` |

Private receipt: `build-pr23/release29-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.156`. Other unresolved reports remain on the numbered checklist.

### Playtest 30 package qualification (2026-10-16)

Items122/123/125/127/128/131/132/134/137/139/141-143 ship in this release.
Native checks also qualify124 (P2-led Dragoon admission),129 (X/P2 Ground
Hunter against Slash Beast), and138 (Sigma2 Web reaction) as working.
Checked fixes remain distinct from unconfirmed reports.
Of the21 reports122-142,15 are fixed or verified working in the qualified
cases. Reports126/130/133/135/136/140 remain unconfirmed or only partially
qualified, with their precise limits recorded in the table above.
The README acknowledgment and requested blog link are above the title and
present in both packages. The packaged README matches the compiled-code snapshot.
Compiled source `86eda19`, framework `9bd4c20f9`,
UI `248c347`. All 35 Windows title checks pass; all 29 Linux
title checks pass. Both production packages pass hidden launcher/controller/netplay
UI checks, 16 matching native option offers (including both cooldown methods),
execution/co-op identity checks, portable startup logs and fresh default input
recordings. Development controls are disabled. The AppImage layout passes;
archive hashes match the exact packages tested. This does not claim a new live
Windows-to-Linux gameplay qualification.

| Asset | SHA256 |
| --- | --- |
| Windows ZIP | `6cc037b478bda6389170ccc58c99ef548b48902276c3e36d06d79e4dc8ac4de1` |
| Linux x86_64 AppImage | `07d1750d951557e936fbeb8e2ca7190553d79abd6936195fbbf8e3a642c2a1f9` |

Private receipt: `build-pr23/release30-qa/release-receipt-final.json`.
Release tracking: `beads-eio.9.162`. Other unresolved reports remain on the numbered checklist.


### Playtest 41 package qualification (2026-10-19)

[Windows ZIP and Linux AppImage](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.41) publish compiled source `c2b74fa7e5bbc945b962022608f2110b28599c1a` through merge `113c0ede98625d5b2150a16e72b2064274513565`. Windows 36/36 and Linux 30/30 production checks pass. Both packages pass launcher/controller, native option-offer, default recording, startup-log and layout checks. Actual GitHub asset sizes/digests match the tested artifacts. Development controls and both loading-speed packages are absent.

Qualified fixes199/203/205/207 ship, plus the bounded scrolling-handoff correction within189. Dragoon206 remains unreproduced after checkpoint and real-portal controls; broader performance/elevator/background reports stay open. Single-slash157 remains unconfirmed. No fresh cross-OS netplay gameplay qualification is claimed.

| Asset | SHA256 |
| --- | --- |
| MegaManX4Recomp-v0.1.1-coop-playtest.41-linux-x86_64.AppImage | `34b12a2eef239e43166c1025d5e2b655bc6071fb10ede2a5588b008f0db8c1c7` |
| MegaManX4Recomp-v0.1.1-coop-playtest.41-windows-x64.zip | `360744d4922e0e7060718a68d68425a20a6cb7fb433a83955ee8c97715f019c7` |

Private receipt: `build-pr23/release41-qa/release-receipt-final.json`; native proof: `build-pr23/feedback41/linux-native-proof/`. Release tracking: `beads-eio.9.202`.


### Playtest 42 package qualification (2026-10-19)

[Windows ZIP and Linux AppImage](https://github.com/mstan/MegaManX4Recomp/releases/tag/v0.1.1-coop-playtest.42) publish compiled source `39377c115fbf82aa7e7442c5a65da5bbb11218a3` through merge `a4696a361f6d94fa7a124500d84e03c712893266`. Windows36/36 and Linux30/30 production checks pass. Both packaged launchers, complete native option offers and default input recordings pass. Startup-log and AppImage layout checks pass. Actual GitHub asset sizes and SHA256 digests match the qualified artifacts. Development controls and both loading-speed packages are absent.

Fixes189/206/208 ship. Their native qualification covers local8:3, including both Dragoon initiators and both Slash scenery controllers. Nearby intro shake198 and bee groups200 remain open; single-slash157 remains unconfirmed. No fresh online or cross-OS gameplay qualification is claimed.

| Asset | SHA256 |
| --- | --- |
| MegaManX4Recomp-v0.1.1-coop-playtest.42-linux-x86_64.AppImage | `fed93f6e859ae015bf440365209b9f4d850a095438fecb7c03772522cce3d109` |
| MegaManX4Recomp-v0.1.1-coop-playtest.42-windows-x64.zip | `410793f986ea5adafd3d079a584a39ec49c6102acd3e9914fb69063bf69a4b91` |

Private receipt: `build-pr23/release42-qa/release-receipt-final.json`. Release tracking: `beads-eio.9.206`.
