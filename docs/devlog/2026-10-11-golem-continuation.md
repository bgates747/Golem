# Golem continuation: review disposition and first implementation tranche

GOLEM-001 delivered the Rally capability comparison and readable-syntax
recommendation. The Author authorized proceeding with a modest implementation
tranche after recording a preference for buffers containing command sequences
in one or more blocks. The first promoted work is GOLEM-003: a task-local scalar
floor/conversion frontend over extracted Rally emission, with host verification
and explicit wire accounting. Production promotion and firmware execution are
not part of this first tranche.

## GOLEM-001 disposition

Completed investigation, accepted as the basis for a narrower incremental path.
Evidence is preserved in `docs/tasks/GOLEM-001/` and review considerations in
`docs/tasks/GOLEM-001.md`. The Author's priority is reduced recurring wire
traffic, with performance optimization deferred. Reuse selected Rally machinery
behind readable source, preserving a modest compiler budget. Longer command
blocks do not require smart optimization, and block boundaries are distinct
from calls and event-loop yields.

This disposition accepts that direction and the first prototype, not every
proposed type, grammar spelling, architecture or phase in the draft report.
Remove GOLEM-001 from the active TODO; retain its stable ID and evidence here.
GOLEM-002 remains queued for native-module research. GOLEM-003 is the immediate
continuation, ahead of that separate research task.

## Checkpoint and next steps

The Author requested committing and pushing all current dirt before proceeding.
That checkpoint includes the existing deployment wrapper/configuration, project
task conventions, GOLEM-001 investigation and review notes, GOLEM-002 research
plan, and the GOLEM-003 plan. The deployment files are included as requested;
they have not been deployed or validated on an emulator/hardware in this work.

GOLEM-003 will trace one useful calculation from readable source through a typed
emitter to resident stock-VDP commands. It will preserve Rally as an oracle,
verify matched command bytes and failure cases on the host, and count initial
installation separately from repeated input updates and calls. All new
experimental files stay in its task silo.

## GOLEM-003 first tranche result

The initial checkpoint was committed and pushed as `a8e3ff8` on
`feature/standardized-deployment`. The task-local C++ prototype now accepts
ordinary float/signed-word declarations and a finite named job containing
`result = sint16(floor(x));`. A typed backend, separate from both parsers,
extracts Rally's scalar load, true-floor correction and signed-word conversion.
No Rally source translation or language-wide IR/optimizer was introduced.

The generated installation is 1,008 bytes, the root command block is 259 bytes,
and a repeated input update/call is 21 bytes. Twenty complete bootstrap/cleanup
comparisons match the frozen Rally oracle, including tiny negative and subnormal
input, signed zero, extrema and narrowed domains. Strict host sanitizer builds,
the real production compiler's compatibility baseline, the preserved Rally
binary32-integer boundary model and an independent command decoder pass. The
decoder checks ownership, spans, conditional helper targets and finite call
structure; negative CLI cases preserve existing output files.

Evidence, provenance, commands and durations are in
`docs/tasks/GOLEM-003/host-checks.json` and its `reference/provenance.json`.
The prototype contract and limitations are in its README. There was no VDP
firmware execution or emulator/hardware deployment, and the maintained compiler
and Rally worktree are unchanged. README, living design and handoff now reflect
the Author's longer-command-block preference rather than repeating the early
one-instruction-per-block assumption.

GOLEM-003 remains in TODO pending review of this concrete result. The next
increment is an agreed integration of this small syntax/backend boundary with
main's frontend and storage policy, plus firmware qualification before production
promotion. Broader expression syntax, module linking and native Xtensa research
are not prerequisites. The output stays resident; the 21-byte recurring count
does not include result readback or another application's use of that output.

## Terminator convention revised; implementation deferred

The Author approved semicolons for ordinary statements only: declarations,
assignments, calls and output. Structural headers, branch markers and closers
(`Sub`/`EndSub`, `If`/`Else`/`EndIf`, `For`/`Next`, `Do`/`Loop`) do not use
semicolons. This supersedes the earlier every-statement rule in design Axes 5
and 15. Newlines remain whitespace; there is no automatic semicolon insertion.

Updated the living specification and readable design examples. The maintained
compiler, prototype parsers and executable `floor.golem` fixture are unchanged,
as the Author explicitly requested no implementation yet.

### Follow-up: remove all statement terminators for now

The Author subsequently chose to remove terminators from ordinary statements
as well, keeping newlines insignificant. The parser should recognize complete
statements from grammar; concrete ambiguities will be addressed through parser
improvements as they arise. `print "foo" print "bar"` illustrates two statements
on one line. This supersedes the intermediate ordinary-only semicolon decision
above. Updated design Axes 5 and 15, handoff and `examples.golem`; compiler code
and executable fixtures remain unchanged under the earlier implementation deferral.

### Indexed switch decision

The Author accepted `Switch selector Range(low, high)` as the initial explicit
jump-table form, with inclusive consecutive slots starting at any representable
integer value. Evaluate once and bounds-check before indexing. Missing cases
silently fill with the default target, or the continuation after the switch
when no default exists. Duplicate/out-of-range cases and oversized tables fail
compilation. Recorded in the living design and readable examples; no compiler
implementation was performed. A non-unit step argument was subsequently raised
as a possible extension and is recorded as unselected, requiring an additional
alignment check.

### Shared Range syntax prototypes

At the Author's request, wrote task-local `GOLEM-003/range-prototypes.md` and
added readable examples for ascending/descending stepped iteration, membership,
stepped indexed switches and an illustrative `Bounds` declaration. Recorded
inclusive limits, alignment, finite counter arithmetic, sparse-case fallback
and proposed edge-case rules. Distinguished integer sequences from continuous
float bounds; additional syntax and the bounds naming remain proposals. No
compiler implementation or executable fixture changes were made.

## GOLEM-004 promoted for review

The Author requested a task for the grammar/integration review, committing and
pushing all accumulated documentation first, then proceeding with that task.
GOLEM-004 will reconcile the terminator-free source convention, current control
flow and Range decisions with the maintained parser and scalar prototype. It
will provide concrete grammar, allocation, compatibility and validation rules
for the next small integration slice. The previous parser implementation deferral
remains in effect; this task authorizes review and read-only source probes.

### GOLEM-004 review delivered

Committed and pushed the accumulated syntax decisions and task plan as
`c1483c2`, then completed the grammar/integration review. The deliverable is
`docs/tasks/GOLEM-004/grammar-integration-review.md`, with proposed minimal grammar,
boundary/diagnostic cases, a shared allocation contract, output-profile handling
and three implementation-sized slices. The living design now reflects ElseIf
and both pre/post-condition Do-loop preferences; stale main compiler/build
descriptions and the competing historical handoff checklist were corrected.

Eighteen current-parser probes build existing C++ sources with strict warnings
and record behavior in `parser-observations.json`. Newline insignificance is
already supported; explicit semicolon checks reject new source. Both main and
the scalar prototype allocate private IDs from 2, preventing safe concatenation.
Main accepts future keyword identifiers and a 65535 loop endpoint; the review
records compatibility and stock conditional-sentinel implications. A huge
integer literal aborts through an uncaught std::out_of_range, identifying a
checked-conversion requirement for the parser tranche.

Reviewed compiler fingerprints are unchanged. No proposed parser was implemented,
no numeric emission or executable fixtures changed, and no firmware/emulator/
hardware was run or modified. GOLEM-004 remains in TODO for review acceptance;
the immediate proposed code tranche is the shared token reader and
terminator-free existing ordinary/scalar source forms, before allocator and
resident-output integration. New control-flow backends and Range extensions
remain separate increments.
