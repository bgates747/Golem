# GOLEM-036 — D1b faithful wide integer math

Provide explicit uint32/sint32 state/intermediates and checked numeric operations
sufficient for Rally's existing units, without substituting floats or implicitly
widening 16-bit expressions. This follows the [accepted contract](GOLEM-033/accepted-contract.md),
requires accepted D0/D1a and Rally's pinned numeric oracles. The Author released
the first bounded wide-integer tranche after accepting GOLEM-035.

## Scope and dependencies

Fuji positions span 0..3,276,799 hundredth-world units. The example route product
180*2*32767 reaches 11,796,120 before division by 12800. Existing checked narrow
expressions cannot represent that intermediate. Programmers explicitly convert
or declare wide operands before forming it; preserve evaluation order and current
narrow behavior. Audit projection bounds before selecting any 64-bit operation.

Rally handoff revision 2 supplies independent numeric/route/contact/lookup vectors;
its 19 file hashes were verified during D0. Signed arithmetic right shift in those
formulas means floor division by a power of two, distinct from signed quotient
truncation. Preserve this explicitly if a shift/helper is selected. Direct map
projection can exceed signed 32; select an exact wider helper before claiming that
path. See GOLEM-034/rally-oracle-handoff-v2.json for the receipt.

Signed quotient truncates toward zero; remainder is n-q*d, with numerator's sign
or zero and abs(r)<abs(d). Zero divisor is status 4. Minimum signed value divided
by -1 is a checked overflow, preserving both outputs. Freeze its error code
before implementing it. Rally accepted language status 6 for minimum signed/-1 overflow in message
2026-10-13_17-18-34_rally_rally25-d0-received-division-six-eleven. Both outputs
remain unchanged, earlier effects remain, and status 4 still means zero divisor.
Interface status 6 is a separate channel. Both paired results pass checks before either store.
Narrowing is explicit, checked and numeric, not a wrapping bit reinterpretation.

- INV-001 [x] Receive pinned Rally formulas, unit/range audit and independent vectors, including projection products.
- NUM-001 [ ] Freeze widths, Bounds, intermediate overflow rules, conversions, signed division/remainder and division-overflow status.
- SYN-001 [x] Add explicit wide declarations/operands, typed comparisons and conversions; retain existing 16-bit behavior.
- EMIT-001 [ ] Implement checked add/subtract/multiply/compare/divmod and narrowing on both supported backends.
- STO-001 [x] Extend GOLEM-035 arrays/descriptors/fixtures to wide elements without reinterpreting prior installed layouts.
- HLP-001 [ ] Where needed, specify an exact checked wider-intermediate mul-div helper or separately reviewed 64-bit requirement; do not promise general 64-bit support prematurely.
- CHK-001 [ ] Verify extrema, sign combinations, zero divisor, minimum/-1, remainder identities, intermediate failures, paired-store preservation and narrowing.
- CHK-002 [ ] Compare route/world/projection replay against Rally's independent oracle on stock and actual Xtensa engines, with resource/traffic receipts.
- VIS-001 [x] Prepare a source-labelled Oryx review grounded in the agreed Rally numeric fixture.
- REV-001 [ ] Obtain human acceptance and explicit commit/push approval.

## Delivery boundary

Split into bounded reviewed implementation tranches rather than promising this
entire numeric surface in one run. Missing buffered support rejects explicitly;
it cannot silently change algorithms or create incompatible semantics. Keep
multi-file reports and fixtures in GOLEM-036/. Golem owns language/emission;
Rally owns authoritative game units and independent results.

## First bounded tranche: wide integer foundation

Implement uint32/sint32 globals and Array elements, checked + / -, all six
same-type comparisons and explicit checked conversions among the four integer
types. Type tags 7=uint32, 8=sint32 preserve existing 1..6. Narrow source expressions
retain their current width: widen operands explicitly before forming a wide
expression. Values/Bounds are numeric and inclusive, packed little endian.

This tranche rejects wide multiplication/divmod/shifts and conversions between
wide integers and floats explicitly. Those remain subsequent reviewed numeric
steps, retaining existing narrow arithmetic/float behavior. Initial array indices
remain uint16/sint16; wider indices require explicit checked narrowing. Four-byte
elements admit at most 16383 entries in the current one-block profile.

Native intermediates may use checked host C 64-bit addition/subtraction, without
introducing source-language 64-bit types. Buffered code uses bounded byte carry
and sign checks; both preserve failed destinations and prior effects, and publish
no success after failure. Oryx review will use wide Rally-scale state and retained
arrays, narrowing only proven screen-relative differences for drawing.

- WID-001 [x] Implement/qualify the first wide storage, + / -, comparison and integer-conversion tranche with Oryx review.
- WID-002 [ ] Follow with checked wide multiplication and paired signed/unsigned division against the accepted status/rounding contract.
- WID-003 [ ] Complete shifts/exact helpers and full Rally route/contact/projection replay as separately bounded reviewed work.

## First tranche evidence / review checkpoint

The foundation is implemented and automatically qualified; see
[README/source/receipts](GOLEM-036/README.md). SYN-001/STO-001 cover the explicit
32-bit declarations/comparisons/conversions and width-aware retained layouts.
VIS-001 is prepared/stopped on Oryx. EMIT/CHK/NUM/HLP remain open for the later
operators/full numeric contract; WID-001 is accepted after human review. The Author subsequently explicitly approved commit/push.

- FND-001 [x] Implement explicit uint32/sint32 grammar, Bounds, casts and width-aware array descriptors without changing prior outputs.
- FND-002 [x] Emit bounded buffered carry/sign arithmetic and byte-wise comparisons, preserving failed destinations.
- FND-003 [x] Emit SDK-admitted native 32-bit code, packed byte access and finite scoped array views under unchanged limits.
- FND-004 [x] Pass sanitized host/model, direct published Rally primitive, maximum-array boundary and independent package reproducibility checks.
- FND-005 [x] Qualify stock and actual Xtensa JIT/interpreter, default/relocated layouts, physical faults, cleanup and 600-frame pixels/state.
- FND-006 [x] Freeze immutable source/package/receipt delivery and stage the source-labelled Oryx review stopped.
- FND-007 [x] The Author reported 'good test.'; retrieved results match590 observations/5846 bytes and runtime/fixture identities. The Author subsequently explicitly approved commit/push.

## Second bounded tranche: checked wide multiplication

The Author released multiplication only after accepted foundation commit 4a62c1d.
Artifacts live in GOLEM-036/multiplication/ to preserve the foundation sources,
packages and Oryx fixture. `*` retains existing precedence and left-to-right
evaluation. Both 32-bit integer types check the full mathematical product before
storing; overflow is language 3. Failed destinations and earlier effects retain
the existing contract. Wide division/shifts/helpers/full Rally replay stay open.

- MUL-001 [x] Implement typed multiplication, constant evaluation and checked native emission.
- MUL-002 [x] Implement one bounded stock shared 32x32 kernel with private full-product scratch and signed magnitude/range handling.
- MUL-003 [x] Verify extrema/sign combinations, nested products/precedence, skipped branches, prior effects and failed array writes against independent Rally vectors.
- MUL-004 [x] Qualify stock and both actual Xtensa engines/default-relocated layouts; record resources and traffic.
- MUL-005 [x] Freeze reproducible source/package delivery and stage a source-labelled Oryx visual review stopped.
- MUL-006 [x] Human review accepted; all 460 observations / 4,546 bytes and runtime/fixture identities verified. The Author explicitly approved commit/push after the mailbox check found no reply.

Multiplication implementation/automatic qualification is complete; see
[multiplication source, resources and receipts](GOLEM-036/multiplication/README.md).
MUL-005 is complete: immutable export rebuild and stopped Oryx identities pass.
MUL-006 retains the pending human/publication gate.
WID-002 stays open because paired division is a separate unreleased tranche.
