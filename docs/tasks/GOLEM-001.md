# GOLEM-001 — Assess Rally functionality and propose a usable language surface

Examine the Rally compiler extension before choosing what to bring into main
Golem. The intended result is a reviewable recommendation for exposing useful
capabilities through readable Golem source, with evidence about implementation,
correctness and hardware cost. This task authorizes investigation and a written
proposal; implementation and integration follow a separate review decision.

Author clarification, 2026-10-11: the primary motive is reduced wire traffic.
Prioritize reusable resident code/data and small input/update/call interfaces.
Performance optimization is deferred; retain execution-time and stack findings
only as correctness constraints and limits on claims.

Overall task status is tracked in [TODO.md](../../TODO.md); stable subtask IDs
and their completion or disposition are tracked below. Compare
`/Agon/mystuff/golem-rally19` with this main project and recommend an incremental
path toward human-friendly access to Rally's implemented capabilities.

Task artifacts belong in `docs/tasks/GOLEM-001/`. Keep exploratory scripts,
examples and reports there until explicitly promoted into the production
package or maintained design documentation.

Investigation delivered 2026-10-11:
[draft recommendation](GOLEM-001/rally-capability-integration-proposal.md) and
[read-only evidence audit](GOLEM-001/evidence-audit.json). SRC items are supported
by the report's baseline/source index; INV items by its capability comparison,
examples, architecture and correctness analysis; DEL items by the report,
proposed phases and preserved read-only boundary. All investigation subtasks
below are complete. On 2026-10-11 the Author authorized a sensible implementation
tranche following review. The accepted direction is incremental, readable syntax
over selected Rally capabilities, with longer command sequences and a modest
compiler budget. The broader proposed language and five-phase roadmap remain
proposals. GOLEM-001 is disposed as an investigation; the authorized first slice
is tracked separately in [GOLEM-003](GOLEM-003.md). See the
[disposition record](../devlog/2026-10-11-golem-continuation.md).

## Author's buffer-layout preference — 2026-10-11

Prefer buffers containing one or more blocks, with each block holding a
sequence of commands. Use longer command sequences rather than requiring one
compiled instruction per block. A small procedure may fit in one block;
multiple blocks are available where useful for addressing and packaging.
Buffer IDs can identify callable procedures or resident jobs, and execution
falls through successive blocks automatically.

This records the Author's preferred direction, not a finalized layout
specification or approval of the entire integration proposal. Exact block
boundaries, label addressing and patched operand locations remain to be
defined. Implementation has not been authorized by this preference alone.

## Design considerations from review — 2026-10-11

Prefer a modest compiler that makes resident VDP work readable and reduces
recurring wire traffic. Longer command blocks are a practical encoding choice;
they do not require a sophisticated optimizer. The source review supports lower
allocation and block-management overhead, but provides no measured speedup.
These considerations qualify the linked draft recommendation; they do not
authorize implementation or settle the final language and addressing model.

### CON-001 — Preserve the original purpose and control compiler scope

The original direction was already a structured, relatively thin language over
the Buffered Commands API, with resident code and runtime helpers avoiding
eZ80/VDP round trips. Main already implements declarations, expressions and
`For` loops. Readable source is therefore a continuation of that direction.

The initial integration report proposes a broader surface: reusable typed
lowering, expressions and conversions, structured conditions, and typed
records/tables. Those remain candidate increments rather than a requirement to
implement them all before producing something useful. A smaller first slice
could expose selected Rally numeric operations through explicit, readable
intrinsics and named finite procedures, preserving existing main features.
General function frames, automatic range inference, elaborate temporary
management and broad optimization can wait. Correct operand types, bounds,
resource ownership and bounded execution still need explicit rules.

### CON-002 — Long generated command sequences need no smart optimizer

Rally emits a finite hosted program's command sequence into one block. A long
sequence can simply be the concatenation of emitted commands; its length does
not demonstrate advanced optimization or imply complicated source syntax.
One source operation may expand into several commands, especially for numeric
conversions, matrix operations and patched addressing.

For scale, the inspected Rally application map contains `commitState` at
7,720 bytes and `renderProof` at 7,506 bytes. Its 115,526-byte bootstrap is an
aggregate installation stream, not one executable block. Rally's explicit
`InlineCalls` facility selectively expands calls to reduce call depth; this is
a separate choice from storing consecutive commands together. Long blocks
alone neither prove efficient arithmetic nor remove nested calls.

### CON-003 — Distinguish instruction, block, buffer ID and call

The early design proposes one compiled instruction per block, with the block
index serving as an address. Multiple blocks can belong to the same buffer ID.
The firmware's `MultiBufferStream` falls through their bytes in sequence;
crossing a block boundary does not inherently execute a call or add a stack
frame. A design using a separate buffer ID and explicit call for every
instruction would have additional costs, but that is a different layout.

Main and Rally already emit command sequences rather than enforcing the early
one-instruction-per-block proposal. Source syntax, intermediate instructions,
physical block placement and procedure call structure should remain separate
design decisions.

### CON-004 — Performance, memory and addressing tradeoffs

For identical command bytes and control flow:

| Consideration | One instruction per block | Longer blocks |
|---|---|---|
| Sequential execution | More transitions between streams and position resets | Fewer transitions; each command is still interpreted |
| Memory and installation | A payload allocation, stream object, ownership bookkeeping and vector entry per instruction | Overhead shared across more commands; fewer block creations |
| Branch addressing | Instruction/block index is a convenient target | Labels need calculated byte offsets within blocks |
| Call stack | Determined by real calls, including tail-call handling | Grouping commands alone does not change call depth |

There are additional implementation details worth retaining without turning
this task into an optimization project. Entry into another buffer constructs a
`MultiBufferStream` and copies its vector of block references, so a buffer with
many blocks also has more entry bookkeeping. Seeking by a byte offset spanning
many blocks scans their lengths; supplying the target block index and its
internal offset avoids that preceding scan.

Grouping straight-line sequences, or whole finite procedures where suitable,
is the preferred candidate. Compiler labels and a layout/fixup pass can compute
branch and patch offsets without sophisticated optimization. Preserve useful
boundaries for control flow, packaging and the 65,535-byte payload limit of an
ordinary write-block command. The exact grouping and addressing policy remains
to be selected, including keeping patched operand locations correct.

### CON-005 — Separate installation traffic from recurring traffic

An ordinary write-block command has an eight-byte header. For an illustrative
1,000-byte sequence consisting of 100 ten-byte instructions, direct uploads
cost 1,800 bytes as 100 blocks versus 1,008 bytes as one block. This excludes
common setup/clear commands. Compression and split/spread packaging change the
accounting and must be counted using the actual loader.

Once installed, both layouts can be invoked with the same short host call.
Block grouping alone therefore does not reduce recurring wire traffic. The
main saving comes from installing reusable resident code/data and then sending
small input updates and calls instead of retransmitting expanded operations.
Measure installation and recurring bytes separately, including amortization
over repeated invocations. Large generated payloads should be assessed against
that budget rather than rejected just because they are long.

### CON-006 — Execution lifecycle and evidence limits

Block boundaries do not yield to the VDP's outer event queue. A long-running job
must still return at appropriate points so normal event processing can resume;
splitting the same non-returning job into many blocks does not fix starvation.

The block-overhead conclusions above follow from the inspected stock firmware
at `c7ac293d2aa81ddfa693390549bcd909069c8fc3`, particularly
`video/multi_buffer_stream.h` (`getBuffer`, `seekTo`, constructor),
`video/buffer_stream.h`, `video/buffers.h`, and `video/vdu_buffered.h`
(`bufferWrite`, `bufferCall`, `bufferJump`). They are structural findings, not
timing measurements; no speed multiplier or allocator-byte total is claimed.
The Rally map example comes from
`/Agon/mystuff/AgonArcade-rally19/docs/tasks/RALLY-19/.work/frontend/hardware-inline/oval.vdp.map`.

Native PC-compiled Xtensa modules remain a separate research direction under
[GOLEM-002](GOLEM-002.md). They are not a prerequisite for this readable,
resident buffered-command approach.

## Investigation plan and completion record

### Scope and sources

- SRC-001 [x] Read both project handoffs and inspect current branches, commits, worktree
   relationships and working-tree changes. Establish the actual common ancestor
   and divergence; do not assume the handoffs describe current implementation.
- SRC-002 [x] Read main Golem's living language proposals, compiler, examples and relevant
   devlogs. Distinguish accepted language decisions from tentative ideas and
   functionality actually implemented.
- SRC-003 [x] Examine Rally's hosted/resident design documents, compiler, examples,
   regression tests and evidence. Consult the `rally_golem` cross-agent contract
   and sibling `AgonArcade-rally19` delivery/review records for integration and
   hardware findings. Use stock Buffered Commands documentation and firmware
   source to resolve lowering questions.

### Investigation

- INV-001 [x] Build a capability comparison covering declarations and types, arithmetic,
   matrices/transforms, conversions and rounding, conditionals and bounded
   loops, callable programs, assets/tables, indexed access, validation, storage
   allocation, resource limits, diagnostics, maps and cleanup. For each item,
   identify main support, Rally support, implementation/test evidence,
   restrictions and dependencies. Clearly separate implemented, proposed,
   emulator-verified and hardware-verified behavior.
- INV-002 [x] Assess usability using representative Rally source. Treat the concern that
   its syntax is unfriendly as a hypothesis to examine with concrete examples.
   Identify manual buffer IDs, offsets, field layouts, scratch matrices,
   explicit calls/copies, validation plumbing and other platform details users
   must currently manage. Distinguish essential low-level control from details
   the compiler or a library could reliably manage.
- INV-003 [x] Propose readable equivalents for scalar computation/conversion, matrix or
   geometry operations, a bounded loop with a conditional call, and validated
   table/record access. Show existing compilable Rally code beside proposed
   source, explicitly labeling proposed syntax. Explain types, allocation,
   bounds, execution lifecycle and expected lowering. Respect main Golem's
   accepted conventions; flag conflicts for user decision rather than silently
   replacing them.
- INV-004 [x] Compare practical approaches: library wrappers, expression/control-flow
   syntax over existing lowering, and selective compiler refactoring or code
   transfer. Recommend the smallest useful combination. Explain whether hosted
   and ordinary modes should remain distinct and how explicit low-level access
   should coexist with the proposed default surface.
- INV-005 [x] Assess correctness and performance independently of syntax. Include bounded
   execution and return to the VDP event loop, physical call-stack limits,
   integer widths/overflow, float admission and rounding, patch-then-execute
   safety, resource ownership and external ABI compatibility. Account for
   Rally's reported poor physical performance; simpler source alone must not
   be presented as faster generated code.

### Deliverable and review boundary

- DEL-001 [x] Write `docs/tasks/GOLEM-001/rally-capability-integration-proposal.md`, marked DRAFT,
   with an executive summary, capability comparison, before/after source
   examples, recommended architecture, alternatives and open user decisions.
   Record examined commit identities and link evidence to files and tests.
- DEL-002 [x] Include a small sequence of implementation-sized phases, with dependencies,
   compatibility choices, acceptance criteria and proportionate validation.
   Prioritize one useful end-to-end example before broad language expansion.
   Specify which existing tests can be reused and where eventual emulator and
   hardware review are required. These phases are proposals, not additional
   active task checklists.
- DEL-003 [x] Keep investigation read-only outside the task silo and task tracking documents.
   Do not merge/cherry-pick, alter either compiler, regenerate Rally artifacts,
   deploy firmware or launch emulator/hardware tests as part of this task.
- DEL-004 [x] Stop after presenting the recommendation for user review. Do not begin its
   proposed implementation. Record eventual acceptance or rejection in the
   dated devlog before removing this task from the register.
