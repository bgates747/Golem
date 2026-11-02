# FSIM frame-cost Pi driver — preparation v1

Prepare the small finite driver needed to qualify the three reviewed counter
clients on the existing Pi runtime. The clients and runtime have passed source,
build and host checks. This tranche implements and tests orchestration only;
it selects no staging, emulator or hardware execution.

1. Accepted consumer readinessfdaa6f64c5e0e9e358d96dfebe8ada9b6fe2b54c5ce6f7f392076e46a1cd2469
   and manifest08494a8f. Root verifies6source files/9input records/8actual Pi
   runtime pins/6packages/42package files and7unchanged physics objects per arm.
   Root's private6host tests match the final source hashes; do not repeat
   unchanged builds/tests or alter the clients, module, firmware or assets.
2. Implement the finite invocation proposal21b2d7b6 in the existing owner silo
   after freezing source scope and stable subtasks. Reuse the established
   capture-command transport and owned-process cleanup; pin its actual source/
   binary and runtime dependencies. No new emulator/helper build, runtime repin,
   old interactive pause/reset sequence or general orchestration framework.
3. Planned target order is control, A, B, one fresh process each, first failed/
   incomplete arm terminal. Preserve the working Pi profile, load-only autoexec,
   baseline/diagnostic packages and previous outputs. Proposed selected packages
   have new arm-specific SD paths. Implementation/testing here may use temporary
   fake fixtures only; it must not write the actual emulator profile or SD.
4. Use actual boot/MOS/expected build/assets/Ready observations and one neutral
   Enter press/release. No fixed-delay readiness assumption or input movement.
   Retain75sboot+195spre-Ready+120safter-Ready-key+30sowned teardown,420smaximum
   per arm. Reserve cleanup time within the bound. A timeout or observed failure
   never extends the budget, launches a retry or proceeds to the next arm.
5. Require complete64presentations/4warm frames, valid raw window endpoints and
   A/B reconciliation, own-tick physics/state evidence, neutral input, safe
   native/error/FIFO/cleanup/mode/cursor/MOS return and normal frontend exit.
   Keep source-clock/wall brackets and transport uncertainty explicit. Use a
   harmless MOS file copy for keyboard/prompt proof, preserve unique logs and
   existing output, and distinguish guest normal cleanup from forced host stop.
6. Device-free checks must exercise real driver control flow: delayed/partial/
   wrong observations, first-failure-stop, source/clock/report invalidity,
   guarded destinations and owned finite cleanup. Reuse current formats and
   helper functions. Do not write parallel fake acceptance logic or replay the
   already-passing renderer scene matrix. No emulator or audio device in tests.
7. Return a frozen driver/source/test/input/runtime/helper/finite-plan closure,
   device-free results and exact invocation, with fresh Pi ownership/admission
   requirements. Identify host work whose completion is needed before a future
   timing run; stop owned builds before it. Root reviews before selecting any
   actual staging or control/A/B execution. No Oryx access/reservation, hardware,
   firmware change, publication or physical performance claim.
