# GOLEM-022 focused stock qualification

The fixture will demonstrate that assembly can select two finite Golem Subs
from one installation and reuse one persistent input block. Stock Fab/MOS on
Oryx will verify a small independent pixel oracle, input bytes, ownership cleanup
and borrowed Wolf assets. A separate human scene will show the actual two Subs
without the automated direct-VDU comparison scenes. No GUI will be launched.

- FIX-001 [x] Prepare initialized `review.golem`: first `drawMarker` uses
  shared colour/x/y globals; later `drawWolf` uses the same x/y globals and a
  borrowed Wolf bitmap. Keep the later-only bitmap reservation/affine case.
- FIX-002 [x] Build default and relocated packages. Assemble real consumers
  calling both named wrappers. Check ordered manifest entries and first-entry
  default without a new binary format or production readback logic.
- CHK-001 [x] Build a small automated tape: install once, call marker, partially
  update x/y, call Wolf, then marker again. Compare selected pixels with both
  an independent model and separately emitted direct stock VDU scenes.
- CHK-002 [x] Read back persistent inputs, check all owned buffers absent after
  cleanup, and verify borrowed bitmap bytes and an unrelated canary survive.
- REV-001 [x] Build a separate 512x384 Wolf review scene showing both named Subs
  from the readable source, with assembly's asset/input/call role labeled.
  Place the MOS `dir` instruction on screen and keep diagnostics out of it.
- DEP-001 [x] Stage a dedicated canonical stock profile at this silo's
  `.emulator` on Oryx, transfer/hash the prepared fixture and canonical helper,
  run automated and human-scene headless checks, retrieve receipts, and leave
  the human fixture selected and the emulator stopped. Preserve GOLEM-021.

The MOS tape transport and canonical AGNB loader are adapted from accepted
GOLEM-018/019 fixtures. Wolf pixel data and provenance are copied from the
accepted GOLEM-019 assets. The new fixture will not rerun their historical
pixel matrices. Human visual/keyboard approval remains pending after staging.

Fresh Oryx runs passed: default 370 checks/415 bytes; relocated 370/415; human
scene 62/99. Receipts and observed bytes are under `evidence/`. The human profile
is selected and stopped, and its machine-run result was removed for a fresh
Author launch. Building/staging the review scene is complete; human acceptance
is still pending in the main GOLEM-022 task register.
