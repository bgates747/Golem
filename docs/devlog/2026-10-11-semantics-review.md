# GOLEM-009 / SEM-001 — Semantic review delivered

The Author requested committing/pushing the accumulated work, then executing
SEM-001 alone. Checkpoint `c83463b` was pushed to
`origin/feature/standardized-deployment`. SEM-001 now has a source-backed report
and reference evidence in its task silo; it awaits Author review. REV-001 and
implementation remain pending.

1. OUT-001: The report maps EX-001 through EX-021 to maintained Golem support,
   Rally intrinsic domains, selected design and recommendations. It includes
   integer/fixed/float conversion, rounding, shape/alias constraints, comparisons,
   finite resident control flow, function parameters/results, scope and ownership.
2. OUT-002: Recommendations awaiting review are truncating checked conversions,
   resident destination-preserving failure, ByVal/caller-owned results, lexical
   scope without shadowing and definite assignment for new locals. Arithmetic
   overflow, handler syntax, ByRef and composite type annotations remain open.
3. OUT-003: Proposed scalar examples now use Bounds; EX-006 retains its executable
   legacy Range spelling. Later review examples receive EX-018 through EX-021
   identifiers. Range prototypes use selected colon spelling and For/In, superseding
   the earlier alternative counted-loop proposal. Historical SYN-001 evidence is
   explicitly labelled as a snapshot rather than claiming its hash is current.
4. OUT-004: Verification checked 21 unique sample anchors, 18 exact-rational/host
   binary32 reference cases, local report links, canonical counted headers and
   unchanged executable EX-006 source. The numeric references include fixed-grid
   precision/range changes, float precision loss, negative rounding, range
   wraparound and the existing example's 1001-value quotient domain. Validation
   timing and source hashes are in semantics-evidence.json. These references do
   not execute or qualify new compiler semantics. No compiler/firmware/emulator
   source changed and no runtime suites were rerun.

The report's four DEC identifiers organize the next review. They do not promote
new implementation work automatically. GOLEM-010/011 remain deferred; GOLEM-002's
focused callback notes do not establish general native-loader feasibility.

## Conversational review follow-up

The Author accepted truncating conversions with resident destination-preserving
failure, ByVal default parameters, caller-owned composite results, lexical scope
without shadowing, definite assignment for new locals, For counter ownership/last
value, and an initially acyclic call graph. Integer / and % were accepted with
truncation toward zero and matching remainder; zero/overflow use resident errors.
The Author also requested quotient and remainder together. The design now includes
paired divmod, EX-021 uses it once and EX-022 demonstrates signed results.

These choices are promoted to the living language document; general handler
syntax, arithmetic overflow and remaining object ABI details are still open.
The original SEM-001 evidence is retained as a delivery snapshot, with an explicit
review amendment for the updated examples/report. Reference checking covers
1040 signed numerator/denominator pairs, including all sign combinations of 17
and 5; this is mathematical checking, not compiler or firmware qualification.
No implementation code changed.
