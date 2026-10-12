# CPY-001 — Checked fields and whole-record copying

Recommend ordinary assignment for typed copies: `depth = raw` for a field/value
and `accepted = incoming` for a whole row. Matching types are required; values
must satisfy destination Bounds. Where runtime checking is needed, perform it
inside VDP before writing the destination. Any failure preserves the entire
assignment destination and ends the job under the existing error model. Whole
record validation must finish before any field commits. The Author accepted this proposal, with the hobby-project scope below.
This is not newly implemented compiler behavior.

## Accepted scope and complexity budget

The Author accepted the proposal while emphasizing that Golem is a hobby project,
not mission-critical accounting software. Keep implementation simple and the
checks useful. Checked copy means validating source values against destination
bounds before writing, not checking whether transmitted/copied bytes match.
Do not add copy readback, checksums, redundant byte comparisons, rollback systems
or host acknowledgements to this feature.

Validate changing external inputs at the appropriate job boundary, then reuse
that established validity for ordinary internal copies while it remains true.
A subsequent write or operation that can violate the relevant bounds needs its
own check. Trusted immutable resident data and proven-compatible values should
not receive repeated admission checks merely because they are copied. Keep this
local and straightforward; no elaborate optimization framework is required.
Preserve the useful destination-on-failure behavior already proposed.

## What Rally implements

| ID | Behavior | Source |
| --- | --- | --- |
| CPY-F01 | `Copy(destination,source,bytes)` is a typed scalar copy despite its byte-count argument. Source/destination widths and types must match, source declared/inferred bounds must fit destination bounds, and overlapping storage is rejected. | [Copy](/Agon/mystuff/golem-rally19/src/hosted.hpp:1062) |
| CPY-F02 | `CopyChecked(destination,source,status)` accepts matching u8/u16/s16/s32/q8s16/q8s32 fields; f32 and cross-type conversion are rejected. It resets status to 1, checks the actual source against destination bounds, then conditionally copies the complete scalar. Failure clears status, preserves the destination and continues the caller. | [Checked copy](/Agon/mystuff/golem-rally19/src/hosted.hpp:858) |
| CPY-F03 | Runtime integer/fixed range checks compare zero-extended ordered bytes. Signed ordering flips the high sign bit. Fixed bounds become representable raw limits with ceil(low×256) and floor(high×256); an empty interval is rejected. Status is sticky during the range check. | [Range validation](/Agon/mystuff/golem-rally19/src/hosted.hpp:600) |
| CPY-F04 | `CopyRecord(destination,source)` requires distinct equally sized buffers, complete gap-free layouts, matching field offsets/types and source declared bounds contained by destination bounds. It then emits one byte copy. It does not implement runtime checked whole-row narrowing. | [Record copy](/Agon/mystuff/golem-rally19/src/hosted.hpp:886), [layout coverage](/Agon/mystuff/golem-rally19/src/hosted.hpp:640) |
| CPY-F05 | Writes to immutable Asset fields and unqualified raw matrix writes are rejected. Status must be separate writable 0:1 uint16 storage. Partial overlap and exact self-copy are rejected by these existing intrinsics. | [Write restrictions](/Agon/mystuff/golem-rally19/src/hosted.hpp:287), [checked-copy validation](/Agon/mystuff/golem-rally19/src/hosted.hpp:858) |

The rebuilt [checked-copy regression](/Agon/mystuff/golem-rally19/tests/test_checked_copy.cpp)
covers guarded scalar narrowing, fixed-format acceptance and alias/type/status
rejections. It does not establish runtime whole-record validation or qualified
floating-point checked copying. Its evidence is compiler/host evidence, not a
new stock-VDP execution test.

## Proposed Golem rules

CPY-R01: Use `destination = source`. Types must match, including fixed-point
fractional precision. Destination Bounds may be narrower than the source's.
Reject a known-invalid constant during compilation; check uncertain values
inside VDP before assignment. Remove manual byte counts and success-flag
boilerplate from the ordinary language surface.

CPY-R02: Keep numeric conversion explicit. A sint16-to-uint16 copy or
fixed16(8)-to-fixed16(4) assignment requires an explicit conversion with its own
representability/precision rules. Copying does not truncate, clamp, reinterpret
bits or silently change signedness. The already accepted conversion rules remain
in force; this review does not implement additional conversions.

CPY-R03: On a runtime destination-bound violation, preserve the complete old
destination, skip later statements and exit to normal VDP processing. Publish
an `assignment_out_of_bounds` status; choose its numeric code during
implementation without changing existing codes 1:4. Reset status each call.
Earlier writes remain. No host acknowledgement or extra wire traffic is required.
This extends the current arithmetic/index-error direction to typed copying.

CPY-R04: Require identical declared row types for initial whole-record assignment.
Matching byte widths alone are insufficient. Validate all fields that need runtime
admission before committing any part of the destination. Do not implement a row
copy by a sequence of independently committing checked field assignments: a late
failure must preserve the earlier destination fields too. Cross-record schema
mapping and automatic projection are deferred.

CPY-R05: A field assignment changes only that field. A whole-row assignment has
value semantics: subsequent source changes do not change the copied destination.
This agrees with the accepted ByVal direction for composites. It does not require
user-visible references or a heap. Owned Array/Table deep copying and resizing
are separate implementation questions; matching element counts alone must not
implicitly establish compatible dimensions or logical bounds.

CPY-R06: Retain runtime admission for externally updateable data until actual
validation has been established for that invocation. An assembly program can
write bytes that violate a declared type's Bounds; declarations alone do not
prove those bytes safe. The compiler interface must identify which inputs need
validation and the generated job checks must run before use. Known validated
immutable build data can use a plain resident copy. Do not depend on broad
whole-program optimization to make this practical.

CPY-R07: Ordinary self-assignment may be a no-op only when the value is known
valid; otherwise admission checks still apply. Reject unsupported partial
storage overlap initially rather than quietly relying on memcpy-like behavior.
For expressions, evaluate and retain the source result before destination commit,
including self-references. For indexed writes, propose selecting/checking the
left-hand indices once, left to right, then evaluating the right-hand side once;
any address, expression or admission failure preserves the selected destination.
This evaluation order is a proposal for review, not a parser guarantee.

CPY-R08: Read-only imported Table/Asset payloads remain unwritable. Mutable
working rows and arrays have explicit initialized storage. For indexed stores,
check all coordinates and address arithmetic before any write, as in IDX-001.
Reject writes to immutable data at compilation. Field selection uses declared
layout, not user-supplied offsets.

CPY-R09: Initial checked-copy implementation should start with the unsigned
integer slice already maintained. Signed/fixed checks can reuse Rally's ordered
byte approach with new qualification. f32 runtime admission needs finite/NaN and
inclusive-bound rules and its own proven lowering; Rally CopyChecked does not
supply that backend. Unsupported types must fail compilation rather than receive
unchecked semantics. Their language design can remain broader than the first
implemented slice.

## Candidate examples

These examples are proposals. Bounds, Record construction and field access here
are not new maintained parser support. All values have explicit types and valid
initialization. The assembly application installs compiler-emitted initial data,
updates any host inputs and invokes the generated entry; these jobs load no file.

### Scalar admission

```golem
uint16 raw = 1200
uint16 depth Bounds(64, 1100) = 64
Sub acceptDepth
    depth = raw
    print "accepted"
EndSub
```

The copy fails: depth stays 64 and the print is skipped. With raw=1100 it succeeds.
No bool/status variable is required in application source. Dynamic values supplied
by assembly need the same validation inside the resident job.

### Whole-record admission

```golem
Record Sample
    uint16 tag
    uint16 depth Bounds(64, 1100)
EndRecord

Sample incoming = Sample(7, 100)
Sample accepted = Sample(0, 64)
uint16 rawDepth = 100
Sub acceptSample
    accepted = incoming
    print "accepted"
EndSub
Sub acceptDepthField
    accepted.depth = rawDepth
EndSub
```

acceptDepthField changes only depth after checking rawDepth; tag stays unchanged.
Initially acceptSample copies both fields. If assembly later supplies incoming=(8,1200),
validation fails and accepted retains its entire previous row, including tag=7.
The initializers are valid; the invalid case is a deliberate external input to
be admitted at runtime. Checking/copying tag first and failing later on depth
would violate the proposed whole-destination preservation rule.

## Implementation constraints and wire traffic

CPY-B01: Reuse Rally's ordered-byte scalar range tests, but adapt failure to the
finite Golem job exit. A nested helper returning after error must not allow the
calling job to continue. Separate validation from commit, and qualify helper
depth under the current stock runtime.

CPY-B02: For separate stable record buffers, validate every required field first,
then copy once. Private temporary storage is needed when expression evaluation
or supported aliasing requires a source snapshot; whole-record scratch is not
mandatory for every copy. Enforce bounded storage and command-size budgets. If a
promised operation cannot fit, reject it explicitly instead of weakening its
preservation guarantee.

CPY-B03: Preservation refers to compiler-defined index/expression/admission
failures with installed storage intact. Stock buffered commands have no general
copy-success acknowledgement. Missing buffers, external mutation, allocation
failure or transport corruption do not become detectable transactional errors
merely because the source says checked assignment. Assembly must respect the
published ownership and serialized invocation contract.

CPY-B04: Assembly updates input once and calls an entry; the job performs checks
and copies locally. Per-field host readback and repeated copying over the wire
are unnecessary. Validation adds resident commands/installation bytes, not an
extra host round trip. Existing 21-byte float-update/call traffic remains the
maintained profile's contract; a future record-input packet needs its own byte
accounting. Generated assembly constants publish IDs/offsets and payload lengths;
assembly remains responsible for actual installation, updates and calls.

## Evidence and review boundary

[Copy audit](copy-audit.json) records source identities, compiler command,
regression stdout and local monotonic duration. The existing Rally checked-copy
suite was rebuilt with warnings as errors and ASan/UBSan under this task's ignored
build directory. The reference checkout remains clean. No Golem compiler changes,
shared-example changes, emulator runs or new runtime qualification occurred.

Future implementation must qualify exact limits, one-below/one-above bounds,
all-one values, signed/fixed boundaries, complete destination preservation on a
late row-field failure, immutable writes, repeated failure/recovery and aliases.
Float/NaN admission requires separate evidence before support is advertised.
These are acceptance considerations, not claims of already passing new features.

CPY-001 was accepted with the simplicity guidance above. SYN-001 consolidates the reviewed
storage/access/copy examples in examples.golem, retaining assembly-owned loading
and keeping build filename mappings outside resident source.
