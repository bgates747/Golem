# Native counter review before the consumer measurement

The pinned emulator implements CCOUNT/PRID reads in both instruction engines.
These support a bounded comparison of modelled native work, including
preemption. They do not establish physical instruction latency, clock frequency
or presentation throughput. Actual counter/core and outer-bound observations
remain prerequisites for accepting a sample.

1. Sources were read from the sealed maintained-r02 source bundle, not assumed
   identical to the current vendor checkout. SHA256: xtensa.c
   `fea1df8d0ec122f1cd6eeb3bc270cffbcb47b0aad43c8f0bad012552183f18f5`,
   jit.c `a7f3c9b5da4515724774121b5f05098c47d6ccf4de69813f53e27a8aa7e6ca42`,
   freertos_stubs.c
   `0294aebd7cba899e526ae18e81549affdca9aa6d89f2b7a446e8ce4d7174d2f9`.
   The bundle is the already pinned 5613a710 source/build closure. The first
   two active source files differ from the unpatched vendor checkout.
2. Interpreter register reads return CCOUNT/PRID; cached interpreter paths add
   the current uncharged instruction prefix to CCOUNT. JIT RSR adds accumulated
   and within-block work and removes already charged work, with explicit ARM64
   and x86 implementations. PRID is read from the actual CPU field. Counter
   advancement includes simulated idle/preemption; no per-instruction physical
   latency claim follows.
3. The actual rows-r1 video setup pins processLoop to core0. The runtime calls
   the module synchronously there. Primary PRID is 0xCDCD. In addition to equal
   endpoint IDs, the consumer should check that expected identity when bounding
   against the adapter's primary-cycle export; equal secondary IDs alone would
   not justify a primary-cycle bound.
4. flexe_vdp_cycles is an atomic snapshot of primary cycle_count published at
   normal batch boundaries, rather than a synchronized instruction checkpoint.
   BATCH is10000 and run_core can overshoot at a completed block/stub. Therefore
   merely accepting any outer delta below2^32 leaves a near-wrap ambiguity.
   The consumer can conservatively restrict its finite samples far below that
   boundary (for example outer delta below2^31), retain actual deltas and reject
   rather than silently accept an unsupported counter/wrap condition. Exact
   metadata and observed model behavior remain part of the target review.
5. The metadata endpoint is output finalization after scratch/input cleanup.
   It excludes final output release, return/reply and host C3. Both timed arms
   exclude hashes/extraction. Native phases include preemption and nested
   boundaries are not blindly additive. End-to-end presentation is separate.

This review changes no emulator, native module or timing service. The consumer
harness had not been finally frozen or executed when these observations were
sent to its owner. Any check refinement is recorded before its target freeze;
prior generic qualification and source pins stay unchanged.
