# GOLEM-005 — Terminator-free parser tranche

Implement the accepted grammar for existing ordinary statements and the narrow
scalar job using a shared token reader. Preserve generated commands and output
contracts. The Author authorized this tranche after reviewing GOLEM-004;
allocation, resident integration and new control-flow backends remain separate.

## Scope

- PAR-001 [x] Add a shared token reader with source spans, case-insensitive
  keyword matching, preserved identifier spelling and checked numeric conversion.
- PAR-002 [x] Migrate maintained print, uint16 declarations/assignments/addition
  and named For/Next to grammar-defined boundaries; reject semicolons explicitly.
- PAR-003 [x] Add a terminator-free scalar frontend in this task's silo using
  the unchanged GOLEM-003 backend. Preserve historical prototype/oracle files.

## Verification and delivery

- VER-001 [x] Compare old/new ordinary and scalar command streams, including
  adjacent/multiline statements, nested loops, casing, literal boundaries and
  failed compilation preserving existing output. Run focused sanitizer checks.
- DOC-001 [x] Migrate maintained source examples, publish syntax/reserved-name
  migration notes, record evidence and update the authoritative register.

## Acceptance and boundaries

No statement terminators or significant newlines. Explicit declarations remain
required. Malformed/oversized numeric input reports a located error and exits
normally. Keywords/builtins are case-insensitive; identifiers remain sensitive.
Reserve the reviewed control-flow/builtin names consistently, without claiming
implementation of If/Do/Switch, general expressions, Range extensions or Bounds.
Production output remains the existing raw payload; experimental scalar output
remains the existing resident installation/update/call contract. No firmware,
emulator, deployment, allocator or native-module change is included.

Artifacts and checks live under `docs/tasks/GOLEM-005/`. The shared token reader
and ordinary parser are maintained compiler changes explicitly authorized by
this task; the scalar successor is experimental, not production promotion.

Delivered: [implementation and migration note](GOLEM-005/README.md),
[host evidence](GOLEM-005/host-checks.json). Address/undefined-behavior sanitizer
checks pass: 20 ordinary and 22 scalar byte matches, 69 rejection checks,
source spans/casing/defaults. Ten CLI rejection cases preserve existing output;
both migrated executable examples match their pre-tranche bytes. No firmware
execution or deployment. Overall review status remains in TODO.md.
