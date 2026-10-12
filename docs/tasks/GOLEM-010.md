# GOLEM-010 — Assets, indexed access and checked copying

All review subtasks are delivered. The Author accepted the storage/copy direction
and authorized final synthesis after example consolidation. The
[implementation recommendation](GOLEM-010/implementation-recommendation.md)
awaits review: generated assembly output first, then a small typed table job.
No compiler implementation has begun.
[Storage findings](GOLEM-010/assets-and-tables-review.md),
[AGNB loading](GOLEM-010/agnb-loading-review.md),
[indexed access](GOLEM-010/indexed-access-review.md),
[checked copying](GOLEM-010/checked-copy-review.md) and
[example review](GOLEM-010/examples-review.md).

- AST-001 [x] Examine assets/table imports, typed layouts, ownership and bounds.
- IDX-001 [x] Examine indexed record/element access and failure behavior.
- CPY-001 [x] Examine checked field/record copying and validation requirements.
- SYN-001 [x] Audit existing examples and add missing syntax proposals to
  `examples.golem`, including explicit typed declarations and initialization.
- REV-001 [x] Present the proposals, constraints and implementation options
  for Author review.

Keep supporting files under `docs/tasks/GOLEM-010/`. Stop for final recommendation review before implementation.

Assembly integration clarification: PC compilation publishes layout and payloads;
the consuming assembly application owns loading, updates, calls and cleanup.
File mappings belong to build configuration. Generated assembly equates and
payload definitions are the intended additional output, still to be implemented.
Review that interface with the remaining proposals; no implementation is
implicitly authorized. Interactive Golem/MOSlet applications remain future options.

The Author accepted named assembly wrappers and authorized the next implementation
tranche. GOLEM-010 review is accepted; GOLEM-016 now tracks assembly emission.
Historical recommendations here remain the rationale for the table follow-up.
