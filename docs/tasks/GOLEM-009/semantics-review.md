# GOLEM-009 / SEM-001 — Types, operations and routine semantics

The readable proposals cover Rally's in-scope operations, but their general
spelling must not imply general implementation. Main Golem implements uint16
statements and one bounded resident floor job; Rally supplies substantially more
math with strict domains, shapes and alias rules. This review maps every EX-001
through EX-017 sample to those restrictions and records the later array/function
choices. Recommended new policies are checked conversions, resident failure
handling and value semantics for routine arguments/results. They require Author
review. No compiler, firmware or emulator implementation is included.

The subsequent [Author decisions](../../design/language-type-proposals.md#semantic-review-decisions--2026-10-11)
accept the conversion, parameter/result-lifetime, scope, loop-counter, acyclic-call
and integer division directions and add paired divmod. The recommendations below
retain their delivery context; accepted decisions take precedence. EX-022 now
illustrates signed divmod. General overflow/handler/ABI details still need review.

## Reading this review

1. ST-001: **Implemented/main** means maintained Golem accepts the source form.
   **Implemented/Rally** means an intrinsic in the examined Rally compiler, not
   the proposed Golem expression spelling. **Selected design** records an Author
   choice; it does not mean implemented. **Recommendation** awaits review.
2. ST-002: [examples.golem](../GOLEM-003/examples/examples.golem) consists of
   independent samples, not one compilable source. EX-006 retains the legacy
   `Range(low, high)` numeric-input spelling because it is the executable resident
   specimen. Other numeric examples now use `Bounds(low, high)`. Discrete ranges
   use the selected `Range(start:end:step)` spelling. Renaming the parser is later work.
3. ST-003: This report reviews semantics of functions/objects requested during
   SYN-001 review. Full assets/table/indexing/checked-copy design remains GOLEM-010;
   typed drawing remains GOLEM-011. No new implementation task is started here.

## Types and storage

| ID | Proposed Golem type | Value/storage contract | Evidence and limitation |
| --- | --- | --- | --- |
| TY-001 | `uint8` | 0..255, one byte | Rally u8 storage/admission; not a main source type yet. |
| TY-002 | `uint16` | 0..65535, two bytes | Main declarations, assignment, copy and addition; addition wraps modulo 65536. Rally also supports storage/addition. |
| TY-003 | `sint16` | -32768..32767, two bytes, two's complement | Main resident output only; Rally general storage. General signed expressions remain new work. |
| TY-004 | `sint32` | -2147483648..2147483647, four bytes, two's complement | Rally storage and restricted conversions/comparison; not general main arithmetic. |
| TY-005 | `fixed16(f)` / `fixed32(f)` | Signed N-bit raw integer divided by 2^f; quantum 2^-f; domain -2^(N-1-f)..2^(N-1-f)-2^-f | Recommend integer constant 0 <= f < N. Only f=8 exposed by Rally; other f values are extensions. No general fixed-expression engine exists. |
| TY-006 | `float32` | Four-byte IEEE binary32, finite admitted values initially | Main one bounded input; Rally scalar matrices and float fields. Floating operations round; no promise of exact decimal arithmetic. NaN/infinity admission and exceptional-result policy are not general implemented features. |
| TY-007 | `bool` | Logical false/true; recommend no implicit numeric conversion | Proposed main language type. Rally represents status/comparison as u16 constrained to 0..1. Physical Golem bool size/ABI remains unselected; do not assume one-byte packing. |
| TY-008 | `matrix(rows,columns)` | Separate type, binary32 elements, fixed shape, row-major logical values | Author explicitly retained separate matrix type. Rally dimensions 1..8, not an arbitrary n-dimensional array. Internal scalar matrices do not make scalar and matrix types implicitly interchangeable. |
| TY-009 | `Array T name(lower:upper, ...)` | Homogeneous elements, inclusive consecutive indices | Exact selected declaration is `Array float32 readings(0:3, 0:9, 0:19)`. Layout/indexing implementation deferred. |
| TY-010 | `Table` | Mixed-type fields with a fixed declared row layout | Author selected the name/direction. Declaration, row types, storage binding and access syntax remain GOLEM-010; no Variant elements are implied. |

TY-009 uses T and name as placeholders. Array shapes in function type annotations
remain unspecified; this declaration spelling does not settle that grammar.

1. TY-011: `fixed16(8)` is -128..127.99609375, quantum 1/256;
   `fixed16(4)` is -2048..2047.9375, quantum 1/16. More fractional bits reduce
   range at a fixed total width. `fixed32(8)` reaches 8388607.99609375, but float32
   cannot represent every such value exactly. Storage precision is not float precision.
2. TY-012: Recommend same-type assignment/operators first, plus contextually
   typed numeric literals. Require explicit conversions between variable types;
   never infer an unsigned value's signedness from its bytes. Mixed fixed
   precisions require an explicit common target. Literal typing and overflow
   diagnostics must be specified before general expressions are implemented.
3. TY-013: Recommend integer `/` yields the same integer type with quotient
   truncated toward zero and `%` the corresponding remainder; reject division
   by zero and any unrepresentable quotient before committing results. This
   explains EX-021's intended result but still needs review, including signed
   remainder rules. It is not inherited from VBA's `/` operator.
4. TY-014: No bare `int` default width is selected. No strings, variant objects,
   dynamic heap or general runtime resizing are implied by these numeric samples.

## Numeric conversions and rounding

The Author requires conversions between integer, fixed and float types, including
width and fractional-precision changes. The following are recommendations, not
new compiler promises. Conversion preserves the numeric value as representability
allows; it is not raw-bit reinterpretation.

| ID | Conversion | Recommended rule | Existing route / gap |
| --- | --- | --- | --- |
| CV-001 | Integer -> containing integer | Exact value; explicit destination type | Rally WidenUnsigned supports u8 -> containing u16/s16/s32 and u16 -> containing s16/s32. Full u16 -> sint16 cannot succeed; bounded u16 <=32767 can. Signed extension/general narrowing remain new work. |
| CV-002 | Integer -> fixed | Multiply raw value by 2^f, check range, exact if it fits | No general Rally conversion intrinsic. Same total width is not enough to ensure fit. |
| CV-003 | Fixed -> fixed | Rescale raw integer for new f; preserve value; discard excess fractional bits toward zero by default; check target range after quantization | Same f with larger containing width can be exact; Q4/Q8 rescaling is new. A right shift of a negative signed raw integer alone can round downward and is not the proposed rule. |
| CV-004 | Float/fixed -> integer | Default truncate toward zero, then check target range | Recommendation distinguishes conversion from `floor`. Main only accepts the exact floor-to-sint16 form; Rally floor-store paths do not implement an unrestricted cast. |
| CV-005 | Float -> fixed | Scale by 2^f, truncate toward zero onto target grid, range-check before store | New work; never rely on firmware negative-float-to-unsigned casts. Explicit alternative rounding API remains undecided. |
| CV-006 | Integer/fixed -> float32 | Round numeric value to binary32; range-check and validate actual rounded result against any Bounds | Rally MatLoad accepts signed/fixed formats. Unsigned inputs require explicit widening to a signed containing field first. Precision loss is possible for wide values, even if finite and in range. |
| CV-007 | Same-type copy | Preserve value; for float copy preserve bits | Rally typed copies constrain spans/bounds. Narrower destination Bounds still require proof or checks. Composite value copies must not introduce observable aliasing. |

1. CV-008: Keep candidate notation `sint16(value)` / `float32(value)` and
   `convert(fixed16(8), value)` pending syntax review. Parameterized target syntax
   remains a candidate; this report does not introduce another spelling.
2. CV-009: Recommend compile errors for known invalid conversions; runtime
   unknowns require qualified resident checks before destination writes. Neither
   wrapping nor saturation is an implicit conversion. If a backend cannot prove
   safety or emit a supported check, reject with the required domain in the diagnostic.
3. CV-010: Recommend exact integer/fixed literal initialization, matching Rally:
   reject a literal that is not on the fixed grid instead of silently rounding it.
   Runtime conversion may deliberately quantize under CV-003/005. Float literals
   round to binary32; test both supplied and rounded values against declared Bounds.
4. CV-011: Explicit rounding remains separate: `floor(-2.5)=-3`, `trunc(-2.5)=-2`,
   `round_away(-2.5)=-3`. These return integer-valued float32 values in the examples.
   Thus `sint16(floor(x))` still means floor even if a plain cast uses truncation.
   `abs` clears the float sign; either zero becomes +0. Rally rounding operations
   preserve signed zero. Do not replace true floor with the signed-store bias trick.

## Per-example operation limits

The limits below describe the examined Rally paths unless stated otherwise.
These are initial reusable slices, not inherent limits of the proposed types.
A future backend may broaden them with new evidence, without changing surface syntax.

| Sample | Intended semantics | Supported path, constraints and gaps |
| --- | --- | --- |
| EX-001 | Typed assignment and explicit widening | Main supports the uint16 subset only. Rally supports the illustrated storage and widening. General signed/wide arithmetic is not implied. |
| EX-002 | Q8 fixed storage loaded as binary32 | Q8 literal must be exactly representable; matrix load formats are q8s16=0xC8, q8s32=0x48. float32 load may lose low bits of wide Q8 values. No fixed-point multiplication/division follows automatically. |
| EX-003 | Float copy, signed-integer load, +, -, *, scalar scale | Rally scalar operations use 1x1 matrix commands. Sources must be initialized, finite intervals admitted and destination storage distinct for these intrinsic operations. Ordinary type assignment is not the same as MatLoad's native format conversion. |
| EX-004 | Reciprocal; division as numerator times reciprocal | Build diag(d,1,1), invert affine, extract [0,0]. Declared interval must exclude zero and prove finite reciprocal. Illustrated denominator 1..4 is safe for this construction. Results are binary32 approximations; reciprocal multiplication need not equal correctly rounded direct division. Not a general `/` backend. |
| EX-005 | Float-valued floor/trunc/round-away/abs | Floor and trunc require finite magnitude <=16777215; round_away <=8388607. abs requires finite bounds. Distinct initialized scalar sources/destinations. Tiny negative fractions must floor to -1, not 0. |
| EX-006 | Main resident floor -> signed-word result | Exact supported source grammar: input bound within -32768..32767, one sint16 output, one Sub with exact floor assignment. Main performs true floor before bias/unbias storage. CLI/host validate input; no resident float admission is generated. No locals, source function calls or general expressions. |
| EX-007 | Floor then unsigned-word conversion | Rally StoreU16Floor source interval 0..65534; excludes 65535 despite uint16 storage allowing it. Explicit floor remains separate. Output range must contain result. |
| EX-008 | Floor then nonnegative signed32 conversion | Rally StorePositive32Floor source interval 0..16777215; excludes negative/full-sint32 domains. Four-byte store, not a uint32 or arbitrary signed32 cast. |
| EX-009 | Paired positive quotient/remainder | Input s32 integer interval within 0..16777215; constant divisor 1..16777215; quotient uint16 <=32767; remainder uint16 or sint32 whose bounds contain 0..divisor-1. All spans distinct. Rally checks every integer in declared input domain for binary32 reciprocal quotient correctness and rejects unsafe domains. Sample 0..1000 /7 satisfies it. |
| EX-010 | Matrix initialization, copy, submatrix, replacement | Matrix literal arity exactly rows*columns. Copy requires same shape. Submatrix origin/extent and replacement coordinates are compile-time, zero-based, within shape. `with_element` returns a new logical value, preserving source; it is not indexed Array access. |
| EX-011 | Matrix algebra | Add/sub require equal shapes; (m×n)*(n×p) yields m×p. `matrix * scalar` scales elements, not an elementwise matrix product. Rally requires distinct output/source buffers; wrapper temporaries may enforce value semantics. No implicit matrix broadcasting. |
| EX-012 | Affine composition | identity accepts 3x3/4x4; illustrated scale/rotate/translate require 3x3. Degrees, not radians. Firmware left-multiplies new transform, so listed calls apply scale, then rotation, then translation to column points. No arbitrary-dimension identity/general trig API is implied. |
| EX-013 | Restricted inverse | Rally InvertAffine accepts 3x3 diagonal matrices with proved finite nonzero diagonal entries; illustrated final diagonal is 1. General nonsingular affine or arbitrary matrix inverse is not supplied by this intrinsic path. There is no command-34 inverse. |
| EX-014 | Point transformation | Logical rows are x,y tuples, with implicit homogeneous 1; no perspective divide. Rally command 41 path requires f32, distinct equal-sized source/destination buffers, checked positive count/stride/span and square transform. Adapting separate matrix objects to this tuple-storage wrapper needs explicit mapping/qualification; the sample is not already accepted Rally syntax. |
| EX-015 | Persistent state and finite source calls | Main persistent uint16 state works, but source calls/multiple jobs/resident loops are missing. Rally named parameterless Programs use an acyclic call graph and constant Repeat 0..512. Structured For/If syntax still needs a backend. |
| EX-016 | Sticky resident integer/fixed admission | Rally RequireRange/RequireEqual accept integer/Q8 fixed fields with separate u16 0..1 status storage; no runtime float checks. Checks only clear status on failure, do not restore success, and do not branch automatically. Sample resets true per job, rejects 21 outside 0..20 and remains rejected after equality succeeds. |
| EX-017 | Cross-type explicit conversions | Candidate type-call/convert notation. All illustrated chosen values convert exactly under proposed rules; no precision-loss edge is demonstrated by them. Non-Q8, general narrowing, fixed rescaling and negative float conversion are new work. |
| EX-018 | Three-dimensional typed Array | Selected syntax, 4×10×20=800 float32 elements. Bounds apply to indices, not stored numeric values. Allocation and indexed access remain GOLEM-010; no runtime allocation is implied. |
| EX-019 | Inclusive stepped iteration | Selected Range/For spelling; 8192 visits, last value 65528. Must finish without incrementing uint16 to wrapped zero. Neither main For/To nor Rally Repeat implements this header. |
| EX-020 | Function, local parameter/result, persistent global | Selected Function direction and any-type requirement; passing/scoping recommendations below. Main cannot compile this example. uint16 multiplication generality is not established by main addition support. |
| EX-021 | Multiple Function results | Wrapper now calls paired divmod once, then returns both values. Sample 17/5 -> (3,2). General runtime-divisor division remains new backend work beyond Rally's bounded constant-divisor intrinsic. |
| EX-022 | Paired signed divmod | Selected quotient/remainder order, truncation toward zero, result -3,-2 for -17/5. Zero denominator and unrepresentable quotient take the resident error path. Source syntax remains unimplemented. |

1. OP-001: General comparison design remains `!= == < > <= >=`, returning bool.
   Rally LessThan handles full matching integer/Q8 widths, including 65535, using
   ordered bytes. Its separate CallIf accepts only bounded u16 <=65534 because
   stock conditional fetching has a 0xFFFF missing-value sentinel. Do not extend
   that limitation to the language type; the backend must avoid the sentinel.
   General float comparisons, NaN rules and mixed-type comparison remain new work.
2. OP-002: Recommend distinct temporary destinations where Rally requires them.
   For `a = a + b`, evaluate into scratch then commit; do not equate source-level
   value semantics with permission to alias an unqualified stock operation.
   No optimization campaign or sophisticated global optimizer is required.

## Bounds, failure and the WOM constraint

1. ER-001: `Bounds(low, high)` limits numeric values; Array bounds and Range
   describe integer indices/sequences. All endpoints are inclusive. Recommend
   finite constant numeric Bounds within the base type, low <= high, containing
   at least one representable value. Bounds does not change the type's storage width.
2. ER-002: Recommend proving safety locally where straightforward; otherwise
   generate a qualified resident check before committing the candidate. On failure,
   preserve the old destination, mark a resident failure and take a finite resident
   handler/exit path. No clamping, wrapping cast or silent success. Compile-time
   invalid constants fail compilation. Simple interval proofs and explicit checks
   are sufficient; unrestricted inference is not required.
3. ER-003: Stock WOM code must not wait for MOS acknowledgement, a host exception
   or a result readback. Handler/state live on the VDP. The optional future vdplet
   return channel may report outcomes, but is not a dependency of basic correctness.
   Handler/status syntax, default handler behavior and recovery remain decisions
   for review. For initial implementation, reject unproved cases until the needed
   resident checker exists rather than pretend the declaration installs one.
4. ER-004: Main's current resident input bounds are a host admission contract,
   not automatically enforced against arbitrary later buffer updates. Existing
   packages retain that documented limitation. New Bounds checking needs new code;
   renaming a keyword cannot establish it. Float checks must validate actual rounded
   binary32 values, not merely the host decimal input or an assumed interval.
5. ER-005: Bounds checking is not automatically intermediate overflow checking.
   Main uint16 addition already wraps. Recommend retaining that existing behavior
   for unbounded uint16 while deciding a separate uniform rule for signed/fixed
   overflow and bounded arithmetic. Check after an explicit conversion's defined
   quantization but before its range-losing store. Do not claim a final Bounds check
   detects arithmetic that wrapped earlier. This is a material open policy choice.
6. ER-006: For multiple returned destinations, recommend evaluating all result
   candidates before any destination write, then committing only after all pass.
   That does not roll back previous global writes or VDU effects inside the function.
   General transactional execution is not proposed.

## Functions, parameters, scope and lifetime

1. FN-001: Selected direction: `Function name(typed parameters) As resultType`,
   explicit Return, EndFunction; Sub has no results. Each normal path must supply
   the declared result count/types. Unreachable code/definite assignment checks
   can be conservative. Error paths must still return control to VDP processing.
2. FN-002: Selected requirement: accept/return any supported type, including
   Array, matrix and future Table. This is a language requirement, not a claim
   that Table/Array storage is already implemented. Function syntax for array
   shapes/table row types remains deferred alongside those types; avoid inventing
   it in this tranche. No implicit scalar-to-object or matrix-to-Array conversion.
3. FN-003: Recommend **ByVal by default**, with value semantics for every type:
   modifying a parameter does not modify the caller's object. Composite arguments
   may need resident copies. Calls/returns happen on the VDP; these copies do not
   themselves create eZ80 wire traffic. Large copies still consume VDP commands,
   memory and time. Consider an explicit ByRef extension only after alias/mutability
   rules are reviewed; do not silently pass objects by shared mutable reference.
4. FN-004: Recommend returned composite values have caller-owned lifetime.
   Caller-provided result storage/copy-out can satisfy this without a heap or a
   smart optimizer. Never expose expired local-frame references. Keep locals live
   until returned values are transferred; forwarding an argument as a result must
   preserve the same value semantics. Storage reuse must preserve observable behavior.
5. FN-005: Recommend evaluating arguments/results once in source order. For tuple
   assignment, first collect results, validate all destinations and then commit;
   initially reject repeated/overlapping destinations. Exact tuple/call grammar
   remains a parser task, especially with terminator-free Return expression lists.
6. SC-001: Recommended lexical scope: top-level variables are persistent globals;
   parameters/routine declarations belong to the invocation; nested-block variables
   are visible only in that block. Global initializers run at installation/reset;
   local initializers run whenever the declaration is reached. Routine-local values
   must not accidentally retain a previous call's state.
7. SC-002: Recommend rejecting declarations that shadow any visible variable,
   including globals, parameters and outer-block locals. Sibling nonoverlapping
   scopes may reuse a name. This strengthens the earlier global-shadowing proposal
   and is not yet a compiler feature.
8. SC-003: Recommend definite assignment for new locals: declaration alone creates
   a typed slot, and every read requires a prior initialization/assignment on all
   possible paths. Parameters and loop-assigned counters start assigned. Existing
   zero-default uint16 globals and EX-006's output retain their executable contract;
   do not silently change them while adding locals. General global default rules
   and any composite zero initialization still need review. No implicit type inference.
9. SC-004: Recommend an acyclic source-call graph and serialized resident entry
   initially. Independent locals do not require unrestricted recursion or dynamic
   stack allocation. Allocate bounded per-call storage for the finite call graph;
   use ordinary function boundaries rather than one permanently running main loop.
   Static locals, recursion, callbacks/reentrancy and shared mutable ByRef parameters
   remain later decisions. A native/VDP call stack overflow is not an acceptable
   substitute for enforcing this restriction.

## Ranges and control flow

1. RG-001: Selected design: `For i In Range(start:end:step)` replaces For/To/Step
   as the canonical counted syntax. Bounds are inclusive, default step 1, end is
   visited only when reached exactly. Counter declared explicitly. Zero step and
   a step pointing away from a distinct end are errors. Recommend equal endpoints
   visit once and compile-time integer range arguments initially, matching the prototype.
2. RG-002: Recommend immutable loop control during iteration: reject assignment
   to the counter in its body or a call that can mutate it. Evaluate header once.
   Determine last/count in sufficiently wide compile-time arithmetic and stop
   before overflow, including uint16 maximum. On normal completion recommend
   leaving counter at its last visited value; early exit syntax is not selected.
3. RG-003: Range membership must check alignment as well as inclusive limits.
   The selected initial indexed Switch requires step 1; stepped Switch remains
   an extension despite the shared Range syntax. Missing cases go to Default or
   continuation; duplicate/outside/misaligned cases are compile errors. No fallback
   to comparison dispatch for an explicit indexed-table request.
4. RG-004: Main still implements the earlier For/To header and requires a matching
   named Next. Its increment/conditional tail-call construction does not qualify
   wraparound-safe maximum-bound loops. Do not run the selected maximum-bound
   sample through that lowering. The new header needs a deliberate implementation.
   Existing source compatibility/migration must be addressed then, not by silently
   accepting both spellings indefinitely.
5. RG-005: Conditions, ElseIf, Do/Loop and general Switch are mostly syntax
   proposals. Resident execution must be finite; initially support statically
   bounded iteration/calls or reject unsupported cases. No general halting proof,
   infinite resident loop or arbitrary timeout interruption is promised.

## Review decisions

1. DEC-001: Accept default truncation toward zero for numeric casts/fixed-grid
   conversion, exact fixed literals and checked out-of-range conversion, plus
   integer quotient semantics for integer `/`? Keep
   explicit floor/round_away for other rounding intentions. Alternative rounding
   conversion syntax and broader arithmetic overflow policy remain open.
2. DEC-002: Accept destination preservation plus finite resident failure handling
   for unproved Bounds/conversion failures? Handler syntax/default policy needs
   a later concrete proposal; host awareness remains optional.
3. DEC-003: Accept ByVal value semantics, caller-owned results, simultaneous
   multiple-result assignment and an acyclic call graph initially? ByRef is deferred.
4. DEC-004: Accept lexical block scope, no visible-name shadowing and definite
   assignment for new locals; and nonmutable For counters ending at the last visited
   value? Existing executable initialization behavior stays intact.

These are the questions for the next Author review, not additional subtasks to
execute automatically. REV-001 remains pending. Stop here; no implementation.

## Source basis and verification limits

1. SRC-001: Main checkpoint `c83463b`, examined `src/ordinary.hpp`, `src/resident.hpp`,
   `src/scalar_backend.hpp` and [maintained resident contract](../../design/resident-scalar.md).
2. SRC-002: Rally clean HEAD `4db7e96f70c3d63f4ab53766f3b32063e9357c8b`,
   [hosted source](/Agon/mystuff/golem-rally19/src/hosted.hpp) and
   [contract](/Agon/mystuff/golem-rally19/docs/design/rally19-hosted-v1.md).
   Source checks override broad wording in the older contract: current inverse
   dispatch explicitly restricts diagonal 3x3. MatFloor is a later true-floor
   primitive; main uses it before the signed bias store. Rally StoreI16Floor
   alone retains its bias-based conversion and must not substitute for MatFloor.
3. SRC-003: [Semantic reference evidence](semantics-evidence.json) records the original delivery
   snapshot, followed by a division-review amendment, with source
   hashes and exact arithmetic/binary32 reference cases, not runtime tests.
   No compiler/backend code changed, so no full regression or firmware suite was
   rerun. Prior qualification stays limited to its already recorded paths.
