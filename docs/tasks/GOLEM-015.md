# GOLEM-015 — Resident uint16 divmod

Implement paired `(quotient, remainder) = divmod(numerator, denominator)` with
uint16 destinations and operands. Evaluate operands once, left to right. Division
by zero preserves both destinations and exits the resident job with status 4.
Use a shared finite VDP routine; recurring application wire traffic stays unchanged.

- IMP-001 [x] Parse paired assignment, reject duplicate/type-invalid destinations,
  add bounded unsigned division and the resident zero-divisor error path.
- TST-001 [x] Qualify full-width operands, quotient/remainder identities, aliases,
  expression arguments, failed operand evaluation, preservation, recovery,
  relocation and lifecycle using independent expectations and sanitizers.
- VDP-001 [x] Qualify stock Fab/MOS and stage the stopped Oryx fixture. The Author
  runs/closes the emulator; the agent retrieves saved results afterward.
- REV-001 [x] Report evidence and stop for human review. Leave this tranche
  uncommitted until human validation and explicit commit approval.

Signed division, / and % syntax, general functions and raw mode remain deferred.
Keep supporting fixtures, scripts and evidence under GOLEM-015/.

Implementation, automated qualification and Oryx human review pass. Saved results
were retrieved and independently verified. The Author explicitly approved commit and push. See the
[review instructions and evidence](GOLEM-015/README.md).
