# GOLEM-003 — Readable scalar floor and conversion over Rally command generation

Build a small C++ prototype connecting readable Golem source to selected Rally
numeric operations. Start with `result = sint16(floor(x))`, retaining Rally's
true floor before conversion, including its tiny-negative-input correction.
The Author authorized proceeding on 2026-10-11 after the GOLEM-001 review.
Keep code, fixtures, tests and reports in `docs/tasks/GOLEM-003/`; promotion into
the maintained compiler is a later decision. Overall status belongs in TODO.md.

First tranche delivered 2026-10-11: [prototype and contract](GOLEM-003/README.md),
[readable example](GOLEM-003/examples/floor.golem), and
[host evidence](GOLEM-003/host-checks.json). All items below are complete for the
task-local prototype. Installation/cleanup match Rally for 20 cases; existing
main compatibility, preserved Rally boundary checks and independent command
decoding pass. Installation is 1,008 bytes and each input update/call is 21 bytes.
No VDP firmware execution, deployment or production promotion occurred. The
concrete prototype is ready for review; wider integration remains unstarted.

## Intended first tranche

Use ordinary-looking declarations, mandatory semicolons and a named finite
`Sub ... EndSub` job. Restrict the prototype to one bounded float32 input, one
signed 16-bit output and the floor/conversion operation. These spellings are
experimental; preserve the existing production compiler and its syntax.

Extract the relevant command-generation machinery from Rally behind a typed
C++ interface, separate from both parsers. The readable frontend should call
that interface directly, rather than generate Rally source text. Preserve an
identified Rally reference and compare output against its checked implementation.
Define only storage, command grouping and patch addresses needed for this slice.
Prefer multiple commands per block; a small job may occupy one block. General
functions, optimizing IR, automatic inlining, arbitrary expressions and native
Xtensa modules are outside this tranche.

The result must install resident input/output/scratch/job buffers once and
provide the small repeated input-update/call sequence separately. Metadata must
identify external input/output and the callable entry. Declared numeric bounds
are a host obligation, not an implicit runtime admission check. No firmware,
emulator profile, deployment or hardware change is part of this tranche.

## Source and contract

- SRC-001 [x] Preserve the examined Rally compiler/test baseline in the task
  silo, with commit identities, provenance and a reproducible oracle build.
- DES-001 [x] Specify the readable example, finite numeric domain, allocation,
  job/block layout, patch locations, initialization and repeated-call contract.
  Show the corresponding existing Rally source.

## Implementation

- IMP-001 [x] Extract the required scalar load, true floor and signed-word
  conversion emission into a typed backend. Keep compiler/application policy
  separate and retain the important numeric restrictions.
- IMP-002 [x] Add a task-local C++ frontend and CLI for the experimental slice,
  with clear errors, compiler-owned scratch and generated interface metadata.
  Do not modify the maintained compiler or the Rally worktree.

## Verification and delivery

- VER-001 [x] Run proportionate host checks: existing main compiler baseline,
  matched Rally bytes for the equivalent program, numeric/range and syntax
  rejection cases, decoded storage/patch structure and repeated wire accounting.
  Use strict compiler warnings and address/undefined-behavior sanitizers.
  Distinguish generated-byte checks from execution on VDP firmware.
- DEL-001 [x] Record results, exact commands, limitations and the next smallest
  integration step in the silo. Update this task and the dated devlog, commit
  and push the tranche, and present the concrete result for review. Keep physical
  execution and production promotion explicitly separate from host evidence.
