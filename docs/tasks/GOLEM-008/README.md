# GOLEM-008 promotion result

The maintained compiler now provides explicit resident mode, with the same
qualified scalar expansion and persistent-state behavior as GOLEM-006.
`ordinary.hpp` exposes shared ordinary emission; `resident.hpp` combines it with
`scalar_backend.hpp`. Production includes no CLI implementation and has no task
code dependency. Historical compatibility policy stays in its original silo.

Run from the repository root:

```bash
make -C src check
.venv/bin/python docs/tasks/GOLEM-008/run_checks.py
```

The recorded host run took 46.443 seconds on the Pi (build plus checks, not a
runtime benchmark). [host-checks.json](host-checks.json) records commands,
durations and production hashes. Verification includes:

- Four GOLEM-007 packages: all bootstrap, cleanup, invoke and update/call hashes
  match the accepted review. Manifest changes are limited to profile provenance
  and removal of `experimental_task`.
- Twenty-four additional resident input/layout comparisons against frozen
  checkpoint `ec91999`, and five raw payload comparisons.
- Twenty-six maintained rejection cases with AddressSanitizer/UndefinedBehaviorSanitizer.
- Fourteen CLI failures preserve an existing output and create no package files.
- Strict C++17 builds, including a standalone copy containing only maintained
  source files. Owned IDs, reserved ranges, repeatability and boundary inputs
  are checked by the maintained contract test.

The existing stock-emulator qualification and successful Oryx human review apply
to identical command bytes. No new emulator was launched, and no firmware,
profile or deployment was changed. This is a promotion, not expanded float or
control-flow support. Physical hardware remains unqualified. Later host input
updates require host validation and serialized execution; see the
[maintained contract](../../design/resident-scalar.md).

GOLEM-003's production promotion follow-up is satisfied. GOLEM-002 remains
queued. GOLEM-007 was committed/pushed as `ec91999`; shared review protocol docs
were committed/pushed in agon-dev-env as `9e89035`.

The historical GOLEM-006 suite also passes against the promoted ordinary API:
20 ordinary and 22 scalar parser comparisons, 69 parser rejections, 22 full
compatibility comparisons, 34 allocation/integration rejections and the
independent host model. See [historical-regression.json](historical-regression.json).
The original GOLEM-006 report was preserved; this rerun's evidence stays here.
Two harness issues caught during verification (renamed-main return and CLI helper
name collisions) were corrected before the successful final runs.
