# GOLEM-012 — Checked uint16 addition and resident error exit

Implement the Author-approved first checked-arithmetic slice: resident uint16
addition detects overflow before destination writes, stops the current job and
returns control to normal VDP processing. Preserve earlier successful writes;
no implicit transaction, custom handler or MOS acknowledgement. Keep recurring
application wire traffic unchanged. Raw-mode wrapping is outside this tranche.

Overall status belongs in [TODO.md](../../TODO.md). Keep task fixtures, scripts,
build products and evidence under docs/tasks/GOLEM-012/. Compiler implementation
and maintained contract tests belong in their existing production files.

- DEC-001 [x] Record accepted checked-arithmetic/default-error decisions and defer
  error handlers, Resume/retry, ByRef and native return transport explicitly.
- IMP-001 [x] Add resident carry checking after every addition, whole-assignment
  commit on success, and a finite error exit with per-invocation status metadata.
  Handle self-reference/chains and avoid the firmware's 0xFFFF conditional sentinel.
  Reject compile-time overflowing literals and unsupported bootstrap additions.
- TST-001 [x] Run maintained host checks and sanitizer checks. Independently
  execute emitted command bytes for arithmetic boundaries, early/late failure,
  preserved destinations/prior state, skipped later statements and recovery.
  Check relocation, owned cleanup, installation and recurring wire bytes.
- VDP-001 [x] Qualify the emitted control flow against stock Fab/MOS in a headless
  run, separate from the host model. Do not modify upstream sources or launch a
  graphical emulator. Stage the authorized Oryx human review; preserve the
  canonical profile-local launcher and explicit keyboard-response instructions.
- REV-001 [x] Report evidence and remaining limits, then stop for review. Leave
  emulator-related changes uncommitted/unpushed until required human testing and
  explicit approval. Do not implement general conversions/division/handlers here.

Delivered [implementation and review notes](GOLEM-012/README.md),
[host evidence](GOLEM-012/host-checks.json),
[stock-emulator evidence](GOLEM-012/emulator-checks.json) and
[Oryx staging evidence](GOLEM-012/oryx-review.json).
Automated qualification is complete and the Author confirmed human review worked.
The guest now prints the keyboard instructions itself; The Author explicitly authorized commit and push.
