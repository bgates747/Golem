# Typed drawing inventory

GOLEM-011 / DRW-001 is complete. Rally has five typed graphics intrinsics:
general PLOT, filled quadrilaterals, bitmap drawing, viewport selection and
scrolling. Their operand, graphics-state and resource contracts are inventoried
under docs/tasks/GOLEM-011/drawing-inventory.md. The Author authorized this one
subtask; stop for review before syntax proposals or implementation.

The main findings are the general PLOT interface hidden behind PlotPoint,
legacy byte bitmap selection rather than stock 16-bit resource selection,
coordinate differences between PLOT and raw bitmap placement, viewport Canvas
clipping synchronization via a non-drawing PLOT, and a scroll-amount precondition
that depends on the current viewport size. Optional bitmap affine transforms
also depend on matrix proof and correct cached-inverse invalidation. Assembly
continues to own graphics setup, resource installation, pacing and cleanup.

Rally source commit 4db7e96f70c3d63f4ab53766f3b32063e9357c8b and both pinned
Fab 1.2.4 / current Fab 1.2.5 source paths are fingerprinted in drawing-audit.json.
Four existing host regressions (hosted/PLOT, elements/quads, bitmap, viewport)
rebuilt and passed with warnings as errors and ASan/UBSan in 173.796s,
measured with the local monotonic clock, including compilation. No new tests,
compiler changes, source-example edits, emulator runs, deployment or reference
checkout modifications were made. Previous Rally native image comparisons are
identified separately as historical evidence, not new Golem qualification.

DRW-001 is checked complete and the authoritative TODO records review pending.
SYN-001, WIR-001 and REV-001 remain unstarted. This tranche is uncommitted.
