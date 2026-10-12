# Rally syntax additions — GOLEM-009 / SYN-001

The Author approved committing/pushing the INV-002 audit, then executing
SYN-001 only. Audit checkpoint `8f1afe1` was pushed. Sixteen independent samples
have been appended to the existing `examples.golem`; SYN-001 awaits review
before SEM-001. No compiler or emulator changes were made.

The additions illustrate all 18 missing and 3 partially covered source-example
inventory rows, often sharing one example across related operations. They reuse
the floor/conversion, procedure/call, matrix/rotation and float-expression
direction, and propose notation for the remaining math and admission operations.
Every new sample includes required declarations/initialization. Proposed type
and function names remain review candidates; brief safety/domain notes do not
replace SEM-001's full contract review.

The original 174 lines are byte-for-byte preserved. Structural document checks
verified identifiers, initialization, no terminators, parentheses, matrix literal
arity and deferred-builtin exclusions. EX-006 was compiled separately in resident
mode: all five artifacts match the maintained floor example. The constant-divisor
sample also satisfies the binary32 quotient proof over its stated 0..1000 domain.
Other new samples are proposals, not compilable or firmware-qualified programs.

`docs/tasks/GOLEM-009/syntax-additions.md` maps sample IDs to inventory entries;
`syntax-additions.json` records line locations, hashes, checks and validation
durations. Generated checks stay in its ignored build silo. Historical inventory
and coverage snapshots remain unchanged. GOLEM-010/011 and optimization work
remain deferred/parked; SEM-001 and REV-001 were not executed.

During SYN-001 review, the Author requested conversion functions between types.
EX-017 adds explicit integer/fixed/float conversions and fixed width/fractional
precision changes, including fixed16(4). Candidate notation for parameterized
targets is `convert(targetType, value)`, alongside existing ordinary type-call
conversions. The examples now total 17. The requirement is recorded under
SEM-001 for later decisions on precision loss, rounding and out-of-range values;
that subtask has not begun. No compiler changes were made.
