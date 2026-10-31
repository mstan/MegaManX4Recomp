# Audited native frame wait

SLUS-00561's complete 156-byte SDK routine at `0x800E4EF8` matches the
qualified Tomba caller after relocating its paired frame counter, warning
string, three callbacks and internal jump. It polls `0x8011DC50`; audited
edges are `0x800E4F20` and `0x800E4F6C`. Timeout callbacks remain
`0x800EDC74`, `0x800EDE8C` and `0x800EDE9C`.

Enhanced builds select portable native induction. Reference builds retain
original polling. Existing netplay/precise exclusions remain. The actual
regenerated routine is checked by `mmx4_vwait_contract`.

The framework pin backports the existing audited store-counter implementation
and campaign binding onto X4's prior framework lineage. This preserves its
host snapshot/guard, co-op, language and GL batch/atlas contracts. It does not
move X4 to the distinct X6 bridge package. Optional scaled-charge batching
stays off, with its selected value recorded in compatibility identity.

X4 additionally declares `NATIVE_IDLE_WAIT_EXCLUDE_DEBUG_TURBO`. Forced,
presentation-free debugger turbo retains original polling and clears detector
training across mode changes. Normal gameplay retrains and resumes induction.
This fixed policy is included in execution identity; ordinary fast-forward is
unchanged. The exclusion addresses a stage-loading stall found with the
qualified controller UI during debugger turbo; its deeper renderer cause is
not established. Normal pacing and the polling build passed that workload.

PR #84 is integrated; no new release or broad performance claim is implied.
Central evidence is tracked in `beads-x79c`, `beads-eio.3.421` and campaign
`beads-2wy8.1`.

## Native-span gameplay budget (1.0.20 regression)

Use the stock guest CPU budget (`guest_cycle_scale = 1`) in both development
and public configurations. The previous 2x gameplay budget was qualified with
the original instruction timing, before 1.0.20 selected native CPU spans.

PsyQ's one-frame wait initializes a 32,768-iteration timeout. The two native
spans in its polling loop charge 12 + 8 CPU cycles per iteration. At 2x CPU
budget that allows only 327,680 device cycles before the timeout callbacks,
less than the 564,480-cycle NTSC VBlank interval. A wait can therefore return
without the next VBlank and let another game update run. At the stock budget
the timeout allows 655,360 device cycles, enough for the next VBlank. The
shared CPU-span and audited-wait optimizations remain enabled.

Exact Windows public-package comparisons reproduced 59.94 co-op updates/s in
1.0.19 versus 120.00 in 1.0.20 and 119.93 in 1.0.23, with presentation and
audio clocks remaining normal. Changing only this setting restored 59.94 in
1.0.20 and 59.93 in 1.0.23; the latter's native main-loop, SDK VBlank and host
VBlank counters also advanced at 59.93 Hz. Independent native main-loop reads
in unmodified 1.0.20 confirmed extra updates between SDK VBlanks, so this is
not merely an added-player diagnostic or a display-refresh difference.

Central issue: `beads-eio.9.292`. Exact inputs, settings, archive digests,
raw wall-clock samples and reports are retained under
`G:/Temp/Codex/psxrecomp/x4-speedup-20261031`.

Single-player qualification on the exact 1.0.23 public executable with the
corrected config measured 59.94 native main-loop, SDK VBlank and host VBlank
updates/s. From the same X spawn, a 1.5-second right input moved 115.0625 px,
matching 1.0.19 exactly; unmodified 1.0.20 moved 289.0625 px.

The corrected public timing config also passed the existing native two-peer
Split delay-sync check on the development executable: independent movement
and attacks, both pause menus, own-view presentation, matched core/mod CRCs
through tick 3639, and native disconnect handling. The existing generated
frame-wait contract test passed. These checks used Windows; no separate
Linux runtime or physical LAN-machine claim is made.
