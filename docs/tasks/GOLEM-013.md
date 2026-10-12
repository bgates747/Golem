# GOLEM-013 — Checked resident uint16 subtraction

Implement checked subtraction in the existing resident scalar job, reusing the
finite error-exit contract. Underflow preserves the complete assignment's
destination and earlier successful statements. Recurring wire traffic remains
unchanged. Raw-mode subtraction, signed arithmetic and other operations are deferred.

- IMP-001 [x] Parse resident binary + and - with equal precedence and left-to-right
  evaluation; check each intermediate result. Reject constant underflow and
  variable bootstrap subtraction. Add resident underflow code 2.
- TST-001 [x] Check zero/full-width/byte-borrow boundaries, mixed chains,
  self-reference, skipped statements, recovery, relocation and cleanup using
  maintained tests, an independent command model and sanitizers.
- VDP-001 [x] Qualify stock Fab/MOS execution and stage a stopped dedicated Oryx
  review with instructions printed by the guest itself.
- REV-001 [x] Report results and stop for human review. Leave this tranche
  uncommitted until human testing and explicit commit approval.

All fixtures, scripts, builds and evidence belong under `docs/tasks/GOLEM-013/`.

Delivered [review notes](GOLEM-013/README.md), [host evidence](GOLEM-013/host-checks.json),
[stock-emulator evidence](GOLEM-013/emulator-checks.json),
[addition compatibility](GOLEM-013/addition-compatibility.json) and
[Oryx staging evidence](GOLEM-013/oryx-review.json). Human review passed and the Author approved commit/push on 2026-10-12.
