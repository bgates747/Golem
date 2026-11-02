# Direct framebuffer goal audit

The desired optimization removes the separate colour-frame workspace and its
publication copy by rendering into the hidden drawing framebuffer. A copied
pixel service and a working bounded direct-row prototype do not establish that
a production application uses this path or runs faster. Wolf has identified
that distinction in its current production renderer. Audit it before selecting
any repair or new performance run.

## Frozen first-tranche scope — 2026-11-01

1. Inspect the generic copied-pixel and scoped-row source, current API wording,
   target evidence and normal/error cleanup. Trace drawing-page selection,
   pending-work synchronization, native layout, token lifetime and presentation.
2. Inspect Golem's owned task/results/guide claims and the consumer code actually
   reviewed: Wolf bounded versus production and FSIM legacy versus live renderer.
   Keep independent 6502/Rally display work in its owner's scope. Report concrete
   consumer problems to the owner; do not take over their audit, fixes or tests.
3. Separate three paths: workspace composition plus pixels_write; bitmap
   storage plus plotting; native final-pixel stores through a framebuffer lease.
   Distinguish necessary guest display RAM, depth, texture and render metadata
   from an avoidable colour output image. Direct rendering can still overdraw
   pixels; zero publication copy does not promise exactly one store per pixel.
4. Check old performance measurements for actual boundaries, fixture identity,
   engine, presentation, correctness instrumentation and clock limitations.
   Preserve measured results, narrow overstated claims, and identify any missing
   complete-frame matched comparison. Do not invent missing measurements.
5. Report concrete affected files and proposed corrections through durable mail
   with immediate chat notification. Update Golem's register and evidence. New
   renderer implementation and benchmarks belong in consumer tasks under their
   Author instructions. Root does not become a per-test approval gate. Any
   demonstrated missing generic hook gets its own shared-runtime plan.

This audit reserves no machine or emulator slot and authorizes no firmware
rebuild, physical operation, source promotion, default change, commit or push.
The authoritative outstanding item is GOLEM-057 DFR-001; this document defines
its scope, rather than creating another actionable checklist.
