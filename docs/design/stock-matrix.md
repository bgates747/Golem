# Stock matrix compiler profile

`golemc --stock-matrix` compiles finite matrix/vertex procedures directly to
stock Buffered Commands API commands. It lets an assembly or C++ host send
only controls, then invoke named Golem entries before the separate drawing
package. All runtime matrix and vertex arithmetic stays in stock VDP.

## Compile and integrate

```bash
make -C src check
build/golemc --stock-matrix --reserve 1000:1001 \
  docs/tasks/GOLEM-045/example.golem build/stock-matrix
```

`--emit-asm` is optional: `golem.inc` is always produced. Flags precede source
and directory. Other targets, scalar example arguments and data mappings reject.
Use a fresh output directory; an old directory is not cleaned of stale files.
The CLI produces bootstrap.bin, cleanup.bin, invoke.bin, update-call.bin,
interface.json and golem.inc after complete semantic/symbol validation. Files
are ordinary writes, not a transactional publication protocol.

1. Compile/install the drawing package first, then generate the matrix source
   with its actual packed input ID and vertex count. Reserve **all** other
   packages' owned IDs and application IDs, not merely the two vertex IDs.
   `--reserve FIRST:LAST` is inclusive; overlapping spans reject.
2. Load each borrowed buffer as exactly its declared record count/width in
   **one consolidated block**. XYZ16 uses6-byte triples, XYZF32 uses12-byte
   triples, XY16 uses4-byte pairs. Transform source and destination must have
   distinct IDs and matching record types/counts. A destination may be the
   resident drawing input only when its entire layout matches those records.
3. Install bootstrap once. It enables the affine test flag, initializes float32
   controls and declared matrix initializers, and installs resident procedures/templates.
   Borrowed buffers are neither initialized nor cleared by bootstrap/cleanup.
   Command41 itself replaces the complete destination, so drawing input must
   never also hold its executable code or command templates.
4. Read input buffer ID and each control's byte offset from interface.json or
   golem.inc. Send finite binary32 values using stock buffer writes while no
   procedure is executing. Initializer strings in source are build-time values.
5. Call `golem_setup` once and `golem_transformFrame` each frame in the example.
   The compiler exports every Sub name. `invoke.bin` calls the **first** Sub;
   update-call.bin restores all initial controls and calls that first Sub.
   Repeated frames should update chosen control bytes and invoke the selected
   entry, rather than replay update-call.bin accidentally.
6. Invoke the separately compiled draw entry after transformation. The returned
   assembly wrapper means bytes submitted, **not completion acknowledged**.
   Host owns stream-order/completion barriers, drawing fences and VSYNC swaps.
   A GP echo orders interpretation; it is not a Canvas drawing-completion fence.
7. After proven completion, cleanup clears only compiler-owned controls,
   matrices, procedures and argument templates. Borrowed XYZ buffers remain.
   The shared affine feature flag stays enabled; ownership does not justify
   disabling a global flag other packages may use.

## Source and semantics

Declarations precede Subs. The fully initialized
[example](../tasks/GOLEM-045/example.golem) supplies seven matrices, three
controls, two borrowed buffers and separate setup/frame entries.

1. `float32 angle = 0`: a packed control. Initializers must round to finite normal
   binary32 or zero. Subnormal/overflow/nonzero underflow literals reject.
2. `Matrix(4,4) model = Identity` and `Matrix(3,3) plane = Identity`: owned
   binary32 matrices. `matrix_values(...)` is also an initializer, with exactly
   16 or9 finite literal values in row-major order. Setup/installation creates
   real typed metadata and restores the declared initializer, including constants.
3. `Borrowed XYZ16 vertices = Buffer(1000,8)`: caller-owned packed buffer at
   offset zero with eight signed16 XYZ triples, little endian. XYZF32 and XY16
   use the same declaration form. Positive counts must fit65535bytes: maximum
   10922/5461/16383records respectively. IDs must be1..65534 and covered by
   --reserve. Borrowed aliases reject. Declarations are build contracts, not
   runtime byte validation; the host installs all initial bytes and retained Z.
4. `matrix_identity(model)` resets the named matrix.
5. `matrix_rotate(model, x, y, z)` composes stock command33 operation2 with
   **decimal degrees**. It is the stock combined XYZ Euler operation, not a
   promise of arbitrary rotation-order equivalence. For a different order,
   issue single-axis operations in the desired explicit sequence. Negative
   camera angles should be supplied as explicit controls/literals. For3x3,
   `matrix_rotate(plane,angle)` is the corresponding2D operation. General
   expressions remain absent; typed `read_float32` operands are described below.
6. `matrix_scale(model, x, y, z)` and `matrix_translate(model, x, y, z)` compose
   stock operations5/6. Arguments are float32 controls, numeric literals or
   typed buffer reads: three values for4x4 and two for3x3. Stock commands32/33
   **pre-multiply**: newOp*existing.
   Translation is in raw coordinate units, not graphics-scaled units.
7. `matrix_multiply(dest,left,right)` writes left*right using command34
   operation6, with matching3x3 or4x4 dimensions. Destination may equal an input matrix;
   firmware consumes the input values before replacing destination storage.
8. `transform_vertices(dest,matrix,source)` uses command41 options8 and the
   compile-time count. XYZ16 and XY16 use format0xC0; XYZF32 uses format0. The
   matrix must be4x4 for XYZ or3x3 for XY. Counts/types must agree and buffer IDs
   must differ. **Input and output formats are identical:** this operation does
   not narrow float records or perform a perspective divide. The explicit limit
   does not truncate/validate copied source shape; exact size/single-block
   storage remains the host's responsibility.
9. `matrix_set_value(dest,source,row,column,value)` emits command34/op1 with
   matching dimensions. Row/column are zero-based literal indices. It copies
   the source and changes one value; source may equal destination. A float
   literal, global control or typed read supplies the value. Buffer operands
   select op0x21/format0 with buffer ID/offset; literals use op1/format0.
10. `matrix_invert(plane)` emits command32/op1 and is limited to3x3. It is not
    a checked scalar division or an implemented4x4 inverse. The caller must
    supply a finite, nonsingular matrix and remain within the qualified domain.
11. `read_float32(buffer,byteOffset)` is an operand within affine operations
    or matrix_set_value. It reads a literal,4-byte-aligned, in-bounds field of
    XYZF32 storage, a matrix or a float control. Offsets are relative to that
    declaration; a packed control's actual offset is added by the compiler.
    The read is performed when the emitted stock operation/template executes.
12. `copy_bytes(dest,destOffset,source,sourceOffset,bytes)` emits command5/E2
    between declared borrowed buffers. Literal positive spans must fit both
    declarations. Overlapping self-copy rejects unless the two spans are
    identical; no memmove semantics are promised. This explicitly low-level
    operation neither converts types nor validates runtime values/shapes.
13. `chunkName()` calls a declared parameterless Sub, including a forward
    declaration, through a six-byte buffered call. Sub names remain case
    sensitive. Cycles, missing names and call chains deeper than four Subs
    (including the root) reject. This is a finite chunking mechanism, with no
    recursion, arguments, locals, new runtime loop or implicit scheduler.

14. `uint16 vertex = 0` declares an initialized loop counter in owned private
    storage. It is separate from packed float controls; it is not a new general
    stock integer arithmetic/assignment feature. `For vertex In Range(0:2684)` /
    `Next vertex` runs inclusively with positive unit step (optional`:1`). Next
    may omit its name. Bounds are uint16 literals; reversed/empty/runtime ranges,
    other steps, nested loops and Sub calls inside a body reject. Every entrance
    resets the counter/cursors and leaves the counter at the final visited value.
15. Inside that loop, byte offsets may be `stride*vertex[+bias]`, with positive
    literal stride and nonnegative literal bias, for read_float32 or either
    copy_bytes offset. Matrix indices/copy length stay literal. Whole visited
    spans and float alignment are proven at compile time using wide arithmetic;
    overflow, out-of-bounds, wrong types, dynamic self-copy or use outside the
    active loop rejects. General expressions such as vertex*stride are outside
    this initial subset. No runtime checked-array/status ABI is implied.

For affine arguments, controls and typed buffer fields are copied into resident command-template
argument slots using stock command5/0xE2, then the template executes. Literal
arguments already reside in that template. No per-vertex host work or per-frame
matrix upload is generated. Arithmetic/rounding, trig and signed16 quantization
are stock firmware behavior; this is not the checked portable arithmetic ABI.

Host inputs must remain finite, intermediate matrices must be representable,
and transformed coordinates must fit signed16. No runtime checks, status root,
rollback or reliable stock error acknowledgement is promised. This wrapper
profile must not be confused with `--portable` float arithmetic/native support.
It targets stock2.16 as independently investigated by fsim; earlier experimental
matrix APIs are not automatically qualified by source compatibility.

Statements have no terminators; newlines are whitespace. Keywords/builtins
ignore case, while references preserve identifier spelling. Declarations with
case-folded name collisions reject for assembly safety. Names are at most40
characters; there are at most256 controls,64 matrices,64 procedures and1024
static operations per procedure. A loop's body is counted once for that
source limit, and its positive iterationCount*bodyOperations must be<=65536.
No new entry-wide work limit rejects older loop-free programs. Each resident
command block must fit65535 bytes.
These are admission bounds, not a guarantee of free VDP RAM or frame time.

The manifest publishes controls, matrices, borrowed XYZ buffers, finite entries
and owned IDs. Assembly symbols use existing `golem_NAME_buffer/offset/bytes/type`
conventions: type3=float32,6=4x4 matrix,7=packed XYZ16,8=3x3 matrix,9=packed XYZF32,
10=packed XY16. Existing artifacts retain their prior bytes/type tags; new
load-plan/header fields publish the actual declared sizes. Named wrappers submit
six-byte buffer calls. Matrices are special stock typed buffers, so never
replace them with an ordinary raw64-byte upload and assume metadata survives.

## Stock perspective construction

The initialized [two-vertex example](../tasks/GOLEM-049/example.golem) shows the
entire source route and every required host-installed buffer. A shared4x4
matrix folds viewport scale/centering into float numerators. For each vertex,
four set-value operations build A from a fresh identity, with diagonal W and
translations -U,-V. Inversion yields U/W,V/W in its translation fields.
Transforming a single integer zero XY point selects those float translations;
command41 then performs the **final** integer quantization. Copy XY into the
retained drawing input; initialize/preserve any Z field separately.

The retained GOLEM-049 route expands finite chunks at build time:128vertices
use896operations per Sub;2685vertices fit21chunks and a parent. That historical
source-only example installs in290447bytes; expansion was an initial profile
limit, not a measured speed optimization.

GOLEM-050 adds an actual compact runtime loop. The initialized
[loop example](../tasks/GOLEM-050/example.golem) expresses the seven operations
once. The matrix compiler installs a reusable body, private counter/cursors and
an actual stock conditional backedge. The build generator supplies the literal
count and geometry, with no per-vertex source expansion or host frame updates.
The counter and every cursor reset on loop entrance, including repeated calls.
There is no per-iteration call-stack growth, new scheduler or general optimizer.

The independently compiled full tank/Corsair matrix sources both install in
1047bytes, with18owned resources and the same32float-control bytes. Each loop
body is324bytes; its bound/scratch/cursors occupy21bytes, plus a3-byte counter
record (2logical bytes and a carry guard). Drawing code and mesh/storage costs
remain separate. A smaller installation does not establish a faster frame;
matched consumer timing is still required. No consumer compiler fork or native
firmware feature is involved.

The initial consumer uses a unit-radius model, depth2..12 and strictly positive
logical endpoints. Its unchanged-stock2.16 probe shows positive final values
truncate toward zero, while negative values **clamp to zero**. Do not assume
signed truncation, clipping, singular-matrix handling, target-independent
float-to-integer behavior or physical performance. This profile adds no reliable
runtime error feedback, input readback, checksum, rollback or native fallback.

## Qualification boundary

Host encoding/ownership/rejection tests are in `make -C src check`. Task-local
CLI tests also compile two real eZ80 assembler consumers. Fsim independently qualified the unchanged compiler on actual stock VDP2.16,
Fab and MOS3.0.2 ARM64: cube and supplied tank each pass1080frames, exact
projection/PLOT endpoint checks, controls/camera/reset and cleanup. See
[peer evidence](../tasks/GOLEM-045/peer-target-evidence.json). Human visual
review remains fsim-owned; compiler publication was explicitly authorized. No physical or performance acceptance
is inferred from these functional emulator checks. No native output, uploaded
Xtensa code or custom firmware fallback exists in this profile.

GOLEM-049 adds the primitives above. Maintained host tests reproduce the peer's
exact93-byte actual-stock sequence, plus51 total matrix rejection cases;
ASan/UBSan, six legacy package cases/68unchanged files, two new actual assembler
consumers and full-sized source generation pass. This establishes compiler
encoding/contracts. Fsim owns independent execution of Golem-generated heavy
tank/Corsair applications and their human review. The peer's earlier command
probe is evidence for the target mechanism, not acceptance of this extension.
New compiler changes remain uncommitted pending review/publication.

Fsim subsequently executed the newly generated stock streams for heavy tank
and Corsair, six poses each:32,580XYcomponents within one logical unit of its
independent double perspective reference and successful raster comparisons
within its one-pixel boundary tolerance. The compiler hash matches the handoff;
see GOLEM-049/peer-target-evidence.json. This VDP-only result does not close the
full MOS upload/human gate. The consumer's Pi setup stalls during a large
805668-byte upload before projection runs; preserved Oryx runtime comparison
remains consumer-owned. No compiler or stock firmware repair is established.

## Compact loop implementation and qualification boundary

Counter declarations add a `counters` manifest array only for new programs:
name/type/buffer/offset, logical bytes2 and storage_bytes3. A `loops` array
publishes entry/counter/body/state IDs, bounds/iterations, source operations,
cursor count, body_bytes and state_bytes. Generated assembly/AGNB headers expose
logical counter fields while loader records retain their carry byte. Cleanup
includes all owned counter/body/state/argument records; borrowed data is retained.
No existing float-control layout or legacy artifact bytes change.

Stock's conditional reader partially fills an int32 initialized to-1, so a
16-bit0xffff value remains its error sentinel. Loop termination therefore uses
zero-extended high/low byte probes, with a conditional exit before advancing
and a conditional backedge afterward. No comparison operand can become0xffff.
Counter/cursor carry-output bytes are reserved; no update occurs after the
endpoint. Private mutable template offsets are patched before their operation.
This preserves the fully inclusive65535 endpoint without a firmware modification.

Host command execution covers2685copies and repeat resets, full65536iteration
termination, carries/continuation, static1024body/65536work limits and25rejections.
Maintained regressions, ASan/UBSan,68legacy files and real assembler/load-plan
checks pass. Unchanged stock2.16 compares every output byte with the retained
unrolled compiler for singleton,30/full2685records and subrange3:17, repeated
after poisoning private state. Retained Z, unvisited records, guards and borrowed
cleanup preservation pass; the real65535endpoint probe now returns correctly.
The first word-comparison endpoint attempt failed and is retained as superseded
evidence. The corrected run completes its numerical assertions and zero-fault
shutdown stats, but exits143 during shutdown; cause is unisolated, not a claimed
normal host exit. Independent full consumer/MOS/human/timing gates remain
wireframe-owned. No production readback, physical result or frame-speed claim.
