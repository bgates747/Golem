# GOLEM-009 — Complete Rally function and syntax review proposal

INV-001 was reviewed and committed/pushed at `bcea0c2`. INV-002 alone was then
authorized; its example coverage audit is complete and awaits Author review.
Do not proceed to SYN-001 yet. The deliverable will inventory
all required Rally capabilities/functions against existing examples and add
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
  [reference evidence](GOLEM-009/example-coverage.json); awaiting Author review.
- SYN-001 [ ] Add proposed syntax for uncovered functions/operations to the
  existing `examples.golem`, retaining already covered examples. Mark unimplemented
  proposals clearly. Every independent sample must include required typed
  declarations and initialization, with terminator-free syntax.
- SEM-001 [ ] State supported types, ranges, conversion/rounding behavior,
  constraints and unresolved decisions beside the relevant proposals. Distinguish
  Rally's implemented behavior from any proposed extension.
- REV-001 [ ] Present the inventory and syntax examples for Author review before
  implementation; record decisions and create implementation tasks only when
  authorized.

Keep supporting audit files under `docs/tasks/GOLEM-009/`; the requested syntax
additions belong in the existing `examples.golem`. Execution is one subtask at a
time, followed by Author review. SYN-001 and later subtasks remain unexecuted;
no examples or compiler code have been changed by INV-001/INV-002.
