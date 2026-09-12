# Golem

Golem is a low-to-medium-level compiled programming language targeting the
Agon Console8 VDP's **Buffered Commands API** — a byte-addressable,
register-less, self-modifying "buffer machine" exposed by `VDU 23, 0, &A0`
commands.

The Buffered Commands API lets command sequences be stored in VDP-resident
buffers and executed there directly, without round-tripping through MOS on
the eZ80. It provides memory (buffers/blocks), byte-wise arithmetic and
logic, conditional and unconditional branching, and call/return — enough
to be Turing-complete, but with no general-purpose integer registers or indexed/pointer addressing mode (array and
pointer access must be compiled to self-modifying "patch, then execute"
code).

Goals for Golem:

- Compile a small, C/Pascal-like language directly to Buffered Commands API
  byte sequences (one instruction per buffer block, so jump/call targets
  are simply block indices).
- Provide a runtime library for operations the VDP has no primitive for
  (multiply, divide, shifts, wide comparisons).
- Prefer loops (conditional jump, zero stack cost) over recursion; support
  tail calls natively since the VDP already optimizes them into jumps.
- Package compiled programs as a single compressed blob that is
  decompressed and spread across target buffers on load, rather than
  streamed as many small commands.

## Status

The isolated `rally19-vdp` branch extends the existing C++ compiler with an opt-in
hosted mode. Stock float/matrix operations now provide arithmetic, affine
reciprocal/rotation and checked signed output; resident imports, guarded indexing
and finite state admission support the Rally offload goal. Full-scene integration
and the frozen native-emulator performance criteria passed on the earlier
programs. Physical stock VDP then exposed its4096-byte processLoop stack limit.
The opt-in `InlineCalls(...)` correction passes compiler, full frozen scene,
both-track long replay and control regression. The Author confirmed it runs after
a board reset on restored official VDP2.16.0, but it remains visibly slow. Sparse
diagnostic observations were about1.19 resident-call returns/second; these are
not a scanout FPS measurement. The current lowering has not provided a useful
hardware acceleration. Final native timing passes the unchanged frozen budgets;
the sibling Rally task's DELIVERY.md and REVIEW.md index the bounded result,
artifacts, commands and hardware limitations. Further optimization is a new task.

Build with `make -C src`; run `.venv/bin/python tests/run_hosted.py` for host
sanitizer/regression tests. `build/golemc --hosted input.golem output.bin` emits
the bootstrap plus `.map` and `.clear`. Ordinary `golemc input output` retains
the legacy language. See [hosted design](docs/design/rally19-hosted-v1.md),
[resident data/admission](docs/design/rally19-resident-v1.md), and the native
examples for headless test commands. Goal progress lives only in the sibling
AgonArcade execution worktree's root TODO.md. Physical performance is not accepted.

See [docs/devlog](docs/devlog/) for chronological evidence.

## Reference material

Design is grounded in the Agon documentation, particularly:

- VDP Buffered Commands API
- VDP Variables

(See the sibling `agon-docs` repository for the full reference docs.)

## Layout

```
docs/
  devlog/     Dated development log entries
  design/     Design notes and specifications
reference/    Vendored/curated reference material (not a build dependency)
runtime/
  legacy-asm/ Hand-written ZDS/ez80asm-style eZ80 runtime library code
  agondev/    (future) eZ80/MOS-side loader/runtime built via agondev
src/          C++ compiler, ordinary and hosted frontends
tools/        Auxiliary tooling/scripts (not the compiler itself)
examples/     Example Golem programs
tests/        Test suite
```
