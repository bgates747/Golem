# AST-001 addendum — AGNB asset loading

Use agon-utils AGNB as the starting point for bulk asset loading by the
eZ80 assembly application that consumes PC-compiled Golem buffers. The
Author identified its central benefits: one sequential container load avoids
repeated MOS searches in crowded directories, and build-time filename/ID
accounting removes per-asset filenames and lookup logic from the runtime.
AGNB already embeds explicit buffer IDs and has reusable image/audio loaders.
Keep extensions local to this task while developing them; useful general
extensions can later be proposed for canonical agon-utils.

## Loading and build contract

AGNB-L01: The build resolves source asset filenames, validates/converts payloads,
assigns final buffer IDs and emits the container plus matching program symbols
and manifest. Compiler-generated assembly equates and payload byte definitions
are the preferred additional integration output, still to be implemented. The
assembly application owns loading, input updates, entry calls, pacing and
cleanup; resident Golem code uses installed storage. Only the container path
needs to be supplied to its loader; it needs no per-asset filename registry,
directory enumeration or name-to-buffer lookup.

AGNB-L02: The assembly application opens a container once through its MOS-side
loader and reads the records sequentially. The Author
reports that MOS file lookup becomes slow in directories with many entries;
packing many loose files avoids repeating that lookup for each asset. Source
filenames may remain in build diagnostics/provenance without entering the
runtime image. A small set of containers is also compatible with this approach.

AGNB-L03: Preserve explicit IDs end to end. The writer supplies each BHDR ID;
the existing loader validates and uses it unchanged. Reordering records does
not change their identities. Source declaration order must not accidentally
change an externally stable asset interface. All IDs must be reconciled with
Golem allocation before compilation emits its buffers. Golem excludes both 0
and 65535, even though AGNB itself only rejects 65535.

AGNB-L04: Support two ownership situations in the build manifest: a container
loaded as part of the Golem package owns its declared assets; assets loaded by
another application are borrowed external reservations. Package cleanup must
respect that distinction and eventually specify form-specific bitmap/sample
teardown. Current `--reserve` represents externally owned space; it must not be
silently repurposed as a compiler-owned import claim.

AGNB-L05: Use one authoritative build description to drive the existing AGNB
writer and Golem allocation/symbol metadata. For an existing prebuilt container,
inspect/validate its embedded IDs first and account for them before allocating
compiler storage. Do not add an independent runtime ID remapper. The precise
build integration and source spelling remain proposals for subsequent review.

## What is implemented in agon-utils

| ID | Evidence | Implication |
| --- | --- | --- |
| AGNB-F01 | RIFF/AGNB envelope; VERS; LIST BUFR with BHDR, form descriptor and DATA. Integers are little-endian; chunks align to four bytes. | Reuse the writers and validators rather than recreating container layout. |
| AGNB-F02 | Image 0.1 records carry explicit ID, width, height and format; implemented format 1 is RGBA2222, with exact width × height DATA bytes. | Existing image assets can use the established format and bitmap finalization. |
| AGNB-F03 | Provisional audio 0.2 records carry explicit ID, unsigned mono PCM format 0x09 and a uint16 sample rate. | Audio already has an implemented path, but its provisional version must be named explicitly. |
| AGNB-F04 | The public eZ80 ADL assembly API exposes image/audio load routines and optional per-record completion callbacks. It owns parser state, a FatFS FIL and an 8 KiB transfer window. Calls are synchronous and nonreentrant. | Load from the MOS-side loader; VDP-resident Golem jobs need no filesystem API. Preserve the API's separation from application UI, plotting and playback. |
| AGNB-F05 | The implementation validates record metadata/enclosing bounds before reading/uploading DATA, streams bounded pieces and consolidates the completed buffer before finalization. | Preserve strict validation and streaming; large DATA is not restricted to a single 65535-byte VDP block. |
| AGNB-F06 | The current assembly readers expect their supported version and exact record layout. The image reader explicitly requires BHDR/IMAG/DATA in sequence. | Check actual implementation when extending forms; the broad format's extensibility does not mean arbitrary new forms already load. |
| AGNB-F07 | No raw/table form is defined by the implemented layouts. Compression is future format work in the 0.1 specification. | Typed table payloads or compressed Golem programs would require a documented extension; do not mislabel them as existing image/audio records. |

The older API précis discusses audio as future work; the public API README,
implemented-layout document, audio writer and assembly implementation establish
that provisional audio now exists. Prefer those current implementation sources.
Loader success is its parser/file/dispatch result; the code does not establish a
general acknowledgement that every VDP allocation/finalization succeeded. It
also does not provide whole-container rollback after earlier records load.

## Proposed local extension path

AGNB-R01: Use existing image/audio records where they fit. Typed Table/Array
declarations describe storage; filename mappings belong to build configuration.
Retain raw binary input as a build-time source format for typed arrays/tables; this does not require
one loose file per asset at runtime. The build can pack those inputs into a
container once a raw/table form is defined.

AGNB-R02: For raw/table data, develop a small versioned form and loader extension
under the Golem task silo if needed. Define payload interpretation, exact size,
metadata, schema relationship and finalization explicitly. Keep logical Table
indices separate from physical BHDR IDs. Do not freeze a new FourCC or invent
an upstream-supported schema in this review.

AGNB-R03: Keep the extension reusable and application-neutral. After it proves
useful and passes its own validation, propose its specification, writer,
validator and loader changes for canonical agon-utils. The Author explicitly
authorized local adaptation and this upstream direction; no canonical utility
change or upstream publication was performed during this review.

## Traffic implications

AGNB removes repeated file discovery and runtime filename bookkeeping. Its
headers and descriptive chunks are parsed on eZ80; the loader sends payloads
and the required VDP setup commands. Packing loose files by itself does not
compress their payloads. The recurring wire saving comes from keeping assets
resident and sending small job parameters after installation. Record both
loading costs and recurring application bytes when this is implemented.

## Sources and scope

Canonical sources examined:

- [Implemented layouts](/Agon/mystuff/agon-utils/examples/agnb/api/file-format.md)
- [Public loader API](/Agon/mystuff/agon-utils/examples/agnb/api/README.md)
- [Assembly implementation](/Agon/mystuff/agon-utils/examples/agnb/api/agnb_api.inc)
- [Container specification](/Agon/mystuff/agon-utils/examples/agnb/docs/agon-buffer-file-format-specification.md)
- [Image writer](/Agon/mystuff/agon-utils/examples/agnb/images/container/scripts/do_assembly.py)
- [Image reader/validator](/Agon/mystuff/agon-utils/examples/agnb/images/container/scripts/view_agnb.py)
- [Audio writer](/Agon/mystuff/agon-utils/examples/agnb/audio/scripts/build_audio_container.py)
- [MOS/VDP loader précis](/Agon/mystuff/agon-utils/examples/agnb/docs/agnb-loader-agon-api-precis.md)

[Source identities](agnb-source-audit.json) record the exact examined files.
This was a source/contract review; no utility files were changed and no new
loader, container, emulator or hardware qualification was performed. This addendum completed the AST-001 follow-up. See the
[task plan](../GOLEM-010.md) for the subsequent review boundary.

## Author clarification — assembly is the host

The intended workflow does not require Golem to become a MOS application.
Compilation occurs on the PC. The consuming hand-written assembly program owns
file operations, reads through eZ80 transfer RAM, and sends VDP commands. Stock
VDP cannot tell MOS to load a file. The earlier ImportBinary source sketches
have been withdrawn from the asset/index reviews to avoid implying that behavior.
An interactive BASIC-like Golem application is a possible future frontend only.

Compiler output should be directly includable assembly where useful: named IDs,
offsets, layouts/entry symbols, labeled VDP byte payloads and lengths, generated
from the same authoritative layout as machine-readable metadata and AGNB build
accounting. Assembly emission is an intended output option, not implemented yet.
The compiler assigns locations; assembly performs actual installation.

Current implementation checkpoint: [GOLEM-016](../GOLEM-016/README.md) now provides
optional generated assembly output and named eZ80 wrappers for the existing
scalar resident profile. Table/Array/Record support and their broader assembly
interfaces remain future work. Earlier output recommendations above record the
review direction that led to this tranche.
