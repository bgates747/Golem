# GOLEM-011 — Typed drawing operations

Review Rally's typed drawing capabilities and propose readable syntax for later
Author review. Following the accepted table tranche, the Author authorized
DRW-001 and then SYN-001 as separate tranches. Both are complete; the Author
approved the syntax and authorized WIR-001, then REV-001. All four review subtasks
are complete. The Author accepted the geometry implementation scope and
authorized GOLEM-018, which now supplies compiler support and a staged demo.

- DRW-001 [x] Examine supported drawing operations, operand types, graphics
  state and external resource requirements.
- SYN-001 [x] Audit existing examples and add missing syntax proposals to
  `examples.golem`, including explicit typed declarations and initialization.
- WIR-001 [x] Describe how resident drawing jobs can reduce recurring wire
  traffic, with explicit installation and invocation contracts.
- REV-001 [x] Present the proposals, constraints and implementation options
  for Author review.

Keep supporting files under `docs/tasks/GOLEM-011/`. The review is complete;
no compiler implementation is included.

DRW-001 is complete: [inventory and evidence](GOLEM-011/drawing-inventory.md).
Four existing Rally host regressions pass with ASan/UBSan.

SYN-001 is complete: [syntax proposals and contracts](GOLEM-011/drawing-syntax.md),
with EX-030 through EX-036 in [examples.golem](GOLEM-003/examples/examples.golem).
All five inventory families now have independently initialized examples.
Named geometry/scroll operations, full bitmap buffer IDs and perimeter-ordered
convex quads are proposed for review. Source inspection clarified Rally's quad
triangle ordering in the inventory. Existing examples are unchanged; no compiler
or emulator changes were made. The Author approved SYN-001.

WIR-001 is complete: [traffic and host contract](GOLEM-011/drawing-wire-contract.md)
and [source-derived byte accounting](GOLEM-011/wire-counts.json). Resident reuse
and shared inputs provide savings; a fully refreshed isolated primitive can cost
more than direct commands. Recommend points/lines/convex quads first, with packed
input storage and named assembly invocation. Installation figures are projections,
not compiled drawing artifacts.

REV-001 is complete: [first pixel implementation recommendation](GOLEM-011/implementation-review.md).
Recommend a standalone resident point/line/convex-quad slice, packed inputs,
generated assembly calls and a staged Oryx pixel demonstration. Reuse compiler
infrastructure and Rally's mechanisms without importing its entire hosted
language. Host/assembler, stock-runtime and human qualification are specified.
The Author accepted this scope and authorized GOLEM-018. Other graphics families
remain later implementation. GOLEM-011 is accepted and removed from the active
TODO; this document preserves the review and rationale.
