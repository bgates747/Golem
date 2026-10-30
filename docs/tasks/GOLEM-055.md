# GOLEM-055 — Reduce native compiler peak RAM use

Make the on-Agon compiler handle the short two-float program that exceeded its
working RAM during GOLEM-054. Find avoidable live copies and buffer growth,
reduce their peak demand, and preserve exactly the same generated packages.
The language, resident formatter and VDP firmware remain unchanged.

## Starting evidence

The preserved [failing source](GOLEM-054/memory-limit-case.golem) declares two
float32 globals, prints one value and their product. The319,489-byte native
worker b275de31 failed a32,768-byte allocation while building stock wrappers,
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

## Work

- BASE-001 [ ] Preserve baseline worker, exact failed source and desktop outputs.
- MEM-001 [ ] Reproduce the target failure and record a proportionate memory
  diagnosis with measured evidence and its limitations.
- FIX-001 [ ] Reduce proven avoidable peak allocations while preserving every
  output byte and current compiler semantics.
- CHK-001 [ ] Verify host/sanitizer/legacy parity and actual MOS compilation,
  stock execution, input preservation and repeated resident calls.
- PKG-001 [ ] Rebuild and verify the standalone distribution with the improved
  compiler and the formerly failing example.
- REV-001 [ ] Qualify and stage the stopped Oryx review in the same tranche;
  obtain the Author's visual/keyboard report and saved-result verification.
- PUB-001 [ ] Obtain explicit implementation publication approval after review.

The Author requests recording, committing and pushing this plan before work.
Implementation remains uncommitted until the resulting emulator review and
explicit publication approval. Keep new artifacts under docs/tasks/GOLEM-055/;
TODO.md owns overall status. No physical flashing, shared emulator replacement
or Flexe audio investigation is included in this tranche.
