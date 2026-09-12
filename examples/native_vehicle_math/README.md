# Vehicle arithmetic primitives

This kernel uses a diagnostic80-byte input layout, **not** Rally's production
80-byte admitted-state ABI. Field names and offsets in `kernel.golem` describe
the input/output records. The standard native_admission loader reads80 output
bytes plus16 bytes of status/proof storage after each finite call.

`DivModPositive` wraps an input up to6553599 by the real Fuji/oval lap lengths,
then divides the wrapped Fuji remainder by100. The quotient remains bounded u16;
the new s32 remainder path carries positive values wider than65535. Compile-time
binary32 reciprocal validation still checks every integer in the declared domain.
`StorePositive32Floor` uses stock command41's32-bit output only after proving a
nonnegative source<=16777215. It never sends a negative float to unsigned conversion.

`LessThan` returns0/1 from matching fixed-width operands, including full signed32
endpoints and unsigned65535. It compares zero-extended ordered bytes, flipping the
top sign bit for signed types. No conditional operand can become the VDP's0xffff
sentinel. The destination cannot overlap either operand; the operands may alias
each other for the well-defined equal-value case. Fixed Q8 comparisons use their
raw signed representation and preserve ordering.

`MatAbs` loads a distinct scalar destination and clears the IEEE sign bit. It
preserves the source and yields positive zero for either signed input zero. The
native probe compares the complete output bytes, including subnormal/sign cases
and untouched sentinel neighbours around the4-byte conversion results.

From the AgonArcade-rally19 execution worktree:

```sh
.venv/bin/python docs/tasks/RALLY-19/probe_vehicle_math.py wide-primitives-a
.venv/bin/python docs/tasks/RALLY-19/probe_vehicle_math.py wide-primitives-b --offset 288
```

Fresh run names are required for reproduction. Together these cover the24x24
signed-boundary operand pairs. Batching preserves the existing loader's512-case
limit; the initial576-case attempt was rejected with guest code4 before tests,
and its evidence remains under wide-primitives. These primitive checks do not
complete the vehicle renderer, visual comparison or full-scene performance goal.
