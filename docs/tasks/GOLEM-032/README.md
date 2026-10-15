# Float32 arithmetic and numeric conversions

This task-local prototype adds float32 addition/subtraction/multiplication,
comparisons and explicit conversions to/from uint16/sint16 on both targets.
Stock execution uses scalar matrix commands; native execution uses generated C.
Common packed storage uses float type tag 3 and four little-endian bytes.
Qualification and human-review status are recorded below. Production compiler
and generic VDPlet runtime/SDK remain unchanged.

```golem
float32 position = -10.0
float32 velocity = 0.375
uint16 screen = 118
bool negative = true
Sub tick
    position = position + velocity
    screen = uint16(position + 128.0)
    negative = position < 0.0
EndSub
```

## Numeric contract

1. Each arithmetic operator rounds independently to binary32, with nearest/ties-even
   behavior qualified on the tested targets. Native and host builds disable
   multiply/add contraction and fast-math is not enabled. Mixed variable types
   require an explicit conversion.
2. Inputs admit finite normal values and signed zero. NaN, infinity and subnormal
   externally supplied globals fail entry admission with status 33, including
   unused globals. Subnormal source literals are rejected. Non-finite arithmetic
   results also fail with status 33. This is a bounded portable profile, not a
   promise of the full IEEE-754 exception or subnormal environment.
3. Expression zero is canonical positive zero. Subnormal arithmetic results are
   flushed to positive zero. A host-written negative zero may remain in its
   original field; reads, computations and assigned results use canonical zero.
4. Inclusive Bounds check the numeric value. Compiler-generated threshold bits
   account for Bounds that lie between representable floats, without rounding an
   upper limit outward or a lower limit downward. Bounds failure is status 32.
5. uint16/sint16 to float32 conversion is exact. Float-to-integer conversion
   truncates toward zero, then checks the result's numeric range. Negative
   fractions above -1 therefore convert to uint16 zero. Neither wrapping nor
   reinterpretation defines conversion behavior. Conversion failure is status 32.
6. Failed destinations are preserved; earlier effects remain; later statements
   are skipped. Errors propagate through logical helpers. Signed and unsigned
   integer behavior is retained. Division, sqrt, pow and floor remain outside
   this tranche; For counters and paired divmod remain uint16.

## Implementation boundary

Buffered arithmetic creates 1x1 float matrices and uses stock add/subtract/scalar
multiply. The stock affine feature flag is enabled during installation. Matrix
scratch belongs to this package and cleanup preserves common application state.
An entry-admission helper is shared by named Subs to keep the package bounded.

Buffered integer-to-float conversion widens into freshly reset signed 32-bit
scratch before using the firmware's fixed/integer input format. Float-to-integer
conversion checks the open numeric domain first, transforms only a nonnegative
magnitude into a safe unsigned 32-bit temporary, then restores the sign. It does
not ask firmware to cast a negative or out-of-range float to an unsigned integer.
Native conversion also checks a safe domain before its int32 cast.

Float comparisons and Bounds use normalized bit ordering, avoiding differences
in floating comparison treatment of signed zero and tiny Bounds thresholds.
Normal arithmetic uses the firmware's existing float implementation; no VDP
firmware changes or software floating-point runtime are added.

## Visual review

[drift.golem](drift.golem) is the complete initialized program. A 0.375 velocity
is multiplied into 0.75-pixel displacement. Golem moves and reverses the square,
converts position to screen coordinates and chooses red for negative positions,
green otherwise. Resident VDU draws the square/gray origin; assembly owns VSYNC
pacing and alternates backends every twelve frames. After 600 calls it ends red
to the left of the origin, at position -30. The 24-byte state includes an
unaligned packed float field; it must be installed explicitly.

On Oryx, once staging finishes:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-032/.emulator
./flexe-agon-emulator
```

Follow the onscreen dir instruction, close and report whether it worked. The
agent retrieves saved results; no second-terminal verifier is required.

## Verification

[floats.golem](floats.golem) exercises arithmetic, all comparisons, Bounds,
conversions, copying and short-circuit failures. Host cases include raw rounding,
underflow, non-finite and signed-zero bit patterns. Previous integer, Boolean,
division and loop fixtures remain covered.

```bash
.venv/bin/python docs/tasks/GOLEM-032/host_checks.py
.venv/bin/python docs/tasks/GOLEM-032/fixture.py all
.venv/bin/python docs/tasks/GOLEM-032/stage_oryx.py
```

See host-checks.json and evidence/ for receipts. Actual-target tapes select edge
cases within the finite guest's capacity. No board is flashed; emulator timing
is functional evidence, not physical speed or hardware numeric qualification.

Host ASan/UBSan, command model/generated C, ELF and maintained checks pass
**24,494 cases, 84 grammar/reference cases and 101 rejections**. Stock float
checks pass 42 calls/43 observations/1,118 bytes; fractional motion logic passes
29 calls/30 observations/720 bytes. Each Xtensa engine passes 84 calls/1,109
observations/11,090 bytes at default and relocated IDs. Both 600-frame visual
runs pass 637 observations/6,289 bytes, all 56,320 final playfield pixels and
actual MOS return; private cleanup preserves shared state.

Steady arithmetic invocation remains six wire bytes. Including the resident
drawing call and VSYNC swap, a normal frame uses 15 wire bytes, excluding review
queries/backend labels. Arithmetic and conversions execute on the VDP.

The task-local guest reserves 128 KiB for its finite tape at 0x050000; guest code
and observations remain below that region. Individual packets still fit 65535
bytes. The enlarged tape retains rounding, underflow and precise Bounds cases.

Host qualification took 84.4 seconds on the Pi. Oryx stock checks
took 3–4 seconds; numeric ESP32 suites about ten seconds per engine; visual runs
20–23 seconds. The dedicated profile is hash-verified/stopped with stale results
removed. The Author accepted review with “tests are good”. Retrieved results match all
637 observations/6,289 bytes and runtime identities pass. Keyboard success was
not separately stated. The Author explicitly approved commit/push on 2026-10-13.
