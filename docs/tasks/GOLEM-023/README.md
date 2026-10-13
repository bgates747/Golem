# GOLEM-023 first research tranche — Golem to native C

**Direction update:** the Author requires the same source to target stock
buffered commands or native VDPlets, with application/end-user selection,
including runtime changes. The [portability requirements](../../design/backend-portability.md)
supersede any suggestion below that shared semantics/layout can be postponed.
Whole-module native compilation remains a staged emitter choice; the shared
frontend/state foundation comes first in [GOLEM-024](../GOLEM-024.md).

Recommend a **native-only Golem source module**, selected by an explicit compiler
option, translated to C and packaged through the existing VDPlet SDK. Keep
source globals in caller-installed VDP buffers and generate named assembly calls
for Subs and Functions. This delivers friendly expressions and control flow
without first implementing mixed-backend calls or synchronized copies of globals.
The proposal needs Author review; no new compiler/backend or runtime service has
been implemented. Physical performance is unmeasured.

Read [the complete initialized example](native-example.golem) first. It contains
one typed Function, an If/ElseIf, a four-iteration loop and persistent globals.
It illustrates the proposed source experience and **does not compile today**.

## The three decisions to review

1. **Select native execution per source module initially.** Candidate CLI:
   `golemc --native --emit-asm source.golem output-directory`. Every procedure in
   that file is native; no `Native` keyword is needed for this first route.
2. **Keep source globals authoritative in VDP buffers.** Native procedures and
   assembly see the same storage. Native locals/parameters are invocation-local;
   there is no hidden copy-in/copy-out of the entire global block.
3. **Start with a small numeric/control-flow subset.** uint16, float32 and local
   Boolean values; checked assignments, If, constant Range loops and scalar
   Subs/Functions. Add maths helpers, composites and graphics through subsequent
   bounded decisions. This does not replace their broader language requirements.

## Option comparison

| Route | Friendly source benefit | Work before first useful result | Execution/wire implications | Recommendation |
| --- | --- | --- | --- | --- |
| Native library calls | Friendly names for supplied kernels | Small library plus call interface | Kernel is native; surrounding buffered arithmetic/patching remains | Useful bridge, insufficient as the whole language direction |
| Selected native procedures | User-authored native expressions and bodies | Native backend plus mixed-call/storage rules | Strong benefit for selected work, but each backend transition must be explicit | Preserve as a later integration option |
| Native source module | User-authored native procedures, ordinary internal calls | Native backend and one host interface | One native entry can perform a complete finite job; resident state keeps repeated wire use small | Preferred first milestone |
| Automatic mixed compilation | Compiler selects execution placement | Adds partitioning, cost policy and data synchronization | Benefit depends on placement and transfer decisions | Defer; unnecessary for the hobby-project budget |

Whole-module selection is an explicit boundary, not automatic placement. Existing
buffered and new native modules may coexist in an application, but their first
integration is through assembly and documented shared-buffer layouts. A module
does not gain a source-level cross-backend call feature merely by sharing storage.

Native compilation should reduce arithmetic/loop dispatch overhead. Wire traffic
is small because work and data remain resident, an advantage already available
to buffered programs. Neither observation establishes a numerical speedup or FPS.

## Candidate first source subset

| Area | Proposed first support | Explicitly outside the first slice |
| --- | --- | --- |
| Types | uint16, float32; bool locals/conditions | Signed/wider integer arithmetic, fixed-point, strings and exported bool layout |
| Values | Explicit declarations; initialized persistent globals; lexical locals with definite assignment | Implicit typing, shadowing visible names, static locals |
| Arithmetic | Same-type `+ - *`; explicit uint16/float32 conversions after checks | `/`, `%`, divmod, pow, sqrt and rounding until helper qualification |
| Conditions | `!= == < > <= >=`; bool literals/assignments; If/ElseIf/Else/EndIf | General Boolean operators pending their syntax/short-circuit rules; Switch lowering |
| Iteration | `For counter In Range(start:end:step)` with compile-time integer endpoints and step | Unbounded Do/Loop, mutable counters and runtime iteration bounds |
| Procedures | Subs and single-scalar-result Functions; typed ByVal scalar parameters; Return | Recursion, ByRef, tuples, arrays/matrices/records as arguments/results |
| Host integration | Named wrappers, parameter/result slots, packed global layouts | Automatic MOS file loading, C ABI calls directly from eZ80, synchronous return in eZ80 registers |
| Services | Contiguous application buffers | Drawing/text emission, dynamic allocation, callbacks and background execution |

No terminators; newlines remain whitespace. Keywords/builtins remain
case-insensitive and identifiers case-sensitive. The native parser must recognize
multi-character comparisons and the selected Range/Function grammar, which the
current maintained parser does not generally implement. Source grammar should
be shared by both backends. A new native operation may be an interim development
artifact, but it is portable only after both implementations are qualified against
the same semantics and interface. Current buffered support remains limited.

Every normal Function path must return its declared type. Arguments evaluate
once, left to right. Calls form an acyclic graph. Compute loop counts in wider
host arithmetic, reject zero/wrong-direction steps, and stop without overflowing
the declared counter; on normal completion it retains its last visited value.
The example's counter therefore ends at 4. Bound combined loop/call work with a
simple documented compile-time ceiling rather than admitting arbitrarily long
finite jobs. The ceiling value remains a later implementation decision.

Functions must eventually accept and return every supported object type. This
first native subset supports scalar types only; it must reject other types
clearly and must not redefine their eventual value/ownership semantics.

## Data and lifetime: one authoritative location

The proposed example's source globals and generated status occupy one 20-byte
packed VDP buffer. Allocation uses the compiler's existing reservation accounting;
actual buffer IDs are build outputs, not handwritten language requirements.

| Field | Offset | Bytes | Initial value |
| --- | --- | --- | --- |
| input | 0 | 4 | binary32 2.0 |
| scale | 4 | 4 | binary32 1.5 |
| bias | 8 | 4 | binary32 0.25 |
| output | 12 | 4 | binary32 0.0 |
| calls | 16 | 2 | uint16 0 |
| runtime_error | 18 | 2 | uint16 0 |

Assembly installs the initializer bytes once. Reads/writes in Golem operate on
this block through an admitted native view; scalar values may use temporary
registers/locals while an expression is evaluated. Do not reinterpret packed
bytes as an aligned C struct. Use byte loads/stores and local values, following
the SDK numeric example; do not cache globals across host updates or invocations.

Persistent language state is this VDP buffer, which survives module UNLOAD/LOAD.
Private native BSS resets when the module is loaded, but is not a second copy of
source globals. A deliberate package reinstall/reset initializes source state;
merely replacing native code does not silently reset it. The first version
requires identical storage layouts for reuse; a changed layout needs explicit
reinstallation. There is no live state-migration feature.

Each externally callable Function has caller-owned parameter/result storage.
For `calibrate`, its frame is eight bytes: value at offset 0, result at offset 4.
Both start at 0.0 on installation. An internal call from compute uses native
ByVal arguments/local results and never acquires, copies or updates that external
frame. Parameterized Subs use parameter slots without a result slot.

Each exported routine has a fixed four-byte RUN descriptor: global buffer ID:u16,
call-frame buffer ID:u16. A parameterless Sub uses 65535 in the second field as
an adapter-defined unused value; it never asks the runtime to acquire buffer
65535. Descriptors are ordinary VDP buffers, initialized by assembly; the runtime
does not interpret their contents. All IDs remain published/reserved.

The adapter acquires its descriptor read-only, globals read/write, and an external
frame read/write if needed: at most three buffer leases, within the existing
four-lease limit. Validate layout sizes and distinct storage before source work;
retain the admitted global view across internal calls. Per-field native byte
access does not need another service call or repeated lease admission.

## Proposed assembly interface and wire cost

The include publishes the familiar `golem_<field>_buffer/_offset/_bytes/_type`
symbols, named `golem_compute` and `golem_calibrate` wrappers, and each Function's
parameter/result layout. It also publishes assigned native entry IDs and the RUN
packet locations. These names describe a proposal, not files emitted today.

Installation is explicit: initialize storage/descriptors, upload the .vdpl, LOAD
with a reply, then bind the returned handle into every resident RUN packet before
using a wrapper. Entry IDs are deterministic routine declaration order: calibrate
0, compute 1 in this example. Never bake in a guessed handle such as 1. Reloading
requires binding the fresh handle again, while globals remain resident.

| Application operation | Required host action |
| --- | --- |
| First setup | Load package through MOS; install emitted buffers; LOAD; bind the returned handle |
| Compute | Update input if changed; `call golem_compute` |
| Call Function directly | Write its value parameter; `call golem_calibrate`; consume its result slot after VDP completion |
| Consume output on VDP | Execute a subsequent resident operation in the ordered stream |
| Read result on eZ80 | Use an explicit reply/read mechanism; wrapper return alone is not completion |
| Replace/uninstall | Serialize UNLOAD, reload/bind or clean up owned storage as appropriate |

Each wrapper sends a six-byte stock call of its resident RUN packet and returns
to eZ80 without a completion acknowledgement, matching existing wrapper semantics.
Ordered follow-on VDP commands can consume native results. eZ80 reads, a module
replacement or an ownership change require an explicit completion strategy.
INFO and READ16 can be requested when needed; no per-call round trip is mandatory.

With the existing stock N-byte update framing, updating input's four bytes and
calling is 21 wire bytes: 15 for the update, six for the call. Reusing input costs
six. Changing input/scale/bias together costs 29 bytes: an 11-byte update header,
12 data bytes and a six-byte call. Four internal Function calls and the loop add
zero wire bytes. Installation, handle binding and optional result reads/replies
are additional. Direct external Function calls similarly cost six, or 21 when
updating their four-byte parameter before calling. There is no recurring global
block copy; these counts do not imply zero ESP32 memory traffic.

## Semantic safeguards without C surprises

Use checked uint16 arithmetic matching the maintained resident profile, leaving
raw-mode wrapping behavior unchanged. Promote operands to uint32 before addition
or multiplication; check subtraction before computing it. C's signed integer
promotions must not turn a nominal uint16 multiplication into undefined overflow.
Every intermediate operation is checked before an assignment commits.

Admit finite float32 host inputs and check declared Bounds; validate expression
results before stores, including narrower destination Bounds. Contextual literals
round to binary32 with the existing supplied/rounded Bounds checks. Preserve
source evaluation order and separate multiply/add rounding (`-ffp-contract=off`);
do not enable fast-math. Explicit float-to-integer casts truncate toward zero,
then range-check before committing. Establish a finite safe widened conversion
domain before a C cast, truncate into a containing temporary and then check the
destination; an out-of-range C cast followed by a check is unsafe. The inspection
probe's bare cast is not a safe conversion implementation. Exceptional results,
subnormals/underflow and rounding-state behavior still need qualified rules;
bounded normal-number examples
do not settle the full floating-point contract.

A language error exits the whole exported job through generated status propagation.
Preserve the failed destination; earlier committed writes remain. Reset the
language status at the outer entry after global storage is safely admitted, not
on each internal Function call. A Function's result slot changes only after a
successful result. Source functions can use generated status-plus-output C
signatures; no exceptions, setjmp or rollback are needed.

Keep the language status field distinct from the runtime's VDPL_E_* namespace.
An optional native reply reports an appropriate runtime-category error; detailed
language failure resides in the generated status. If global storage itself cannot
be acquired, it cannot receive a trustworthy new language status: use the runtime
status/reply instead. Exact extended language codes remain later design work.

## Actual compiler inspection

The [receipt](compiler-inspection.json) records a fresh compile-only inspection
using the SDK's exact GCC 8.4 flags. The [probe](compiler_probe.c) and
[reproduction script](inspect_compiler.py) stay in this silo. Run from the root:

```bash
.venv/bin/python docs/tasks/GOLEM-023/inspect_compiler.py
```

| Inspected operation | Observed output with this compiler/profile |
| --- | --- |
| `x * scale + bias` | `mul.s`, then `add.s`; no helper import |
| Float subtraction | `sub.s`; no helper import |
| Float `<` | `olt.s`; no helper import |
| Checked uint16 addition/multiplication | Inline integer instructions and check-before-store; no helper import |
| uint16 to float / float to uint16 | `ufloat.s` / `utrunc.s`; the probe omits admission checks |
| Plain float division | Unresolved `__divsf3` |
| Builtin sqrt/floor | Unresolved `sqrtf` / `floorf` |

Every probe function reports a 32-byte static compiler frame. That is not a
complete entry/call-chain stack bound. The object has 791 bytes across code,
literals and metadata; it is neither a linked module nor a package-size estimate.

These findings support starting with +, -, *, comparisons and finite control
flow while separately resolving maths helpers. The current module linker rejects
unresolved symbols. Merely writing `/`, sqrt or floor in generated C will not
produce an admitted self-contained image. Internal helper implementations or
compatible linked support need further inspection for imports, relocations,
code size, stack and licensing. This research makes no native timing claim.

## Implementation path after review

1. **First numeric module:** implement the selected grammar and checked semantics,
   generated C, native entry adapter and existing SDK invocation. No source-level
   mixed calls, new runtime services or automatic partitioning.
2. **Assembly delivery:** manifest/global/call-frame layouts, initialized data,
   LOAD/handle binding and named wrappers. Qualify reload/error/lifetime behavior
   and measure the first finite example physically when authorized.
3. **Useful expansion:** qualify missing math helpers and types, then consider
   arrays/records/matrices and graphics by explicit small tasks. Existing buffered
   modules can consume published storage meanwhile.
4. **Optional mixed procedures:** add explicit per-procedure placement only when
   a real application demonstrates the need and the native whole-module contract
   is established. Automatic placement remains deferred.

These are proposed milestones for review, not separately authorized execution
checklists. The task plan remains the sole detailed subtask tracker.

## What remains after this tranche

The generated-C approach and interface are described here, but a complete
translation contract, helper closure and stack/resource admission policy are not
finished. A reasonable next research slice would inspect math-helper packaging
and propose the first qualification workload after the Author chooses the three
decisions above. Graphics service expansion and a fair hardware comparison remain
separate work; no hardware was flashed or emulator launched for this tranche.

The initialized example predicts output 3.25/calls 1 for input 2.0, then
-5.75/calls 2 for input -4.0; input 8.0, scale 4.0 and bias 1.0 explicitly clamp
to 16.0. With calls already 65535, checked increment fails before output commits.
These are source-level reference expectations, not results from executing a new
Golem backend.

Source basis: main checkpoint `97c1651`, runtime SDK checkpoint `33a554c`, the
[maintained scalar contract](../../design/resident-scalar.md),
[existing semantic review](../GOLEM-009/semantics-review.md), and the runtime's
[API](../../../../agon-vdplet/docs/API-reference.md),
[numeric example](../../../../agon-vdplet/examples/numeric/numeric.c) and
[loader/service sources](../../../../agon-vdplet/src/vdplet_runtime.h).
