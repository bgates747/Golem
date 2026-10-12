# GOLEM-014 — Checked resident uint16 multiplication

Implement checked multiplication in the finite resident scalar job. Multiplication
has ordinary precedence over + / -, with left-to-right checking within each
operator level. Overflow preserves the destination and ends the job with resident
status 3. Keep recurring application traffic unchanged. Human testing and explicit
commit approval remain the final gate.

- IMP-001 [x] Add parsing/constant validation, bounded local VDP multiplication
  and an overflow exit. Preserve previous addition/subtraction command bytes.
- TST-001 [x] Qualify boundaries, precedence, chained products, self-reference,
  preservation, skipped statements, recovery, relocation and lifecycle with
  maintained checks, an independent command model and sanitizers.
- VDP-001 [x] Qualify stock Fab/MOS and prepare a stopped dedicated Oryx fixture.
  The guest prints keyboard instructions. The Author launches and closes it;
  the agent retrieves/verifies saved results afterward.
- REV-001 [x] Report results and stop for human review; leave this tranche
  uncommitted. Signed/general multiplication, division and raw-mode changes defer.

Keep all supporting fixtures, scripts, builds and evidence under GOLEM-014/.

Delivered [review instructions](GOLEM-014/README.md),
[host checks](GOLEM-014/host-checks.json), [expression checks](GOLEM-014/expression-checks.json),
[stock-emulator evidence](GOLEM-014/emulator-checks.json),
[compatibility evidence](GOLEM-014/arithmetic-compatibility.json) and
[Oryx staging evidence](GOLEM-014/oryx-review.json). Human review passed; [saved-result evidence](GOLEM-014/human-review.json) matches
all checks. The Author explicitly approved commit and push.
