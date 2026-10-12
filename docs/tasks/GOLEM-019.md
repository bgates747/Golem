# GOLEM-019 — Resident bitmaps using AgonWolf3D assets

Use recognizable Wolf images at their native sizes for the next drawing review:
a 320×160 splash and 64×64, 32×32 and 40×20 sprites/icons. Asset selection and
plain `draw_bitmap(bitmapBuffer, x, y)` implementation are complete. Stock
bitmap qualification passes 18,526 checks and 18,869 saved bytes. A separate
labelled human demo calls review.golem once; its 864 pixel checks and Oryx staging
pass. This replaces the confusing combined diagnostic/visual presentation.
The Author accepted the corrected static pictures, and retrieved results match
all 864 bytes. The separate keyboard check is superseded by the successful GOLEM-020 review.
Assembly loads the AGNB assets and owns display setup, input updates and calls.

- AST-001 [x] Select existing large/small Wolf assets, preserve their payloads and
  embedded buffer IDs, and supply the expected layout before emulator review.
- BMP-001 [x] Implement plain resident bitmap drawing and publish the assembly
  interface, including borrowed asset IDs and their ownership contract.
- CHK-001 [x] Qualify stock bitmap placement/transparency, persistent updates,
  asset preservation through cleanup and existing compiler profiles.
- ORY-001 [x] Stage the dedicated Oryx fixture, leave it stopped and provide the
  usual single-terminal launch commands and onscreen keyboard instruction.
- HUM-001 [x] Static visuals accepted and saved results verified. The separate
  keyboard check is superseded by the successful GOLEM-020 animation/MOS review.

The Author requested real AgonWolf3D bitmaps rather than invented test artwork.
Use the [asset reference](GOLEM-019/README.md) for the initial display. All
supporting files live in this task's silo. Transforms, scaling and viewport/scroll
operations remain later work. Implementation, qualification and the staged visual
test form one reviewable tranche; pause when the visual test is ready. Emulator
changes remain uncommitted until human validation and explicit commit approval.
