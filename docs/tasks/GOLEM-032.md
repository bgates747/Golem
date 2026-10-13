# GOLEM-032 — Portable float32 arithmetic and integer conversions

Implement bounded float32 + / - / * and explicit conversions to/from uint16 and
sint16 on both task-local backends, with float comparisons for motion control.
Use stock 1x1 matrices and generated native C. Keep source, layout, error and
failed-destination semantics common. Production compiler and generic SDK/runtime
remain unchanged. Division, sqrt, pow and floor are outside this tranche.

Initial numeric contract: binary32, round-to-nearest ties-even with separate
operator rounding and no contraction. Admit finite normal inputs and signed zero;
reject NaN/infinity/subnormal host inputs with status 33. Canonicalize expression
zero to positive zero. Flush subnormal arithmetic results to positive zero;
qualify underflow boundaries explicitly before claiming this policy portable.
Non-finite results report status 33. Bounds and conversion failure use status 32.
Float-to-integer conversion truncates toward zero, checks numeric result range
before writing, and never relies on an out-of-range C/firmware cast. Integers
convert exactly to float32. Mixed arithmetic requires an explicit conversion.

- NUM-001 [x] Inspect stock matrix/conversion paths and qualify numeric edge behavior.
- SYN-001 [x] Extend typed conversions and comparisons to float32; preserve prior grammar.
- EMIT-001 [x] Emit scalar matrix/native arithmetic, numeric admission and safe conversions.
- CHK-001 [x] Verify bits, rounding, underflow/non-finite handling, Bounds, failures and prior regressions.
- CHK-002 [x] Qualify stock and actual Xtensa engines with relocated buffers and state retention.
- VIS-001 [x] Stage complete fractional-motion source and stopped Oryx review.
- REV-001 [x] Obtain human review and explicit commit/push approval.

## Evidence

[Report/source/review command](GOLEM-032/README.md),
[host receipt](GOLEM-032/host-checks.json), [target receipts](GOLEM-032/evidence/).
Stock and both actual Xtensa engines pass. Human review is staged/stopped on
Oryx. The Author accepted review with “tests are good”; saved results match all
637 observations/6,289 bytes and runtime identities pass. The Author explicitly approved commit/push on 2026-10-13.
