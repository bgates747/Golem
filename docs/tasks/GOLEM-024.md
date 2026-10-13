# GOLEM-024 — Shared scalar frontend and portable state ABI

Implement the first shared foundation for the Author's one-source/two-backend
requirement: a task-local C++ typed frontend and deterministic state planner.
The same source must produce identical common interface/data artifacts for
`buffered` and `vdplet` selections. This tranche establishes an emission boundary;
it does not claim to compile executable buffered or native programs yet.
High thinking is appropriate for this semantic/interface boundary.

`TODO.md` owns overall status. Requirements and the staged roadmap are in
[backend-portability.md](../design/backend-portability.md). All prototype code,
examples, tests and reports belong under `docs/tasks/GOLEM-024/`. Production
promotion, backend emitters, runtime services, graphical profiles and physical
deployment are outside this first tranche.

## Scope and acceptance

Start with explicitly initialized uint16/float32 globals, optional inclusive
Bounds, finite named parameterless Subs and same-type assignment expressions
using literals, globals, parentheses and +, - and *. Use the maintained token
reader and buffer allocator. Newlines remain whitespace; keywords ignore case
and identifiers retain case. Unsupported control flow, calls, conversions and
types must fail clearly rather than imply their implementation.

The common model records source locations, types, expression structure, global
layouts and logical entry identity. Shared storage is allocated before anything
target-specific; private code IDs/addresses are not part of this common layout.
Publish a target-independent manifest, initialized state bytes and assembly
layout equates, plus a separate target-requirements descriptor stating that no
executable output is supplied. Include generated language-error storage with a
documented scope; do not redefine the runtime's VDPL_E_* codes.

A host reference evaluator may exercise assignment/error/state semantics for
this subset. It is a reference model, not either backend and not target evidence.
Check uint16 intermediates and assignment Bounds before stores, preserve the
failed destination and earlier writes, reset status per invocation, and retain
state across calls. Use ordinary binary32 rounding for host reference examples
with contraction disabled; physical exceptional/subnormal behavior remains open.

## Subtasks

- FND-001 [x] Build the shared typed parser/model for the bounded scalar grammar;
  reject implicit declarations, wrong-type operands, unsupported statements and
  invalid known constants before publication.
- LAY-001 [x] Allocate and serialize one common packed state layout, inclusive
  field Bounds, initial bytes and logical entries. Reserve externally owned IDs
  before allocation. Keep target identity out of common artifacts; publish honest
  target requirements separately. Guard generated assembly symbol collisions.
- REF-001 [x] Add the small host reference evaluator and explicit source/expected
  cases for successful arithmetic, overflow/underflow, narrow Bounds, failed
  destination preservation and repeated invocation without state reset.
- TST-001 [x] Verify both selections emit byte-identical common artifacts for
  default and relocated layouts. Exercise malformed/type-invalid source and
  reservation failures, and ensure rejection happens before output files change.
- TST-002 [x] Run targeted ASan/UBSan checks and maintained compiler checks;
  verify the existing compiler CLI and output modes remain untouched.
- REV-001 [x] Deliver readable initialized examples, reproduction commands,
  results and the exact boundary to the next emitter tranche. Stop for review;
  do not promote the prototype or imply native/stock execution was tested.

The implementation must stay proportionate: no optimizer, general IR framework,
automatic backend partitioning, runtime capability probe, heap or state migration.
Further language/backend work should build on this common model after review.

## First-tranche delivery — 2026-10-13

[Report and commands](GOLEM-024/README.md),
[initialized shared source](GOLEM-024/shared-example.golem) and
[source-bound receipt](GOLEM-024/checks.json) are ready for review.
ASan/UBSan host contracts pass 44 checks and 42 rejection cases. Default/relocated
plans give eight identical common artifact pairs across target selections;
ten CLI rejection cases preserve files, and two real assembler consumers check
33 equates each. Maintained compiler checks, including 132 rejection cases, pass.
This is host frontend/layout/reference evidence only. No backend executable,
native runtime change, emulator fixture or physical deployment was produced.
