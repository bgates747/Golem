# GOLEM-009 — Complete Rally function and syntax review proposal

Queued at the Author's request; do not begin yet. The deliverable will inventory
all required Rally capabilities/functions against existing examples and add
missing readable syntax examples to `examples.golem` for Author review.
Comparators and conditionals are already largely covered at the syntax-design
level; missing compiler implementation does not make an example a syntax gap.
This task does not authorize compiler implementation.

For the next tranche, defer assets/tables, indexed access and checked copying
to GOLEM-010, and typed drawing operations to GOLEM-011. Record these as deferred
inventory entries; do not develop their syntax examples in this tranche.

## Inventory and proposals

- INV-001 [ ] Reconcile the GOLEM-001 capability comparison with current
  maintained Golem support and the examined Rally implementation. Distinguish
  already implemented features, existing syntax proposals and remaining gaps.
- INV-002 [ ] Audit the existing `examples.golem` against that inventory. Record
  where each capability is illustrated and identify anything not covered.
  Treat comparator and conditional examples as the starting design; extend them
  only for actual uncovered cases rather than reproposing covered syntax.
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
additions belong in the existing `examples.golem`. Do not generate the inventory
or edit examples merely by queuing
this task. Await the Author's instruction to begin after the current list review.
