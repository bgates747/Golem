# GOLEM-020 — Vblank-paced Wolf bitmap animation

Provide a readable, finite animation using the existing Golem drawing
backend. Assembly loads the Wolf assets once, advances bounded bouncing positions
using stock VSYNC swaps, updates the packed Golem inputs and calls drawFrame. Golem
clears and draws every picture. No new language syntax or compiler extension is
needed. Stage and qualify the visual demo on Oryx before requesting review.

- ANI-001 [x] Build a complete Golem frame routine and explicitly labelled
  assembly motion/pacing driver. Use double buffering and finite execution.
- CHK-001 [x] Check sampled moving frames and position state against an independent
  model, including wall reflections and final return to a single-buffered MOS.
- ORY-001 [x] Stage and headlessly verify the exact Oryx visual demo; leave stopped.
- HUM-001 [x] Review animation and MOS keyboard response; retrieve saved results.

All artifacts stay in GOLEM-020/. Existing GOLEM-019 evidence is preserved.
Record the Author's acceptance of the corrected static images, without inventing
an explicit MOS keyboard confirmation or commit approval.

Local and Oryx headless runs pass all 841 checks / 946 saved
bytes. Execution including setup and observations: local 22.481s,
Oryx 21.927s. Assembly width audit: 23 wide accesses, no hazards.
The Author reported success. Retrieved human results match all 841 checks /
946 bytes, with the staged runtime and fixture identities unchanged. Animation
and MOS keyboard review pass; the Author explicitly approved commit and push on 2026-10-12.
