# GOLEM-029 — Compound Boolean expressions

Add typed Not, And and Or to the shared task-local frontend and both backends.
Precedence is arithmetic, comparison, Not, And, Or; parentheses override it.
And skips its right operand when false; Or skips it when true. Evaluation is
left to right. A skipped expression cannot raise an arithmetic error. Entry
admission still checks every externally supplied global. Preserve canonical
0/1 storage, prior effects and failed destinations. No implicit numeric truthiness.
Production compiler and generic VDPlet SDK/runtime remain unchanged.

- SYN-001 [x] Implement grammar, precedence, parentheses and bounded nesting.
- EMIT-001 [x] Implement short-circuit evaluation on both backends.
- CHK-001 [x] Test truth tables, precedence, nested expressions and skipped/evaluated failures.
- CHK-002 [x] Qualify stock and actual Xtensa engines with relocation and state retention.
- VIS-001 [x] Stage the complete Golem source and visual review on Oryx, stopped.
- REV-001 [x] Obtain human review and explicit commit/push approval.

## Evidence

[Report/source/review command](GOLEM-029/README.md),
[host receipt](GOLEM-029/host-checks.json) and [target receipts](GOLEM-029/evidence/).
Both Xtensa engines and stock VDP pass. The Author accepted human review with
“checks out”; retrieved results match all 533 observations/5,249 bytes and runtime
identities pass. The Author explicitly approved commit/push on 2026-10-13.
