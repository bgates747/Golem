# GOLEM-009 — Complete Rally function and syntax review proposal

INV-001 was accepted at `bcea0c2`, and INV-002 at `8f1afe1` (both pushed).
SYN-001 and subsequent syntax discussions were checkpointed at `c83463b` (pushed).
The Author then authorized SEM-001 and reviewed its decisions conversationally.
The inventory and syntax review is delivered; GOLEM-012 is the separately
authorized checked-addition implementation. The deliverable inventories
all required Rally capabilities/functions against existing examples and adds
missing readable syntax examples to `examples.golem` for Author review.
Comparators and conditionals are already largely covered at the syntax-design
level; missing compiler implementation does not make an example a syntax gap.
This task does not authorize compiler implementation.

For the next tranche, defer assets/tables, indexed access and checked copying
to GOLEM-010, and typed drawing operations to GOLEM-011. Record these as deferred
inventory entries; do not develop their syntax examples in this tranche.

## Inventory and proposals

- INV-001 [x] Reconcile the GOLEM-001 capability comparison with current
  maintained Golem support and the examined Rally implementation. Distinguish
  already implemented features, existing syntax proposals and remaining gaps.
  Delivered [inventory](GOLEM-009/inventory.md) and
  [source audit](GOLEM-009/inventory-audit.json); accepted at checkpoint `bcea0c2`.
- INV-002 [x] Audit the existing `examples.golem` against that inventory. Record
  where each capability is illustrated and identify anything not covered.
  Treat comparator and conditional examples as the starting design; extend them
  only for actual uncovered cases rather than reproposing covered syntax.
  Delivered [coverage report](GOLEM-009/example-coverage.md) and
  [reference evidence](GOLEM-009/example-coverage.json); accepted at `8f1afe1`.
- SYN-001 [x] Add proposed syntax for uncovered functions/operations to the
  existing `examples.golem`, retaining already covered examples. Mark unimplemented
  proposals clearly. Every independent sample must include required typed
  declarations and initialization, with terminator-free syntax.
  Delivered 17 independent additions in
  [examples.golem](GOLEM-003/examples/examples.golem), with
  [traceability notes](GOLEM-009/syntax-additions.md) and
  [verification evidence](GOLEM-009/syntax-additions.json). Later review revisions
  add EX-018 through EX-021 and update Range/Bounds spelling; original SYN-001
  evidence is a historical snapshot, not the current file hash.
- SEM-001 [x] State supported types, ranges, conversion/rounding behavior,
  constraints and unresolved decisions beside the relevant proposals. Distinguish
  Rally's implemented behavior from any proposed extension.
  Author review requirement: explicit numeric conversions between integer,
  fixed-point and float types, including width changes and fixed-point fractional
  precision changes such as fixed16(4) to fixed16(8). Review precision loss,
  rounding and out-of-range handling; Rally's current fixed types only expose
  eight fractional bits. EX-017 illustrates candidate conversion notation.
  Include the [functions and scope review writeup](../design/language-type-proposals.md#functions-and-scope--review-writeup-2026-10-11):
  arbitrary supported parameter/result types, multiple results, lexical scope,
  shadowing, initialization, passing mode and composite ownership/lifetime.
  Delivered [semantics review](GOLEM-009/semantics-review.md) and
  [reference evidence](GOLEM-009/semantics-evidence.json); decisions recorded below.
  Subsequent conversational review accepted conversion, ByVal/result lifetime,
  scope/initialization, counter ownership and acyclic-call directions, then integer
  `/`, `%` and paired divmod. Decisions are recorded in the living language document;
  EX-021 uses shared divmod and EX-022 illustrates signed results. Checked
  arithmetic and finite resident error exit are accepted; custom handlers and
  ByRef are deferred. Concrete composite ABI work belongs with later feature
  implementation. Only GOLEM-012's checked-addition tranche is authorized now.
- REV-001 [x] Present the inventory and syntax examples for Author review before
  implementation; record decisions and create implementation tasks only when
  authorized.

Keep supporting audit files under `docs/tasks/GOLEM-009/`; the requested syntax
additions belong in the existing `examples.golem`. Execution is one subtask at a
time, followed by Author review. The review is disposed by recorded decisions,
explicit deferrals and the GOLEM-012 implementation authorization. This audit
itself does not authorize the remaining proposed compiler features.
