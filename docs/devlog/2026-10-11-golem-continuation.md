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

## GOLEM-004 accepted; GOLEM-005 parser tranche delivered

The Author authorized the recommended next parser tranche. Disposed GOLEM-004
as an accepted review and removed it from TODO; its grammar and source findings
provided the implementation contract. Promoted GOLEM-005 before changing code.
The review's allocator/resident integration and firmware qualification slices
remain separate proposed increments, not additional implementation in this task.

Added maintained `src/token_reader.hpp` with half-open source spans, line/column
errors, case-insensitive keyword matching, preserved identifier spelling,
complete decimal tokens and checked integer/float conversion. The maintained
ordinary parser now recognizes print, uint16 declarations/assignment/addition
and named For/Next without statement terminators. Newlines remain whitespace;
semicolons outside strings/comments reject with an explicit migration message.
Future control-flow keywords are reserved consistently but not implemented.
Oversized integers now exit with a compile error instead of an uncaught
std::out_of_range. Finite double subnormals remain accepted; double underflow
to zero, overflow and malformed numeric text reject explicitly.

Kept GOLEM-003's parser, backend, fixtures, frozen Rally oracle and historical
reports unchanged. The new experimental scalar frontend/CLI and floor source
live in GOLEM-005's silo and use the same maintained token reader, returning the
same scalar::Spec to the unchanged backend. This does not combine scalar and
ordinary allocation/output profiles or promote scalar emission into production.
Scalar bootstrap, cleanup, invoke and input-update/call bytes are unchanged;
only the interface manifest's experimental task label changes to GOLEM-005.

Host evidence is `docs/tasks/GOLEM-005/host-checks.json`. C++17 builds with
-Wall -Wextra -Werror pass. Address/undefined-behavior sanitizer checks compare
20 ordinary source pairs against the exact pre-tranche compiler and 22 scalar
pairs against GOLEM-003, including float/double subnormals and numeric bounds.
All command streams match. Sixty-nine rejection checks, source spans, casing
and zero initialization pass. Ten negative CLI cases exit 1 and preserve
existing output. Migrated hello and loop executable sources match bytes from
their actual pre-tranche Git source; scratch.golem remains a design sample with
unsupported str conversion. Commands, fingerprints and reusable build/run
durations are preserved in the report. Standard main and task-local Make builds
also pass. No firmware/emulator/hardware was run, modified or deployed.

Updated living design, handoff and maintained examples for the implemented
subset and documented reserved-name/semicolon migration. GOLEM-005 remains in
TODO with its concrete result awaiting review. The next proposed increment is
shared checked allocation and an explicit resident output profile; the existing
loop endpoint/sentinel limitations and scalar host-bound/nonreentrancy contract
remain as recorded in GOLEM-004/003. This tranche is left uncommitted.

## GOLEM-005 accepted and pushed; GOLEM-006 integration delivered

The Author requested committing/pushing all dirt, then proceeding on high with
the next shared-allocation/resident integration tranche. Verified GOLEM-005's
source fingerprints and committed all accumulated changes as `5cb1756`, then
pushed `feature/standardized-deployment` successfully before starting new work.
Disposed GOLEM-005 as accepted and removed it from TODO. Promoted GOLEM-006
before implementation; its detailed subtasks and acceptance evidence live in
`docs/tasks/GOLEM-006.md` and its silo.

Added maintained `src/buffer_allocator.hpp` and injected ownership into ordinary
Compiler state. Default ordinary output keeps accumulator ID 1 and allocation
from 2. Claims/reservations check collisions, external ordering, invalid/special
IDs and namespace exhaustion; source-located failures replace overflowing IDs.
Only materialized ordinary data is passed into resident installation/cleanup.
Maintained sources do not depend on task artifacts.

The GOLEM-006 scalar adaptation passes the same allocator to every named buffer,
scratch constant/matrix and helper target. An exact Rally compatibility policy
remains available for tests; normal resident IDs are relocated at emission time.
No blind modification of encoded bytes is used. Its task-local integrated CLI
reuses maintained ordinary parsing and has an explicit --resident mode alongside
the existing raw output contract. Complete counted-floor source combines
persistent uint16 globals, assignments/addition and print around one selected
scalar floor/conversion in one finite Sub. Resident loops/local declarations and
unsupported source forms reject rather than expanding the qualification scope.

The resident interface publishes input/output/entry/global metadata, external
reservations and every installed resource's role/size/kind. Installation clears
owned buffers and initializes ordinary globals once, then installs scalar data
and programs and sets initial input. It does not invoke the job. Update, call
and exact-owned cleanup stay separate. The example's 1120-byte installation
supports repeated fresh inputs with 21-byte update/call packets; ordinary state
and math then run on the VDP without more eZ80 command traffic. Output readback
is not included. Host validation of input and binary32 rounding, serialized
nonreentrant jobs and non-atomic file publication remain explicit contracts.

All host checks pass. The unchanged GOLEM-005 sanitizer regression retains 20
ordinary/22 scalar byte comparisons and 69 rejection checks. The new sanitizer
suite passes 22 complete scalar compatibility comparisons and 34 allocation/
integration rejection cases. Independent decoding validates ownership/spans,
carry landing bytes, relocated helper targets and a finite four-program graph
with logical depth at most three. Normalized scalar and mixed layouts preserve
operations while excluding reserved assets. Seven fixtures each pass 807 calls
in a bounded host model (5649 total), covering tiny negatives, signed bounds,
fresh inputs, persistent counters, uint16 wrap, unchanged code, reinstall reset
and exact cleanup with external assets preserved. Sixteen negative CLI cases
preserve existing output; raw output retains pre-tranche loop bytes and rejects
resident source. Standard Make builds pass.

Commands, source fingerprints and reusable durations are in
`docs/tasks/GOLEM-006/host-checks.json`. Read-only firmware/docs source fingerprints
and matching commits are in its provenance.json. GOLEM-003/004/005 artifacts and
their reports remain unchanged. The decoder/model is host evidence, not VDP
execution or qualification. No firmware/emulator/hardware was run, modified or
deployed. The experimental resident/profile code stays in its task silo; only
the shared allocator/ordinary ownership is maintained source. Updated living
design, README and handoff accordingly. GOLEM-006 remains in TODO awaiting review
and stock-firmware qualification before scalar/profile production promotion.
The new tranche is left uncommitted.

## GOLEM-006 accepted and pushed; GOLEM-007 stock emulator qualification

The Author requested committing/pushing all dirt, then proceeding with emulator
qualification. Confirmed GOLEM-006's tested source fingerprints still matched
the working files, committed the complete accumulated tranche and comparison
examples as `76458a9`, and pushed `feature/standardized-deployment`. Disposed
GOLEM-006's host integration as accepted and removed it from TODO; scalar
production promotion remains a separate decision. Promoted GOLEM-007 in TODO
before implementing its test tools.

The Pi inherited x86-64 Fab and ez80asm binaries. Built native ARM64 tools in
Golem-owned ignored directories: official Fab 1.2.5 detached at
`c22876d1f7ad903537c308cf65afd6553b37cdb8`, with its recorded submodules, and
assembler source copied from commit `5dc733c286b7864e3eb05ef93462c7e1637ba51e`.
Copied assembler C/header bytes match that commit. Used Fab's top-level Makefile
and left official checkouts/other profiles untouched. The canonical setup tool
generated Golem's `.emulator/` wrapper, pinned MOS 3.0.2 Arthur image/map and
isolated SD tree. Only the generated test binary is mapped into `/golem`, avoiding
a recursive project/SD mapping.

GOLEM-007's generic assembly guest streams a host-generated tape through MOS
and the emulated UART. A stock command-5 fetch patches a GP echo packet; two
tagged nibbles transport every possible byte without colliding with the wait
sentinel. Observations are saved through MOS and compared independently of
GOLEM-006's host model. No scalar math is performed on the eZ80. The Author
authorized serial-debug observations during this work. Inspection confirmed
that the stock `force_debug_log` path still calls an empty userspace `print`,
despite a working DBGSerial byte-write stdout path. GP supplied the required
observations, so no firmware patch or message to Tomm was needed.

The final stock-emulator run passes 855 checks across 150 resident calls and
9,204 observed bytes. Four packages cover pure/mixed jobs with default and
relocated layouts; 33 binary32 inputs each cover signed limits, fractional
neighbors, signed zeros and subnormals, including the smallest negative
subnormal. Counter persistence and explicit 65534/65535/0/1 wrap pass. Full
program bytes remain unchanged; live reinstall and reinstall after cleanup
reset state and restore the declared input. Controlled present/absent probes
pass for every owned buffer after two cleanups per layout, preserving root 0,
external canaries and the diagnostic asset. All 256 byte values round-trip
initially and after each layout. Reservations force IDs across 255/256.

The runner observes `/proc` to verify the actual loaded native module and Fab
executable, and checks runtime/source hashes before/after execution. Durable
report, raw observed/expected bytes, manifests and provenance live in the
GOLEM-007 silo. Final setup took 0.086 seconds with cached tools; execution and
retrieval took 22.634 seconds with process identity sampling. Comparable runs
before that sampling took about 12 seconds; neither is a speed benchmark.
Initial native tool builds were not timed. The qualified bundled stock VDP
revision is `1056ee38ec1cc68ed327bd32ee47d04b1c30597c`, distinct from the
canonical ESP32 revision consulted by GOLEM-006's host model. Physical hardware,
raster completion and complete heap/metadata balance remain unqualified.

Updated living status/design and retained the 1120-byte installation/21-byte
fresh-call application contract; diagnostic traffic is explicitly separate.
Launched the prepared graphical review through `.emulator/`'s canonical entry
on the Pi desktop. The finite guest prints a transport-completion message and
returns to MOS; human review should confirm the prompt and keyboard response.
Per the canonical environment AGENTS.md, all new emulator-related work remains
uncommitted and unpushed pending explicit human testing and approval. GOLEM-007
remains in TODO for that gate; scalar code is still experimental/task-local.

## Review-host correction: dedicated local Oryx instance, manual launch

The Author clarified that the Pi desktop is never available for viewing: it
has no mouse, keyboard or monitor. Human emulator reviews default to Oryx unless
the Author chooses another host. Agents must prepare a dedicated local Oryx
instance, leave it stopped, and provide Bash commands and review instructions;
do not launch graphical instances or attention alerts on the Author's behalf.
Updated the shared environment AGENTS/emulator guide and Golem's handoff to
replace the former launch-for-attention instruction with this explicit protocol.

Stopped only the previously launched Pi review process after verifying its
executable identity. Prepared Oryx's dedicated local Golem instance at
`/home/smith/Agon/mystuff/golem/.emulator`, with local x86-64 stock runtime
artifacts, tagged MOS 3.0.2, a canonical generated wrapper, isolated SD files and
the exact qualified eZ80 guest. Created its local project Python environment.
Verified native architecture, resolved runtime libraries, MOS hashes and exact
guest/expected-data/verifier correspondence to the Pi fixture. No emulator was
launched on Oryx. These are setup checks, not Oryx execution or human acceptance.

Added a small result verifier and documented manual launch, transport completion,
MOS `dir` keyboard check, close, and raw result verification. Oryx preparation
evidence is in GOLEM-007/oryx-review.json. GOLEM-007 remains pending human review
and permission to commit; the preceding automated Pi evidence remains valid.

## Oryx review passed; verifier instructions made explicit

The Author manually launched the dedicated Oryx fixture and supplied a screenshot
showing transport completion and the MOS prompt. Remote verification and the
Author's own verifier run both matched all 855 checks and 9,204 observed bytes.
The Author subsequently confirmed that typing `dir` inside the emulator returned
the expected directory listing and prompt. Human emulator validation has passed;
this confirmation did not explicitly authorize committing the tranche.

At the Author's request, replaced the verifier's vague human-confirmation message
with: "In the emulator window, type dir at the MOS prompt and report to the agent
that it works." Updated the review instructions to leave the emulator open while
running the Bash verifier, then return to the emulator for `dir`. Recorded the
successful human review in current task/status documents. No fixture, compiler,
firmware or emulator execution was changed or relaunched.

## GOLEM-007 accepted; production promotion authorized

The Author explicitly approved committing/pushing the successful qualification
tranche and proceeding with promotion of its scalar backend and resident mode
into the maintained compiler. Disposed GOLEM-007 as accepted, removed it from
TODO, and recorded the approval in its task/review evidence. The shared Oryx
manual-review protocol documentation is included in a separate, narrowly staged
agon-dev-env commit; unrelated environment work is preserved.
