# DRAFT — Bring Rally capabilities into readable Golem

Reuse Rally's resident-program lowering, typed operations, resource maps and
cleanup support behind Golem's ordinary declarations, expressions and
keyword-delimited control flow. The primary benefit sought is less wire
traffic: install reusable commands/data once and invoke finite jobs with small
inputs. Rally already demonstrated substantial recurring UART savings. Its
physical rendering speed was poor, but performance optimization is outside this
proposal. Start with a small resident drawing example that proves the wire
benefit before expanding the numeric and asset surface. Review the three
direction decisions below before implementing anything.

Prepared 2026-10-11 for [GOLEM-001](../GOLEM-001.md). This is an investigation
deliverable, not an accepted language specification or implementation task list.
All new syntax in this document is proposed and does **not** compile today.

## Evidence and repository baseline

| Item | Examined identity / finding |
| --- | --- |
| Main Golem | `feature/standardized-deployment`, `0d25ac69be76d43c2f76323cd3173adc3b267eae` |
| Rally Golem | `rally19-vdp`, `4db7e96f70c3d63f4ab53766f3b32063e9357c8b` |
| Common ancestor | `a083173a084fadefa1fbe42926c6945a758b1757` |
| Divergence | Main has 1 subsequent commit; Rally has 25. Both are worktrees of the same Git repository. Main's committed change after the ancestor is network-access documentation. |
| Rally compiler implementation checkpoint | `f9e7d8d0d0d105c26cc2ee8c2f30a2267a905895`; later delivery/network commits do not add compiler semantics |
| Sibling Rally application | `/Agon/mystuff/AgonArcade-rally19`, `cea012005086cafc81654e0d35ba21fd2a4a153e` |
| Stock VDP reference | `/Agon/agon-vdp`, `c7ac293d2aa81ddfa693390549bcd909069c8fc3` |
| Local platform docs | `/Agon/agon-docs`, `f9806bd3cbff6ed5d1c08bef1d51fed11764b86b` |
| Work in progress preserved | Main's handoff/task edits and untracked `deploy.py` / `deploy.toml`; Rally Golem was clean |

The shared worktree relationship matters: implementation should preserve the
Rally branch as a reproducible reference and bring selected code into main
through ordinary reviewed commits. A wholesale branch merge would also bring
Rally-specific handoff, evidence, examples and ownership assumptions.

The main compiler is already 732 lines and implements `uint16` declarations,
assignment, addition and `For`/`Next`, as well as literal `print`. Main's README,
handoff and portions of `scratch.golem` understate current support. Current
source and the living language decisions take precedence. [M1] [M2]

Rally leaves that ordinary frontend effectively intact and adds a 1,872-line
`hosted.hpp`, with an opt-in CLI dispatch. The diff in `golemc.cpp` is confined
to that include and CLI/output handling. This is a substantial reusable
implementation, but it is not an existing typed three-address IR or a unified
language frontend. Its `Node` tree represents intrinsic invocations; semantic
checking and byte emission largely happen together in one compiler class. [R1]

The early "one instruction per block" addressing proposal is not what these
compilers currently emit: main places a sequence in a loop-body buffer, and
hosted mode places a finite program's command sequence in one block. Neither
implements the proposed general frame-relative local stack or heap model.
Reuse current tested operations without claiming those architectural proposals
have already been realized. The typed operation layer should leave addressing
and final block placement as explicit backend decisions.

### What was verified during this investigation

Inspected both compilers, current contracts, representative fixtures and test
sources, stock firmware routines and the final Rally delivery/review records.
A fresh **read-only hash audit** matched all 17 files named by the recorded
hardware-inline host-test report: both compiler files, 14 C++ test suites and
the Python runner. That report records all 14 suites passing, plus 3 accepted
and 12 rejected CLI design cases. [R2]

[evidence-audit.json](evidence-audit.json) records this audit and the examined
identities. The JSON design-case input is not among the historical report's
17 source hashes; its reported outcomes should not be mistaken for a fresh
run against a separately hash-bound case file. No compiler suite, native
emulator, hardware test, artifact rebuild or deployment was run here. Historical
native/hardware results below are attributed to their records; this task has
not independently repeated or re-audited all those experiments.

The cross-agent `rally_golem` README/message captures the initial handoff and
still describes implementation as awaiting authorization. It is historical
coordination context, not an up-to-date completion ledger. Later Golem devlog
and Rally DELIVERY/REVIEW records establish what was actually implemented and
tested. [C1] [R3] [A1]

## Capability comparison

Here, **implemented** means present in examined source; **host-tested** means
covered by the source-matched historical host report. **Native evidence** means
recorded execution of the stock VDP implementation as a host library, not
physical ESP32 qualification. Physical coverage is described separately below.

| Capability | Main Golem now | Rally implementation and evidence | Integration limits |
| --- | --- | --- | --- |
| Text, declarations, assignment | Literal `print`; zero-default `uint16`; literal/variable assignment | Ordinary mode preserved; `test_existing.cpp` freezes key bytes and rejects invalid syntax | `str`, computed text printing, general functions and other widths remain missing in ordinary mode |
| Integer arithmetic | Chained `uint16` `+`, modulo wrap, three-byte accumulator | Hosted `Set`, `Copy`, `AddU16`; private carry storage; host/native neighbour checks | Hosted addition requires a full-range u16 destination; this is not general signed/wide integer arithmetic |
| Fixed/signed/float storage | `uint16` only | Fields: u8/u16/s16/s32/q8s16/q8s32/f32; optional declared bounds; exact literal checks | Field support does not imply full expression/operator support for each type |
| Scalar floating arithmetic | Absent | 1x1 matrix load/add/subtract/multiply/scale; native arithmetic kernels | Manual scalar matrices; float32 precision and interval constraints; unsigned input needs explicit widening |
| Division | Absent | Affine diagonal inversion plus extraction for reciprocal; restricted `DivModPositive` | No general integer division/remainder. Constant division exhaustively checks a finite nonnegative domain below 2^24; quotient <=32767 |
| Rounding/conversion | Absent | `MatFloor`, `MatTrunc`, `MatRoundAway`, `MatAbs`, word stores and positive32 store; host boundary checks and native probes | Different domains and meanings. Word store range excludes unsigned 65535; positive32 store only 0..16777215. These are not unrestricted casts |
| Matrices/geometry | Absent | Strict 1..8 dimensions; matrix operations; 3x3 rotation/translation/scale, diagonal inversion; f32 tuple transform | `InvertAffine` currently proves a diagonal 3x3 case, not arbitrary inverse. Public `TransformData` rejects fixed formats; no perspective W divide |
| Comparisons | Only internal loop condition | Bounded-u16 `CallIf`; full-width fixed-type `LessThan`; `RequireEqual` | `CallIf` excludes 65535 and has no signed/float generality. A Boolean expression frontend needs normalized comparison lowering |
| Control flow | `For`/`Next`, named declared counter, tail-call loop body | Parameterless named `Program`, acyclic calls, constant `Repeat(0..512)`, conditional calls | No user function parameters/returns, general `If` blocks or general loops. Hosted parser does not accept main's expressions/For syntax |
| Resident lifecycle | Raw output runs through example loaders; variable/loop buffers are created by generated commands | Bootstrap installs persistent data/programs, optional finite setup; `.map` and `.clear`; explicit job calls | No automatic hosted render. Callbacks/event-resident language integration are not implemented |
| Assets/imports | No source-level import facility | Source-relative binary `Asset`/`Table`, exact sizes, imported-field checks, fingerprints; host/native import tests | Binary input formats remain explicit; no general text import or user type schema. Table records use consecutive IDs in 4000..4999 |
| Indexed access | Absent | `LoadIndexed` selects a buffer record; `LoadElement` selects an offset in a typed array buffer | Separate patched instruction template; checked bounds/status; failure preserves destination. Not a general pointer/heap facility |
| Validation/copy | No user-facing facility | Sticky `RequireRange`/`RequireEqual`; `CopyChecked`; fully laid-out `CopyRecord`; admission/lifecycle evidence | Metadata is not runtime validation. Float admission unsupported; application must validate external input before use |
| Drawing | Literal VDU passthrough possible; no typed graphics surface | `PlotPoint`, two-triangle `PlotQuad`, affine bitmap drawing, viewport/scroll; host templates and native pixels | Graphics state and external artwork ownership remain contractual; bitmap indices 0..255 map to external buffers 64000+n |
| Allocation/resources | Automatic variables/internal IDs from 2; lazy accumulator at 1 | Explicit user IDs >=1000; compiler-private 2..999; hardcoded excluded ranges; resource ledgers/caps | Rally-specific namespace, not main's proposed general asset reservation model. No general local frames/heap allocator |
| Diagnostics/tooling | Source-line errors; raw blob | Source-line/type/shape/range/alias errors; owned IDs, relocations, byte ledgers, import hashes, cleanup | Useful foundation; map is ad hoc text and CLI publishes several files separately, not a transactional package |
| Resource reduction | Lazy main accumulator | Opt-in shared scratch, exact-byte literal pooling, selected immutable call inlining | These options solved Rally constraints. Keep current behavior available; automatic policy changes need separate qualification |

Evidence entry points: [M1], [R1], [R2], hosted/resident contracts [R4], native
example documentation [R5], and final scene review [A1]. The actual implementation
is narrower than some broad statements in the hosted design.

## Where the authoring burden comes from

The concern about usability is supported by source examples. A scalar expression
is written as declarations for buffers, fields and several matrices, followed by
load/operate/store commands. A conditional body is often lifted into another
numbered program. Arrays require repeated field declarations describing each
record. None of those are inherently required to express the application intent.

As an illustration of scale, the existing generated oval scenery source has
20,281 lines and 1,285,993 bytes, including 19,435 `Field` declarations, 70
matrices and 92 programs. Most fields describe generated data layouts; this
count does not mean a programmer hand-wrote 19,435 declarations. It does show
that users presently need generators or a great deal of mechanical source to
represent typed resident data. The source is generated by tracked Rally builders,
and should not be edited directly. [A2]

| Current burden | Recommended ownership |
| --- | --- |
| Ordinary variable IDs, field offsets and temporary scalar matrices | Compiler allocation/type checking |
| Literal constant fields and widening before numeric operations | Compiler materialization, with explicit numeric conversion semantics |
| Separate numbered programs solely to express an `If` body | Compiler control-flow lowering |
| Array stride, per-record field repetition, address arithmetic and patch offsets | Typed record/array layout and indexed-access lowering |
| Which externally owned assets and entries the host can access | Explicit export/asset interface, backed by a generated manifest |
| Whether external data is admitted and what happens on invalid input | Visible checked operation and branch/status contract |
| Degrees versus other angular units, floor versus truncation, matrix multiplication order | Named library operations and documented type rules |
| Raw protocol experimentation | Retained low-level intrinsic surface, with explicit storage ownership |

A thin library alone cannot remove the main burden: Rally's parser has no
general expressions, parameterized procedures or record types in which to write
such wrappers. A small frontend extension is necessary. A library becomes useful
after expressions, typed values and calls have a common representation.

## Proposed author-facing surface

Preserve decided main conventions: mandatory semicolons including block
delimiters; case-insensitive keywords and case-sensitive identifiers/paths;
`=` assignment and C-family comparison symbols; zero-default ordinary variables;
wrapping integer arithmetic; explicit value-to-string conversion; compiler-owned
temporary storage. Keep the preferred `For ... Next i` spelling. [M2]

`Sub`/`EndSub`, `Record`/`EndRecord`, `Range`, long numeric type names beyond
existing `uint16`, matrix type notation and import/checked-load spellings below
are illustrative proposals. They follow the main document's lean toward keyword
blocks but are not already accepted decisions. Parameterized user procedures,
recursion, stack frames, heap allocation, unrestricted floating division and a
complete standard library are not prerequisites for the first useful slice.

### EX-001 — Scalar expression and explicit conversion

Existing hosted syntax, using supported operations and the tiny negative input
from the scalar-floor regression. The entry is defined here; installation alone
does not call it:

```golem
Buffer input(1000,4);
Field x(input,0,f32,-32768,32767);
Set(x,-0.00006103515625);
Buffer output(1001,2);
Field result(output,0,s16);
Matrix source(1100,1,1);
Matrix integral(1101,1,1);
Program compute(2000) {
    MatLoad(source,x);
    MatFloor(integral,source);
    StoreI16Floor(result,integral);
};
```

Proposed equivalent:

```golem
float32 x Range(-32768,32767) = -0.00006103515625;
sint16 result;
Sub compute;
    result = sint16(floor(x));
EndSub;
```

Expected result is -1. `floor` returns an integer-valued float; the explicit
`sint16` conversion requires a representable result. Retain Rally's true
`MatFloor` before the word store: directly treating the bias-based
`StoreI16Floor` as a universally exact floor of an arbitrary tiny negative float
would lose the distinction its later floor work addressed. The compiler manages
scalar matrices and intervals. Inputs changed externally must first satisfy the
declared finite range; `Range` does not itself insert admission code. [R6]

### EX-002 — Geometry composition without manual intermediate matrices

Existing supported rotation portion of the arithmetic fixture, made
self-contained:

```golem
Buffer input(1000,4);
Field angle(input,0,s32,-90,90);
Set(angle,45);
Matrix rotation(1110,3,3);
Program rotate(2000) {
    Identity(rotation);
    RotateDegrees(rotation,angle);
};
```

Proposed equivalent:

```golem
sint32 angle Range(-90,90) = 45;
matrix(3,3) rotation;
Sub rotate;
    rotation = rotate_degrees(identity(3),angle);
EndSub;
```

The compiler must install real matrix metadata and preserve stock composition
order; merely allocating zero bytes does not make a usable matrix. A later
`transform_points(destination,source,rotation)` can infer count/stride from typed
point arrays, initially restricted to Rally's qualified f32 path. That wrapper
must not silently add perspective division or claim arbitrary fixed output.
Angle units stay explicit. Matrix values are statically sized and resident. [R5]

### EX-003 — Finite loop and readable conditional call

Existing supported hosted syntax:

```golem
Buffer state(1000,4);
Field enabled(state,0,u16,0,1);
Field count(state,2,u16);
Set(enabled,1);
Buffer constants(1001,2);
Field one(constants,0,u16,1,1);
Set(one,1);
Program tick(2001) { AddU16(count,1); };
Program work(2000) {
    Repeat(8) { CallIf(tick,enabled,eq,one); };
};
```

Proposed equivalent:

```golem
uint16 enabled = 1;
uint16 count;
uint16 i;
Sub tick;
    count = count + 1;
EndSub;
Sub work;
    For i = 1 To 8;
        If enabled == 1 Then;
            tick();
        EndIf;
    Next i;
EndSub;
```

Both increment `count` eight times when enabled is 1, preserving modulo-65536
arithmetic. The proposed example also exposes `i`, unlike Rally's private repeat
counter, so byte identity is not claimed. The compiler materializes constants
and branch bodies, keeps the loop back edge last, and validates the call graph.
Initial resident loops should have static finite bounds and reject mutation of
their control variable or a wrapping endpoint. Main's current ordinary loop
parser does not enforce those restrictions. General full-width conditions need
Rally's byte-normalized comparisons, rather than copying restricted `CallIf`
onto every `uint16` expression. [M1] [R1] [R2]

### EX-004 — Typed imported records and checked selection

Existing supported core of `native_lookup/kernel.golem`:

```golem
Table samples(4094,8,4,"records.dat");
Buffer state(1000,4);
Field index(state,0,u16);
Field ok(state,2,u16,0,1);
Buffer constants(1001,2);
Field one(constants,0,u16,1,1);
Set(one,1);
Buffer record(1500,8);
Field a(record,0,s32,-100,100);
Field b(record,4,s32,1,10);
Matrix left(1100,1,1);
Matrix right(1101,1,1);
Matrix product(1102,1,1);
Program fetch(2000) {
    LoadIndexed(record,samples,index,ok);
    CallIf(project,ok,eq,one);
};
Program project(2001) {
    MatLoad(left,a);
    MatLoad(right,b);
    MatMul(product,left,right);
};
```

This source requires the fixture's 32-byte `records.dat` beside it. Proposed
equivalent:

```golem
Record Sample;
    sint32 a Range(-100,100);
    sint32 b Range(1,10);
EndRecord;
Table(Sample) samples = import_records("records.dat",4);
Sample selected = Sample(0,1);
uint16 index;
float32 product;
Sub fetch;
    If try_load(selected,samples,index) Then;
        product = float32(selected.a) * float32(selected.b);
    EndIf;
EndSub;
```

Derive the eight-byte record width and field layout from the type; validate all
four imported records at compile time. `try_load` produces an internal Boolean,
leaves the old record unchanged on failure and guards the dependent computation.
The explicit initializer keeps `selected.b` within its nonzero range; default
zero cannot silently establish that invariant. Integer-to-float conversions here
are exact for the stated small domains. Selection patches a separate instruction
template; no raw buffer offset appears in application source. A named contiguous
asset reservation may retain base-plus-index access without pinning ordinary
variables. Externally stable placement is a separate interface decision. [R7]

The existing snippets above are assembled from the supported grammar and
examined fixtures/tests, with minor self-contained declarations. They were not
freshly compiled during this read-only investigation. The proposed snippets
specify desired semantics, not a frozen grammar or executable replacement.

## Recommended implementation architecture

| Approach | What it buys | Limitation / recommendation |
| --- | --- | --- |
| Rename or wrap intrinsics only | Friendlier operation names, minimal backend change | Keeps explicit layout and intermediate-state burden; insufficient alone |
| Extend main syntax over reusable checked operations | Normal variables, expressions, structured branches and typed data; existing lowering remains useful | Recommended, in small slices with a typed operation layer |
| Refactor/transfer selected Rally code | Reuses numeric edge-case fixes, relocations, ownership, tests and cleanup | Also recommended; remove application policy from the common implementation |
| Merge all Rally commits or rewrite the compiler outright | Fast availability of the existing surface, or complete design freedom | Neither is needed for the intended incremental path; both enlarge review and compatibility work |

Use one semantic model for names, types, storage, entrypoints and checked
operations. Keep the existing ordinary parser/output path as a compatibility
baseline while migrating features incrementally. Lower the new expression AST
to explicit typed operations such as load, add, matrix product, checked convert,
guard, indexed copy and finite call. Start with the operations needed by the
first example; a full optimizing IR is unnecessary.

Retain `--hosted` and its current intrinsic syntax as a compatibility entrypoint
for reproducing Rally artifacts. Long-term, make batch, hosted and event-resident
execution **profiles of the same language**, with lifecycle/export metadata.
The shared language should not require the Rally game's MOS controller or packet
ABI. Event callback support remains a future extension. Ordinary files that use
only today's syntax should retain their existing compilation contract.

Move hardcoded Rally ID exclusions, table window and external bitmap ownership
into an explicit target/application layout policy. A new allocator should claim
all declared asset/export reservations before placing ordinary storage, while
the Rally compatibility policy preserves its numeric IDs. Main's Axis 23 leans
toward low programmer-owned asset IDs and compiler-managed ordinary variables;
Rally instead puts its compiler scratch below 1000. Do not adopt that conflict
silently or introduce arbitrary ordinary-variable pinning. [M2] [R1]

Preserve the existing guarded patch-then-execute design. A map/manifest should
explain named entrypoints, packed inputs, typed storage, ownership, relocation
sites, initialization and cleanup. Let callers use generated encoders or a
documented manifest instead of reconstructing numeric IDs and field offsets.
The implementation should publish a matching output/map/cleanup set; current CLI
failure tests cover semantic errors, not atomic recovery from all file I/O
failures while writing three outputs.

## Wire traffic is the acceptance target

Rally demonstrated the right communication shape: static code/art/coefficients
resident once, then **80 input bytes + 11 update-header bytes + 6 call bytes =
97 bytes** per submitted job. With its three-byte swap, that is 100 scene bytes;
HUD traffic is counted separately. Final records report recurring scene UART
reductions of 88.53047% for oval and 86.69439% for Fuji. These are results for
that particular scene/protocol, not promises for every Golem program. [A1] [A3]

For a new resident job with one packed input payload of P bytes, the same update
and call form costs **P + 17 bytes** per invocation. A call with unchanged inputs
can cost six bytes. Static program/data transfers belong to installation, not
each invocation. Include actual update headers, call commands, swaps and any
required status traffic when comparing with direct VDU emission. Small jobs can
cost more than direct commands; code residency alone does not guarantee savings.

Use a simple repeated marker-strip job for the first demonstration: host supplies
an x/y origin, resident code derives eight quads using a bounded loop, and no
vertices or drawing templates are resent. Under Rally's 27-byte `PlotQuad`
sequence, direct emission is 8 * 27 = 216 bytes; a single four-byte packed input
plus update/call is 21 bytes, excluding any common setup/swap. This is a proposed
byte budget to verify against actual generated streams, not a measured result.
It needs small integer additions, finite looping and a typed drawing primitive;
it does not require the full Rally renderer or broad float support.

Record the one-time installation cost I and the saving per job S. When S > 0,
break-even is ceil(I / S) jobs, with installation cost measured relative to the
direct-emission baseline. Report reset/reload/cleanup costs separately and show
several invocation counts. This establishes amortized wire savings without
requiring a rendering-speed optimization campaign.

## Correctness conditions retained from Rally

**Returning jobs.** `bufferCall` executes synchronously; the inner
`processAllAvailable` does not service the outer event queue. A loop can be
finite yet occupy the VDP for an inconvenient interval. Bounded calls must
return, and the first examples should remain small. GP echo demonstrates parser
progress after a call, not display completion. [M3] [F1]

**Stack use.** Rally hit the physical 4096-byte processLoop stack despite its
16-level compiler guard and passing native tests. Selective inlining reduced
observed nesting from eight to six; the diagnostic recorded 164 bytes minimum
spare stack, then the Author confirmed slow continued operation after a reset on
official VDP 2.16.0. That is observed-path evidence with narrow headroom, not a
general stack proof or cold-power-cycle result. Avoid creating a nested call for
every newly introduced source-level wrapper. Preserve tail position and account
for helper/primitive calls. Hardware stack qualification is a correctness gate,
even though speed optimization is deferred. [A1] [R3]

**Integer semantics and comparisons.** Preserve main's modular arithmetic and
carry landing byte. The stock conditional reader starts values at -1 and copies
only one or two bytes into them. Equal-width fetched operands share that upper
byte contamination, but all-ones collides with the missing-value sentinel;
literal and existence forms have additional pitfalls. Reuse byte normalization
for full-width/signed comparison, and reject or handle wrapping counted-loop
endpoints deliberately. No blanket range-trapping arithmetic is proposed. [F1]

**Floating semantics.** There is no command-34 matrix inversion operation in
the consulted docs/firmware. Rally's reciprocal uses command-32 diagonal affine
inversion. Stock fixed-output conversion casts through unsigned integers; preserve
the positive-magnitude/sign-restoration paths and their bounded domains. Floor,
truncation and ties-away rounding are distinct operations. Matrix result creation
allocates a new buffer in firmware, so a scalar expression need not behave like
a hardware ALU operation. This observation limits claims; optimizing it is not
part of this task. [F1] [F2] [R6]

**Trusted values and ownership.** A declared range is not admission of arbitrary
wire bytes. `Require*` is sticky rejection, whereas `CopyChecked` resets its own
status; retain that distinction. Input validation must precede copying to active
state and computation. Rejected lookup must preserve the destination. Shared
scratch must finish its expansion before another use. Asset/import fingerprints
detect accidental mismatch, not authentication. Cleanup must clear only owned
IDs and preserve external artwork/foreign state. [R1] [R4]

**Host interface.** Encode integer widths and endianness explicitly; do not copy
a host/eZ80 C struct directly into a wire packet. Rally's sequence/seal protocol
is useful integration evidence but need not become mandatory for every Golem
job. Interrupted update streams must not be followed by a trigger; the stock
parser is not made transactional by a nicer source language. [A3]

Physical rendering speed remains unaccepted in the Rally delivery. Its roughly
1.19 root returns per second is sparse diagnostic timing, not scanout FPS. The
native VDP runs on the host CPU. Keep this context in the retained evidence, but
do not make speedup or new performance benchmarking an acceptance criterion for
this integration. The goal here is the wire-saving programming model. [A1]

## Proposed phases after review

These phases describe dependencies and acceptance; they are not authorized work
items. Promote approved work into the task register before implementation.

### PH-001 — Establish a reusable lowering boundary

Bring selected source/tests into a task silo, preserve an immutable Rally
reference, and separate checked operations/storage policy from the intrinsic
parser. Initially retain existing encoders and allocation order for the
compatibility mode. Keep Rally resource exclusions in its policy.

Acceptance: ordinary goldens/rejections still pass; hosted goldens and owned
cleanup match the examined implementation; semantic errors retain existing
output. Reuse `test_existing`, `test_hosted`, `test_imports`, `test_admission`
and `test_inline` as applicable. No language-wide rewrite or optimization pass.

### PH-002 — Prove a readable resident job saves wire bytes

Depends on PH-001 and review of procedure/export syntax. Add compiler-managed
`sint16` storage/addition, named parameterless finite procedures, direct source
calls and a small typed quad drawing builtin. Reuse main's `For` form with the
finite resident checks. Provide a packed x/y external input binding and generated
entry/layout metadata for the eight-marker strip example.

Acceptance: the user-facing example has no manual buffer IDs, offsets, scalar
matrices or copied VDU templates. Separate installation from repeated invocation;
an independent stream decoder counts exact startup and recurrent bytes, checks
the proposed 21-versus-216 budget and reports amortization. Repeated calls use
fresh inputs without reinstalling code; a native visual/lifecycle check verifies
geometry and cleanup, with the required human emulator review. A small physical
run verifies return/stack safety. No FPS or speedup target.

### PH-003 — Add qualified expressions and structured conditions

Depends on PH-001/PH-002. Add `If` blocks and normal comparison operators,
explicit widening/conversion, the qualified scalar float operations and a small
matrix/geometry library surface. Use EX-001 through EX-003 as semantic examples.
Expose supported domains in diagnostics; reject operations outside them instead
of inventing unrestricted division/casts. Keep `str` work separate.

Acceptance: source expressions agree with corresponding intrinsic operations
on results and rejection behavior, including 65535 comparisons, signed extrema,
zero/subnormal/half/integer boundaries and alias cases. Reuse `test_vehicle_math`,
`test_divmod`, `test_rounding`, `test_floor`, `test_checked_copy` and the native
arithmetic/quantization probes. Recheck stack/return behavior when lowering adds
calls. Retain wire accounting; do not broaden into a performance project.

### PH-004 — Replace repeated field plumbing with typed resident data

Depends on the common type/layout model and conditionals. Add record layouts,
typed imported tables/arrays, checked selection and named asset reservations.
Use EX-004; keep existing binary formats explicit and checkable. Keep external
input admission visible and specify invalid-input retention/status behavior.

Acceptance: one record type replaces per-element field repetition; sizes/offsets
are derived and mapped; imports validate every record; boundary indices, byte/ID
carries and legitimate all-ones integer values behave correctly. Reuse
imports/elements/admission/checked-copy tests and native lookup/admission drivers.
Verify repeated jobs send changing indices/state, with static records resident.

### PH-005 — Package the supported surface for ordinary authors

Depends on the preceding slices. Publish the accepted grammar/semantics, a few
short examples, supported numeric domains, export/loader manifest and cleanup
contract. Promote only accepted compiler/library/fixture artifacts from the silo.
Keep the Rally intrinsic compatibility path reproducible; label unsupported
operations and deferred features clearly.

Acceptance: a user can build, install, invoke repeatedly, reset and unload the
small examples using documented tooling without inspecting compiler internals.
Show wire totals including installation and recurring overhead. Document the
batch/hosted seam without requiring a universal MOS controller. Event handlers,
general parameterized functions, recursion/heap allocation and performance work
require their own later scope decisions.

## Decisions for the Author

| ID | Recommended direction | Choice this resolves |
| --- | --- | --- |
| DEC-001 | Adopt incremental syntax over selected Rally lowering, retaining the existing intrinsic mode for compatibility | Whether the task should move toward one usable Golem language and reusable backend |
| DEC-002 | Demonstrate a small resident drawing job and count wire bytes first | Whether to prioritize a concrete install-once/call-many example before broad float/matrix and asset features |
| DEC-003 | Use keyword-delimited procedures and `If` blocks; compiler-manage ordinary storage, with a separate explicit host/asset interface | Direction for the source surface and allocation ownership; exact new type/range/record/export spellings can be reviewed with the first grammar slice |

No compiler, runtime, loader, generator, external worktree or firmware was
changed in this investigation. The report and supporting audit remain in the
task silo. Implementation awaits this review.

## Source index

[M1]: ../../../src/golemc.cpp
[M2]: ../../../docs/design/language-type-proposals.md
[M3]: ../../../docs/devlog/2026-07-28-execution-lifecycle.md
[R1]: /Agon/mystuff/golem-rally19/src/hosted.hpp
[R2]: /Agon/mystuff/golem-rally19/docs/evidence/rally19-hardware-inline/hosted-tests.json
[R3]: /Agon/mystuff/golem-rally19/docs/devlog/2026-09-12-rally19.md
[R4]: /Agon/mystuff/golem-rally19/docs/design/rally19-resident-v1.md
[R5]: /Agon/mystuff/golem-rally19/examples/native_arithmetic/README.md
[R6]: /Agon/mystuff/golem-rally19/tests/test_floor.cpp
[R7]: /Agon/mystuff/golem-rally19/examples/native_lookup/kernel.golem
[C1]: /Agon/mystuff/agon-dev-env/cross-agent/rally_golem/README.md
[A1]: /Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/DELIVERY.md
[A2]: /Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/BUILDING.md
[A3]: /Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/ABI.md
[F1]: /Agon/agon-vdp/video/vdu_buffered.h
[F2]: /Agon/agon-vdp/video/types.h

Primary compilation-target prose consulted:
[Buffered Commands API](/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md),
commands 5/6, 32/33/34 and 41. Firmware tie-breakers: `bufferCall`,
`bufferConditional`, `bufferMatrixManipulate`, `convertFloatToValue`, and
[processAllAvailable](/Agon/agon-vdp/video/vdu_stream_processor.h:587).
Additional validation sources: Rally's
[REVIEW.md](/Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/REVIEW.md),
[PERFORMANCE.md](/Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/PERFORMANCE.md),
Golem's [hosted design](/Agon/mystuff/golem-rally19/docs/design/rally19-hosted-v1.md),
[test runner](/Agon/mystuff/golem-rally19/tests/run_hosted.py) and adjacent
`test_*.cpp` / `examples/native_*` files.
