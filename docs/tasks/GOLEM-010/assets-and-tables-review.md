# AST-001 — Rally assets and tables: storage review

Rally already imports binary data once and keeps it resident in the VDP. Golem
should retain that behavior while giving tables an explicit row type, deriving
field offsets and record width, and validating every configured row during the build.
Use [AGNB bulk loading](agnb-loading-review.md) to move filename resolution and
asset-ID accounting into the build and avoid repeated MOS directory searches.
Keep ordinary data placement compiler-managed; expose numeric placement only for
assets that need a stable external interface. This is a storage recommendation
for review, not an implemented language feature. Indexed access and checked
copying remain subsequent subtasks.

## Findings from Rally

| ID | Finding | Source |
| --- | --- | --- |
| AST-F01 | `Asset name(id,"path",bytes)` reads one binary file into one buffer. `Table name(firstId,recordBytes,count,"path")` splits a binary file into one buffer per record. Neither imports CSV, database files or source code. | [Declarations](/Agon/mystuff/golem-rally19/src/hosted.hpp:430) |
| AST-F02 | Paths are relative to the source directory. Absolute paths, literal `..` components, empty paths and NULs are rejected. Required file length is exact, not a maximum. This is a spelling restriction, not a symlink containment guarantee. | [Import checks](/Agon/mystuff/golem-rally19/src/hosted.hpp:434) |
| AST-F03 | Each asset/record is 1:65535 bytes. Table count is requested in 1:1024, but placement within IDs 4000:4999 limits it to at most 1000 records, fewer for a later starting ID. Each import and all imports together are capped at 512 KiB. | [Declaration limits](/Agon/mystuff/golem-rally19/src/hosted.hpp:446), [aggregate limits](/Agon/mystuff/golem-rally19/src/hosted.hpp:1844) |
| AST-F04 | A table declares record width and count, but has no row schema. `Field` declarations describe typed views on buffers, matrices or assets; a Table cannot own Fields. During `LoadIndexed`, each record is checked against fields declared on that particular destination. An unused table therefore receives size checks but no typed-row validation. | [Fields](/Agon/mystuff/golem-rally19/src/hosted.hpp:497), [record validation](/Agon/mystuff/golem-rally19/src/hosted.hpp:1008) |
| AST-F05 | Types are u8, u16, s16, s32, q8s16, q8s32 and f32. Integer/float fields decode little-endian; fixed-point fields have eight fractional bits. Import validation rejects nonfinite floats and values outside declared field bounds. Bytes outside declared fields are not typed or validated as fields. | [Types](/Agon/mystuff/golem-rally19/src/hosted.hpp:159), [decode/validate](/Agon/mystuff/golem-rally19/src/hosted.hpp:533) |
| AST-F06 | Assets are read-only through the supported operations. Tables are imported data and are not accepted as writable Buffer/Field destinations. This is a compiler contract, not VDP memory protection: external commands can still change their buffers. | [Write checks](/Agon/mystuff/golem-rally19/src/hosted.hpp:287), [symbol kinds](/Agon/mystuff/golem-rally19/src/hosted.hpp:241) |
| AST-F07 | Imported buffers are owned by the compiled package, installed in bootstrap and included in cleanup. Records occupy consecutive IDs. External resources, such as separately installed bitmaps, have a different ownership contract. | [Installation and ownership](/Agon/mystuff/golem-rally19/src/hosted.hpp:1815), [cleanup](/Agon/mystuff/golem-rally19/src/hosted.hpp:1861) |
| AST-F08 | User IDs must be at least 1000; private scratch uses 2:999. Additional application-specific ranges are excluded. Collision tests cover table records against declarations in either order. These are Rally allocation policies, not requirements of the Buffered Commands API. | [Claims](/Agon/mystuff/golem-rally19/src/hosted.hpp:256), [import tests](/Agon/mystuff/golem-rally19/tests/test_imports.cpp:56) |
| AST-F09 | The map records path, size, count and FNV-1a-64 content fingerprint. Cleanup lists every owned ID. This supports reproducibility but is not a typed import schema or cryptographic integrity check. | [Import map](/Agon/mystuff/golem-rally19/src/hosted.hpp:468) |

Rally's [worked lookup fixture](/Agon/mystuff/golem-rally19/examples/native_lookup/kernel.golem)
imports four eight-byte records and an all-byte asset. Its
[generator](/Agon/mystuff/golem-rally19/examples/native_lookup/make_assets.py)
packs pairs of signed 32-bit values as `<ii`, without native C/C++ struct padding.
The separate [lookup README](/Agon/mystuff/golem-rally19/examples/native_lookup/README.md)
records historical stock-emulator lifecycle probes. That evidence belongs to
Rally's historical implementation; it does not qualify a new Golem import backend.

## Comparison with maintained Golem

| ID | Current Golem | Consequence for integration |
| --- | --- | --- |
| AST-C01 | No source-level Asset, Table, Record or Array implementation. Inclusive Array bounds are an accepted design; Table declaration grammar remains undecided. | Add typed storage deliberately; do not treat the older GOLEM-001 pseudocode as a current specification. |
| AST-C02 | The shared allocator supports external inclusive `--reserve FIRST:LAST` ranges before compiler allocation; IDs 0 and 65535 are unavailable. | Reuse collision accounting. An external reservation is borrowed space that the package must not install or clear; imported data needs owned claims instead. |
| AST-C03 | Resident packages publish owned buffers and explicit cleanup; uint16 globals persist across calls and reset on reinstall. | Extend the existing manifest/lifecycle contract for imports rather than adding a separate asset loader ownership model. |
| AST-C04 | Resident arithmetic failures preserve the failed destination and end the job; there is no automatic MOS acknowledgement. | Preserve the resident error model when indexed access is reviewed in IDX-001. Storage declarations alone should not invent a host error round trip. |

References: [allocator](/Agon/mystuff/golem/src/buffer_allocator.hpp:10),
[resident grammar](/Agon/mystuff/golem/src/resident.hpp:15),
[resident contract](/Agon/mystuff/golem/docs/design/resident-scalar.md),
[accepted Array/Range design](/Agon/mystuff/golem/docs/design/language-type-proposals.md:797),
[asset placement discussion](/Agon/mystuff/golem/docs/design/language-type-proposals.md:1196).
Axis 23's illustrative low-ID region does not override the maintained exclusion
of IDs 0 and 65535. No fixed low/high split has been selected.

## Assembly owns installation

The Author clarified the target workflow: the PC compiler assigns resident
storage and emits its payloads/interface; a hand-written assembly application
loads assets, installs buffers, updates inputs, calls entries and cleans up.
The build maps data files to declared symbols. VDP Golem code does not open files
or ask MOS to load them. Generated assembly definitions for IDs, offsets, entry
points, payload bytes and lengths are the preferred additional output, still to
be implemented. They can be included directly in application assembly.

## Recommended storage contract

AST-R01: Keep three concepts explicit. An Asset is an opaque read-only byte
payload, an Array has homogeneous typed elements, and a Table has homogeneous
row structure with explicitly typed, potentially different fields. A table is
not a Variant array. Matrices retain their separate arithmetic type.

AST-R02: Give a Table its row schema at declaration. Derive field offsets and
record size from it, validate every configured row during the build, and reject the entire input
on any mismatch. Do not postpone validation until a particular destination is
used. Numeric `Bounds` constrain values; inclusive `lower:upper` bounds describe
indices. Keep these separate from `Range` sequences.

AST-R03: Start with exact-size binary inputs in build configuration: packed fields in declaration
order, little-endian integers and IEEE binary32 floats, no implicit padding,
coercion or missing rows. Fixed-point imports must declare fractional precision
and use the corresponding signed representation; Rally's q8 spelling does not
establish support for every fixed16(n) format. Reject nonfinite/out-of-bounds
fields at build time. CSV and database adapters can produce the binary file
outside the compiler later.

AST-R04: Keep configured asset payloads read-only to resident jobs initially. Mutable working rows and arrays are
separate typed storage with explicit valid initialization. No runtime file I/O,
heap or resize facility is needed for this first storage contract. Replacement
of imported payloads while installed is outside it; changing source data means
rebuilding/reinstalling.

AST-R05: Let the compiler choose ordinary table/array storage IDs. Allow named
contiguous placement for assets only where external consumers require stable
base-plus-index IDs. Distinguish package-owned imported ranges from borrowed
externally installed ranges; cleanup clears only the former. Validate all claims
before emission and use the common allocator, without copying Rally's hardcoded
4000:4999 window or private/user split. The contiguous-run allocation mechanism
still needs implementation design; current `allocate()` provides single IDs.

AST-R06: Keep one-buffer-per-record as a simple first Table backend candidate,
with the same logical declaration available to a packed backend later. This
avoids requiring a clever optimizer: compile-time schema handles layout, and
Rally's existing guarded selection handles addressing. Packed Arrays remain a
distinct candidate. Account for ID count, metadata and byte budgets before
choosing storage; a large record count can exhaust IDs well before payload RAM.
Do not present Rally's application caps as universal hardware limits.

AST-R07: Publish generated assembly symbols/payload definitions and extend
the package manifest with logical index bounds, row/element
schema, physical mapping, ownership, exact bytes and source/content hashes.
SHA-256 is suitable for recorded identity; it does not change the wire protocol.
Report import bytes separately from program/scratch installation and recurring
invocation bytes. Preserve source-relative paths and explicit formats.

## Syntax sketch for review

This is a candidate spelling confined to this review. `Record`, `Table` and `Asset`
here are not supported compiler grammar. Filename mappings belong to the PC
build configuration; the earlier ImportBinary spelling is withdrawn. The existing accepted
Array spelling remains `Array float32 readings(0:3, 0:9, 0:19)`.

```golem
Record Sample
    sint32 a Bounds(-100, 100)
    sint32 b Bounds(1, 10)
EndRecord

Table Sample samples(0:3)
Asset everyByte(256)
```

Build configuration maps samples to records.dat and everyByte to bytes.dat.
The assembly application installs both configured payloads at the generated IDs
before a resident job uses them; these declarations do not initialize or load
files by themselves. `records.dat` must contain
exactly four packed eight-byte rows: (7,9), (-4,6), (31,2), (80,1). `bytes.dat`
contains exactly the bytes 0 through 255. These are the worked Rally fixture's
values. The row type determines eight bytes; bounds 0:3 determine four rows.
`Asset everyByte(256)` proposes a byte count, not an index bound or inferred
numeric type. Exact declaration keywords and reservation spelling remain open.
No access, copy or error-handler spelling is selected in this subtask. Shared
examples will be amended in SYN-001 after the related reviews.

## Why this reduces wire traffic

Static coefficients/assets are installed once. Each finite job then receives an
index or other small parameters and consumes the resident data inside the VDP.
The host does not resend the selected row on every call.

For an illustrative two-byte index update followed by one six-byte call, Rally's
literal Set encoding takes 13 + 6 = 19 bytes. Sending an N-byte row through the
same Set-plus-call mechanism takes N + 11 + 6 bytes. A 228-byte row would therefore
take 245 bytes instead of 19, saving 226 bytes per invocation. This compares
parameter transport only, excluding any other required inputs, result retrieval
and framing. It is an encoding calculation, not a new executable Golem profile;
GOLEM-015's float-update/call contract remains 21 bytes.

One-buffer-per-record incurs installation overhead: each record receives a
six-byte clear plus an eight-byte write header. Four eight-byte rows therefore
cost 4 × (8 + 14) = 88 bytes to install, before code, scratch, other assets and
bootstrap transport. Consecutive IDs enable resident selection; they do not
require separate record uploads on every frame. Import compression/bulk packing
can be considered later without changing the logical table declaration.

Encoding references: [write block](/Agon/mystuff/golem-rally19/src/golemc.cpp:224),
[literal Set](/Agon/mystuff/golem-rally19/src/golemc.cpp:246),
[bootstrap clear/write](/Agon/mystuff/golem-rally19/src/hosted.hpp:1824).

## Evidence and review boundary

The read-only source audit used Rally commit
`4db7e96f70c3d63f4ab53766f3b32063e9357c8b`. Its existing import regression was
rebuilt with warnings as errors and ASan/UBSan, with all generated files under
this task's ignored build directory. [Import audit](import-audit.json) records
source hashes, command, verdict and duration. It exercises exact sizes, typed
import rejection, source-relative paths, collisions in both declaration orders,
immutable asset writes, cleanup ownership and fourteen negative cases. The
Rally reference checkout remains clean. No emulator was launched, and Golem's
compiler and shared examples were unchanged.

AST-001 findings were endorsed by the Author, with the subsequent assembly
integration clarification incorporated here. See the [task plan](../GOLEM-010.md)
for the current review boundary. CPY-001 covers checked copying; SYN-001
consolidates reviewed syntax in examples.

## Author review and AGNB follow-up

The Author endorsed this direction and requested investigation of agon-utils
AGNB containers. The [AGNB addendum](agnb-loading-review.md) records the build-time
filename/ID contract, existing image/audio support and a local extension path
for other payloads. No subsequent subtask or compiler implementation began.

Current implementation checkpoint: [GOLEM-016](../GOLEM-016/README.md) now provides
optional generated assembly output and named eZ80 wrappers for the existing
scalar resident profile. Table/Array/Record support and their broader assembly
interfaces remain future work. Earlier output recommendations above record the
review direction that led to this tranche.
