# REV-001 — Implementation recommendation

The asset/index/copy review is complete. Build toward one small assembly-driven
example: install a typed table once, update an index, call a finite VDP job and
select a whole row locally. Implement generated assembly output first against
existing qualified packages, then add the table job. Keep each tranche separately
reviewable. This document recommends the next work; it does not authorize or
implement it.

## Contract to keep

| ID | Decision |
| --- | --- |
| REC-001 | Compile on the PC. The assembly application owns loading, installation, input updates, calls, pacing and cleanup. Stock VDP cannot initiate MOS file loading. |
| REC-002 | Compiler output publishes assigned IDs, offsets, layouts, entry points and ownership. Direct generated assembly equates and payload byte definitions are the intended integration output, alongside current tool-readable metadata. |
| REC-003 | Source describes typed resident storage. Build configuration accounts for filenames, initial payloads and placement. No ImportBinary runtime operation or per-asset filename lookup belongs in the resident job. |
| REC-004 | Ordinary indexing/assignment provides useful bounds/admission checks. On a defined failure, preserve the assignment destination and end the job; earlier writes remain. |
| REC-005 | Keep checks proportionate to a hobby project. Validate changing external inputs, reuse established validity and avoid copy readback, checksums, rollback systems and unnecessary repeated checks of trusted data. |
| REC-006 | AGNB is a reusable assembly-side bulk-loading option. It avoids repeated MOS directory searches and embeds build-assigned IDs. Existing image/audio forms are available; raw Table/program forms require an extension. |

## Recommended implementation order

### IMP-A — Generated assembly output for the existing package

Add an optional assembly include generated from the same compiled Result/layout
as interface.json and the four binary command streams. Publish namespaced
symbolic IDs/offsets, scalar types/width information, the entry buffer ID and
labeled bootstrap/invoke/update-call/cleanup bytes with lengths. Exact identifier
escaping and assembler syntax must be chosen in the implementation task. No
hand-maintained duplicate numeric addresses.

The application includes the generated definitions and uses its established VDU
transport. Generated output supplies data/command bytes; it does not take over
the application's loop or file loading. Keep raw output behavior intact.

Qualify by assembling a small consumer and comparing its included command/data
bytes to the existing binary artifacts, including a relocated package. Existing
reviewed buffer commands should remain identical. Existing scalar input update
remains 21 bytes including call; plain invocation remains 6. This first step
needs no new VDP operation, Table parser or AGNB form.

### IMP-B — One-dimensional typed table and whole-row selection

Start with one flat row type containing uint16 and sint16 fields, a read-only
four-row table, an initialized mutable selected row, a uint16 index and one
finite Sub. Signed fields initially need only typed storage/copying, not new
signed arithmetic or conversion. Use zero-based consecutive logical bounds for
the first executable fixture; retain the reviewed broader inclusive-bounds
language direction and explicitly reject unsupported cases.

Candidate source, still proposed:

```golem
Record Sample
    uint16 tag
    sint16 delta
EndRecord
Table Sample samples(0:3)
Sample selected = Sample(0, 0)
uint16 index = 0
Sub fetch
    selected = samples(index)
EndSub
```

The PC build maps samples to four packed little-endian rows (1,-2), (2,3),
(3,-4), (4,5), totaling 16 bytes. Assembly installs those rows and the emitted
selected/index initial state before calling fetch. This example adds no
resident filesystem operation. The compiler exports all IDs, field offsets and
entry symbols through its assembly output.

Use compiler-owned consecutive record buffers and reuse Rally's separate
patched-copy-template pattern. Allocate the whole run with collision checks,
working around external reservations. Validate configured row bytes during the
build. Check the changing index inside the VDP; on failure leave the selected
row intact and end the job with an index error. Trusted immutable row data with
full natural type bounds requires no repeated per-field admission.

There is a real prerequisite: current src/resident.hpp requires exactly one
bounded float input and sint16(floor(input)) assignment. Generalize the finite
resident job/package path enough for this table-only Sub, preserving the old
scalar profile and its reviewed bytes. Do not make the user add a dummy float
operation. Keep resident mode explicit; this is not a full functions/locals or
control-flow implementation. The CLI/build mapping format and manifest profile
versioning need concrete selection in the implementation task.

Prove whole-row selection, first/last/invalid indices including 65535, destination
preservation, repeated failure/recovery, table-ID carry, relocation, ownership and
cleanup. Use a deliberately different initial selected row to expose stale or
partial results. Assemble a real consumer with the generated include. Qualify
stock headless output, then stage the stopped Oryx fixture for the Author's
normal launch/dir/close review; the agent collects the saved results afterward.

For a two-byte index update plus six-byte entry call, the illustrative encoding
is 13+6=19 bytes, excluding framing/readback and any extra inputs. Table bytes
and code are installed once. Measure the actual generated fixture before
advertising its byte budget; no table backend has been qualified yet.

### IMP-C — Broader storage and admission, as needed

After the small table works, select the next useful capability rather than
implementing all prototypes at once. Candidates include packed Array reads,
nonzero/signed lower bounds, field Bounds admission, indexed writes and checked
whole-record input admission. Qualify preservation and evaluation order for each
added operation. Multidimensional Arrays, general float admission, arbitrary
prebuilt ID maps and composite deep copies can wait until an application needs
them.

Develop an AGNB raw-data/program form locally when bulk packing is needed; reuse
canonical writers/validators and propose persistent general improvements
upstream. Existing image/audio assets can already use their canonical loaders.
A new form is not a prerequisite for compiling and including a 16-byte table
payload in assembly.

## Why this order

Assembly emission makes the current compiler usable with the Author's existing
toolchain conventions and offers straightforward byte-parity validation. The
next table slice demonstrates resident data selection and reduced recurring
wire traffic without requiring clever optimization or a broad language rewrite.
It also removes the accidental floor-job restriction from the intended use case.

Generated assembly output is a small focused task. Table-only job integration
requires more care because parser, package lifecycle, allocation and conditional
failure handling meet there. Use medium reasoning for the output tranche and
reassess for the table implementation once its precise scope is fixed.

## Evidence and status

Rally source/import/element/checked-copy reviews and host sanitizer regressions
are recorded in this silo. Existing Golem arithmetic already has stock Pi/Oryx
qualification. None of that evidence establishes new Table, Array, Record,
Bounds or assembly-emission implementation. This synthesis changed documentation
only; no additional runtime tests were run.

[Storage review](assets-and-tables-review.md), [AGNB contract](agnb-loading-review.md),
[indexing](indexed-access-review.md), [checked copying](checked-copy-review.md),
[examples and initialization](examples-review.md) and
[shared examples](/Agon/mystuff/golem/docs/tasks/GOLEM-003/examples/examples.golem)
provide the supporting detail. All GOLEM-010 review subtasks are delivered;
Author acceptance of this recommendation and the next implementation scope is
pending. GOLEM-011 drawing and GOLEM-002 module research remain queued separately.

Current implementation checkpoint: [GOLEM-016](../GOLEM-016/README.md) now provides
optional generated assembly output and named eZ80 wrappers for the existing
scalar resident profile. Table/Array/Record support and their broader assembly
interfaces remain future work. Earlier output recommendations above record the
review direction that led to this tranche.
