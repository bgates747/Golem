# Golem

**Golem Obtains Less Excessive Messaging**

The intended language supports [one source with interchangeable execution
backends](docs/design/backend-portability.md): stock VDP buffered commands or
native VDPlets, selected by developers or application end users. Shared semantics,
storage and call interfaces will support startup choice and later state-preserving
runtime selection. The maintained `--portable` mode emits buffered commands and
native C with shared storage and named entries; assembly owns backend selection.
Smaller buffered profiles remain available separately.

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

`make -C src check` builds and checks the maintained host compiler. To compile
the [initialized portable example](examples/portable/counter.golem):

```bash
make -C src check
build/golemc --portable examples/portable/counter.golem build/counter
```

Portable mode supports typed 16/32-bit integers, float32, bool, checked arithmetic,
comparisons/Boolean expressions, finite If/Range control flow and indexed integer
arrays. It emits both backends and shared assembly/layout artifacts; packaging
native C uses the external VDPlet SDK. Read the
[portable compiler guide](docs/design/portable-compiler.md) for exact supported
syntax, errors, options and assembly integration.

Explicit `--target native` also supports float32 division, sin/cos in radians
and floor. The [Flower mathematical tranche](docs/tasks/GOLEM-044/native-math/README.md)
qualifies these operations under actual ESP32 firmware; stock lowering remains
unsupported. Package its generated C with the VDPlet SDK's opt-in `--math` flag.
The interactive Flower client and visual review are still ahead.

Raw mode
supports literal print, uint16 declarations, assignment/addition and For/Next.
Explicit `--resident` mode adds bounded float32 floor-to-sint16 jobs with
persistent uint16 state, checked addition/subtraction/multiplication and paired
`divmod`. Arithmetic failure preserves the assignment's
destination and ends the job, allowing normal VDP processing to resume.
Statements have no terminators; newlines are whitespace.
See the [resident contract](docs/design/resident-scalar.md) and
[counted-floor example](examples/resident/counted-floor.golem).

[GOLEM-007](docs/tasks/GOLEM-007/README.md) records passing stock-emulator
qualification and successful human review on Oryx.
[GOLEM-008](docs/tasks/GOLEM-008.md) promotes that slice while preserving its
reviewed command bytes. [GOLEM-012](docs/tasks/GOLEM-012/README.md) adds checked
resident addition; automated qualification and Oryx human review pass.
[GOLEM-013](docs/tasks/GOLEM-013/README.md) adds checked subtraction and mixed
chains; automated qualification and human review pass.
[GOLEM-014](docs/tasks/GOLEM-014/README.md) adds checked multiplication; automated
qualification and Oryx human review pass.
[GOLEM-015](docs/tasks/GOLEM-015/README.md) adds paired uint16 quotient/remainder;
automated qualification and Oryx human review pass.
Additional portable syntax is documented in the guide above; other syntax
shown in design proposals remains proposed.

The intended integration is PC compilation followed by an eZ80 assembly
application that loads, updates and calls the resident VDP buffers. Compiler
output publishes their assigned locations. Optional `--emit-asm` now generates
assembly symbols, payload bytes and a named eZ80 call wrapper for current resident
packages. [GOLEM-016](docs/tasks/GOLEM-016/README.md) records assembler validation. See the [assembly integration contract](docs/design/resident-scalar.md#assembly-application-integration--direction-agreed-2026-10-12).

The separate [resident table profile](docs/design/resident-table.md) now supports
one immutable typed table and checked whole-record selection, with a named
assembly wrapper and field/table layout symbols. Build-time --table-data supplies
the packed rows. [GOLEM-017](docs/tasks/GOLEM-017/README.md) passes host, assembler
and stock Pi/Oryx headless qualification. Oryx human review passed; the agent
verified the saved results.

The separate [`--stock-matrix` profile](docs/design/stock-matrix.md) adds bounded
stock-only 4x4 affine composition, matrix multiplication and signed16 XYZ buffer
transforms. It exports controls, owned matrices, borrowed vertex storage and
named assembly calls; drawing and completion remain application-owned. Host
contracts and fsim's independent stock-target cube/tank qualification pass;
human visual review remains fsim-owned; compiler publication was authorized.

The separate [resident drawing profile](docs/design/resident-drawing.md) supports
named points, lines and filled convex quads with initialized packed inputs and
assembly call wrappers. [GOLEM-018](docs/tasks/GOLEM-018/README.md) passes host,
assembler and stock Pi/Oryx headless pixel checks. Oryx human visual/keyboard
review passed, and the saved results were retrieved and verified.
Plain `draw_bitmap(bufferId,x,y)` now supports full 16-bit asset IDs and named
assembly calls. [GOLEM-019](docs/tasks/GOLEM-019/README.md) uses familiar Wolf
bitmaps at native sizes; compiler/sanitizer/assembly and stock pixel checks pass.
The Author accepted the corrected static bitmap display; its saved 864-byte
result matches. GOLEM-020 subsequently passed animation and MOS keyboard review.
[GOLEM-020](docs/tasks/GOLEM-020/README.md) adds a finite Wolf bitmap animation: Golem
draws each frame; assembly updates positions and presents with stock VSYNC swaps.
Its saved human run matches all 841 checks / 946 bytes.

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
src/          Maintained C++ compiler, portable backends and host Makefile
tools/        Auxiliary tooling/scripts (not the compiler itself)
examples/     Example Golem programs
tests/        Test suite
```

Streamed installation is available explicitly through `--emit-load-plan` and
`tools/package_agnb.py`: see [AGNB packages](docs/design/agnb-packages.md).
Assembly owns loading/setup/calls; the existing embedded form remains available.
