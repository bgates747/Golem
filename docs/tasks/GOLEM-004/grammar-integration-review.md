# Grammar and scalar integration review

The next implementation should be a small shared token reader and a
terminator-free parser slice, preserving existing integer emission and the
scalar prototype's numeric behavior. The reviewed statement forms have clear
grammatical boundaries without significant newlines; they do not need an
optimizing compiler. Scalar integration must additionally unify allocation:
main and GOLEM-003 independently allocate buffers from ID 2 and cannot safely
be concatenated. Eighteen host probes confirm the current parser/specification
gap and expose one numeric diagnostic defect. This review implements neither
parser changes nor production promotion.

Prepared 2026-10-11 for [GOLEM-004](../GOLEM-004.md). Source baseline:
`c1483c2`; the full examined commit and
compiler fingerprints are in [parser-observations.json](parser-observations.json).
The [probe script](probe_parsers.py) builds existing compilers into this task's
ignored `build/` directory and preserves reviewed compiler source unchanged.

## Direction and implementation status

| Item | Review disposition | Current implementation |
|---|---|---|
| Statement boundaries | No terminators; newlines are whitespace | Both parsers still require semicolons |
| Names | Case-insensitive keywords/builtins, case-sensitive identifiers; explicit typed declarations | Existing main/prototype follow this split, but reserve different small keyword sets |
| Integer statements | Preserve declarations, assignment, chained addition, literal print and named For/Next | Main implements these in one parser/emitter |
| Finite scalar job | Keep readable bounded float input, signed output, Sub/EndSub and explicit floor/conversion | GOLEM-003 implements one rigid source shape and its own output profile |
| Branches | Working reviewed form is If/ElseIf/Else/EndIf; first matching branch only | Neither maintained frontend nor scalar prototype implements it |
| Do loops | Pre-condition Do While/Until; post-condition Do ... Loop While/Until | Not implemented; these are now preferred forms rather than an unresolved post-condition preference |
| Switch | Evaluate once, no fall-through; optional Default | Not implemented |
| Indexed Switch | Explicit Range(low,high), inclusive step-1 slots; fill holes with fallback | Design decision, not an existing backend feature |
| Shared Range uses | Iteration, membership, non-unit steps and Bounds naming are separate prototypes | Unselected extensions; do not make them prerequisites for scalar integration |

`examples.golem` contains independent illustrations with repeated names, not one
compilable translation unit. Each example has declarations and initial state.
Its whole-file rejection would not be evidence of a parser defect. Frozen Rally
source and executable main/scalar fixtures retain their historical syntax.

## Minimal grammar recommendation

The following is a proposed implementation contract, not a newly implemented
grammar. Keywords are matched without case sensitivity. Identifier spelling is
preserved. Whitespace, including newlines, separates tokens without ending
statements. Keep the existing `//` comments and string escape rules for this
tranche; the separate preference for `#` comments is not yet a migration task.

```ebnf
ordinary_program = { ordinary_statement }, EOF ;
ordinary_statement = print | u16_declaration | u16_assignment | for_loop ;
print = PRINT, STRING ;
u16_declaration = UINT16, identifier, [ ASSIGN, sum ] ;
u16_assignment = identifier, ASSIGN, sum ;
sum = term, { PLUS, term } ;
term = unsigned_integer | declared_u16_identifier ;
for_loop = FOR, declared_u16_identifier, ASSIGN, sum, TO, sum,
           { ordinary_statement }, NEXT, matching_identifier ;

scalar_program = FLOAT32, input_name, RANGE, LPAREN, signed_number,
                 COMMA, signed_number, RPAREN, [ ASSIGN, signed_number ],
                 SINT16, output_name,
                 SUB, entry_name,
                 output_name, ASSIGN, SINT16, LPAREN, FLOOR, LPAREN,
                 input_name, RPAREN, RPAREN,
                 ENDSUB, EOF ;
```

The punctuation in this EBNF is grammar notation, not Golem statement
terminators. The separate entry rules allow the scalar source contract to
remain narrow while using the same token reader. Keep main's vector-of-terms
integer emission and scalar::Spec as the initial semantic boundaries. A general
AST/IR, arbitrary expressions and general user function arguments/returns are
not needed to implement this slice.

For tokens, distinguish integer/decimal literals, strings, identifiers,
keywords and punctuation; retain source spans for diagnostics. Do not parse
all integer tokens through double, and range-check conversions without an
uncaught library exception. Reject numeric text such as `1foo` as a malformed
literal rather than inventing two tokens. Preserve adjacent string/keyword
boundaries such as `print "foo" print "bar"`.

For the new syntax path, reject a statement semicolon with an explanatory
diagnostic; a semicolon inside a string remains ordinary string data. Do not
strip punctuation from raw source as a migration strategy. Parentheses and
commas inside expressions/calls remain meaningful punctuation, not statement
terminators.

## Concrete boundary and diagnostic cases

| Proposed source | Required interpretation |
|---|---|
| `print "foo" print "bar"` | Two print statements, no separator needed |
| `uint16 a = 1 uint16 b b = a + 2` | Two explicit declarations, then an assignment |
| `uint16 a = 1 uint16 b b = a +` followed by newline and `2` | The addition continues across the newline |
| `uint16 a = 1 a = 1 + print "tick"` | Missing expression operand at print, not a guessed statement split |
| `uint16 a = 1 b = 2` | Undeclared b; assignment never declares it |
| `uint16 a a = 1 a = a + 2` | A declaration and two assignments |
| `uint16 a = 1foo` | Malformed numeric token |
| An integer literal too large for the conversion library | Source-located range error; no process abort |
| `uint16 i For i = 1 To 2 print "tick" Next other` | Mismatched closer naming other instead of i |
| An unmatched Else, Case, EndSub or Next | Unexpected block delimiter at its source location |

Do not promise unrestricted expression statements or callable variables.
For example, `a = b (compute())` becomes ambiguous if arbitrary call-valued
expressions and parenthesized standalone statements are both later introduced.
The first slice has neither. Named calls such as `compute()` can be introduced
as a specific statement form; builtin numeric calls are explicit expression
forms. Extend the grammar deliberately when new forms genuinely overlap.

Each block parser owns its closing tokens. For/Next checks the repeated counter
name. If bodies stop at ElseIf, Else or EndIf at their own nesting level; require
Then and explicit EndIf rather than introducing a dangling single-line variant.
Do bodies stop at Loop; select a condition at the header or at the closer, not
both. Switch bodies stop at Case, Default or EndSwitch. The selector ends before
the reserved Range modifier or the first Case/Default. Implement these block
forms in later slices rather than adding unimplemented promises to the first
parser release.

Main currently accepts names such as `If` and `Range` as variables, and the
scalar prototype accepts `If` as its input name. New keyword reservation will
therefore break some previously accepted source. Publish the reserved set with
the grammar change and diagnose the conflict; do not silently case-fold names.
Treat the use of `Range` in a float declaration as interval metadata, distinct
from its switch-table or proposed sequence role. Keep that existing spelling
for the first scalar integration; `Bounds` has not been selected.

## Allocation and packaging contract

Main's Compiler allocates ordinary variables and loop buffers from ID 2 and
reserves ID 1 for a three-byte addition accumulator. GOLEM-003 Lowering also
starts private allocation at 2; changing its Layout only moves five named
buffers, not private scratch/helpers. Concatenating their streams would overwrite
storage. Relevant sources are `src/golemc.cpp`'s declareVariable,
allocInternalBuffer and ensureAcc, and `scalar_backend.hpp`'s Layout/scratch/run.

Before combining features, introduce one checked allocation/ownership policy
for ordinary globals, accumulator, scalar scratch, helper programs and named
entries. Claim externally reserved asset/interface ranges before allocating
compiler-owned IDs. Reject namespace exhaustion and reserved ID 65535. Do not
turn the prototype's >=1000 compatibility IDs or the illustrative asset range
in design Axis 23 into the language ABI. The allocator need not implement a
heap, local frames or module linking.

Pass allocation into the scalar emitter; do not add a numeric offset to encoded
bytes afterward. IDs occur inside literals and nested command operands as well
as headers, so blind byte replacement is unsafe. A compatibility policy can
retain the exact old Rally IDs for oracle checks. Under another layout, compare
decoded operations, ownership, spans and numeric behavior rather than requiring
whole-stream byte equality.

Keep stock scalar load, true floor and signed-word conversion expansions intact.
Host inputs and their binary32 rounding must satisfy the declared finite bounds;
declarations do not insert runtime float admission. Preserve tiny-negative and
signed-boundary cases. The 25-buffer prototype is serialized and nonreentrant;
general callbacks/concurrent jobs are not part of the first integration.

Ordinary output is a payload for the current loader's root buffer; the scalar
bootstrap is a VDU installation stream that does not invoke its job. Do not
silently reinterpret one as the other. Recommend an explicit resident output
profile alongside the existing ordinary output contract, with the exact CLI
spelling reviewed during that implementation. Publish input/output/entry
metadata and cleanup ownership. Installing, updating, calling and cleanup remain
distinct lifecycle actions. Large command groups are allowed; the current small
job needs one root block and three finite helper programs, not a per-instruction
block layout.

## What the host probes establish

[parser-observations.json](parser-observations.json) records all 18 source cases,
return codes, messages, build flags and source fingerprints. Both existing
compilers build with C++17, -Wall -Wextra -Werror and -O2. These are parser
observations, not a new run of all scalar sanitizer/numeric tests.

The legacy statements and fixtures still compile; terminator-free equivalents
reject at required-semicolon checks. Multiline addition already compiles, so
newline insignificance is real existing behavior. Float interval input 0.75
within Range(-1,1) compiles, while Bounds and a third declaration-Range argument
reject. This supports keeping numeric intervals distinct from sequence proposals.

An oversized integer literal aborts main with std::out_of_range from std::stoul
(return code -6), whereas normal parse errors return 1. The CLI catches only
CompileError. Numeric-token diagnostics belong in the next parser tranche;
this review does not change that code. Rejected probes preserve pre-existing
outputs; core dumps were disabled for the probe subprocesses.

Main also accepts a For endpoint of 65535 at compile time. This is not evidence
that it runs correctly. The stock bufferConditional implementation initializes
32-bit operands to -1, fills only the requested low bytes and rejects a value
that remains -1; raw 0xffff word operands can therefore hit the invalid-operand
sentinel. A wrapping u16 loop is another termination concern. Before bringing
general loops/comparisons into resident jobs, normalize comparisons or reject
unqualified domains, and prove finite iteration. No firmware execution occurred.

## Recommended implementation-sized sequence

| Slice | Concrete change | Acceptance evidence |
|---|---|---|
| REC-001 | Shared token reader/source diagnostics, terminator-free existing ordinary statements and the narrow scalar source shape | Adjacent/multiline source, explicit declarations, complete/malformed literals, nested For closers and casing; paired new/legacy source emits the same integer/scalar commands |
| REC-002 | Shared checked allocation and an explicit resident output profile; integrate the selected scalar operation into main's toolchain | Collision/reservation/exhaustion cases, relocated spans and helper targets, exact compatibility oracle, interface/cleanup ownership, fresh inputs without reinstall |
| REC-003 | Qualify the integrated finite scalar job with stock firmware before production promotion | Numeric results for tiny negatives, boundaries and repeated input changes; return/event lifecycle and stack behavior; installation versus recurring byte counts |

These are proposed slices, not additional active checklists. Promote an agreed
implementation task into TODO before beginning it. The first code change should
be REC-001, with no new arithmetic behavior or silent output-profile migration.
Preserve the frozen Rally oracle and historical GOLEM-003 fixtures. Migrate
maintained fixtures deliberately to the new syntax and require equivalent bytes;
do not maintain a permanent second legacy frontend unless compatibility needs
justify that extra work. Existing users need a concise migration note.

Do not include general If/Do/Switch backend support, stepped Range normalization,
membership, Bounds renaming, function parameters, an optimizing IR, native
Xtensa modules or firmware modifications in REC-001. Unsupported constructs
should produce useful errors. Indexed dispatch will later need checked signed
normalization, a supported table budget, safe patch-then-execute and fallback
tests; it is not obtained by recognizing a Range token alone.

GOLEM-003 remains host-qualified experimental code. This review and the grammar
reconciliation are complete; compiler implementation and production promotion
remain deferred pending a new task decision.
