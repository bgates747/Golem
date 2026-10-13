# GOLEM-030 — Paired integer division on both backends

Implement (quotient, remainder) = divmod(numerator, denominator) in the shared
task-local compiler. Destinations must be distinct uint16 globals. Arguments
are evaluated once, left to right, before either output changes; input/output
aliasing is supported. Division by zero reports status 4. Arithmetic failures
or either destination Bounds failure preserve both outputs and prior effects,
then stop the invocation. Both destinations are admitted before either write.
Keep production compiler and generic VDPlet SDK/runtime unchanged.

- SYN-001 [x] Parse paired assignments, type-check destinations/arguments and reject active-counter writes.
- EMIT-001 [x] Reuse the qualified buffered kernel and emit native division/remainder with matching failures.
- CHK-001 [x] Verify boundaries, aliasing, evaluation order, zero divisors, Bounds and loops on both backends.
- CHK-002 [x] Qualify stock and actual Xtensa engines at default/relocated IDs with retained state.
- VIS-001 [x] Stage complete source and stopped Oryx animation using quotient for phase and remainder for green blinking.
- REV-001 [x] Obtain human review and explicit commit/push approval.

## Evidence

[Report/source/review command](GOLEM-030/README.md),
[host receipt](GOLEM-030/host-checks.json) and [target receipts](GOLEM-030/evidence/).
Stock and both actual Xtensa engines pass; Oryx visual review is staged/stopped.
The Author accepted visual review with “looks good”; saved results match 637
observations/6,289 bytes and runtime identities pass. The Author explicitly approved commit/push on 2026-10-13.
