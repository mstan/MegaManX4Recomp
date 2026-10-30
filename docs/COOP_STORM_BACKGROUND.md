# Storm Owl independent background scrolling

Owner report: P2's background jumps for a frame, and the aircraft appear at
different positions while both heroes stand together at the stage entrance.
This was reported in local 8:3 Split. Central Beads intake is pending because
the database on port3307 is unavailable.

A read-only sample from the owner's running preview confirms identical
foreground X origins at0. P1's near/far origins are1655/1220.5; P2's are
6263/-2363.5. The layers use native mode5, with authored screen bounds2..7
and2..8. Native renderer273F8..274CC wraps these scrolling origins during
drawing. Private drawing restores RAM afterward, so that wrap cannot persist
in a saved camera by itself.

The existing scrolling-plane adapter already retains those native wraps for
Slash Beast but explicitly excludes Storm Owl. Apply the same mode5 wrap to
Storm Owl's saved planes. Each camera keeps its own phase, fractional X,
previous origins and foreground; the shared controller still advances once.

Boundary regression coverage includes both canonical owners, rightward near
scroll and leftward far scroll, fractional preservation, previous-origin
preservation and one native controller call. Native original/fixed validation
uses a hidden, muted copy, preparing the scrolling origins just before their
authored bounds and allowing the real controller and renderer to cross them.
This is a correctness check, not a runtime performance measurement.

Private evidence: `G:/Temp/Codex/psxrecomp/x4-stage-select-20261030/`, including
`storm-background-evidence`, `qa-storm-baseline`, `qa-storm-fixed` and the
capacity/source/binary receipts. The original build reproduces escaped
origins: after the boundary, P2 reaches2092.75/231.125 while P1 has wrapped
to559.75/2021.375. The corrected build keeps both planes within their native
ranges for all102 recorded observations. Inspected captures retain the
aircraft in both views. Their small prepared phase offset comes from the
asynchronous origin setup; no claim of exact image equality is made.

The split, lifecycle, combat and hook-registration regression checks pass.
This native comparison used local8:3; a new network gameplay check is not
claimed. The temporary Windows job cap applies only to these build/QA
processes and their children, not the owner's game or shared services.
