# Loaded display-admission source review

Review the smallest possible owned firmware correction if the separately
instrumented target confirms the suspected failed palette/signal-list path.
The purpose is a clean refusal that preserves a usable prior display, not a
promise that every module and display mode coexist. This is source research;
no implementation or new emulator run is released by this document.

- SRC-001 [x] Pin the actual b42/a10 firmware and graphics sources. Read the
  mode transition, replacement controller construction, palette/signal DMA
  allocation, failure cleanup and old-controller retirement paths.
- ERR-001 [x] Identify returned/ignored errors and when old usable state is
  lost. Relate the frozen mode/address evidence without calling a static PC
  match or heap total a captured origin or physical allocation failure.
- FIX-001 [x] Recommend one bounded failure/rollback design with concrete
  source points, cleanup/lifetime invariants and likely resource consequences.
  Preserve normal successful behavior, no mapping/alias changes or module
  policy increases. Compare guard-only refusal with complete staged admission.
- TST-001 [x] Propose proportional fault-injection host and exact target
  admission/retained-mode/keyboard/lifecycle tests. A new frozen implementation
  and explicit central grant would precede execution.

Use the existing SDK source export under build/vdpl-010 or pinned upstream
references read-only. Do not edit upstream, owned firmware source, current
exports or any profile. Findings belong in this task silo. The independent
palette21 consumer proof and CPU cache gates continue without waiting for this
review. Publication/hardware decisions remain separate.

Source review completed in mode-admission-source-review.md; source/artifact
pins are in mode-admission-source-pins.json. Recommendations and tests are
conditional proposals, not implementation/launch authorization or causal proof.
