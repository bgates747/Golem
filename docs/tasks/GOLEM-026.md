# GOLEM-026 — Portable uint16 conditions and Golem-controlled reversal

Extend the shared-source experiment with six uint16 comparisons and structured
If/ElseIf/Else/EndIf on both backends. One Golem tick Sub will choose the moving
meter's direction; assembly selects the backend, draws and paces. The Author
authorized this tranche after approving the GOLEM-025 checkpoint on 2026-10-13.

Keep implementation, fixtures and evidence under `docs/tasks/GOLEM-026/`. Fork
the accepted task-local frontend/emitter there with provenance; historical tasks
and production compiler remain unchanged. No runtime/SDK changes or flashing.

## Contract

Support == != < > <= >= between checked uint16 expressions, evaluated once left
to right. Evaluate only conditions reached and the selected branch. Limit nested
If statements to eight levels and ElseIf chains to 64 alternatives in this slice.
Condition arithmetic failure exits
the whole invocation, retaining earlier effects and existing error status rules.
No bool storage, Boolean composition, source loops or new drawing grammar.

Buffered comparisons must work for 65535, avoiding the firmware's whole-word
sentinel. Use finite branch/join jumps so failure cannot resume later statements.
Native code shares the model, state and entry contract.

## Subtasks

- SYN-001 [x] Add typed conditions and branch nodes with useful rejections;
  preserve terminator-free grammar and case rules.
- EMIT-001 [x] Emit byte-safe buffered comparisons and finite branch/join jumps;
  extend generated native C with identical evaluation rules.
- CHK-001 [x] Check all operators, byte boundaries, 65535, nested joins, ElseIf,
  skipped/selected failures and earlier effects. Run sanitizer, ELF and maintained checks.
- CHK-002 [x] Qualify stock buffered and actual Xtensa JIT/interpreter execution,
  relocated IDs and alternating state, retaining results.
- VIS-001 [x] Stage a finite single-tick animation on Oryx, with source and
  pixel/state checks. Leave it stopped for review.
- REV-001 [x] Supply launch/source, retrieve human results after the Author
  reports, and await explicit commit approval for this tranche.

## Evidence

See [report and human review command](GOLEM-026/README.md),
[host receipt](GOLEM-026/host-checks.json) and [target evidence](GOLEM-026/evidence/).
The task-local compiler leaves production, historical tasks and the generic
SDK/runtime unchanged. The Author accepted the visual review ("looks good");
retrieved results match all 533 observations/5,249 bytes. Keyboard success was
not separately stated. The Author explicitly approved commit/push on 2026-10-13;
REV-001 is complete.
