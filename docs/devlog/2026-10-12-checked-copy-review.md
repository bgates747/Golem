# GOLEM-010 / CPY-001 — Checked-copy review

CPY-001 is complete and awaits Author review. Recommend ordinary assignment for
typed fields and whole rows, explicit cross-type conversions, and runtime
admission before destination writes. Failure preserves the whole destination
and exits the job under the existing resident error direction. Compiler code
and shared examples are unchanged; SYN-001 has not begun.

Rally CopyChecked supports runtime admission for matching integer/fixed scalar
formats but rejects f32. Its caller receives a status flag and continues after
failure. CopyRecord relies on compile-time field-layout/range compatibility and
has no runtime whole-row narrowing path. Golem should check every uncertain
field before committing any part of a whole row, retaining value semantics and
avoiding status boilerplate in ordinary assignments.

The proposal addresses external host updates, known constants, aliases,
read-only assets, explicit initialization, indexed-write evaluation order and
bounded validation/commit lowering. It distinguishes compiler-defined admission
errors from VDP memory/transport failures for which stock buffered commands
provide no general acknowledgement. Assembly owns installation, updates and
calls; generated interface metadata/assembly definitions expose the locations.

The existing Rally checked-copy regression passed warnings-as-errors and
ASan/UBSan in 41.54 seconds, measured locally with a monotonic clock. Sources
remained unchanged and generated artifacts stayed in the task silo. This is
host/source evidence, not new whole-record or float runtime qualification.
No emulator was launched.

See [checked-copy proposal](../tasks/GOLEM-010/checked-copy-review.md) and
[copy audit](../tasks/GOLEM-010/copy-audit.json). Pause before SYN-001 until review.

The Author accepted CPY-001 and clarified the complexity budget: this is a hobby
project, so keep useful checks proportionate. There is no post-copy verification
in Rally's copy path. Avoid readback/checksum/rollback machinery and repeated
admission for trusted values; validate changing inputs at the job boundary and
reuse validity until a write invalidates it. SYN-001 remains unstarted.
