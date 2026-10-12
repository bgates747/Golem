# GOLEM-008 — Promote the qualified resident scalar slice

Make the GOLEM-007-qualified scalar backend and resident output mode available
through maintained `golemc`, without changing numeric command expansion or the
21-byte fresh-input/call contract. The Author explicitly authorized promotion
after successful Oryx human review and approval to commit GOLEM-007.

## Promotion

- API-001 [x] Extract ordinary parsing/emission into a maintained reusable API,
  eliminating the production need to include a CLI implementation or rename main.
- API-002 [x] Promote scalar emission and resident grammar/manifest into `src/`,
  keeping the limited finite/serialized/host-validated contract explicit.
- CLI-001 [x] Add explicit `--resident` and inclusive `--reserve FIRST:LAST`
  options to maintained golemc. Preserve raw payload bytes and failure-before-write
  behavior; do not infer output mode from source.
- DOC-001 [x] Promote initialized floor/counted-floor examples and the output
  contract into maintained documentation; preserve all task historical artifacts.

## Validation and disposition

- VER-001 [x] Compare maintained raw and resident command streams against the
  exact accepted checkpoint, across relocated layouts and boundary values.
  Run focused sanitizer/parser/allocation/CLI rejection checks.
- VER-002 [x] Verify production compilation with only maintained source files
  available and compare all GOLEM-007 package artifacts. Reuse existing firmware
  evidence only when the relevant generated command bytes are identical.
- DEL-001 [x] Record hashes, commands, reusable durations and limitations in
  this silo, update status/specification, and deliver the completed tranche.

No new syntax, numeric functions, firmware, profile or deployment changes are
part of promotion. Keep historical test-only policies in their original silo;
no new runtime admission checks, loop semantics or optimization pass. Semantic
errors must precede file writes; filesystem failure publication remains non-atomic.

## Completed tranche

Promotion delivered in maintained `src/`, `tests/` and `examples/resident/`.
See [results](GOLEM-008/README.md) and [host evidence](GOLEM-008/host-checks.json).
GOLEM-003 promotion follow-up is satisfied; GOLEM-002 remains queued.
