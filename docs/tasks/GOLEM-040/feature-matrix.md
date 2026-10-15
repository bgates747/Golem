# Feature inventory and evidence

The accepted general prototype contains substantially more language support than
the normal maintained compiler. The rows below identify where each capability
is implemented and qualified; they do not claim these checks were rerun during
this static audit. Historical task snapshots remain immutable.

## Current maintained profiles

| Profile | Actual capability and boundary | Maintained source / contract |
| --- | --- | --- |
| Raw | Literal print; uint16 globals, zero default when omitted; assignment/addition; For counter=... To... / Next. No portable root/native output. | `src/ordinary.hpp`, `src/golemc.cpp`, `tests/test_compiler.cpp` |
| Resident scalar | One bounded float floor-to-sint16 job with optional persistent uint16 calculations; checked add/subtract/multiply and paired uint16 divmod; named assembly output. Legacy Range declaration spelling persists here. | `src/resident.hpp`, `src/scalar_backend.hpp`, `docs/design/resident-scalar.md`; GOLEM-008/012–016 |
| Resident table | Flat integer record, immutable zero-based table, checked whole-record fetch, build-mapped data. Separate profile, not the portable Array language. | `src/resident_table.hpp`, `docs/design/resident-table.md`; GOLEM-017 |
| Resident drawing | Packed uint8/uint16/sint16 inputs; points/lines/quads/bitmaps; multiple named finite Subs already in source. No source assignments, branches, loops or calls in this profile. | `src/resident_drawing.hpp`, `src/assembly_output.hpp`, `docs/design/resident-drawing.md`; GOLEM-018/019/022. GOLEM-022 human status remains pending in its task. |

## General portable implementation to promote

All rows below are present in the selected four-file successor under
`docs/tasks/GOLEM-036/right-shift/`. Earlier tasks identify provenance and
specialized qualification, not additional source dependencies to merge.

| Capability | Actual supported slice | Acceptance / useful recorded evidence |
| --- | --- | --- |
| Shared frontend/model/layout | Explicitly initialized globals; parameterless finite named Subs; typed expressions; Bounds; shared allocation and state. | [GOLEM-024](../GOLEM-024.md): 44 host contracts / 42 rejection cases; host-only first prototype. |
| Dual executable output | Checked uint16 +, -, *; shared data and disjoint private backend ownership; native C generation and stock buffered commands. | [GOLEM-025](../GOLEM-025.md): 218 host cases, stock/actual-Xtensa qualification and accepted visual review. |
| Comparisons/branches | == != < > <= >=; If/ElseIf/Else/EndIf; same-type operands and skipped unselected branches. Current nesting/arm admission limits retained. | [GOLEM-026](../GOLEM-026.md): 2,236 host cases and accepted target/visual evidence. |
| Finite Range loops | Predeclared uint16 counter; inclusive constant start:end[:step]; positive/negative step; final visited value retained; protected active counter; finite work admission. | [GOLEM-027](../GOLEM-027.md): 2,488 host cases, stock/native retained-state evidence, accepted review. |
| Typed Boolean values | Distinct bool type, two-byte canonical 0/1 storage, literals/copy/comparison results. No numeric truthiness. | [GOLEM-028](../GOLEM-028.md): 4,804 host cases, admission and accepted visual evidence. |
| Boolean composition | Not/And/Or, precedence and short-circuiting; skipped expressions cannot raise arithmetic faults, but entry admission remains whole-global. | [GOLEM-029](../GOLEM-029.md): 5,284 host cases and accepted target/visual evidence. |
| Paired uint16 division | `(q,r)=divmod(n,d)`; distinct global destinations, operand evaluation once, pair preservation on failure. | [GOLEM-030](../GOLEM-030.md): 9,104 host cases, zero-divisor/aliasing/pair checks and accepted review. |
| Signed 16-bit scalar | sint16 checked +, -, *, comparisons and explicit conversions; no sint16 divmod or variable unary negation. | [GOLEM-031](../GOLEM-031.md): 14,912 host cases and accepted target/visual evidence. |
| float32 scalar | +, -, *, comparisons, Bounds and explicit uint16/sint16 conversions. Finite normal/zero input domain; defined rounding/flush behavior. No float division/floor/sqrt/pow in this profile. | [GOLEM-032](../GOLEM-032.md): 24,494 host cases, 84 grammar/reference cases and 101 rejections; stock/Xtensa and accepted visual evidence. |
| Root/host interface | Schema-1 112-byte header; separate interface/language/runtime statuses, requested entry, session and successful generation; four optional external binding roles; named wrappers. | [GOLEM-034](../GOLEM-034/README.md), [root contract](../GOLEM-034/root-contract.md): accepted review and saved results. |
| Indexed retained storage | One-dimensional integer Array / ReadOnly Array; inclusive ascending bounds, typed index, Fill/Values or exact build-mapped bytes; checked access and separate application ownership. | [GOLEM-035](../GOLEM-035/README.md): 2,838 host cases, 23 frontend rejections, 1,302 saved human observations; accepted. |
| Wide scalar/array foundation | uint32/sint32 globals and elements, checked add/subtract/comparisons and explicit integer narrowing/widening. No wide/float conversions. | [GOLEM-036 foundation](../GOLEM-036/README.md): 3,872 host cases, 15 frontend rejections and accepted review. |
| Wide multiplication | Checked signed/unsigned 32-bit multiplication with explicit operand types and preserved failed destination. | [Multiplication tranche](../GOLEM-036/multiplication/README.md): 1,682 host cases and accepted target/human review. |
| Wide paired division | uint32/sint32 global destination pairs; signed truncation toward zero and numerator-sign remainder; zero and minimum/-1 errors. Array destinations and sint16 pairs excluded. | [Division tranche](../GOLEM-036/division/README.md): 2,528 host cases, published primitive vectors, stock/both Xtensa engines and accepted review. |
| 32-bit right shift | uint32 logical / sint32 arithmetic >> uint16 count 0..31; explicit error on invalid count, no masking; defined precedence/ordering. | [Right-shift tranche](../GOLEM-036/right-shift/README.md): 2,032 host cases, 10 frontend rejections, stock/both Xtensa engines, relocated/cleanup checks and 382 accepted human observations. |

## Boundaries to keep separate

1. GOLEM-037/039 renderer composition is native-only. Its extra config/hooks,
   helper, resources and completion semantics are application integration,
   not part of the general portable grammar. The generic core copies are
   identical; its driver/glue are not. Generic binding promotion needs a later
   separately scoped decision.
2. Portable arrays currently admit uint16/sint16/uint32/sint32 elements only.
   They do not implement float/bool arrays, heterogeneous tables, multidimensional
   arrays or matrix objects. Per-array one-block size and index-width limits
   remain admission rules, not inferred physical capacities.
3. Portable Subs have no source parameters, return values, locals, calls or
   recursion. Functions/objects, Do/Loop, Switch/jump tables, fixed-point,
   general bitwise/exponentiation/math functions and string/print facilities
   discussed in design examples remain outside this accepted implementation.
4. Accepted compiler semantics do not create snapshot synchronization,
   autonomous timers, cancellation or event transport. Those remain firmware/
   application contracts. Physical hardware performance and unrelated UART
   recovery are not compiler promotion acceptance criteria.

## Contracts that must survive the move

1. No statement terminators; newlines are whitespace. Keywords/builtins retain
   case insensitivity; user identifiers retain their existing case rules and
   assembly collision validation. No implicit declaration or type conversion
   is added to portable source.
2. Typed operator precedence and left-to-right evaluation, short-circuit logic,
   array index-before-value order and all-or-neither paired-result stores remain
   unchanged. Failure preserves the failed destination and earlier effects;
   it is not transaction rollback.
3. Language statuses retain 1 add, 2 subtract, 3 multiply, 4 zero-divisor,
   5 index, 6 signed division overflow, 32 Bounds/conversion/shift-count and
   33 non-finite/input-domain behavior. Interface and transport results remain
   separate from language status and successful-generation publication.
4. Preserve initialized little-endian storage, bool width/tag, scalar tags,
   generated IDs/offsets/lengths, root header/descriptor schema, session/entry
   checks and application/private ownership. Shared buffers are installed once;
   backend cleanup/native unload must not reset them.
5. Preserve the bounded portable float environment documented in GOLEM-032,
   disabling contraction/fast-math in relevant builds. Do not widen arithmetic
   implicitly or expand supported numeric domains during promotion.
6. Assembly owns installation, updates, pacing, selection, completion checks
   and cleanup. Golem source does not load files. Generated eZ80 call wrappers
   submit work and do not by themselves acknowledge VDP completion.
