# GOLEM-007 — Stock VDP qualification of resident scalar jobs

Run GOLEM-006's integrated payload through the actual stock emulator VDP and
MOS transport. Observe results independently of the host execution model,
including signed boundaries and tiny negatives, persistent uint16 state,
relocation, finite returns and exact cleanup. The Author authorized this tranche
after requesting the preceding checkpoint be committed and pushed.

## Fixture and transport

- FIX-001 [x] Keep test tools, guest assembly, fixture metadata and evidence in
  this task silo. Generate the ignored project `.emulator/` profile through the
  canonical setup tool; preserve upstream runtime/firmware and other profiles.
- FIX-002 [x] Use a generic MOS guest to stream installation/update/call/cleanup
  packets and read bytes through stock tagged GP echo, with bounded waits and
  explicit completion. Count diagnostic traffic separately from application data.

## Qualification

- VDP-001 [x] Execute pure and mixed scalar jobs with default and externally
  reserved layouts. Check signed limits, fractional values, both signed zeros,
  tiny negatives and subnormals using binary32 inputs and an independent oracle.
- VDP-002 [x] Check repeated calls, counter persistence/wrap, reinstall reset,
  unchanged program bytes, exact-owned cleanup and external canary survival.
- VDP-003 [x] Check diagnostic readback for all 256 byte values and distinguish
  missing buffers from valid data using controlled pre/post-cleanup probes.
- EVD-001 [x] Record runtime/source/payload hashes, observed bytes, assertions,
  commands and reusable durations. Record failures without weakening criteria.

## Review boundary

- REV-001 [x] Prepare the dedicated local Oryx instance and complete explicit
  human emulator validation. The Author launched it manually, verified the saved
  output (855 checks, 9204 bytes), and confirmed `dir` responds at the MOS prompt.
- REV-002 [x] Keep emulator-related changes uncommitted until the Author
  explicitly approves committing them, as required by the canonical environment
  AGENTS.md. Human validation passed and the Author explicitly approved committing the tranche.

No general language expansion or scalar promotion is part of this tranche.
GP establishes command-parser completion and observable buffer state, not
raster completion, hardware timing or full allocator balance.
