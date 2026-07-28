# Golem

Golem is a low-to-medium-level compiled programming language targeting the
Agon Console8 VDP's **Buffered Commands API** — a byte-addressable,
register-less, self-modifying "buffer machine" exposed by `VDU 23, 0, &A0`
commands.

The Buffered Commands API lets command sequences be stored in VDP-resident
buffers and executed there directly, without round-tripping through MOS on
the eZ80. It provides memory (buffers/blocks), byte-wise arithmetic and
logic, conditional and unconditional branching, and call/return — enough
to be Turing-complete, but with no general-purpose registers, no hardware
multiply/divide/shift, and no indexed/pointer addressing mode (array and
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

Early design/exploration stage. See [docs/devlog](docs/devlog/) for
progress notes and [docs/design](docs/design/) for design documents.

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
src/          Compiler/toolchain source (not yet started)
examples/     Example Golem programs
tests/        Test suite
```
