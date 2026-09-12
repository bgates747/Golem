# Typed element lookup and unsigned conversion (R19-06 in progress)

`make_kernel.py` generates readable Golem fixture source containing 32 twelve-byte
signed records. It is a fixture generator, not a VDU emitter or another compiler.
The existing C++ Golem compiler handles declarations, guards, integer offset
calculation, command-field relocation and matrix arithmetic.

```sh
.venv/bin/python examples/native_elements/make_kernel.py
make -C src
.venv/bin/python tests/run_hosted.py
cd /home/smith/Agon/mystuff/AgonArcade-rally19
.venv/bin/python docs/tasks/RALLY-19/probe_elements.py UNIQUE_NAME
```

The native run reuses the opaque `native_admission` transport and its 96-byte
readback layout. It supplies index, raw progress and a diagnostic float; the VDP
selects the record and calculates results. Eight cases cover a byte-offset carry,
invalid retained-state behavior, unsigned widening through 65535, positive floors
near 512/32768/65534 and interval division at 6399/6400/6401/3276799. The float is
an arithmetic test input only; production Rally's frozen ABI still contains only
integer state. All tests use stock, normal-clock, headless profiles.

`LoadElement(dst,src,index,status)` requires complete repeated typed layouts and
disjoint destination/source/status/index ownership as checked by the compiler.
It clears 0..1 status first, admits index <= recordCount-1, then patches a separate
copy instruction's source offset. Offset calculation is exact bounded integer
constant multiplication (double/add); it does not depend on host projection or
floating-point rounding. Invalid indices preserve the selected record.

`WidenUnsigned(dst,src)` explicitly zero-extends u8/u16 to a signed s16/s32 field
whose range contains the source. Overlap and narrowing are rejected.
`StoreU16Floor(dst,scalar)` accepts proved finite values 0..65534 and emits the
nonnegative stock transform conversion without a large bias. The earlier signed
floor path remains unchanged. Copying matrix fields now uses their actual
propagated matrix bounds, preventing an arbitrarily narrow field declaration
from weakening the destination's range check.

This checkpoint qualifies helpers required to select real road coefficients.
It does not complete the road section or R19-06. See the Rally execution
worktree's `docs/tasks/RALLY-19/SECTION.md` for current integration notes and its
root TODO.md for the sole execution register.
