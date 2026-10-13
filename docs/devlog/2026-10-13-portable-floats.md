# Float32 arithmetic and numeric conversions on both backends

The Author accepted GOLEM-031 and authorized committing/pushing before float32
work. Checkpoint b37aa3c is pushed. GOLEM-032 adds basic float arithmetic,
comparisons and integer conversions to both task-local emitters. Stock and
actual Xtensa numeric cases pass; human visual review and explicit commit/push
approval remain pending. Production compiler and generic SDK/runtime are unchanged.

The bounded numeric contract admits finite normal inputs and signed zero,
rejects NaN/infinity/subnormal globals with status 33, and rejects subnormal
literals. Expression zero is canonical positive zero; arithmetic subnormal
results flush to zero. Non-finite results report 33. Each operator rounds
independently; native and host builds disable contraction. Tested nearest/ties-even
and smallest-normal/underflow boundaries match on the qualified engines. This
is not full IEEE exception/subnormal support or physical numeric qualification.

Integer-to-float conversion is exact. Float-to-integer truncates toward zero,
then checks numeric range; safe open pre-cast domains ensure no undefined
out-of-range cast. Negative fractions above -1 convert to unsigned zero.
Conversion/Bounds failure is status 32 and preserves destinations and prior
effects. Decimal Bounds thresholds are rounded inward to preserve their numeric
inclusive meaning, including tests immediately around +/-10.1.

Stock arithmetic uses 1x1 matrices and the existing affine enable flag. Safe
float-to-integer conversion transforms a nonnegative magnitude to unsigned
32-bit scratch before restoring the sign; integer-to-float uses explicit
signed-32-bit widening. Host tests caught stale sign-extension bytes when a
positive conversion followed a negative one; widening scratch is now reset in
full on every invocation. A shared entry-admission helper keeps multiple-Sub
packages bounded. Matrix/private cleanup retains application state.

ASan/UBSan, independent command-model/generated-C behavior, ELF admission and
maintained checks pass 24,494 cases, 84 grammar/reference cases and 101
rejections. Prior signed/unsigned arithmetic, comparisons, Boolean, divmod and
Range fixtures remain covered. The finite task-local guest tape was expanded
from 64 to 128 KiB in unused memory at 0x050000; code/observations remain below
that region and individual packets remain bounded to 65535 bytes.

The complete source drift.golem moves a square with 0.375 velocity multiplied
into 0.75-pixel displacement. Golem owns movement, reversal, casts and colour;
resident VDU draws the square/origin, assembly paces with VSYNC and alternates
backends every twelve frames. It ends at position -30, red to the left of zero,
after 600 calls. Shared state is 24 bytes and includes an unaligned float field.
Buffered installation is 15,865 bytes; native package is 2,104 bytes including
1,180 text bytes and four data bytes. No physical target was flashed; emulator
times are functional evidence only.

Final host qualification took 84.4 seconds on the Pi. Stock float
checks pass 42 calls/43 observations/1,118 bytes; motion logic passes 29 calls/30
observations/720 bytes. Each actual Xtensa engine passes 84 calls/1,109 observations/
11,090 bytes at default and relocated IDs. Both visual runs pass 600 calls/637
observations/6,289 bytes, 56,320 final pixels, MOS return and retained state after
cleanup. Oryx stock checks took 3–4 seconds, numeric ESP32 suites about ten seconds
per engine, visual runs 20–23 seconds. Human staging is hash-verified/stopped with
stale results removed. Acceptance and explicit commit/push approval remain pending.

The Author accepted review with “tests are good”. Retrieved results match all
637 observations/6,289 bytes and runtime identities pass. Keyboard success was
not separately stated. The Author explicitly approved commit/push on 2026-10-13.

GOLEM-032 is accepted and removed from TODO at the explicit commit/push checkpoint.
