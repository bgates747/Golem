# GOLEM-009 / SYN-001 — Missing syntax examples added

Historical SYN-001 delivery snapshot. Later Author review changed original
Range/For examples, selected Bounds for numeric constraints, and added
EX-018 through EX-021. Current semantics, domains and remaining decisions are
in the [SEM-001 review](semantics-review.md); the original hash and prefix-preservation
claims below describe the earlier delivery, not the current file.

Seventeen independent samples have been appended to the existing
[examples.golem](../GOLEM-003/examples/examples.golem), preserving its original
174 lines unchanged. They illustrate the uncovered math/storage/control
capabilities identified by INV-002. Existing comparator and conditional designs
remain intact. New spellings are review proposals; EX-006 alone reproduces the
maintained resident floor slice. **SYN-001 is complete and awaits Author review.**

## Example map

| Sample | Purpose | Inventory coverage |
| --- | --- | --- |
| EX-001 | Typed integers, ordinary assignment and explicit unsigned widening | DAT-001, DAT-002, INT-002 |
| EX-002 | Candidate fixed-point types and explicit float conversion/load | DAT-001, FLT-001 |
| EX-003 | Float copy, integer conversion, addition, subtraction, multiplication and scale | DAT-002, FLT-001, FLT-002 |
| EX-004 | Nonzero bounded reciprocal and division via multiplication | FLT-003 |
| EX-005 | Float-valued floor, truncation, halfway-away rounding and absolute value | RND-001, RND-002, RND-003, RND-004 |
| EX-006 | Existing bounded floor-to-sint16 resident job | RND-001, CNV-001, CTL-001 |
| EX-007 | Restricted float-to-uint16 conversion after floor | CNV-002 |
| EX-008 | Restricted nonnegative float-to-sint32 conversion after floor | CNV-003 |
| EX-009 | Bounded positive integer quotient/remainder with a constant divisor | INT-003 |
| EX-010 | Matrix literal initialization, typed copy/load, submatrix and cell replacement | MAT-001 |
| EX-011 | Matrix addition, subtraction, product and scalar scale | MAT-002 |
| EX-012 | Identity, geometric scale, degree rotation and translation | GEO-001 |
| EX-013 | Restricted diagonal affine inverse | GEO-002 |
| EX-014 | Rowwise float point transformation without data indexing or drawing | GEO-003 |
| EX-015 | Persistent uint16 state, finite work and conditional source calls | DAT-002, CTL-001, RUN-001; reuses INT-001/CTL-002 |
| EX-016 | Sticky integer range/equality admission with explicit per-job reset | VAL-001; reuses CMP-001/CTL-001 |
| EX-017 | Explicit integer/fixed/float conversions, fixed width and fractional precision changes | Author review extension of DAT-001, INT-002, FLT-001 and conversion families |

All 18 missing and 3 partially covered source-example inventory rows now have
illustrations. This is notation coverage, not implementation or exhaustive
numeric qualification. Matrix copy/load illustrates existing typed matrix data;
general bulk loading from imported/custom layouts remains with deferred data
work. Conversion samples illustrate selected safe domains, not unrestricted casts.

## Candidate notation for this review

Existing long numeric names, explicit conversions, `Sub`/`EndSub`, ordinary
operators, `identity`, `rotate_degrees` and `transform_points` follow the earlier
examples/proposal. Newly filled gaps use these candidate spellings:

- NAM-001: `fixed16(8)` and `fixed32(8)` describe signed total width and fractional
  bits. They illustrate Rally's q8s16/q8s32 storage without adopting its internal
  type names as a language decision.
- NAM-002: `reciprocal`, `trunc`, `round_away`, `abs` and `divmod_positive` name
  operations explicitly. The paired divmod builtin takes two declared output
  arguments first; this does not introduce tuple assignment or user functions
  with output parameters.
- NAM-003: `matrix_values` supplies row-major literal initialization; assignment
  expresses typed copying. `submatrix` and `with_element` use constant zero-based
  coordinates, avoiding a general data-indexing proposal in this tranche.
- NAM-004: Geometric `scale` and `translate` extend the existing expression style
  for degree rotation. The diagonal inverse is named `invert_affine`; only the
  restricted Rally-backed case is illustrated.
- NAM-005: `require_range` and `require_equal` take an explicit sticky boolean
  status. The example resets it per job and shows a failed range check remaining
  failed after a successful equality check. No float admission or checked copy
  is introduced.
- NAM-006: Author review calls for conversions between numeric types and fixed
  precisions, including four fractional bits. EX-017 keeps ordinary type-call
  conversions and proposes `convert(targetType, value)` for parameterized fixed
  targets. This spelling is a candidate, not a second accepted conversion rule.
  Rescaling preserves numeric value as representability permits; rounding,
  precision loss and out-of-range policy await SEM-001. Non-Q8 fixed support
  extends beyond Rally's currently exposed formats.

The file keeps existing `Bounds` prototypes unchanged and uses the established
`Range` notation for new numeric bounds. That naming question is not resolved.
Basic constraints needed to read each example are noted briefly; SEM-001's full
type/range/rounding/alias/shape contract review remains pending. All new examples
include declarations and required initialization; the supported floor output
uses the existing zero-default declaration rule.

Host installation/update/cleanup, external reservations and diagnostics remain
in the [maintained contract](../../design/resident-scalar.md), with no new
installation or allocation language builtins. GOLEM-010/011 areas and optimization
controls remain deferred/parked. No compiler, runtime, firmware or emulator changes
were made. Historical coverage/provenance reports retain their original hashes;
[syntax evidence](syntax-additions.json) records this revision separately.
