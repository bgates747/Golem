# GOLEM-024 — First shared frontend and layout prototype

The task-local C++ prototype parses one explicitly typed scalar program and
produces identical common interface, typed-program, initialized-state and assembly
layout artifacts for buffered/native target selections. It establishes the shared
frontend boundary required by the Author. It **does not emit executable buffered
commands, native C or a VDPlet**; the maintained compiler is unchanged. Read
[shared-example.golem](shared-example.golem) for the complete initialized source.

## Use the prototype

From the repository root:

```bash
.venv/bin/python docs/tasks/GOLEM-024/run_checks.py
docs/tasks/GOLEM-024/build/planner --target buffered \
    docs/tasks/GOLEM-024/shared-example.golem docs/tasks/GOLEM-024/build/buffered
docs/tasks/GOLEM-024/build/planner --target vdplet \
    docs/tasks/GOLEM-024/shared-example.golem docs/tasks/GOLEM-024/build/vdplet
```

Repeated inclusive `--reserve FIRST:LAST` options precede the source/output
arguments. For example, `--reserve 1:99` moves the shared state from ID 1 to 100
for either selection. IDs 0 and 65535 remain unavailable. No GUI, emulator,
firmware build or hardware deployment is performed.

## Source supported by this tranche

Explicitly initialized uint16/float32 globals, optional inclusive numeric Bounds,
one or more named parameterless Subs, and assignments using literals/globals,
parentheses, +, - and *. Multiplication has higher precedence; + and - associate
left to right. Keywords ignore case, identifiers retain case, newlines are
whitespace and semicolons reject.

Variable operands must have the destination's type. Numeric literals take the
expression's declared type and must fit it; no variable-type inference or implicit
conversion is supplied. Global initializers are numeric literals. All globals
precede Subs. Control flow, locals, functions/parameters, conversions, division,
draw calls and other types reject rather than imply implementation. The larger
[research example](../GOLEM-023/native-example.golem) remains a future language
target; this smaller slice is deliberately the shared foundation.

The source is parsed/type-checked once through `portable.hpp`'s common model.
Logical entry IDs follow declaration order. Target selection only changes the
separate requirements descriptor, which states that there is no executable output
or target qualification. Accepting a plan for both selections is not evidence that
either emitter currently supports its operations.

## Generated artifacts

| File | Contents |
| --- | --- |
| interface.json | Experimental common global types/Bounds, packed offsets, shared ownership, external reservations, language-status field and logical entries |
| program.json | Typed expression/assignment trees with source positions; global indices refer to interface declaration order |
| state.bin | Initial little-endian global bytes followed by zero language status; raw data, not a VDU upload command stream |
| layout.inc | Real assembly equates for fields/state and logical entry IDs; no executable wrapper or call packet |
| target.json | Selected target, native-loader requirement and explicit lack of executable/qualification evidence |

Only target.json differs between target choices for the same source/reservations.
Initialization bytes are separate from the interface: changing initial values
does not change field offsets or call identity. Changing procedure bodies affects
the typed program, while backend-private scratch/code never determines common
data placement. Later emitters must honour the same plan and the recorded external
reservations. Dual-installed code will also need coordinated private IDs/ownership;
this planner does not allocate those resources yet.

## The example's persistent state

| Field | Offset | Bytes | Initial value |
| --- | --- | --- | --- |
| calls | 0 | 2 | 0 |
| amount | 2 | 2 | 3 |
| accumulated | 4 | 2 | 0 |
| input | 6 | 4 | binary32 2.0 |
| scale | 10 | 4 | binary32 1.5 |
| bias | 14 | 4 | binary32 0.25 |
| output | 18 | 4 | binary32 0.0 |
| runtime_error | 22 | 2 | 0 |

Total: 24 bytes. Float fields intentionally need not be naturally aligned; byte
serialization and the host model use explicit little-endian encoding rather than
casting the packed block to a C struct. This protects the common layout from host
or native alignment choices. Assembly symbols are checked together for case-folded
collisions and ez80asm's 64-character symbol limit.

`advance` is logical entry 0 and `compute` entry 1. The reference starts with
calls=0 and accumulated=0. Two advance calls produce calls=2, accumulated=12.
compute produces output=3.25, then output=-5.75 after input is changed to -4.0.
These are host reference results, not execution of either target.

Application-owned state persists between entries/invocations; only language
status resets at invocation. Backend cleanup will have to preserve this state.
Initial installation/reset is explicit. The prototype has no runtime dispatcher,
capability probe, handle binding or live backend change implementation.

## Reference semantics and scope

The host evaluator admits the full state block before running assignments:
values must be finite and within declared Bounds. uint16 arithmetic checks every
intermediate; candidates commit only after destination checks. Failure stops the
job, preserves its failed destination and retains earlier successful writes.
Correcting inputs permits a later successful invocation and clears prior status.

Prototype language status codes are 0=success, 1=uint16 addition overflow,
2=subtraction underflow, 3=multiplication overflow, 32=Bounds failure,
33=nonfinite float. Codes 32/33 and full entry admission are proposed common
rules for this experimental slice, not changes to maintained profiles or the
runtime's VDPL_E_* namespace. In particular, existing resident-scalar input
admission remains its documented host responsibility.

Known invalid constant intermediates and constant destination violations reject
before publication. Float initializers are checked both as supplied and after
binary32 rounding; nonzero literals lost to float32 zero reject. Reference
arithmetic rounds as host float32 with contraction disabled and rejects nonfinite
results. Physical subnormal/underflow and other exceptional behavior still need
qualified shared rules before float execution can be labelled portable.

All semantic, reservation and symbol checks precede file creation/overwriting.
An actual I/O failure during publication can still leave partial output, following
the maintained compiler's proportionate publication model.

## Checks and evidence

[checks.json](checks.json) binds the source files and production lexer/allocator
dependencies to the result. The reproduction script runs:

1. ASan/UBSan host contracts: 44 checks and 42 rejection cases, including
   precedence, typed operands, Bounds/rounding, nonfinite inputs/results,
   failed destinations, earlier writes, repeated calls and reservation limits.
2. Both target selections at default and relocated IDs: eight byte-identical
   common artifact pairs, with independent expected initial bytes and layout.
3. Ten CLI rejection cases preserving prior files or leaving rejected output
   absent, including late assembly-symbol validation and exhausted namespaces.
4. Two real ez80asm consumers, each checking 33 exported equates (99 binary
   bytes), covering both normal and relocated layouts.
5. Maintained compiler checks, including its existing 132 rejection cases.

All target execution/qualification flags are false. No new runtime code, graphical
fixture or physical run forms part of this evidence.

## Next implementation boundary

After reviewing this foundation, a sensible next tranche is a small checked
uint16 emitter pair using this exact model and shared state. That would require
actual generated buffered commands and native packaging, common entry/error
semantics, separate private ownership, and appropriate execution qualification.
Existing buffered scalar helpers generally address per-variable buffers; their
emitter must deliberately adapt to the common packed offsets rather than inherit
the old storage layout. Sharing arithmetic algorithms alone does not provide
interchangeable state.
Start with integer behavior, where exact matching is straightforward, then add
portable float operations and larger control-flow features through shared cases.

Startup dispatch and state-preserving live selection remain later roadmap stages.
The existing compiler's raw/resident modes and artifact contracts are unchanged.
Task artifacts stay in this silo until promotion is agreed.
