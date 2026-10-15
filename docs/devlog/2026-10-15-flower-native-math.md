# Flower native mathematics passes headless execution

Readable Golem now calculates Flower with ordinary float division, sine, cosine
and floor on the ESP32. Actual JIT/interpreter checks pass; resident rendering
and the interactive C++ client are still ahead. The Author deferred visual
staging until after sleep, so this tranche has no desktop review or deployment.

The prior feasibility work was committed/pushed as `f17f805` before implementation.
MTH-001..004 are complete. An explicit --target native capability keeps the same
frontend/state/root model while rejecting unqualified stock mathematics. The
default compiler's 55 compared output files remain byte-identical. SDK VDPL-002
adds opt-in static libm/libgcc linking without changing its three default images,
firmware/runtime/loader/ABI or imports policy.

Host/sanitizer/assembler checks pass, including 1244 generated-C cases, six SDK
loader packages and 468 prior XOR/array cases. Actual Pi JIT and matched Oryx
JIT/interpreter runs each verify 278 calls / 27092 replies / 270920 reply bytes.
Six Flower cases cover defaults, fractional/full-circle angles, integer count
and zero/full shrink; defaults produce 396 segments. The observed maximum float
difference from the independent model is one ULP. All Oryx reply bytes match
between engines apart from identified command-task stack diagnostics.

Minimum observed free command-task stack is 1844 bytes on Pi JIT, 2120 on Oryx
JIT and 2008 on Oryx interpreter. The full Flower module is 13668 bytes. Runs
execute actual firmware and uploaded code through a VDP-only ABI; they do not
boot MOS or establish physical hardware behavior. All backend fault counters
remain zero. An initial Oryx receipt-directory failure was fixed and the matched
runs repeated; complete evidence is retained.

The Author accepted the native mathematics and SDK library tranche on
2026-10-15 and explicitly authorized commit/push of both repositories. MTH-005 visual staging is deliberately
deferred by the Author's later instruction, superseding the original staging
plan for this tranche. See the [report](../tasks/GOLEM-044/native-math/README.md)
and [working mathematics source](../tasks/GOLEM-044/flower.golem).
