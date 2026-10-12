# Golem

**Golem Obtains Less Excessive Messaging**

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

`make -C src check` builds and checks the maintained host compiler. Raw mode
supports literal print, uint16 declarations, assignment/addition and For/Next.
Explicit `--resident` mode adds bounded float32 floor-to-sint16 jobs with
persistent uint16 state and checked addition. Overflow preserves the assignment's
destination and ends the job, allowing normal VDP processing to resume.
Statements have no terminators; newlines are whitespace.
See the [resident contract](docs/design/resident-scalar.md) and
[counted-floor example](examples/resident/counted-floor.golem).

[GOLEM-007](docs/tasks/GOLEM-007/README.md) records passing stock-emulator
qualification and successful human review on Oryx.
[GOLEM-008](docs/tasks/GOLEM-008.md) promotes that slice while preserving its
reviewed command bytes. [GOLEM-012](docs/tasks/GOLEM-012/README.md) adds checked
resident addition; automated qualification and Oryx human review pass.
Broader syntax shown in design examples remains proposed.

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
