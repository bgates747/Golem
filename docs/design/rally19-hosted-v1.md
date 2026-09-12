# RALLY-19 hosted Golem design v1

This is the implementation contract for the existing **C++ golemc**, frozen at
R19-03 before its renderer intrinsics are implemented. The shared execution
register is `../AgonArcade-rally19/TODO.md`. This document specifies compiler
behavior; Rally's packet/track/render contract lives in its RALLY-19/ABI.md.
The old broad language proposals remain historical design context, not a mandate
to implement strings, a standard library, exceptions or a different language.

## Compilation and execution model

1. Preserve ordinary `golemc input output` and its current print, uint16,
   assignment/addition and For/Next bytecode. New functionality is opt-in through
   `golemc --hosted input output`; it extends the same compiler and diagnostics,
   not an external compiler or an opaque hand-authored binary generator.
2. Hosted output is still raw stock VDU bytes, suitable for buffer 0 and one
   bootstrap call. It initializes declared storage/constants and installs named
   finite programs. It does not run render implicitly. An optional text symbol
   map records buffer IDs, sizes, fields, entrypoints, generated scratch,
   resources and field relocations for inspection and loader verification.
3. All resident declarations are top-level and initialized once. Bootstrap clears
   each owned ID before writing it (command 0 otherwise appends). Every render
   entry returns to the stock VDP's outer loop; no autonomous infinite loop.
   Program calls form a DAG. The only cycles are compiler-generated bounded
   repeat jobs with private counters and a static trip cap. User recursion and
   jumps are rejected. All program bodies end with `};`.
4. A hosted program may use existing print statements and expression machinery,
   but declarations/resources are hoisted and stable. Legacy For/Next remains
   available in the ordinary mode; hosted use must have a proven finite bound
   and an unmodified loop counter, or is rejected in favor of bounded Repeat.
   In particular uint16 wrapping at 65535 must not create an endless job.

## Declarations and types

Keywords/intrinsics are case-insensitive; user symbols remain case-sensitive.
All statements require a semicolon. Hosted-only declarations are:

```text
Buffer name(id, bytes);
Field name(buffer, byteOffset, type[, minimum, maximum]);
Matrix name(id, rows, columns);
Program name(id) { statements };
Asset name(id, "relative/path", expectedBytes);
```

IDs, dimensions, offsets and sizes above are compile-time integer literals.
Explicit IDs make the loader/artwork boundary reviewable. Assets are relative
to the source file, hash-identified in the map, read as binary data and emitted
as upload data, never treated as source or commands to execute. Table declarations
add fixed record width/count and index bounds to Asset metadata at R19-05; an
asset is not callable. A program is not usable as arithmetic storage.

Field types: `u8`, `u16`, `s16`, `s32`, `q8s16`, `q8s32`, `f32`.
Signed integers are two's-complement; matrix data is IEEE binary32 row-major.
Optional inclusive field bounds narrow the storage type's range. Entry admission
must validate imported values against these bounds before a kernel uses them;
they are not an unchecked cast. Literal/field intervals propagate through matrix
arithmetic. Signed stores/inversion require a bounded interval proof or an
explicit validated finite-entry precondition recorded in the symbol map. Do not
silently lower an unrestricted float into an unchecked integer cast. R19-04
probes supply bounded inputs; R19-05 qualifies production admission on VDP.
Fixed-point float-read formats are s16=0xC0, s32=0x40, q8s16=0xC8,
q8s32=0x48; f32=0. Generic u16 values above 32767 are **not** silently treated as
signed matrix arguments: widen into a positive s32 field explicitly. Buffer
fields may expose an initialized matrix's f32 storage, but arithmetic writes
to a matrix must preserve its metadata and renderer-cache rules below.

Matrix dimensions are 1..8 for this goal, with strict algebraic shape checks.
The firmware's permissive square padding is not the Golem type rule. A scalar
is a 1x1 matrix. Large sets of road points use command 41's strided data, not
one enormous matrix whose stock multiplication would pad to a huge square.
Numeric literals in intrinsic argument lists accept signed decimal integers
and finite decimal floats. Existing uint16 expression grammar does not change.

## Required intrinsic surface and lowering

The following is the finite surface for this renderer. Buffer/field/matrix
operands are symbolic references, never magic integer IDs in application code.
Literal arguments can be interned as typed constant fields during bootstrap.
Generated internal instruction buffers/fields appear in the symbol map.

| Intrinsic | Meaning and stock lowering |
| --- | --- |
| `Bytes(buffer, byte, ...);` | Initialize an exactly sized data/VDU template; 0..255 literals. Bootstrap clear/write, not arbitrary emitted runtime opcodes |
| `Copy(dstField, srcField, bytes);` | Checked byte-span copy, command 5 op 0xE2; no partial overlap |
| `Set(field, literal);` | Typed representable literal store, command 5 op 0xC2 |
| `AddU16(field, operand);` | Modular integer add using a three-byte private carry destination; no adjacent-field corruption |
| `MatValues(m, value, ...);` | Exact row*column finite f32 literals, command 34 op 0 |
| `MatLoad(m, firstField);` | Consecutive same-format elements, checked source span, command 34 op 0x20 |
| `MatPut(dst, src, row, col, value);` | Copy matrix and replace one element, command 34 op 1 with typed field operand |
| `MatAdd(dst,a,b);`, `MatSub(dst,a,b);` | Strict same shape, command 34 ops 4/5 |
| `MatMul(dst,a,b);` | Strict matching inner dimensions/result shape, command 34 op 6; 1x1 gives scalar product |
| `MatScale(dst,src,value);` | Command 34 op 7 with typed scalar argument |
| `MatExtract(dst,src,row,col);` | Checked contained rectangular selection, command 34 op 8 |
| `Identity(m);` | 3x3/4x4 affine identity, command 32/33 op 0 |
| `RotateDegrees(m,angle);` | 3x3 affine rotation, command 32 op 2; Golem name explicitly says degrees. Game 256-circle angles require scale 360/256 |
| `Translate(m,x,y);`, `Scale(m,x,y);` | 3x3 affine composition, command 32 ops 6/5; fields may use separate formats via 0x60 modifier |
| `InvertAffine(m);` | Command 32/33 op 1, only a proved nonsingular affine matrix; no command-34 inversion exists |
| `TransformData(dst,src,m,fieldType,offset,stride,count,components);` | Command 41, explicit positive finite count/stride/span. Same source/output format. Does not divide by homogeneous W |
| `StoreI16Floor(dstField, scalar);` | Checked/bounded scalar output to signed integer using positive bias and word unbias, as described below |
| `PlotPoint(mode,xField,yField);` | Two signed16 field relocations into an owned six-byte PLOT template then finite call; opcode is compile-time |
| `Call(program);` | Command 1, checked acyclic edge |
| `CallIf(program,left,comparison,right);` | Command 6, both u16 buffer-fetched operands; comparisons eq/ne/lt/le/gt/ge, domain 0..65534 |
| `Repeat(count) { statements };` | Finite count 0..512 with private protected counter; zero does nothing; count is compile-time for this goal |
| `LoadIndexed(dst,table,indexField);` | R19-05: checked resident fixed-size record copy; patch source-ID/offset in a separate finite instruction template before calling it |

These are named stock-command intrinsics, not an expression optimizer pretending
to execute general C on VDP. R19-04 implements/tests the arithmetic subset first;
R19-05 implements assets/indexing/admission/lifecycle. Syntax/type-negative cases
are listed in tests/hosted-design-cases.json; their implementation is not claimed
by a design-only pass. Ordinary-language regressions remain executable throughout.

## Numerical and stock-firmware hazards

1. Command 34 does support matrix multiply/add/subtract/scalar multiply. Product
   and multiply-add are real native operations. Reciprocal construction is
   `diag(d,1,1)` plus affine inversion, then extract [0,0]. Require finite
   positive d in a declared range; Rally visible depth is 64..1100 world units.
   Invalid/zero/near-zero depth branches away before inversion. Reusing a resident
   reciprocal table is permitted for the final renderer if measured cheaper.
2. Command 41 converts **both** source and destination using one format; an
   identity transform is not f32-to-s16 conversion. To store a scalar as an
   integer, put it in a translation matrix applied to an integer zero template.
   Stock convertFloatToValue casts through unsigned types. Do not rely on
   negative float-to-unsigned behavior, which is not portable C++ semantics.
   For `StoreI16Floor`, establish -32768 <= value <= 32767, add 32768 in float,
   transform a zero s16 pair, then XOR 0x8000 from the resulting word. Conversion
   therefore sees only [0,65536); output is floor(value) at representable float
   precision, not C truncation toward zero. Account for rounding near integers
   in numerical probes and the one-pixel geometry budget. Clip or branch away
   before conversion; never wrap an out-of-range coordinate onto the screen.
3. Stock buffer conditionals initialize words with -1 before short reads. Two
   real buffer operands avoid asymmetric high-byte contamination, but **0xFFFF
   still collides with the firmware's missing-value sentinel**. Hosted u16
   conditional operands exclude it. Sequence numbers use modulo 65535. Do not
   test a general 32-bit position by blindly comparing its low word; it can
   validly be 65535. Use bounded high-word guards or normalized scratch values.
   No unsigned field silently becomes a signed-order comparison.
4. Command 34 replacement reads inputs before replacing its target; arithmetic
   destination/source aliases may be enabled only for that qualified operation.
   Initially require separate buffers. Command 41 source and destination are
   distinct. Mutable instruction templates cannot target themselves or a
   currently executing program. No self-modifying live instruction stream.
5. Bitmap transforms cache inverse matrices. A raw byte patch must not leave a
   stale inverse or alias a matrix still used by queued drawing. Give each car
   its own draw matrix; regenerate/invalidate with the stock affine API before
   use. Never overwrite the source artwork to obtain a transform. Keep scratch
   scalar/math matrices separate from bitmap-bound matrices.
6. Hosted kernels can allocate bounded replacement buffers internally because
   that is how stock commands work; live buffer IDs/count and retained bytes
   must remain bounded. No appending every frame, no leaked matrix users. Clear
   owned buffers only, then reconstruct on mode reset/reload. Maximum call depth
   16, compiler-generated repeat trip cap 512, matrix dim cap 8, data/command
   span checked before emitting. Bounds failures stop compilation, or take a
   finite rejected-frame path for runtime values.

## IDs, resources and errors

Rally ownership map: 0 bootstrap, 1..999 compiler-private legacy/temporaries,
1000..1099 state/status/constants, 1100..1499 arithmetic matrices,
1500..1999 mutable command/data scratch, 2000..2999 finite programs,
4000..4999 track records. IDs 30000..30009 and 63800..64100 are existing Rally
road/artwork resources and unavailable for automatic allocation. Exact extern
bitmap/matrix IDs come from Rally's manifest. IDs 65535 and all undeclared IDs
are forbidden. Reject collisions including forward declarations; hosted allocator
must stop rather than spill into an explicit/extern namespace. Scalar/field
references never allocate silently per render call.

Preliminary ceilings, to be replaced by measured totals: <=1024 owned buffer IDs,
<=512 KiB resident projection/track data, <=128 KiB programs, <=64 KiB scratch,
<=1 MiB total additional Golem storage excluding existing artwork/framebuffers.
These are safety/design budgets, not measured stock performance claims. Emit
startup byte count and resource ledger; fail construction if a declared bound
would exceed it. No generic matrix-multiply cost is assumed cheap before R19-04.

Errors report source line and symbol: duplicate/colliding ID, unknown/forward
symbol not resolved, invalid dimensions/shape, span overflow, invalid field
format, NaN/infinity, out-of-range literal, type mismatch, illegal alias,
unbounded/cyclic program, illegal loop mutation, malformed asset/size, and
exceeded resource limit. Resolve all symbols/relocations before writing output;
failed compilation must not publish a partly generated blob.

## R19-06 additions

`DivModPositive(q,r,input,divisor)` accepts a bounded nonnegative s32 domain
below2^24, constant divisor1..16777215, a u16 quotient<=32767 and a u16 or s32
remainder destination containing0..divisor-1. The wider divisor/s32 remainder
form was added for R19-08; the original u16 form emits unchanged code. All
operand/destination spans must be distinct. The compiler checks every integer in the declared domain
against binary32 reciprocal multiplication before emitting that construction.
It rejects domains that round an integer index upwards; it never emits an answer
table. Runtime computes q, reloads it and obtains r=x-q*divisor exactly.
Fetched scalar matrix scaling requires operation0x27, including its fetch bit.

`MatTrunc(dst,src)` and `MatRoundAway(dst,src)` produce scalar integer-valued
matrices using positive-magnitude stock conversion and explicit sign restoration.
See examples/native_quantization for bounds, half-boundary qualification and
native evidence. They preserve signed zero and require distinct scalar matrices.

`LoadElement` now also accepts a read-only Asset with a complete repeated typed
record layout. `WidenUnsigned` also permits u8 to a containing u16 field.
`PlotQuad(colour,x1,y1,x2,y2,x3,y3,x4,y4)` accepts a constant stock colour0..63
and eight s16 coordinates, patches a private GCOL/PLOT block and issues the
accepted two-triangle sequence4,4,85,85. It performs no hidden geometry.

The stock native polygon rasterizer excludes a flat top edge and the right edge,
and includes the bottom edge. This differs from inclusive filled rectangles.
R19-07's rectangular road/footer clears therefore use right extent320 and tops
103/223 to cover screen column319 and rows104/224. The intrinsic preserves its
input coordinates; it does not silently offset arbitrary polygons. This was
confirmed against pinned fillPath source and native edge/complete-row images.

Literal range proofs use the actual rounded binary32 representation, including
underflow to zero; narrowed float stores and nonsingularity cannot be justified
by an unrepresentable double literal. Field metadata no longer duplicates an
entire imported asset's data vector per field. Host qualification records source
hashes before and after tests and rejects a run if its source changed mid-test.

## R19-08 vehicle arithmetic additions

`StorePositive32Floor(destination,scalar)` writes a4-byte s32 field from a finite
scalar proven within0..16777215. The destination must contain the floored bounds.
It reuses qualified stock command41 format0x40 on a nonnegative translated zero
tuple, then copies exactly4 bytes. It is not a signed negative conversion and
does not broaden TransformData's fixed-output support.

`MatAbs(destination,source)` requires distinct scalar matrices, an initialized
source and finite source bounds. It loads the source into the destination, clears its IEEE
sign bit and propagates the absolute interval. The source is unchanged; either
signed zero produces positive zero.

`LessThan(result,left,right)` requires a writable u16 result with bounds0..1 and
matching fixed input types (u8/u16/s16/s32/q8s16/q8s32). The result must not overlap
either input; inputs may alias each other. It writes a fresh Boolean each call,
compares ordered zero-extended bytes from high to low, and flips the high sign
bit for signed types. This handles full ranges, including65535 and signed32
extremes, without passing0xffff to a conditional operand. CallIf's existing
bounded-u16 restriction is unchanged.

Native proof is in examples/native_vehicle_math and Rally's
evidence/golem-vehicle-math:576 complete96-byte readbacks cover signed boundary
pairs, wide real lap division, signed zeros/subnormals, positive floor boundaries
and sentinel neighbours. Host sanitizer/golden/negative checks also pass. The
R19-07 complete-road bytecode remains identical on both tracks. These additions
support, but do not complete, the R19-08 vehicle renderer.

## Checked clipping and shared scalar word temporaries

`CopyChecked(destination,source,status)` copies one matching fixed-type field
only when its runtime value fits the destination's declared range. It first
resets a writable Boolean u16 status to1; rejection sets it to0 and preserves
the entire old destination. Source bounds remain broad. Destination/source spans
must not overlap, and status must have a separate buffer ID from either. Float
fields are rejected. The destination must be initialized to an in-range value
before a rejected copy can leave it in use. CallIf on the returned status can
then guard reciprocal evaluation without pretending the untrusted source had
narrower bounds. The existing unchecked Copy continues to reject narrowing.

The native depth probe covers1110 inputs, including every integer0..1101 and
full-u16 extrema. All1037 visible depths64..1100 produce exact floor(8000/z);
invalid depths retain the old selected depth/result and do not increment the
projection count. Complete96-byte records also check neighbours. See
`examples/native_vehicle_math/clip.golem` and Rally evidence/golem-vehicle-clip.

`SharedWordScratch();` is an optional, argument-free first top-level declaration.
It makes all StoreI16Floor/StoreU16Floor expansions (including DivModPositive's
internal word stores) reuse a private36-byte transform and two private4-byte
tuple buffers. Default compilation is unchanged. Each expansion resets the
transform, computes/converts, and copies its output before returning; there is
no user Call or yield inside the expansion. Nested user calls and repeats are
therefore safe. The zero source is private and never modified. Positive32 stores
and MatTrunc temporaries are not shared by this directive.

This opt-in addressed a measured resource failure: full road+numeric vehicles
used1022 owned IDs on the oval and exceeded the frozen1024 cap on Fuji. Sharing
reduces these to848/867 without raising any ceiling. The native shared.golem
probe executes165 signed/unsigned input pairs in each of isolated/shared modes,
with three repeats and nested calls per input. All330 complete output records
match the independent floor oracle and each other. Host tests check resource
savings, default behavior, invalid placement/arity, aliases and checked-copy
rejection. These resource and numeric proofs do not establish full-scene timing.

## Primary source qualification

Read locally on 2026-09-12: agon-docs Buffered-Commands-API.md (commands 0,5,6,
32,33,34,41), and the **actual pinned runtime** under
AgonJukebox/.emulator/runtime/fab-1.2.4/src/vdp/vdp-console8/video/:
vdu_buffered.h, vdu_stream_processor.h, types.h. Firmware is authoritative where
the docs differ: matrix diagonal takes one argument per diagonal cell in this
runtime, despite the older prose table saying a single value. Therefore this
design avoids that ambiguous intrinsic and sets explicit matrices. No upstream
files or firmware modules were edited. R19-04 must prove behavior natively;
reading source and golden bytes alone are not a runtime pass.
