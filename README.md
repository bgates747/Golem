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
  byte sequences, preferring multiple commands per block and one or more
  blocks per buffer. Exact branch-address layout remains a design decision.
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

`make -C src` builds the host compiler for literal print, uint16 declarations,
assignment/addition and For/Next. Statements have no terminators; newlines are
whitespace. [GOLEM-005](docs/tasks/GOLEM-005/README.md) documents parser migration
and the separate experimental scalar-floor frontend.
[GOLEM-006](docs/tasks/GOLEM-006/README.md) adds checked allocation and a task-local
resident CLI that combines scalar math with persistent uint16 state; host checks
pass. [GOLEM-007](docs/tasks/GOLEM-007/README.md) adds passing stock-emulator
qualification through MOS/UART and successful human review on Oryx; production
promotion remains pending.

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
src/          Maintained toy C++ compiler and host Makefile
tools/        Auxiliary tooling/scripts (not the compiler itself)
examples/     Example Golem programs
tests/        Test suite
```
