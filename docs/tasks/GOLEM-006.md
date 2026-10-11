# GOLEM-006 — Shared allocation and resident scalar integration

Integrate the selected floor/conversion job with ordinary uint16 state through
one checked allocation policy and an explicit experimental resident output mode.
Preserve the ordinary payload contract and the Rally-compatible numeric command
expansion. The Author authorized this tranche after committing GOLEM-005.
Firmware qualification and scalar production promotion remain separate.

## Allocation

- ALC-001 [x] Add shared checked buffer claims, external reservations and
  exhaustion handling. Preserve ordinary IDs under its compatibility policy.
- ALC-002 [x] Pass allocation into the scalar emitter for every public/private
  buffer and helper; retain an exact Rally compatibility policy for verification.

## Integration and lifecycle

- INT-001 [x] Provide a task-local integrated CLI with explicit resident output;
  reuse maintained ordinary parsing/emission and shared token reading. Support
  uint16 globals and print/uint16 assignment around one scalar floor/conversion
  in a single finite Sub job. Reject unqualified resident loops and local declarations.
- INT-002 [x] Publish input/output/entry/global metadata, external reservations,
  exact installed ownership and cleanup. Keep installation, update, invocation
  and cleanup distinct; preserve host-bound/serialized-job obligations.

## Verification and delivery

- VER-001 [x] Check collisions, reserved IDs, exhaustion and independent decoders
  for relocated spans/helper targets. Compare compatibility bytes and normalized
  relocated operations; demonstrate repeated input updates without reinstall.
- VER-002 [x] Run focused sanitizer/CLI checks and ordinary parser compatibility;
  reject invalid sources/reservations before touching output files.
- DOC-001 [x] Deliver initialized examples, reproducible host evidence, migration/
  output-profile contract and task disposition in the devlog and TODO.

## Boundaries

Only the shared allocator and its ordinary-parser use are maintained compiler
changes in this tranche. Adapted scalar emission, resident frontend, integrated
CLI, reports and tests stay under `docs/tasks/GOLEM-006/` until qualification and
promotion are agreed. No maintained source may depend on task-silo files.
Do not change GOLEM-003/004/005 historical artifacts, stock firmware, emulator,
deployment, general If/Do/Switch/Range behavior, local frames, or native modules.
No runtime float admission, concurrency, optimizing IR or automatic output-mode
selection. File publication retains the existing non-atomic I/O limitation.

Delivered: [integration contract and results](GOLEM-006/README.md),
[host evidence](GOLEM-006/host-checks.json), and complete initialized
[floor](GOLEM-006/examples/floor.golem)/[counted-floor](GOLEM-006/examples/counted-floor.golem)
examples. Host checks pass; no firmware execution or deployment. Overall review
status remains in TODO.md.
