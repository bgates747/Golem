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
2. Load the original XYZ source as exactly count*6 bytes in **one consolidated
   block**. Source and destination must have distinct IDs. The destination may
   be the resident drawing input, whose entire layout must be precisely the same
   packed signed16 XYZ sequence, without prefixes or unrelated fields.
3. Install bootstrap once. It enables the affine test flag, initializes float32
   controls and identity matrices, and installs resident procedure/templates.
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
2. `Matrix(4,4) model = Identity`: a compiler-owned, initially identity 4x4
   binary32 matrix. No other matrix size or initializer is implemented here.
3. `Borrowed XYZ16 vertices = Buffer(1000,8)`: caller-owned packed buffer at
   offset zero with eight signed16 XYZ triples, little endian. Count must be
   1..10922; IDs must be 1..65534 and covered by --reserve. Borrowed aliases
   reject. A declaration is a build contract, not runtime byte validation.
4. `matrix_identity(model)` resets the named matrix.
5. `matrix_rotate(model, x, y, z)` composes stock command33 operation2 with
   **decimal degrees**. It is the stock combined XYZ Euler operation, not a
   promise of arbitrary rotation-order equivalence. For a different order,
   issue single-axis operations in the desired explicit sequence. Negative
   camera angles should be supplied as explicit controls/literals; expressions
   and inverse operations are absent from this bounded profile.
6. `matrix_scale(model, x, y, z)` and `matrix_translate(model, x, y, z)` compose
   stock operations5/6. Arguments are controls or numeric literals, exactly
   three per operation. Stock command33 **pre-multiplies**: newOp*existing.
   Translation is in raw coordinate units, not graphics-scaled units.
7. `matrix_multiply(dest,left,right)` writes left*right using command34
   operation6, with rows/columns4. Destination may equal an input matrix;
   firmware consumes the input values before replacing destination storage.
8. `transform_vertices(dest,matrix,source)` uses command41 options8, format0xC0,
   and the compile-time count. This transforms signed16 XYZ without a perspective
   divide or host readback. Counts must agree and source/destination must differ.
   The explicit limit bounds transformed records but does **not** truncate or
   validate the copied source shape: the exact source-size/single-block contract
   remains the host's responsibility.

For affine arguments, global controls are copied into resident command-template
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
operations per procedure. Each resident command block must fit65535 bytes.
These are admission bounds, not a guarantee of free VDP RAM or frame time.

The manifest publishes controls, matrices, borrowed XYZ buffers, finite entries
and owned IDs. Assembly symbols use existing `golem_NAME_buffer/offset/bytes/type`
conventions: type3=float32,6=4x4 matrix,7=packed XYZ16. Named wrappers submit
six-byte buffer calls. Matrices are special stock typed buffers, so never
replace them with an ordinary raw64-byte upload and assume metadata survives.

## Qualification boundary

Host encoding/ownership/rejection tests are in `make -C src check`. Task-local
CLI tests also compile two real eZ80 assembler consumers. Fsim independently qualified the unchanged compiler on actual stock VDP2.16,
Fab and MOS3.0.2 ARM64: cube and supplied tank each pass1080frames, exact
projection/PLOT endpoint checks, controls/camera/reset and cleanup. See
[peer evidence](../tasks/GOLEM-045/peer-target-evidence.json). Human visual
review remains fsim-owned; compiler publication was explicitly authorized. No physical or performance acceptance
is inferred from these functional emulator checks. No native output, uploaded
Xtensa code or custom firmware fallback exists in this profile.
