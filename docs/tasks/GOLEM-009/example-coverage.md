# GOLEM-009 / INV-002 — Existing example coverage

Comparators, conditionals and loop forms already have readable examples and
should be retained. Most Rally math is absent from `examples.golem`; some of
its syntax is illustrated elsewhere, so those additions can reuse existing
proposals. Of the 32 reconciled inventory rows, the central file covers 3,
partly covers 3 and lacks examples for 18. Another 3 concern host/tooling
contracts, 1 is parked optimization work and 4 are deferred. These are grouped
capability counts, not counts of distinct functions or required new keywords.
**INV-002 is complete and awaits Author review before SYN-001.**

Only this coverage report and task status changed. No examples, syntax rules,
compiler code, emulator configuration or deferred tasks were edited.

## Sources and interpretation

The central review file is
[examples.golem](../GOLEM-003/examples/examples.golem), currently 174 lines.
It is a collection of independent snippets, not one compilable program; repeated
variable names across snippets are intentional. Coverage here means the source
notation is demonstrated, not that a backend exists or every numeric edge case
is tested. Missing implementation alone is not a syntax gap.

| Key | Evidence |
| --- | --- |
| E | [Central examples](../GOLEM-003/examples/examples.golem): comparisons at line 6; If/ElseIf/Else at 21; Switch at 33; dispatch at 50; Range at 74; membership at 90; numeric bounds at 116; counted loops at 121; Do/Loop forms at 141. |
| F | [Maintained floor example](../../../examples/resident/floor.golem): bounded input, zero-default output, Sub and explicit floor/conversion at lines 2–6. |
| C | [Maintained counted-floor example](../../../examples/resident/counted-floor.golem): persistent globals, copy/addition, floor/conversion and literal print at lines 2–11. |
| P | [GOLEM-001 proposal](../GOLEM-001/rally-capability-integration-proposal.md): EX-001 floor at 181; EX-002 matrix/rotation at 215; EX-003 calls/conditions at 250; EX-004 float conversion/product at 318. Historical samples use semicolons. |
| S | [Design scratch example](../../../examples/scratch.golem): initialized uint16 variables, sum assignment and proposed `str` printing at lines 4–8. |
| H | [Resident host contract](../../design/resident-scalar.md): CLI at 7; package/lifecycle at 25; input validation at 42; reservation option at 49. |

The [coverage evidence](example-coverage.json) binds these references to file
hashes and exact line text. It accounts for every row of the
[INV-001 inventory](inventory.md). Host/tooling and deferred rows remain visible
without inventing source functions for them.

## Coverage by inventory item

“Missing” means absent from E. The evidence column explicitly distinguishes an
existing example elsewhere, a prose-only hint and no readable example found in
the inspected design/example sources.

| ID | Coverage in E | Existing evidence and remaining example gap |
| --- | --- | --- |
| DAT-001 | Partial | E:7/22/118 demonstrate uint16, sint16 and a bounded float32 initializer. No uint8, sint32 or Q8 fixed-point declaration/initialization example in E. P:215 illustrates sint32 elsewhere; fixed-point source names remain undesigned. Numeric bounds in E use proposed `Bounds`, whereas executable F uses `Range`. Preserve that distinction pending review. |
| DAT-002 | Partial | E includes initialization, literal print and boolean reassignment (18). Plain numeric variable-to-variable assignment is illustrated in C:7; sum assignment in S:7. General typed copy is not illustrated in E. Bootstrap raw bytes are a storage/interface question, not automatically a needed public intrinsic. Computed `str` printing in S remains outside Rally's implemented feature set. |
| INT-001 | Covered | E:146 demonstrates uint16 addition/reassignment. C:8 and S:7 additionally show persistent increment and two-variable addition. No new addition spelling needed. |
| INT-002 | Missing | No readable unsigned-to-containing-integer widening example found. The explicit conversion style in P/F is an existing direction, not a complete widening example. |
| FLT-001 | Partial | E:118 has a float literal and bounds proposal, but no float copy, integer-to-float conversion or computation. F:2 supplies an executable bounded float declaration; P:318 illustrates integer-to-float conversion elsewhere. |
| FLT-002 | Missing | P:318 shows converted operands multiplied, within a deferred imported-record example. Its math spelling can be reused independently of imports. No standalone initialized float addition/subtraction/multiplication/scaling example appears in E. |
| FLT-003 | Missing | No readable scalar reciprocal/division example found. The Rally intrinsic construction is evidence of capability, not an existing friendly syntax sample. |
| INT-003 | Missing | No readable bounded constant quotient/remainder example found. Full signed/general division must not be implied by the restricted Rally operation. |
| RND-001 | Missing | F:5 and C:9 already demonstrate true floor with conversion. Reuse that syntax in E; a standalone float-valued floor result is not yet illustrated. |
| RND-002 | Missing | No readable truncation-toward-zero example found. |
| RND-003 | Missing | No readable nearest-rounding/halfway-away example found. |
| RND-004 | Missing | No readable scalar absolute-value example found. |
| CNV-001 | Missing | F:5 shows the explicit signed-word conversion after floor. E does not. This can share a sample with RND-001 rather than introduce a duplicate builtin. |
| CNV-002 | Missing | No readable unsigned word conversion example found. Existing uint16 declarations do not demonstrate conversion from float. |
| CNV-003 | Missing | No readable bounded positive32 store/conversion example found. P's sint32 input declaration does not cover this output conversion. |
| MAT-001 | Missing | P:216 illustrates a matrix declaration; P:218 initializes its value through identity/rotation. No general literal construction, typed load, extraction or cell update sample in E. Shape notation exists as a proposal; the other operations still lack examples. |
| MAT-002 | Missing | No readable non-scalar matrix add/subtract/multiply/scaling sample found. P:318's scalar product does not demonstrate matrix multiplication. |
| GEO-001 | Missing | P:218 illustrates identity and degree rotation. Translation, geometric scale and a composition example are not shown in E. Reuse the existing direction, keeping units and order explicit. |
| GEO-002 | Missing | No readable qualified diagonal-affine inverse sample found. An arbitrary inverse would exceed Rally's proved operation. |
| GEO-003 | Missing | P:224 mentions a possible point-transform wrapper in prose, but supplies no independent initialized source sample. It is a hint, not covered example syntax. Typed geometry math stays in scope; imported/indexed data and drawing stay deferred. |
| CMP-001 | Covered | E:10–18 shows all six comparator symbols, boolean initialization and reassignment; E:24 adds a signed comparison. Keep these forms. Type coverage/float comparison behavior still needs semantic review, not six newly designed operators. |
| CTL-001 | Missing | F:4 shows one Sub; P:253/256/259 shows two Subs and a conditional source call. E has branch bodies but no procedure declaration/call. Reuse the existing proposal; no parameters/returns or recursion implied. |
| CTL-002 | Covered | E:124 shows counted For/Next; E:132 nests a conditional; E:144/153/162/171 show the four Do/Loop variants. Range/dispatch proposals also exist. No replacement loop syntax needed. A finite resident job using the existing loop/call forms can draw on P:257; backend admission restrictions are a separate issue. |
| VAL-001 | Missing | E:93 tests sequence membership, not sticky runtime admission/status. H:42 describes host float validation, not a source admission builtin. No readable sticky range/equality validation example found. Copy-related admission stays deferred to GOLEM-010. |
| RUN-001 | Host/tooling | C demonstrates persistent globals and one job; H documents install/update/call/cleanup. E has no resident-job example. Share the job illustration with CTL-001/DAT-002; refer to H for transport/lifecycle instead of inventing Golem installation commands. |
| RES-001 | Host/tooling | H:49 gives `--reserve`; ownership/layout is in generated metadata. No allocator/reservation source function is required by existing design. Keep resource configuration outside application snippets unless separately selected. |
| TOL-001 | Host/tooling | H documents artifacts/host validation. Type/shape diagnostics and maps are compiler behavior, not uncovered language functions. They do not require a new source sample per diagnostic. |
| OPT-001 | Parked | Scratch sharing, pooling and inlining controls are not illustrated in E. Remain parked, consistent with the priority on wire traffic over optimization work. |
| DEF-001 | Deferred | Assets/tables/imports: GOLEM-010. P EX-004 is historical evidence, not authorization to expand it now. |
| DEF-002 | Deferred | Indexed record/element access: GOLEM-010. Switch jump-table syntax in E indexes control targets; it does not cover data indexing and does not reopen that deferred work. |
| DEF-003 | Deferred | Checked field/record copying: GOLEM-010. Ordinary assignment coverage does not establish checked-copy behavior. |
| DEF-004 | Deferred | Typed drawing: GOLEM-011. Geometry transforms above are math examples, not point/quad/bitmap/viewport rendering. |

## Implications for the next authorized subtask

- GAP-001: Retain comparison, If/ElseIf/Else, For/Do and Switch/Range samples.
  They already show the intended notation; implementation status is independent.
- GAP-002: Bring existing floor/conversion and procedure/call examples into the
  central file when SYN-001 is authorized. Reuse the matrix/rotation and float
  product proposals where appropriate, following current terminator-free rules.
- GAP-003: Missing math examples concern typed storage/widening, general float
  arithmetic, restricted reciprocal and quotient/remainder, truncation/rounding/
  absolute value, further conversions, matrix operations and remaining transforms.
  Several inventory rows can share one independent initialized sample.
- GAP-004: Sticky admission needs an example distinct from a boolean test.
  Its copy-associated cases remain deferred. Runtime float admission cannot be
  represented as already implemented by Rally.
- GAP-005: Host lifecycle/reservation/tooling deserve references or short context,
  not an invented source API. Public names and numeric contracts that are not
  already selected remain proposals for SYN-001/SEM-001 review.

SYN-001 has not begun. Example additions will need required typed declarations,
valid initialization and clear proposal labels. Existing historical files need
not be migrated or relocated to perform that work. No new function spellings,
fixed-point naming or numeric semantics are settled by this report.
