# 1.0.5 Sigma, Magma and opening-stage follow-up

Thanks also to Moro and JMOTZU for the continuing co-op playtests.

## Corrections and native checks

All runs use the verified, unmodified USA disc, private blank/cloned memory
cards, hidden windows and muted output. Checkpoint and positioning fixtures
bypass travel prerequisites; they never set a boss victory, door completion,
movie completion or scene-success flag. Captures marked `composed` show both
local Split panes. Ordinary screenshots alone do not qualify both panes.

## Mechanisms

Sigma 1 originally copied paired-door room bounds between the cameras every
frame. The trailing hero's stair-return strip could replace the partner's room
maximum (4160 with 3776), clamping the hero in the vestibule back to 4090.
The door now publishes its original enclosure once. Return strips remain local;
a hero who returns to the original admission box receives that enclosure again.
Door state and its enclosure participate in current-build snapshots and digests.

Native Nova sets a personal attack lock in `PLAY+1C`. The ordinary scene-camera
scope rejected that lock and culled the partner's waiting door against P1's
distant camera (native write 2B298 from C1994). Only an active native Nova
permits that scope; real scene/health freezes retain their existing guards.

Magma's original 7DF50 boss admission used the canonical P1 camera when P2
entered. The entering hero's camera now scopes that one admission and commits
only when the shared script selects that hero. Fight targeting and refights
retain the existing behavior. Both entry routes show intact arena scenery.

Sigma 2 reuses type 36 cannon beams already corrected for earlier stages. Its
stage guard now admits that same family in stage 11. Parent/pool/type checks
remain. Allocation, collision, animation and lifetime still run natively once.
Both beam directions stay beside the actual emitters (world X823..1344), with
either leader and either campaign; the waiting player's pane stays clear.

## Evidence and exclusions

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-release105/`.
Qualified collections include `sigma-qualified-x`, `movies-en-zero` (Sigma
controls), `magma-fresh-upper`, `magma-warp-probe`,
`magma-lower-admission-final`, `native-gateways-x/continued-probes`, and
`bees-final`. The earlier before captures are under
`F:/Temp/Codex/psxrecomp/x4-sigma-magma-103/`.

Invalid early fixtures are excluded: unsupported checkpoint 3 in Sigma 1;
teleports that bypassed the tower camera strips; premature Ride Armor boarding
or destinations inside solid/lava terrain; and a development armor fixture that
also marked all bosses cleared. That fixture now grants armor without boss
clears. Bee trace filtering was corrected to retain the relevant native PCs;
a full trace buffer is not a missing-enemy result.

The original boot EXE matches `SLUS_005.61;1` read from the verified USA disc.
SHA-256: `dae43d44a9f3176208830ddbba0607bb627ebcd7faf9b36fc03dd0ffda34d2e8`.
No modified base-disc support is added. This is bounded regression coverage,
not a newly completed full campaign or a claim that every older report is fixed.

A fresh Windows same-host two-peer Split run with simulated latency/jitter
passes input-only boot, both players' controls, own-view render isolation,
independent menu ownership/freeze, repeated matching core/mod CRC checkpoints
through tick 3360 and clean disconnect. Both own-view HUD captures were
inspected. This does not establish cross-OS netplay or fan-movie netplay; the
current trusted online profile does not activate offline language/movie mods.
