# Stock matrix runtime loop contract — draft v0.2

A stock matrix program can process an entire vertex buffer with one installed
loop body rather than thousands of expanded command copies. The host still
updates controls and calls the same root once. This contract targets compact
resident code, with exact projection equivalence and measured frame cost;
it does not promise faster rendering.

Status: Golem draft, awaiting explicit fsim-wireframe acceptance. Implementation
is blocked until mutual acceptance of this exact revision AND a scoped Golem
freeze commit/push receipt. Once frozen, substantive changes require a mutually
accepted versioned addendum before affected implementation proceeds.

## Scope and baseline

The maintained --stock-matrix profile consumes the already-qualified GOLEM-049
matrix primitives. Ordinary official stock VDP2.16 firmware remains SHA256
b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f. Current
GOLEM-049 source/binary hashes are in baseline.json; they are a working-tree
dependency, not implementation publication authorized by the contract freeze.

Preserve the existing per-iteration four matrix_set_value operations,3x3 inverse,
zero XY16 transform and4-byte XY copy into XYZ16 output. The first set starts
from the immutable identity each iteration; Z is host-initialized and retained.
The bulk camera/screen transform remains float32, with no change in operation
order, degree conventions, rounding or positive-depth/positive-endpoint domain.
All arithmetic/division remains in stock matrix routines. The buffered
interpreter may manage counters, offsets, copies and branches.

Golem owns reusable language/compiler/layout/tests. Fsim-wireframe owns source
mesh/consumer generation, independent reference/pixel/application testing and
visual staging. Shared inputs are read-only; an immutable task-local intake is
allowed. Neither party edits the other's implementation, profiles or separately
owned Pingo work. No native module, firmware change, runtime repair, private
compiler fork, hardware action or agent graphical launch is included.

## Source syntax and observable semantics

The initialized declaration is mandatory, matching the existing bounded portable
loop idiom. This example's borrowed single blocks are installed by the host:
clip holds30float32 (-U,-V,W) triples, zero holds signed16(0,0), pixel holds4zero
bytes, and screen holds30signed16 XYZ triples with retained Z initialized to0.
All IDs are covered by external reservations. The source below is the proposed
syntax, not a claim of existing implementation.

```golem
uint16 vertex = 0
Matrix(3,3) base = Identity
Matrix(3,3) a = Identity
Borrowed XYZF32 clip = Buffer(1000,30)
Borrowed XY16 zero = Buffer(1001,1)
Borrowed XY16 pixel = Buffer(1002,1)
Borrowed XYZ16 screen = Buffer(1003,30)
Sub project
    For vertex In Range(0:29)
        matrix_set_value(a,base,0,0,read_float32(clip,12*vertex+8))
        matrix_set_value(a,a,1,1,read_float32(clip,12*vertex+8))
        matrix_set_value(a,a,0,2,read_float32(clip,12*vertex))
        matrix_set_value(a,a,1,2,read_float32(clip,12*vertex+4))
        matrix_invert(a)
        transform_vertices(pixel,a,zero)
        copy_bytes(screen,6*vertex,pixel,0,4)
    Next vertex
EndSub
```

Bounds are inclusive uint16 literals, start<=end, with an omitted step or explicit
:1. This first scope is positive unit step only; other steps, descending ranges,
empty/reversed ranges and runtime bounds reject. A0:65535range is finite65536
iterations, not an overflowing loop-count value. No implicit declaration,
assignment to an active counter, nested loops, Break/Continue, arbitrary While,
conditional exit or Sub call inside a loop is supported. Existing finite Sub
calls outside loops remain available; a called Sub may contain a loop.

Every loop entrance sets the counter to start, regardless of its initializer or
previous final state. After normal completion it retains the last visited value
(end); no one-past write/increment occurs. Each entrance resets all private
cursors, including after repeating the same Sub. Sequential loop sites may use
the same declared counter. No reentrancy/event scheduler is introduced.

uint16 declarations in this first stock scope are loop counters only. They live
in compiler-owned state, are exported as typed counters in interface/assembly
metadata and included in cleanup. They do not change the packed float-control
input bytes/offsets or require new host updates. Counter use as float input,
general integer arithmetic/assignment or an unchecked writable host parameter
is not implied. Manifest additions are additive; old source packages retain
identical prior artifacts. Next may omit its counter as in portable syntax;
when present it must name the active counter. Statements remain terminator-free.

## Indexed offsets and static proof

Existing literal byte offsets remain supported. An indexed offset is
stride*counter[+bias], where stride is a positive uint16 literal, bias is an
optional nonnegative uint16 literal and counter is the active lexical loop
counter. Whitespace is immaterial. General expressions, counter*stride,
subtraction, multiple counters and runtime stride/bias reject in this first
scope. This restricted expression is not a new general arithmetic grammar.

The expression is supported in read_float32's offset and copy_bytes' source or
destination offsets. Matrix rows/columns and copy length remain static literals;
all prior typed resource checks remain. The compiler proves the entire visited
offset interval using wide host arithmetic: first/last offset and complete
access width fit the declared storage and16-bit addressing. Every float read
must be4-byte aligned at every visited index. No pointer wrap, truncation,
clamping or silently skipped vertex is allowed. Dynamic overlapping/self copies
may conservatively reject; the agreed consumer copies between distinct buffers.

This is compile-time admission, not a new checked-array ABI or runtime bounds
status. Host-installed exact single-block shapes, finite values, positive depth,
legal final endpoints and quiescent control updates remain application contracts.
No per-iteration readback, checksum, duplicate validation or rollback is added.
Borrowed data is neither initialized nor cleared by compiler cleanup.

## Required stock execution and footprint

Emit one reusable body per lexical loop site, compiler-owned iterator/cursor and
argument-template storage, and an actual stock conditional backedge. Runtime
indices/offsets advance within VDP. Patch template offsets before executing the
associated stock operation; retained public command/data layouts stay valid.
No per-vertex eZ80/host command generation, transformed-vertex download, repeated
code upload, source-count unrolling or call-stack growth is allowed.

The existing1024 source-operation limit is not applied to expanded iteration
work. A loop body contains at most1024 static source operations, with no nested
loop or Sub call. The product of its positive literal iteration count and body
operation count must be at most65536 primitive source-operation executions per
lexical loop. Thus the Corsair2685*7=18795body executions are admitted, as is a
single-operation0:65535loop. This cap is not a machine-instruction budget or wall
time promise. Sequential sites/calls remain finite under existing source/call
bounds; no new entry-wide reinterpretation rejects old loop-free programs.

Index/cursor storage must accommodate command5 add-with-carry's extra output
byte without corrupting adjacent state or instructions. Do not advance index or
cursors after the final iteration. Branches must preserve return to the caller
and subsequent statements/Sub calls. New state/templates use checked allocation,
respect all reservations and appear in ownership/cleanup/load-plan metadata.

For fixed source body, changing N from30 to2685 must not grow matrix-loop code,
helper/template count or private iterator storage linearly. Show exact sizes.
Declared mesh/input/output storage may grow, and the separately installed
edge-drawing program remains outside this matrix-loop compaction scope. Report
its cost in whole-application totals, rather than calling the entire renderer
constant size. Do not raise existing IDs, per-block or procedure admission
limits merely to accommodate this consumer.

## Qualification and measurement

Golem checks parser/semantic rejections, inclusive termination, singleton and
nonzero-start ranges, counter65535endpoint without wrap, byte carries,
first/last/all2685records, float alignment, offset overflow, typed storage,
repeated calls/cursor reset, continuation and ID reservation/relocation/cleanup.
Maintain legacy package bytes and portable loop semantics. Host sanitizer,
assembler and load-plan checks accompany actual unchanged-stock headless checks.
Unsupported forms reject predictably before publishing a partial output package.

Fsim-wireframe independently compares tank30/Corsair2685records against the
retained unrolled packages and the same mathematical reference. With unchanged
matrix operation order, loop/unrolled endpoint bytes must match exactly; their
existing tolerance against the double reference is a separate comparison.
Raster/playfield equivalence and control/reset/repeat/cleanup/MOS return remain
required. Full Corsair coverage includes the pinned shared-v2 geometry/73poses
at SHA2561fc93e2b857a1b53a076886689cacd7f4a48226f01bf2dc406179e4b23ae9581.
The six earlier poses remain useful smaller regressions. Production never reads
vertices back; diagnostics may observe them for qualification only.

Report matrix code/helpers/private state, immutable input/output storage and
whole installed application sizes separately. Measure completed whole frames
for loop and unrolled paths under matched firmware/runtime/mesh/pose/display,
console, transport/pacing and C3/GP completion fences. Use at least the same six
poses in each arm, record samples/order/clock units and keep initialization
outside frame timing. Distinguish emulation wall time from physical performance.
A slowdown does not fail correctness or compactness; disclose it without tuning
projection algebra or silently changing the benchmark.

The consumer retains its accepted bounded installation transport and host-owned
loading/updating/calls/fences/pacing/cleanup. One root invocation per frame and
unchanged float controls remain sufficient. No runtime-pair substitution is
inferred. Stage and qualify the consumer's Oryx visual fixture before requesting
human review; the Author launches it, and the agent retrieves saved results.

## Authority and publication gate

The Author's direct task/contract negotiation and freeze-commit/push instruction
was verified in the sending chat. Mutual acceptance must cite this revision and
its exact content hash. The scoped freeze publishes only this task, contract,
baseline/acceptance provenance and its task-register entry; it excludes pending
GOLEM-049 code/docs, unrelated emulator/runtime work and all fsim changes.
Implementation begins only after the freeze is actually committed AND pushed,
the remote hash is verified and a durable receipt is sent to fsim-wireframe.
No additional permission round is needed for agreed implementation then.

Implementation remains subject to the normal independent consumer/human review
and explicit publication gate. The documentation freeze does not itself approve
publishing code or declare prior GOLEM-049 human acceptance. A semantic collision
or material scope change returns to mailbox negotiation before affected work.
