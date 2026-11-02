# FSIM module cost observation — source planning v1

The finite Pi cohort passes independent checks. Its original-order arm spends
most observed source-clock time between composition/render submission and the
native completion reply. A separate composition fence changes that breakdown
and the overall duration. The next useful step is to plan a small diagnostic
inside the consumer module, so optimization can follow measured work rather
than an assumption about VDPlet overhead.

1. Preserve accepted client/driver/result freezes fdaa6f64/bd5f2995/1fcd95ed.
   Root independently checks61 frozen outputs,21 actual copied package files,
   every arm's raw report/wire/state/MOS proof and native stopped inventory.
   All arms complete4warm+64measured; minimum command stack1364B. A's combined
   composition/render bucket is5432/6190source units (87.75%). B separately
   observes composition1024/3852 (26.58%) and render2316/3852 (60.12%).
   These are within-arm fractions, not physical CPU percentages or a speedup.
2. Prepare a source-only plan for the smallest separate consumer-module
   diagnostic described in frame-cost-plan-v1.md's native follow-on. Inspect
   actual qualified P066 entry/helpers and existing P065 CCOUNT/PRID precedent.
   Check counter semantics in the retained ARM64 backend and physical ESP32
   distinction. No generic profiler, firmware service, ABI allocation or build.
3. Propose a few aggregate boundaries that distinguish validation/setup,
   command preparation, native rendering and row binding/release where these
   are actually separable. Account for the two entries per frame and repeated
   scratch clearing. Do not insert per-pixel/triangle logging or assume an
   unmeasured operation is expensive. No check removal or optimization yet.
4. Define explicit caller-owned telemetry storage/lifetime and after-stop
   readback, without guessed scratch offsets or new concurrent ownership.
   Bound per-call counter differences, core checks, wraps, aggregate overflow,
   instrumentation overhead and invalid/unavailable results. CCOUNT includes
   preemption and is not an isolated CPU budget; host mock zeros are not data.
5. Propose the smallest useful control, command/pixel/physics parity and
   diagnostic clock/overflow checks, plus one finite Pi-first qualification
   with declared ownership, admission, duration and cleanup. Explain what the
   existing changing-pose live cohort can establish and whether a fixed-pose
   diagnostic is necessary for any comparison. Avoid a full scene matrix or
   target run at this planning stage. Existing published sources stay intact.
6. Freeze the owner plan and stable task/TODO/dependency entries, then return
   exact interfaces, source evidence, resource/stack estimates and finite
   validation proposal for root review. Source consultation with Flexe is
   allowed through durable mail and immediate steering chat if counter
   semantics need clarification. No source implementation, compilation,
   staging, emulator, Oryx, hardware, default adoption or publication selected.
