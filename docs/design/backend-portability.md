# One Golem source, interchangeable VDP execution backends

The Author requires one Golem language and reusable source for stock Buffered
Commands API execution or native VDPlet execution. Developers can select a target
for local requirements; an application can ship both versions and let end users
choose, including at runtime. Shared language semantics, persistent storage and
entry interfaces must be designed before backend-specific emission. This is an
accepted direction recorded on 2026-10-12; the mechanisms below are requirements
and a staged roadmap, not claims of implemented dual-backend support.

## Requirements

- REQ-001: A portable source program has the same declarations, expressions and
  procedure bodies for either target. Backend selection belongs to compiler/build
  configuration and application dispatch; portable source needs no Native marker.
- REQ-002: Parse and type-check portable source through one frontend and shared
  typed representation. Implement target-specific emission behind that boundary;
  do not create independently evolving native and buffered dialects.
- REQ-003: Portable operations preserve agreed types, evaluation order, numeric
  conversions, Bounds, error behavior, results and persistent-state effects on
  both targets. Define and qualify floating behavior explicitly; similar-looking
  computations or host-only models do not establish backend equivalence.
- REQ-004: Publish one target-independent data and call interface: field types,
  offsets, initial bytes, entry identity and parameter/result layouts. Allocate
  shared data before code and backend-private scratch; a target choice must not
  move globals. Retain explicit external buffer reservation accounting.
- REQ-005: The application owns initial installation, updates, invocation, result
  consumption, pacing and cleanup. Existing named assembly-call conventions
  continue through a selected dispatcher. These calls enqueue work; returning
  on eZ80 alone is not proof of VDP completion.
- REQ-006: A portable distribution may contain both compiled implementations and
  common initialized data. Install shared state once. Backend installation,
  removal or replacement must preserve it unless the application explicitly
  requests reset. Backend-private resources have separate ownership/cleanup.
- REQ-007: Stock VDP can run the buffered implementation. Runtime-enabled firmware
  can offer either implementation if its capabilities/resources suffice. Native
  execution requires the loader-enabled firmware; selection never silently
  flashes or changes firmware.
- REQ-008: End users may choose a path through application configuration/startup
  controls. Later live switching occurs only after the outstanding invocation
  completes, with the same compatible interface and persistent state. Explicitly
  synchronize before rebinding calls or unloading code; no mid-routine switching.
- REQ-009: Automatic availability detection needs a qualified stock-safe protocol
  and bounded waiting, or use an explicit user setting. The existence of D8/D9
  commands alone does not prove it is safe to probe unsupported firmware. A busy
  native slot must not cause the application to evict another owner's module.
- REQ-010: Native-only extensions need explicit target requirements and diagnostics.
  A portable build must reject unsupported operations; it cannot silently change
  algorithms, omit work or label an unqualified native-only feature portable.
- REQ-011: Runtime selection uses precompiled artifacts; it does not compile
  source on the Agon. Selecting a whole module first avoids automatic instruction
  partitioning and repeated cross-backend transitions inside a routine.
- REQ-012: Compare implementations against shared reference cases, including
  invalid inputs, failed destination preservation, earlier committed effects,
  repeated calls and state-preserving changes of backend. Report installation,
  steady-state wire traffic, execution and presentation measurements separately.
- REQ-013: No automatic replay after a routine has begun and failed. Earlier
  writes/drawing may have happened; falling back and rerunning can duplicate
  effects. Availability fallback before executing a job is a separate policy.

## Roadmap and current limits

This table describes dependencies. `TODO.md` and its linked task plans own
execution status and checkboxes; this is not another actionable task register.

| Stage | Deliverable | Boundary |
| --- | --- | --- |
| Shared foundation | Common typed frontend, deterministic initialized state layout and target requirements | GOLEM-024 accepted; task-local prototype |
| First portable routine | Same checked uint16 source lowered to buffered commands and a native package | GOLEM-025 accepted task-local emitters; stock/actual-Xtensa and saved human results pass |
| Common language growth | Expressions, conversions, floats, conditionals, loops and routines, incrementally qualified on both targets | A supported syntax/type does not imply every operation on it is portable |
| Startup selection | Dual-artifact packaging, named-call dispatch and explicit user choice; qualified capability detection when available | Stock buffered path remains usable without the native loader |
| Live selection | Quiescent backend change with state/ownership preservation and fresh native handle binding | No mid-call replacement, implicit state reset or error replay |

Generated C remains the preferred native route to investigate. Native-only
compilation can be an interim development artifact, but source compatibility
and shared storage must remain first-class constraints. The maintained compiler
currently has separate small resident profiles rather than this shared frontend;
it has no native output, dual-package dispatcher or live switching implementation.

GOLEM-024's first task-local prototype now parses the same scalar source into a
typed model and publishes identical common layout/data/assembly artifacts for
both target selections, including relocated reservations. Sanitized host reference
and real assembler checks pass. Its target descriptors explicitly state that no
executable backend is emitted or qualified; this establishes the shared foundation
without claiming portable execution. See [the prototype report](../tasks/GOLEM-024/README.md).

GOLEM-025 adds task-local executable uint16 emitters to that shared model.
Initialized globals, Bounds and named Subs with checked + / - / * assignments
compile into buffered commands or generated C packaged by the native SDK. Both
implementations coexist with disjoint private ownership and one common state
buffer. Stock VDP and actual Xtensa checks pass; a labelled moving square/meter
alternates backends without resetting coordinates or ticks. The application
explicitly selects and binds calls; no capability probe, automatic replay or
mid-call switching is added. Float output and production promotion remain later
work. See [the executable tranche](../tasks/GOLEM-025/README.md).

GOLEM-026 extends the task-local integer subset with all six uint16 comparisons
and structured If/ElseIf/Else/EndIf. Both emitters preserve left-to-right
condition evaluation, skipped branches and whole-invocation failure exits.
Stock VDP and actual Xtensa JIT/interpreter qualification pass; the single tick
animation now lets Golem choose direction while assembly retains pacing and
drawing invocation. The Author accepted the visual review and saved human results
pass; explicit commit/push approval was given on 2026-10-13. Native/stock packages share a
new explicitly installed 20-byte layout; this does not migrate an older installed
state automatically. See [the conditional tranche](../tasks/GOLEM-026/README.md).

GOLEM-027 adds bounded For/Range loops to the task-local uint16 subset, preserving
inclusive range semantics, optional Next names and the last visited counter.
Ranges are computed on the PC; both backends reject body writes to active counters
and cap worst-case source work. Full uint16 traversals, selected/skipped failures,
nested loops and relocated buffers pass stock/actual-Xtensa qualification. The
visual performs four simulation steps per host invocation, showing how a batch
can reduce arithmetic call traffic. Both paths share a new explicitly installed
22-byte layout. The Author reports the review worked and saved results pass;
explicit commit/push approval was given on 2026-10-13. Production promotion, runtime
ranges, other counter types and source drawing remain later work. See
[the loop tranche](../tasks/GOLEM-027/README.md).

GOLEM-028 extends the task-local model with stored Boolean values and uint16
comparison expressions. Literal/copy/comparison assignments and Boolean If/ElseIf
are qualified on both backends using one canonical two-byte 0/1 layout and type
tag 6. Invalid external encodings are rejected; numeric coercion and compound
Boolean operators remain unsupported. A distinct traffic-light review exposes
those stored decisions visually. The Author accepted the visual review and
saved human results pass. Explicit commit/push approval was given on 2026-10-13.
See [the Boolean tranche](../tasks/GOLEM-028/README.md).

The first research proposal's whole-module target choice remains useful. Its
suggestion of making buffered support a later optional concern is superseded by
REQ-001 through REQ-004: the shared contract is required from the outset, even
while emitter implementation is staged.

## References

- [Native integration research and remaining questions](../tasks/GOLEM-023.md)
- [First shared implementation task](../tasks/GOLEM-024.md)
- [Existing native research report](../tasks/GOLEM-023/README.md)
- [Maintained resident assembly contract](resident-scalar.md)
- [VDPlet runtime contract](../../../agon-vdplet/docs/API-reference.md)

GOLEM-029 adds task-local Boolean Not/And/Or to both targets. Precedence is
arithmetic, comparison, Not, And, Or, with explicit parentheses. And/Or short
circuit left to right; skipped arithmetic cannot raise runtime errors. Selected
failures preserve the Boolean destination and stop the invocation, including
through buffered logical helper frames. All external globals are still admitted
at entry. Boolean storage and the shared assembly interface remain unchanged.
See [the compound-Boolean tranche](../tasks/GOLEM-029/README.md) for qualification
and accepted human review. The Author explicitly approved commit/push on 2026-10-13.
Production compiler and generic SDK are unchanged.

GOLEM-030 extends the task-local uint16 subset with paired divmod assignments.
Distinct initialized uint16 destinations receive quotient and remainder together
only after both arguments evaluate and both outputs pass destination Bounds.
Arguments evaluate once left to right and may alias outputs. Zero divisor is
status 4; arithmetic and Bounds errors retain both outputs and prior effects.
The existing qualified buffered kernel and native division share the same
source, layout and named assembly contract. The visual uses quotient for phase
and remainder for green blinking. See [the division tranche](../tasks/GOLEM-030/README.md)
for qualification and human-review status. Production compiler and generic SDK
remain unchanged.

GOLEM-031 adds task-local signed 16-bit arithmetic and explicit signed/unsigned
conversions to both targets. Shared two's-complement storage uses signed tag 2.
Same-type comparisons follow numeric signed ordering; conversions preserve
numeric values and reject out-of-range results with status 32. Each arithmetic
intermediate is checked; failure preserves its destination and prior effects.
For counters and divmod remain uint16. The visual demonstrates signed movement
and conversion into screen coordinates; source, qualification and human-review
status are in [the signed tranche](../tasks/GOLEM-031/README.md). Production compiler
and generic SDK remain unchanged.

GOLEM-032 adds task-local float32 arithmetic/comparisons and explicit integer
conversions to both targets. The bounded profile admits finite normal inputs
and signed zero, rejects non-finite/subnormal inputs with status 33, canonicalizes
expression zero and flushes subnormal results to zero. Non-finite results also
report 33. Integer-to-float conversions are exact; float-to-integer truncates
toward zero, checks numeric range and preserves failed destinations. Bounds use
representable thresholds that preserve their inclusive numeric meaning. Stock
uses matrix operations; native uses uncontracted binary32 arithmetic. The tested
rounding and underflow edges match across stock and actual Xtensa engines; full
IEEE exception/subnormal support and physical qualification are not claimed.
See [the float tranche](../tasks/GOLEM-032/README.md) for source and evidence.
Production compiler and generic SDK remain unchanged.

## Accepted Rally integration planning contract — R25-G0 v0.2

Rally accepted all eight clauses of Golem's counterproposal without amendments
on 2026-10-13. [Accepted requirements and ownership](../tasks/GOLEM-033/accepted-contract.md)
are the reconciled planning record; the original proposal and immutable peer
acceptance are linked there. GOLEM-033 negotiation is complete. The Author subsequently released D0/GOLEM-034;
firmware work, capacity probing and later game implementation remain gated.

D0 pins reproducible compiler/artifact identities and the shared root interface;
D1 adds bounded typed arrays and faithful explicit wide integer math; D2 composes
generated Golem C and Rally renderer/helpers into one native module; D3 requires
a separately owned serialized firmware scheduler, complete snapshots and
collision-only event transport. Source/storage/error semantics remain common
across supported targets; unavailable buffered helpers require explicit rejection.
No silent floating substitution or implicit widening is permitted for game math.
Current admission limits are policy, not established physical memory ceilings.

The authoritative TODO links GOLEM-034 through GOLEM-038. Their detailed plans
preserve implementation-sized reviews, independent Rally oracles and existing
human validation gates. Snapshot interpolation/prediction/staleness, recovery
policy, firmware ownership and any admission-limit change remain explicit decisions.

GOLEM-034 delivers a task-local frozen GOLEM-032/25901e5 compiler closure plus
an agreed R25-G0/root-v1 layer. The common 112-byte header and generated globals
layout are identical for buffered/native packages; retained descriptor bytes are
fixed build-time bindings. Native validates actual lengths by SDK leases; stock
depends on assembly's installed shape/lifetime invariant. Generation/entry/session
advance only after successful source return. Runtime results remain host owned;
no status field proves rollback, snapshot synchronization or scheduling.
See [D0 source, immutable delivery and qualification](../tasks/GOLEM-034/README.md)
for exact interfaces, resources and pending human review. Production src and the
generic SDK/runtime are unchanged.

GOLEM-035 adds retained one-dimensional uint16/sint16 arrays to both task-local
backends. Inclusive numeric indices, Fill/Values or exact build-supplied initial
data, ReadOnly source permissions and index-before-RHS stores are shared. Logical
index failure is language status 5; earlier effects remain. Application-owned
array buffers survive backend changes/unload/private cleanup. Optional generated
array descriptors extend manifests without changing root-v1; native checks actual
shapes/ranges and stock depends on the host-installed invariant. No array-content
scan, per-element host updates or runtime readback verification is required.
See [array source/qualification](../tasks/GOLEM-035/README.md) for limits and pending
human review. Wider world-coordinate math and renderer linking remain later work.
