# GOLEM-036 — D1b faithful wide integer math

Provide explicit uint32/sint32 state/intermediates and checked numeric operations
sufficient for Rally's existing units, without substituting floats or implicitly
widening 16-bit expressions. This follows the [accepted contract](GOLEM-033/accepted-contract.md),
requires D0 and Rally's pinned numeric oracles, and awaits Author release.

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
projection can exceed signed32; select an exact wider helper before claiming that
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
- SYN-001 [ ] Add explicit wide declarations/operands, typed comparisons and conversions; retain existing 16-bit behavior.
- EMIT-001 [ ] Implement checked add/subtract/multiply/compare/divmod and narrowing on both supported backends.
- STO-001 [ ] Extend GOLEM-035 arrays/descriptors/fixtures to wide elements without reinterpreting prior installed layouts.
- HLP-001 [ ] Where needed, specify an exact checked wider-intermediate mul-div helper or separately reviewed 64-bit requirement; do not promise general 64-bit support prematurely.
- CHK-001 [ ] Verify extrema, sign combinations, zero divisor, minimum/-1, remainder identities, intermediate failures, paired-store preservation and narrowing.
- CHK-002 [ ] Compare route/world/projection replay against Rally's independent oracle on stock and actual Xtensa engines, with resource/traffic receipts.
- VIS-001 [ ] Prepare a source-labelled Oryx review grounded in the agreed Rally numeric fixture.
- REV-001 [ ] Obtain human acceptance and explicit commit/push approval.

## Delivery boundary

Split into bounded reviewed implementation tranches rather than promising this
entire numeric surface in one run. Missing buffered support rejects explicitly;
it cannot silently change algorithms or create incompatible semantics. Keep
multi-file reports and fixtures in GOLEM-036/. Golem owns language/emission;
Rally owns authoritative game units and independent results.
