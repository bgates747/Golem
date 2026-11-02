# FSIM frame-cost counters — implementation preparation v1

Implement the reviewed consumer-only measurement candidate and qualify its
state handling offline. It should distinguish costly frame stages before
optimization, without adding firmware services or claiming isolated native
compute time. No emulator or hardware execution is selected by this tranche.

1. Accept the owner's frame-cost-plan-v1.md SHA256
   0f732ea11fb5ba4482e60ee9838eced34013502713d4ebb05ad14718fd2c05ac,
   returned in pingo066-frame-cost-plan-ready-twentytwo. Preserve the qualified
   textured baseline8d77/modulecadf/runtime8d98/assets/physics/controls and all
   completed startup/restoration evidence. Freeze the implementation plan and
   stable owner subtasks before edits, under the existing PINGO-066 silo.
2. Prepare the three bounded arms: counters-off control, counters with original
   order, and counters plus the explicit composition-GP split. Keep module,
   firmware, renderer, native ABI and all seven physics objects unchanged.
   Restrict differences to eZ80 measurement/lifecycle/output and the declared
   extra GP. Do not add a generic profiler, native cycle telemetry or hook.
3. Implement the stated T0..T6 counters and interframe gaps. Service/advance/
   pose costs overlap those intervals and must remain separately labelled.
   Exclude warmup, initial INFO17, reset/trim and cleanup. Count only matching
   successful completions, stop before another submission after presentation64,
   preserve existing error/uncertain-work handling and restore cursor/mode/input.
4. Four completed warm frames precede the64presentation window. Keep60nominal
   source seconds/120host seconds after Ready as independent finite limits.
   No automatic repeat or arm continuation after a failure. Clock discontinuity,
   low24 carry risk, count shortfall and invalid reply produce explicit invalid
   or incomplete measurement, not zero cost or successful timing. Raw values,
   coarse resolution and all clock/calibration caveats remain in output.
5. Accumulate in bounded application memory; no per-frame filesystem/serial/
   INFO/READ16 and no printer mirror during timing. Emit after timing stops.
   Counters-off means disabled measurement, with the same finite harness. Keep
   normal HUD/physics/service work. Differing arm durations can advance physics
   differently; do not demand identical final states across arms or call a
   changed elapsed-time pose a renderer mismatch. Use fixed-pose command/oracle
   checks for semantic equivalence and validate actual state against its own
   tick history later.
6. Add proportionate device-free checks for matching/delayed/wrong replies,
   GP split routing, exclusion of warmup/INFO, exact stop-before-next-submit,
   errors/Escape/shortfall, overlapping sums and clock discontinuities/wrap.
   Demonstrate unchanged control/A command sequences for the same immutable
   poses; B differs only by its declared ordered GP. Reuse existing oracles and
   guards; do not replay the full scene matrix or build a new test framework.
7. Build the isolated AgonDev client candidates on the Pi under the consumer's
   policy. Report eZ80 binary/RAM increments, unchanged module/physics/asset
   hashes, tests, source/input closure and runnable finite invocation plan.
   No Oryx access/staging/build/emulator, Pi emulator, hardware, flash, default,
   publication or firmware rebuild is selected. Return readiness for technical
   review before choosing any execution; respect newer direct Author work.
