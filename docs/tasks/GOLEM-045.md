# GOLEM-045 — Stock matrix and vertex-transform interface

Provide the small source-level interface needed by fsim's stock-only wireframe
experiment. Stock VDP performs all matrix and vertex arithmetic. Fsim owns the
consumer, geometry, emulator profiles and target qualification.

The Author directly approved the extension and mailbox replies in source chat
01a16a53-6d76-72a0-94eb-51a7abded231; verified with read_thread on 2026-10-23.
No native output, custom firmware, publication or physical actions are released.

- API-001 [x] Define a bounded separate `--stock-matrix` profile and initialized example.
- CMP-001 [x] Implement commands 33/34/41, allocated ownership, borrowed buffers, named assembly exports and finite entries.
- CHK-001 [x] Check exact command encoding, parser rejection, relocation/ownership and maintained regressions; return handoff to fsim for independent stock qualification.
- DOC-001 [x] Document arithmetic/format/completion limits and send implementation details through the mailbox.

Declarations precede Subs. Controls are explicitly initialized float32 globals.
Matrices use `Matrix(4,4) name = Identity`. Borrowed buffers use
`Borrowed XYZ16 name = Buffer(id,count)`, with packed contiguous signed16 XYZ
records at offset zero and IDs covered by external reservations. Buffers are
host-installed; the compiler never installs or clears borrowed buffers.

Builtins: `matrix_identity(m)`, `matrix_rotate(m,x,y,z)` (decimal degrees),
`matrix_scale(m,x,y,z)`, `matrix_translate(m,x,y,z)`,
`matrix_multiply(dest,left,right)`, `transform_vertices(dest,m,source)`.
Affine arguments are float32 globals or finite numeric literals, not expressions.
Each affine operation pre-multiplies the existing matrix, as stock command 33
specifies; multiplication explicitly means left times right. No inverse operation
or perspective divide. Borrowed source/destination must differ and have the same
record count. The whole source buffer must contain exactly those records in one
block: command 41 replaces the complete destination, including its shape.

This is a stock API wrapper profile, not the checked portable arithmetic runtime.
Host input must remain finite and coordinate results within signed16 range.
Calls submit commands without acknowledgements. Host owns completion fencing,
installation order and no concurrent control updates. The affine feature flag
is enabled at installation and left enabled, as in existing drawing support.
No global feature-flag reset or automatic draw/pacing is emitted.


Host implementation complete: `src/stock_matrix.hpp`, explicit CLI selection,
maintained `tests/test_stock_matrix.cpp`, documentation in
`docs/design/stock-matrix.md`. Existing raw/resident/portable contracts pass;
26 matrix rejection cases, exact wire encodings, external ownership, relocation,
ASan/UBSan and two actual assembler consumers pass. `check_cli.py` also proves
semantic failure preserves an existing output package. Evidence: `check.log`,
`sanitizer.log`, `cli-evidence.json`. No emulator or physical target launched.
Fsim accepted this interface and independently qualified both cube and supplied tank on actual stock VDP2.16/Fab/MOS3.0.2 ARM64; see peer-target-evidence.json.
This checkpoint preceded publication approval; see the release below.


- TGT-001 [x] Fsim independently qualified the unchanged compiler and both full-Golem packages: cube8vertices/12edges and supplied tank30vertices/47edges,1080 replay frames each. Source geometry, projection/PLOT endpoints, controls/camera cancellation/reset and all-owned-buffer cleanup pass. Peer source summaries and hashes are preserved in `peer-target-evidence.json`; authoritative immutable result is fsim target-qualified-four.
- REV-001 [ ] Human visual review remains outstanding and fsim-owned. The Author explicitly authorized publishing all pending compiler work on2026-10-23; no human visual pass is inferred. Selected Lenovo hostname was unavailable at the qualification checkpoint.

The tank preserves21quads and2pentagons without triangulation or added diagonals.
Host wall times are71.92s cube and73.72s tank; replay portions37.27s/37.26s
include launch/UART/file I/O and do not measure physical performance. This
satisfies the independent stock-target predicate only, not human/physical gates.


### Publication release — 2026-10-23

The Author explicitly requests commit/push of all pending work in logical
groups. This releases the qualified compiler changes for publication while
retaining the separate fsim-owned visual review; it does not invent a visual
pass or release physical operations.
