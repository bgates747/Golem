# GOLEM-009 / INV-001 — Rally capability reconciliation

The maintained compiler now covers ordinary uint16 work and one resident
float32 floor-to-sint16 job, with persistent uint16 state, checked allocation
and installation/cleanup metadata. Most Rally math remains available only in
Rally's intrinsic compiler. Existing comparison and conditional syntax remains
the design baseline, regardless of missing implementation. Assets/tables,
indexed access and checked copying are deferred to GOLEM-010; typed drawing is
deferred to GOLEM-011. **INV-001 is complete and awaits Author review.**

This reconciles GOLEM-001's historical comparison; it does not replace that
record or perform INV-002's example-by-example coverage audit. No new syntax,
example edits or compiler changes are included.

## Basis and status rules

| Reference | Examined state |
| --- | --- |
| Main Golem | `1affbc5` (compiler promotion at `66be792`) |
| Rally Golem | `4db7e96f70c3d63f4ab53766f3b32063e9357c8b`, clean worktree |
| Original comparison | [GOLEM-001 capability comparison](../GOLEM-001/rally-capability-integration-proposal.md#capability-comparison) |
| Current executable contract | [resident scalar profile](../../design/resident-scalar.md), `src/ordinary.hpp`, `src/resident.hpp`, `src/scalar_backend.hpp` |
| Existing design | `docs/design/language-type-proposals.md`; GOLEM-001 EX-001 through EX-004; `docs/tasks/GOLEM-003/examples/examples.golem` |
| Evidence | [source audit](inventory-audit.json), GOLEM-007 qualification and GOLEM-008 promotion checks |

**Implemented** below means accepted by maintained `build/golemc`, rather than
merely present as an internal helper or in a historical task prototype.
**Proposed** means documented syntax/design, without claiming compiler support.
**Remaining** means a capability gap; it does not automatically mean a missing
example. INV-002 will make that distinction item by item.

Rally source has 44 distinct statement operations, seven field types and six
resource declaration forms. The audit maps every statement operation to an
inventory row below; grouping does not imply interchangeable semantics. The
source-matched historical Rally report covers 14 host suites. This subtask
rechecks its 17 recorded source hashes, without rerunning those suites or
expanding their firmware/hardware qualification.

## Storage, assignment and arithmetic

| ID | Rally capability | Maintained Golem | Existing design / remaining capability |
| --- | --- | --- | --- |
| DAT-001 | `Buffer`, `Field`; types u8/u16/s16/s32/q8s16/q8s32/f32 | Ordinary uint16; resident profile adds one bounded float32 input and sint16 output | Long numeric names and explicit conversions are proposed. General signed, wider and Q8 fixed-point variables/expressions remain missing. Storage support in Rally does not imply all arithmetic on every type. |
| DAT-002 | `Set`, typed `Copy`, literal `print`, bootstrap `Bytes` | uint16 declaration/assignment/copy/addition and literal print; resident input initialization | Ordinary assignment design already exists. General typed assignment/copy and raw bootstrap data are not exposed. Unchecked typed `Copy` is distinct from the checked-copy features deferred below. Computed text and `str` remain missing in both compilers. |
| INT-001 | `AddU16` | Chained uint16 `+`, including persistent resident globals; modulo-65536 wrap | Covered operation, with different frontend spelling. Rally requires a full-range u16 destination; this is not general signed/wide arithmetic. |
| INT-002 | `WidenUnsigned` | No source-level widening | Explicit conversions are proposed; general unsigned-to-containing-integer widening remains missing. |
| FLT-001 | `MatLoad`, scalar `MatValues`; scalar storage/load | Internal load/storage used by the fixed floor job | Existing float declarations/conversion proposals provide a starting point. General float variables, literals and typed loads remain missing from executable source. Matrix forms also belong to MAT-001. |
| FLT-002 | Scalar use of `MatAdd`, `MatSub`, `MatMul`, `MatScale` | No general float expression parser/emission | Readable expressions are the intended surface; GOLEM-001 EX-004 includes float conversion/multiplication. Arithmetic beyond the floor job remains unimplemented; INV-002 must check actual sample coverage. |
| FLT-003 | Reciprocal through diagonal `InvertAffine` and `MatExtract` | None | No general scalar division is implemented. Rally's restricted reciprocal construction is available for reuse; arbitrary matrix inversion and unrestricted `/` cannot be claimed from it. |
| INT-003 | `DivModPositive` | None | Restricted constant-divisor quotient/remainder remains missing. Rally requires bounded nonnegative s32 input <=16777215, nonzero constant divisor and quotient <=32767, and exhaustively checks its integer input domain. Not general integer division/modulo. |

## Rounding and conversion

| ID | Rally capability | Maintained Golem | Existing design / remaining capability |
| --- | --- | --- | --- |
| RND-001 | `MatFloor` | Implemented only inside the exact resident `sint16(floor(input))` assignment | Existing readable floor example is now executable in the resident profile. A reusable float-valued `floor` expression remains missing; the source input must stay within its declared -32768..32767 envelope. |
| RND-002 | `MatTrunc` | None | Truncation toward zero remains to expose; not interchangeable with floor. |
| RND-003 | `MatRoundAway` | None | Nearest rounding with halfway ties away from zero remains to expose; its supported magnitude differs from truncation/floor. |
| RND-004 | `MatAbs` | None | Finite scalar absolute value remains to expose. |
| CNV-001 | `StoreI16Floor` | Internal signed-word store after true floor; no standalone cast | General explicit sint16 conversion remains missing. Keep true floor separate from the biased storage conversion, especially for tiny negative fractions. |
| CNV-002 | `StoreU16Floor` | None | Restricted unsigned word store remains missing; Rally's accepted source bound excludes 65535. |
| CNV-003 | `StorePositive32Floor` | None | Restricted nonnegative conversion to s32 storage remains missing; not a full signed32 or uint32 cast. |

These rows identify distinct operations for the later syntax audit. Their exact
public spellings and full semantic contracts are not selected by this subtask.
In particular, internal S32 support in `scalar_backend.hpp` does not make a
source-level `sint32` declaration or conversion available.

## Matrices and geometry

| ID | Rally capability | Maintained Golem | Existing design / remaining capability |
| --- | --- | --- | --- |
| MAT-001 | `Matrix`, `MatValues`, `MatLoad`, `MatExtract`, `MatPut` | Scalar matrices are compiler-private implementation details | GOLEM-001 EX-002 proposes a matrix type. General typed matrix construction, load, submatrix extraction and cell update remain missing. Rally checks dimensions 1..8 and operand shapes. |
| MAT-002 | Matrix `MatAdd`, `MatSub`, `MatMul`, `MatScale` | None as public operations | Matrix arithmetic remains missing. Scalar reuse in FLT-002 does not cover matrix multiplication semantics. |
| GEO-001 | `Identity`, `Translate`, `Scale`, `RotateDegrees` | None as public operations | GOLEM-001 EX-002 illustrates identity/degree rotation. Composition, units and initialization remain relevant when exposing the rest. |
| GEO-002 | `InvertAffine` | None | Rally proves a nonzero diagonal 3x3 case; general affine/matrix inverse is not implemented there. This also supplies the restricted reciprocal path in FLT-003. |
| GEO-003 | `TransformData` | None | GOLEM-001 discusses a typed point-transform wrapper. Rally's public path accepts f32 tuples, rejecting fixed formats; no perspective divide is supplied. Geometry math remains in this audit's scope; drawing and imported/indexed storage stay deferred. |

## Comparisons, control and admission

| ID | Rally capability | Maintained Golem | Existing design / remaining capability |
| --- | --- | --- | --- |
| CMP-001 | `LessThan`, six relations in `CallIf` | Internal uint16 loop condition only | `!= == < > <= >=`, boolean initialization/assignment and If/ElseIf/Else examples already form the design baseline. Remaining compiler work must respect signedness/full width. Rally's `LessThan` excludes f32; `CallIf` requires u16 bounds excluding 65535. Neither supplies general float comparison automatically. |
| CTL-001 | Named parameterless `Program`, `Call`, `CallIf` | One named resident `Sub`; host entry invocation only | GOLEM-001 EX-003 proposes multiple Subs and source calls; examples illustrate structured conditions. Multiple jobs, source calls and conditional calls remain missing. Rally rejects recursive call graphs; no general parameters/returns. |
| CTL-002 | Finite constant `Repeat(0..512)` | Ordinary `For/Next`; loops explicitly rejected in the resident profile | Existing For/Do/Loop/Switch/Range syntax is the starting design. Resident finite repetition remains missing; ordinary For support does not establish resident-loop safety. Switch/jump tables and general Do loops are proposals beyond Rally's intrinsic control structures. |
| VAL-001 | Sticky `RequireRange`, `RequireEqual` | Compile-time/CLI input range validation only | Runtime admission/status handling remains missing. These are validation operations, not merely boolean comparisons. General admission stays visible for later syntax review; copy-associated usage belongs to deferred GOLEM-010. Rally does not provide runtime float admission. |

## Lifecycle, ownership and tooling

| ID | Rally capability | Maintained Golem | Remaining capability / treatment |
| --- | --- | --- | --- |
| RUN-001 | Bootstrap, persistent data/programs, setup, calls and `.clear` | Resident bootstrap/invoke/update-call/cleanup plus `interface.json`; persistent uint16 globals | Core install-once/call-many lifecycle is implemented for the scalar slice: 21 bytes per fresh input/call, excluding result retrieval/framing. General jobs, setup/source calls and event/callback integration are not supplied. |
| RES-001 | Explicit user IDs, private scratch, application exclusions and owned-ID ledgers | Shared checked allocator, external `--reserve` ranges, owned IDs/roles/cleanup | Promotion replaces Rally's application-specific allocation policy for the supported slice. General typed resources and externally bound arrays are not implied. |
| TOL-001 | Type/shape/range/alias diagnostics, relocations, imports/fingerprints, maps/byte ledgers | Source diagnostics, range/ownership checks, scalar interface and byte counts | Broader operation diagnostics/maps remain missing as those operations are added. Imported-resource metadata stays deferred with GOLEM-010. Semantic errors precede writes; neither publication path is filesystem-transactional. These are tooling capabilities, not necessarily language functions. |
| OPT-001 | `SharedWordScratch`, `SharedScalarScratch`, literal pooling and `InlineCalls` | No opt-in source controls corresponding to these Rally policies | Resource/performance options are recorded for completeness and remain parked. They are not prerequisites for the requested syntax work or the wire-traffic priority. |

## Explicitly deferred

| ID | Rally capability | Maintained Golem | Disposition |
| --- | --- | --- | --- |
| DEF-001 | `Asset`, `Table`, typed imported fields, sizes/fingerprints | No source imports | GOLEM-010; no syntax development this tranche. |
| DEF-002 | `LoadIndexed`, `LoadElement` | No source indexed access | GOLEM-010; includes checked bounds/status and preserved destination on failure. |
| DEF-003 | `CopyChecked`, `CopyRecord`, associated admission/record layout | No checked-copy API | GOLEM-010. General type-preserving `Copy` is separately recorded at DAT-002. |
| DEF-004 | `PlotPoint`, `PlotQuad`, `DrawBitmap`, `GraphicsViewport`, `ScrollGraphics` | No typed drawing surface | GOLEM-011; includes artwork ownership/graphics-state contracts. |

## Reconciliation findings

- REC-001: GOLEM-001's “absent” rounding/storage/lifecycle entries are historical.
  GOLEM-008 fills the bounded floor/conversion/resident subset; it does not fill
  the entire corresponding capability families.
- REC-002: Comparators and structured conditionals already have readable design
  examples. Missing implementation must not trigger needless syntax redesign.
  Float comparison semantics still need care because Rally's comparison paths
  are not general float comparators.
- REC-003: Existing design material includes floor/conversion, matrix rotation,
  procedure/condition examples and an imported-record float product. This is
  sufficient to acknowledge an existing syntax direction; it is not a completed
  audit of which functions are illustrated in `examples.golem`.
- REC-004: GOLEM-001's proposed samples retain historical semicolons. Current
  conventions are terminator-free, keywords insensitive, identifiers sensitive
  and explicit typed declarations/initialization. Preserve the historical report;
  later example additions must follow current conventions.
- REC-005: Golem's general runtime-library ambitions (strings, shifts, arbitrary
  signed/wide arithmetic, unrestricted division) and native-module research are
  separate from functionality actually found in Rally. Rotation does not establish
  standalone `sin`/`cos`, and no standalone sqrt/log/exp intrinsic was found.

Stop here for Author review. INV-002, SYN-001 and SEM-001 remain unexecuted;
GOLEM-010/011 remain deferred. New examples and implementation decisions follow
only after review/instruction.
