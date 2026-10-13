# GOLEM-028 — Typed Boolean values on both backends

Implement explicitly initialized bool globals, true/false literals, Boolean
copying, assignment from uint16 comparisons and If/ElseIf Boolean conditions.
The Author approved this bounded tranche and light thinking on 2026-10-13.
Compound Boolean operators remain deferred.

Use one task-local shared frontend and both emitters, forked from GOLEM-027.
Preserve historical tasks, production compiler and generic runtime/SDK. Store
bool as a two-byte canonical 0/1 value for this prototype, with distinct type
tag 6. Reject other external encodings at entry with Bounds status 32. No implicit
numeric/Boolean conversions or arithmetic on bool. Comparisons accept uint16
operands, evaluate once left to right and preserve failed Boolean destinations.

## Subtasks

- SYN-001 [x] Add bool declarations/literals, comparison expressions and typed
  Boolean conditions; preserve existing uint16/If/Range grammar.
- EMIT-001 [x] Emit canonical Boolean results, admission and conditions on both
  targets with common layout, error and ownership behavior.
- CHK-001 [x] Verify all six comparison assignments, copying, literals, loops,
  failures and invalid external encodings; run sanitizer, rejection and maintained checks.
- CHK-002 [x] Qualify stock buffered and actual Xtensa JIT/interpreter paths,
  relocated IDs, backend changes and state retention.
- VIS-001 [x] Stage a visibly different Boolean-driven traffic-light review on
  Oryx with readable source, pixel/state checks and a stopped dedicated profile.
- REV-001 [x] Retrieve and verify human results; the Author explicitly approved commit/push.

## Evidence

See [report/source/review command](GOLEM-028/README.md),
[host receipt](GOLEM-028/host-checks.json) and [target evidence](GOLEM-028/evidence/).
The Author accepted the visual review ("looked good to me"); retrieved results
match all 533 observations/5,249 bytes. Keyboard success was not separately
stated. The Author explicitly approved commit and push on 2026-10-13.
