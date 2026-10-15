# GOLEM-042 — Portable integer XOR

Add `^` to the maintained portable compiler so programs can toggle flags and
combine packed integer data on the VDP. One source must produce the same results
through stock buffered commands or a native VDPlet. The Author approved this
bounded tranche on 2026-10-15 after accepting native cleanup qualification.

## Contract

1. Binary `^` accepts two operands of the same uint16, sint16, uint32 or sint32
   type, and returns that type. Mixed types require the existing explicit
   numeric conversions; bool and float32 XOR reject. This is bitwise XOR,
   not exponentiation or short-circuit Boolean XOR.
2. XOR uses exactly the operand width. Signed values use their two's-complement
   bit patterns, including the sign bit; the resulting pattern denotes a signed
   value of that same width. XOR itself cannot overflow. Existing destination
   Bounds checks still apply and preserve a failed destination, retaining prior
   effects and using status 32. Operand evaluation failures retain their existing
   statuses and evaluate once, left to right.
3. Precedence, high to low: multiplication, addition/subtraction, right shift,
   XOR, comparisons, Not, And, Or. XOR chains associate left to right.
   Parentheses override grouping. Constant folding follows identical rules;
   declarations retain their existing literal-only initializers.
4. Support XOR wherever current integer expressions are accepted, including
   scalar/array assignments, indices, conversions, comparison operands and
   existing arithmetic/shift combinations. No new variable types or implicit
   declarations, wire schema, root layouts or scheduler are introduced.
5. Buffered emission uses stock bytewise XOR over a copied result. Native C
   operates on unsigned stored bit patterns, avoiding signed C overflow or
   implementation-defined narrowing. SDK/runtime/firmware and peer code remain
   unchanged. Other bitwise operators and left shift are separate work.

```golem
uint16 flags = 5
uint16 mask = 3
uint16 toggled = 0
sint32 signedValue = -1
sint32 signedMask = -2147483648
sint32 signedResult = 0
Sub toggle
    toggled = flags ^ mask
    signedResult = signedValue ^ signedMask
EndSub
```

After the call, toggled is 6 and signedResult is 2147483647.

## Subtasks

- SYN-001 [ ] Add the typed operator, precedence, constant/reference evaluation and expression serialization; document initialized examples.
- EMIT-001 [ ] Implement stock bytewise and native unsigned-pattern emission without changing output for source that does not use XOR.
- CHK-001 [ ] Add maintained type/rejection/precedence tests and independent buffered-model/generated-C checks for all four types, sign/extreme/bit patterns, arrays, Bounds, operand failures and short-circuit skips.
- CHK-002 [ ] Run maintained checks, focused sanitizers, default/relocated parity and a real assembler consumer; retain accepted fixture hashes.
- QUA-001 [ ] Package/pin new native code against the unchanged SDK, then qualify stock and actual Xtensa JIT/interpreter default/relocated behavior with bounded finite fixtures.
- VIS-001 [ ] Prepare and qualify a source-labelled XOR animation on a new dedicated Oryx profile, leave it stopped and provide the launch command. Automated runs must be both windowless and silent.
- DOC-001 [ ] Record implementation, resource/traffic evidence and limits under docs/tasks/GOLEM-042; update the maintained contract and task register.
- REV-001 [ ] Obtain human review and explicit publication approval before committing/pushing the new implementation.

## Acceptance and boundaries

All four integer types agree across independent reference, stock command
execution and generated native code, including signed sign-bit changes and
checked assignment behavior. Existing no-XOR packages retain byte identity
against the accepted GOLEM-041 compiler; historical fixtures are not rewritten.
Qualification and human staging are completed in the same tranche. No physical
flashing, SDK service expansion, peer game replay or unrelated UART repair is
authorized by this task. Multifile artifacts remain in the task silo; maintained
compiler/test improvements belong in src/ and tests/.
