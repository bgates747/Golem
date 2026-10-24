# Stock matrix source interface for fsim

Golem can now compile the stock matrix/vertex stage required by fsim's isolated
wireframe experiment. All matrix/vertex arithmetic remains in stock VDP; host
sends controls and submits named calls. Fsim owns geometry, drawing integration,
emulator qualification and human review.

The Author directly approved this bounded extension and mailbox replies in
fsim's source chat, verified through read_thread. GOLEM-045 records scope;
`docs/design/stock-matrix.md` is the maintained interface. A separate explicit
`--stock-matrix` CLI avoids expanding portable checked-arithmetic promises or
changing existing resident drawing packages. Named initialized4x4 matrices,
float32 controls, borrowed contiguous signed16 XYZ buffers and finite Subs emit
only stock commands33/34/41 plus buffer patch/call operations. There is no raw
VDU escape, inverse, native backend or custom firmware requirement.

Host contracts, ASan/UBSan and two actual assembler consumers pass. The new
profile has26 rejection cases covering types, ownership, shape, lexical and
symbol failures; existing132 raw/resident and57 portable rejections remain
passing. Semantic failures leave an existing package unchanged. Exact argument
patch offsets, operand order, bounded transform count and borrowed preservation
are checked. No target performance or physical acceptance is claimed.

Fsim received the early syntax and accepted it in immutable mailbox message
2026-10-23_16-20-42_fsim_fsim-stock-matrix-interface-accepted-three. The working
compiler and complete handoff were delivered in Golem implementation-two;
independent actual-stock integration remains with that project. No emulator
was launched and nothing was committed or pushed.


## Independent stock-target qualification

Fsim's immutable target-qualified-four result reports PASS for both full-Golem
cube and supplied tank on unchanged stock VDP2.16/Fab/MOS3.0.2 ARM64. Both
1080-frame runs preserve source XYZ, produce independently checked projection
and actual PLOT endpoints, support controls/camera cancellation/reset and clear
all owned buffers on interactive/replay/missing-setup exits. The original
profiles are restored. The supplied30vertex/47edge tank retains21quads and
2pentagons without triangulation. Compiler/source hashes match our host receipt.
The two peer summaries and provenance hashes are preserved in
GOLEM-045/peer-target-evidence.json. TGT-001 is satisfied; human visual review
and publication remain pending because the selected Lenovo hostname is
unavailable. No Golem-side emulator action, commit, push or physical action.


## Publication release

The Author explicitly authorized commit/push of all pending work in logical
groups on2026-10-23. The bounded stock matrix compiler and its independent
functional qualification are published separately from AGNB integration.
Fsim retains ownership of its human visual review; no new visual or physical
acceptance is claimed.
