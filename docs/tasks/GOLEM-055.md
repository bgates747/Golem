# GOLEM-055 — Reduce native compiler peak RAM use

Make the on-Agon compiler handle the short two-float program that exceeded its
working RAM during GOLEM-054. Find avoidable live copies and buffer growth,
reduce their peak demand, and preserve exactly the same generated packages.
The language, resident formatter and VDP firmware remain unchanged.

## Starting evidence

The preserved [failing source](GOLEM-054/memory-limit-case.golem) declares two
float32 globals, prints one value and their product. The 319,489-byte native
worker b275de31 failed a 32,768-byte allocation while building stock wrappers,
then returned an ordinary compiler error to MOS. A one-float example succeeded.
The [original receipt](GOLEM-054/distribution-initial-memory-limit.json) and
screen remain immutable. The desktop compiler already handles both programs.

```golem
float32 reading = 1.5
float32 scale = 2.0
Sub show
    print "reading=" print str(reading)
    print " scaled=" print str(reading * scale)
EndSub
```

Expected initial output: `reading=1.5 scaled=3`.

## Scope and acceptance

1. Freeze the accepted native worker and desktop package references before
   rebuilding. Reproduce the exact failure in a separate owned stock-Fab
   profile; preserve the accepted GOLEM-054 instance.
2. Measure allocation demand or the large live buffers at the failing stage.
   Identify target/host differences and instrumentation overhead explicitly.
   A saved allocation failure alone is not a complete heap-use measurement.
3. Prefer small lifetime/copy/growth improvements justified by the evidence.
   Keep output order, buffer IDs, metadata, diagnostics and language behavior
   unchanged. Do not add an allocator framework, change the Agon's available
   RAM, reduce numerical precision or substitute a smaller language profile.
4. Qualify actual native compilation of the unchanged two-float source and
   compare every package file byte for byte with the desktop reference. Run
   its output on stock VDP, including updated inputs and repeated calls.
5. Verify maintained contracts, old package parity, sanitizer checks and
   representative native compile/error cases. Report remaining finite-RAM
   limits without claiming arbitrary program sizes now fit.
6. Refresh the native compiler distribution, source, how-to and example together.
   Stage a dedicated stopped Oryx Fab review with readable source and useful
   visible results. Only the Author launches its graphical instance.

The first copy/lifetime fix lets the original source compile through both the
plain dispatcher and direct worker. The optional `golemc @file.args` route still
fails: its adapter keeps an 8,193-byte allocation for a short arguments file.
Size that allocation to the file contents while retaining the existing 8,192-byte
input limit and parser behavior. Preserve the intermediate worker and failed
attempt so the two improvements can be distinguished. This is another measured
working-memory fix within this task, with no allocator or SDK change.

## Work

- BASE-001 [x] Preserve baseline worker, exact failed source and desktop outputs.
  [Freeze receipt](GOLEM-055/baseline-freeze.json) records the accepted worker,
  adapted source and all eleven reference package files before rebuilding.
- MEM-001 [x] Reproduce the target failure and record a proportionate memory
  diagnosis with measured evidence and its limitations.
  [Baseline reproduction](GOLEM-055/baseline-evidence.json) and
  [paired heap observations](GOLEM-055/memory-measurements.json) distinguish
  allocator fragmentation, removed live copies and diagnostic overhead.
- FIX-001 [x] Reduce proven avoidable peak allocations while preserving every
  output byte and current compiler semantics.
  Completed wrapper vectors are moved into retained storage, then serialized
  directly. [Host parity](GOLEM-055/host-parity.json) matches all eleven frozen
  files; the [diagnosis](GOLEM-055/memory-analysis.md) explains the lifetimes.
- ARG-001 [x] Size response-file storage to its actual contents; check ordinary,
  quoted, empty, malformed and oversized inputs and the original two-float case.
  The adapter allocates file length plus one byte. Eight
  [host boundary checks](GOLEM-055/response-evidence.json) pass; the final native
  worker also compiles the original program through `golemc @file.args`.
- CHK-001 [x] Verify host/sanitizer/legacy parity and actual MOS compilation,
  stock execution, input preservation and repeated resident calls.
  [Maintained contracts](GOLEM-055/maintained-checks.log),
  [ASan/UBSan](GOLEM-055/sanitizer-evidence.json),
  [Pi stock qualification](GOLEM-055/stock-evidence.json) and
  [actual MOS response boundaries](GOLEM-055/native-response-evidence.json) pass.
  [Oryx stock qualification](GOLEM-055/stock-oryx-evidence.json) independently
  matches all package files, five text/pixel pairs and sixty input/status bytes.
- CUR-001 [x] Restore the text cursor before every graceful review-client exit.
  The Author's numerical/keyboard screenshot and retrieved results match, but
  exposed a disabled cursor at MOS. A common exit wrapper now enables it after
  normal and error returns. [Cursor evidence](GOLEM-055/cursor-evidence.json)
  verifies a blinking 8-by-8 cursor cell on both paths; the project guidance
  now requires cursor restoration.
- PKG-001 [x] Rebuild and verify the standalone distribution with the improved
  compiler and the formerly failing example.
  [Archive verification and Oryx delivery](GOLEM-055/distribution-evidence.json)
  cover all four rebuilt workers, source, guides and the restored example.
- REV-001 [x] Qualify and stage the stopped Oryx review in the same tranche;
  obtain the Author's visual/keyboard report and saved-result verification.
  [Review instructions](GOLEM-055/REVIEW.md),
  [headless review check](GOLEM-055/review-headless-evidence.json) and
  [fresh stopped staging](GOLEM-055/staging-verification.json) are ready.
  [Human acceptance](GOLEM-055/human-review.json) records the Author's pass,
  saved results and subsequent cursor correction; no new human readback of the
  corrected client is claimed.
- PUB-001 [x] Obtain explicit implementation publication approval after review.

The Author requests recording, committing and pushing this plan before work.
The Author accepted the test and explicitly approved implementation commit/push
on 2026-10-30, completing the review and publication checkpoints. Keep new artifacts under docs/tasks/GOLEM-055/;
TODO.md owns overall status. No physical flashing, shared emulator replacement
or Flexe audio investigation is included in this tranche.
