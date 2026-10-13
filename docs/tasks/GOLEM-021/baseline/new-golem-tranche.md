# Optional Golem tranche: multiple resident drawing Subs

Multiple finite named drawing Subs sharing one packed global input buffer are a
useful, bounded next compiler change. Existing lowering already produces finite
entry streams and private patchable drawing templates; it needs a collection of
procedures and assembly entry exports, rather than a new execution model. Keep
the first declared Sub as the legacy default and preserve every existing
single-Sub package byte for byte. Implementation is conditional on GOLEM-021's
primary vdplet/emulator/6502 outcomes being delivered with time remaining.

This is an assessment and proposed scope under GOL-001, not production work.
No compiler, maintained test, profile or runtime source was changed for it.

## Proposed source and host contract

```golem
uint8 colour Bounds(0,63) = 11
sint16 x = 40
sint16 y = 30
Sub drawMarker
    draw_point(colour, x, y)
EndSub
Sub drawLine
    draw_line(colour, x, y, 100, 80)
EndSub
```

Assembly installs the package once, updates `x`/`y` through their existing
buffer/offset exports, then chooses `call golem_drawMarker` or
`call golem_drawLine`. Both read the same persistent globals. Each wrapper
transmits one six-byte buffered call and has the existing HL/BC clobber and
completion contract. Updating N contiguous bytes then invoking still costs
17+N bytes. Multiple Subs do not make several calls cost only six bytes; their
wire benefit is sharing inputs between independently selected jobs and avoiding
duplicated input storage/installations for separate packages.

Keep globals before all Subs, explicit typed initializers, terminator-free
statements and case-insensitive keywords/builtins. Keep variable identifiers'
current case sensitivity. Each Sub must contain at least one currently supported
drawing operation and terminate with EndSub. Arguments, local declarations,
assignments, branching, loops, nested Subs and calls from one Sub to another
remain outside this tranche. Consequently every entry remains finite.

All calls and host input updates remain serialized and nonreentrant. Assembly
owns loading, pacing and cleanup. There is no implicit call to every Sub, new
completion acknowledgement, scheduler, frame wait or native vdplet dependency.

## Bounded implementation

- PAR-001 [ ] Replace the single procedure body in `resident_drawing::Spec`
  with an ordered collection of named procedure bodies. Reuse the existing
  operand parser and declaration-name checks for every Sub. Reject empty,
  incomplete, duplicate or unsupported later bodies before publication.
- LOW-001 [ ] Keep one input allocation for the complete declaration-ordered
  global block. Allocate each Sub's entry and private operation templates in
  source order: first entry and its templates, then the next entry/templates.
  Preserve single-Sub IDs, owner-role strings, initial bytes and all emitted
  commands. No template sharing or optimizer is needed.
- LOW-002 [ ] Scan bitmap operands across every body before compiler
  allocation, checking their initial/literal IDs against external reservations.
  Enable stock affine support once if any body uses a bitmap. Install and clean
  each compiler-owned buffer once; borrowed assets remain outside ownership.
- ABI-001 [ ] Retain `Result.entry`/`invoke` as the first declared Sub's default
  call and keep CLI output filenames unchanged. `invoke.bin` and
  `update-call.bin` target that default. Publish all entries in an ordered
  manifest `entries` array for multi-Sub packages while retaining legacy
  `entry`; each entry gives name, ID, block count and byte size. Keep single-Sub
  JSON exactly unchanged, including schema 1 and field ordering.
- ASM-001 [ ] Extend assembly rendering to emit every `golem_<SubName>` wrapper,
  `_buffer`, `_call` and `_call_len`, with common global/package exports only
  once. Prefer a collection-based internal renderer plus the existing
  single-entry adapter for scalar/table callers. Use one symbol-validation set
  across the whole include; preserve the single-entry text exactly.
- DOC-001 [ ] Update the maintained drawing contract and an initialized source
  example to describe selection/default semantics and shared globals. Keep the
  exploratory scripts, receipts, assembler consumers and review fixture in the
  agreed task silo, promoting only agreed production documentation/tests.

The likely production surface is `resident_drawing.hpp`, `assembly_output.hpp`,
maintained contract tests, and the drawing contract/example. `golemc.cpp` can
remain unchanged if the default `Result.invoke` interface is retained.

## Targeted qualification and acceptance

- TST-001 [ ] Add maintained positive cases with two and three distinct Subs,
  shared global references, literal-only procedures, and a later bitmap Sub.
  Assert distinct entry/template IDs, exactly one input block, six-byte calls
  and complete cleanup ownership. Add negative cases for duplicate names,
  malformed/empty later bodies, declarations between Subs and unsupported
  body statements. Check exhausted IDs and oversized entry streams using the
  existing allocator/write-block limits.
- TST-002 [ ] Exercise whole-include collisions and rejection before output
  writes: case-folding collisions, a field colliding with an entry export,
  `paint` versus `paint_call`, and package payload names. Keep the current
  normal-source identifier policy; requested assembly exports must satisfy
  ez80asm's case folding and 64-character limit.
- TST-003 [ ] Run maintained host checks and the existing ASan/UBSan pattern.
  Compare all six old artifacts against an archived current compiler for
  representative raw/scalar/table and single-Sub geometry/bitmap packages,
  including relocated IDs. Compare raw payloads separately where applicable.
  Existing accepted single-Sub output must remain byte-identical.
- TST-004 [ ] Assemble a real ADL consumer of every new named wrapper, verifying
  call packets, wrapper opcodes, field offsets and shared package payloads.
  Use both ordinary and relocated IDs. Confirm invoking one entry does not
  accidentally execute another body.
- TST-005 [ ] Run one small stock MOS/VDP qualification using existing pixel
  query and saved-result idioms: install once, call A, partially update shared
  fields, call B, then call A again without reinstalling. Compare selected
  pixels against independent direct VDU commands, verify input persistence,
  cleanup absence and borrowed-asset preservation. Prefer a focused fixture
  over rerunning the entire historic bitmap pixel matrix.
- REV-001 [ ] Stage the same completed tranche on the stopped dedicated Oryx
  profile, with readable Golem source and a recognizable Wolf bitmap if using
  images. Identify which named Sub each scene invokes and assembly's host
  role. Let the Author launch, observe and type dir; retrieve saved results
  afterward. Leave emulator-related changes uncommitted until human validation
  and explicit commit approval.

The key acceptance result is independent selectable entries using the same
updated resident inputs, with unchanged legacy packages and complete ownership
accounting. There is no need for benchmarks, new readback machinery in the
production hot path, general linking, stack frames or inter-Sub calls.

## Specific pitfalls from the current source

- PIT-001: `profile()` uses the first Sub's first statement to distinguish a
  source beginning with uint16 from the scalar backend. Keep that selection
  behavior; do not scan later Subs and accidentally reclassify an existing
  scalar program as drawing. The selected drawing parser validates every body.
- PIT-002: The present bitmap reservation/affine scan iterates one flat
  `spec.operations` vector. A partial refactor could silently omit bitmaps
  appearing only in later Subs; LOW-002 explicitly covers this.
- PIT-003: `renderPackage()` currently owns one entry name and one invoke
  payload. Calling it once per Sub would duplicate global/package symbols and
  payloads. Extend the internal rendering once, preserving scalar/table output.
- PIT-004: Current callers and tests use default `Result.entry` and `.invoke`;
  keeping these derived defaults avoids an unrelated CLI/model migration.
  Extra entries must not silently change the meaning of old invoke artifacts.
- PIT-005: `writeBlock()` already rejects a program block above 65535 bytes,
  and allocation exhaustion already rejects too many buffers. Apply those
  checks to every new entry before output publication. Aggregate bootstrap
  length is a separate host payload size and need not acquire an invented
  65535-byte package limit.
- PIT-006: The old rejection fixture `EndSub Sub two EndSub` can still reject
  because the second body is empty. Add a valid second body as a positive
  regression so that this old test cannot mask missing multi-Sub support.

This is moderate implementation work with no new lowering algorithm. A sensible
tranche is the compiler/export change plus focused host, assembler and stock
emulator checks, followed by the required Oryx staging. Oryx availability and
human review are external timing dependencies; do not spend the primary vdplet
delivery budget expanding this optional feature.

## Assessment source identities

Reviewed at repository HEAD `0f08f10a0bef508fab2617cbd41c26944a6cc33c`:

| File | SHA-256 |
| --- | --- |
| src/resident_drawing.hpp | `cd84a55b1c97a96200b1eba1cbd8bd9effb866ec37b5e091e11b7262db26c994` |
| src/assembly_output.hpp | `c5745e4d23d591c363f748ce11fd3d443f6e4851c2a0be04530adf49788fea4a` |
| src/golemc.cpp | `a7b30dacaa79523d7e31cde654bcc0f8de0167316932465636c659d55062c1f4` |
| tests/test_compiler.cpp | `a0e39ab146ba786bc392ad07ab3385eb419ceedc06f941893c0ebbe31bc91042` |
| docs/design/resident-drawing.md | `61b844227ac2186e70a825cac2ba8addca6bf3a698cffa988a8d7bea762fb794` |

Existing GOLEM-018/019 qualification scripts and the GOLEM-020 initialized Wolf
animation source were read as fixture patterns. No new compiler checks or
emulator sessions were run for this read-only assessment.
