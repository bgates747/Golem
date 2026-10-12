# IDX-001 — Indexed array and table access

Recommend `value = values(index)` for an array element and
`selected = samples(index)` for a complete table row. Check inclusive bounds
inside the VDP before computing an address or copying data. On failure, preserve
the assignment destination and end the resident job using the established error
model. Rally supplies useful guarded-copy machinery, but its success-flag API
should be hidden behind ordinary Golem assignment. These are proposals for
review; compiler code and the shared examples remain unchanged.

## Rally behavior and gaps

| ID | Observed behavior | Evidence |
| --- | --- | --- |
| IDX-F01 | `LoadIndexed(destination,table,index[,status])` selects a zero-based record. It tests `index < recordCount`, adds the admitted index to the first record ID, patches a separate copy template's source ID and calls it. | [Table lookup](/Agon/mystuff/golem-rally19/src/hosted.hpp:986) |
| IDX-F02 | `LoadElement(destination,source,index,status)` selects a fixed-width element/record from one packed Buffer or Asset. Element width is the destination buffer size. It checks `index <= count-1`, multiplies the admitted index by the byte stride and patches a separate copy template's source offset. | [Packed lookup](/Agon/mystuff/golem-rally19/src/hosted.hpp:920) |
| IDX-F03 | Both require uint16 indices and compatible, separate storage. `LoadElement` demands complete repeated field layouts with matching types and compatible field bounds. `LoadIndexed` checks every imported row against the destination fields, but does not itself require complete typed coverage of the destination buffer. | [Layout checks](/Agon/mystuff/golem-rally19/src/hosted.hpp:936), [record layout](/Agon/mystuff/golem-rally19/src/hosted.hpp:640), [imported row checks](/Agon/mystuff/golem-rally19/src/hosted.hpp:1008) |
| IDX-F04 | A provided status is reset to 0, then set to 1 after an admitted copy. Invalid access preserves the destination and continues the caller. Table lookup may omit status only when the declared index bounds fit the table; it still emits a runtime guard. Packed lookup always requires status. | [Status and guard](/Agon/mystuff/golem-rally19/src/hosted.hpp:991), [packed status](/Agon/mystuff/golem-rally19/src/hosted.hpp:947) |
| IDX-F05 | Neither operation provides a general indexed store, multidimensional source syntax, arbitrary lower index bounds, pointer dereferencing or heap access. | [Supported operations](/Agon/mystuff/golem-rally19/src/hosted.hpp:920) |
| IDX-F06 | The existing native fixture crosses table IDs 4095/4096 and rejects indices 4, 65534 and 65535 for a four-record table. This establishes historical Rally behavior, not new Golem qualification. | [Lookup evidence and limits](/Agon/mystuff/golem-rally19/examples/native_lookup/README.md) |

Rally's status is a software guard result. The emitted template call has no
general VDP copy acknowledgement. Its correctness assumes the installed buffers
exist, retain their declared sizes and remain exclusively controlled. An
external command clearing a supposedly resident source is not an index error
that the guard can detect.

## Proposed source contract

IDX-R01: Use parentheses for indexing, consistent with the accepted Array
prototype and VBA influence. A homogeneous array access yields its element type;
a Table access yields its declared row type. A table's row may contain different
explicitly typed fields. Indexing does not introduce implicit variable types or
implicit numeric conversion.

IDX-R02: Both endpoints are inclusive. For each dimension require
`lower <= index <= upper`. Indices are integer expressions; reject float/fixed
indices unless explicitly converted to an accepted integer type. Negative values
are ordinary declared indices when the lower bound allows them, not reverse
indexing from the end. A literal out-of-bounds index is a compile-time error.
A dynamic out-of-bounds index takes the resident error path.

IDX-R03: Evaluate index expressions once, left to right. Check their original
values against the declared bounds before narrowing, subtracting the lower bound,
multiplying by a stride or patching an instruction. Normalize an admitted index
to `index - lower`. Compiler-private address calculations need sufficient width;
they must not wrap merely because a source index is uint16/sint16.

IDX-R04: Default access failure preserves the entire destination, skips later
statements and returns control to normal VDP processing. Earlier successful
statements remain committed. Record an `index_out_of_bounds` resident status;
choose its numeric value when implementing the error catalogue. Existing codes
1:4 keep their meanings. Reset status on the next invocation, as current jobs do.
No host readback, acknowledgement or extra wire command is required for this
control flow. Do not silently clamp an index, wrap it or substitute zero.

IDX-R05: Check every dimension before any source copy or destination write. For
`total = values(i) + values(j)`, retain the old total until both reads and the
arithmetic succeed. For a selected row, preserve all of the old row if access
fails. The same preservation requirement should apply to a future indexed store;
store syntax, evaluation order with the right-hand side, field conversion and
checked-copy compatibility remain for CPY-001 and implementation review.

IDX-R06: Require compile-time compatible element/row types. Start with selecting
a whole declared row into a destination of the same row type. This gives the
one-shot mixed-field access the Author wanted. Field names can subsequently be
used on that local working row. Direct `samples(i).field`, slices, partial rows
and mixed-type projection are useful future possibilities, not prerequisites.
No unchecked byte count or field offset appears in application source.

IDX-R07: Keep runtime guards for dynamic indices in the first implementation,
even if a declared Bounds clause appears to prove them safe: externally updated
host fields are not validated by a declaration. Constants can be resolved during
compilation. More elaborate guard elimination is unnecessary for wire savings.

IDX-R08: Do not add a second error style in this tranche. A future explicitly
named try-access operation could return bool and preserve its destination while
continuing, for code that expects missing indices. Ordinary access should follow
the existing halt-on-error policy; Rally's mandatory status boilerplate need not
become application syntax. Future user handlers can extend the existing model.

## Candidate examples

These blocks are proposed syntax, not executable maintained programs. Record
construction remains a candidate spelling. Each block includes its required
build mapping and installation precondition below: the assembly application
must load/initialize the declared buffers using generated IDs before calling
fetch. Source declarations do not perform filesystem operations. The earlier
ImportBinary spelling is withdrawn. SYN-001 will consolidate reviewed examples
later.

### Selecting a whole typed row

```golem
Record Sample
    sint32 a Bounds(-100, 100)
    sint32 b Bounds(1, 10)
EndRecord

Table Sample samples(0:3)
Sample selected = Sample(0, 1)
uint16 index = 2
Sub fetch
    selected = samples(index)
    print "selected"
EndSub
```

The build maps samples to records.dat. Assembly installs that data and the
compiler-emitted initial state for selected/index before invoking fetch. The
file contains exactly the four packed little-endian sint32 pairs (7,9),
(-4,6), (31,2), (80,1). Index 2 selects the complete row (31,2). Index 4 preserves
selected's previous row and skips the print. Initial selected.b is 1 because its
declared bounds prohibit zero. No separate lookup per field is required.

### A signed lower bound

```golem
Array sint16 values(-2:1)
sint16 index = -1
sint16 selected = 99
Sub fetch
    selected = values(index)
EndSub
```

The build maps values to signed-values.dat. Assembly installs that payload and
the emitted initial state for index/selected before invoking fetch. The file
contains exactly four little-endian sint16 values, 10, 20, 30, 40.
Index -1 selects 20; index -3 is invalid and preserves 99 on the initial call.
The compiler handles normalization, without unsigned subtraction underflow.

### Three-dimensional access

```golem
Array float32 readings(0:3, 0:9, 0:19)
uint16 first = 2
uint16 second = 5
uint16 third = 7
float32 selected = 1.0
Sub fetch
    selected = readings(first, second, third)
EndSub
```

The build maps readings to readings.dat, containing all 800 elements, for
example 3,200 zero bytes. Assembly installs those bytes and the emitted initial
state for coordinates/selected before invoking fetch. Proposed
layout is last dimension varying fastest: the element number is
`((first * 10) + second) * 20 + third`, yielding 507 and byte offset 2028 here.
Each coordinate must be checked individually: an invalid third=20 must not
alias the next second-dimension entry. This specifies useful future semantics;
a first implementation can explicitly support only one dimension while keeping
the declaration model extensible. Reject unsupported dimensions at compilation.

## Lowering, buffers and AGNB

IDX-B01: For a table stored as consecutive one-record buffers, normalized index
selects `firstBufferId + index`. For a packed Array, normalized coordinates select
a byte offset. Keep the logical indexing rule independent of those two physical
layouts. The compiler owns bounds, stride and patch offsets.

IDX-B02: Borrow Rally's guard-then-patch-then-call pattern. The template must be
separate from the executing command buffer. Scratch/template state resets on
use; jobs remain serialized and nonreentrant. A Golem error must exit the calling
job rather than merely return from a nested helper and resume later statements.
Helper depth and failure propagation need explicit qualification when promoted.

IDX-B03: Use the current Golem byte-normalized comparison approach rather than
copying Rally's word guard unchanged for every domain. The stock conditional
implementation initializes read values to -1 and reads only one/two bytes; its
failure sentinel can collide with all-one data. Unsigned full-width endpoints,
signed comparisons and 0:65535 indices require deliberate decomposition.
A count of 65536 does not fit uint16 even though both endpoints 0 and 65535 do.
Inclusive comparisons or wider private counts avoid that truncation.

IDX-B04: Calculate total extent, strides, last offset and last byte with wide
compile-time arithmetic. Reject unsupported address/resource sizes; never wrap
or silently truncate them. Rally's packed source is limited to 65535 bytes and
short offsets; its fixture ends at offset 65534. Larger arrays require qualified
advanced-offset or partitioned storage. Their logical syntax must not promise
that this backend already exists. One-record-per-buffer tables are also limited
by available buffer IDs, irrespective of logical index width.

IDX-B05: AGNB's embedded IDs are authoritative physical identities, not logical
row numbers. AGNB records need not have consecutive IDs. A first contiguous-table
backend can require its build output to assign consecutive owned table IDs and
validate that manifest before installation. Arbitrary prebuilt container IDs
cannot use firstId+index unless that property has been checked; supporting them
needs a build-generated mapping or another explicit backend. This does not
require a runtime filename lookup or silent ID remapping.

IDX-B06: The assembly application sends the changing index/parameters and an
entry call;
address calculation and data copy run inside VDP. Retain the storage review's
19-byte illustrative uint16-update/call calculation, excluding other inputs and
readback. The current maintained float-update/call contract stays 21 bytes.
Selecting an eight-byte row can use one resident copy instead of separate host
requests for each field; local command cost is not recurring wire traffic.

Firmware references: [conditional evaluation](/Agon/agon-vdp/video/vdu_buffered.h:1056),
[adjust offsets](/Agon/agon-vdp/video/vdu_buffered.h:842),
[maintained comparison precautions](/Agon/mystuff/golem/docs/design/resident-scalar.md:176).
See also [AGNB build accounting](agnb-loading-review.md).

## Compiler-to-assembly output

The PC compiler publishes assigned IDs, offsets, element/row layout, dimensions,
entry addresses and owned cleanup resources. The Author favors direct assembly
output: symbolic constants and labeled command/data byte definitions with
lengths, so the application can include them directly. Exact assembly spelling
and output mode still require implementation review. Current interface.json and
binary streams remain the implemented artifacts.

The application selects its loader (AGNB where supported), installs the data,
updates inputs and invokes fetch through the generated interface. Golem's fetch
only reads installed VDP buffers. Stock VDP cannot request MOS file loading.
An interactive on-hardware Golem application is a separate possible future
frontend. See the [maintained contract](/Agon/mystuff/golem/docs/design/resident-scalar.md#assembly-application-integration--direction-agreed-2026-10-12).

## Validation and review boundary

Rally's existing element-access regression was rebuilt with warnings as errors
and ASan/UBSan in this task's ignored build directory. It checks emitted source
offset relocation, typed layouts and alias rejection alongside several other
legacy helpers. [Index audit](index-audit.json) records exact source hashes,
command, stdout and duration. The earlier [import audit](import-audit.json)
records table declaration/guard/ownership regression; it was not rerun here.
The reference checkout remains unchanged. No emulator or hardware run occurred.

Future implementation evidence must cover first/last/one-past indices, 65535,
negative/nonzero lower bounds, normalization/ID carry, offset carry, per-dimension
bounds, failed-expression destination preservation, repeated failure/recovery,
relocation and source/destination aliasing. These are acceptance considerations
for this proposal, not passing tests of new Golem functionality.

IDX-001 is complete and ready for review. CPY-001 is next after approval;
compiler implementation and SYN-001 example changes have not begun.

Current implementation checkpoint: [GOLEM-016](../GOLEM-016/README.md) now provides
optional generated assembly output and named eZ80 wrappers for the existing
scalar resident profile. Table/Array/Record support and their broader assembly
interfaces remain future work. Earlier output recommendations above record the
review direction that led to this tranche.
