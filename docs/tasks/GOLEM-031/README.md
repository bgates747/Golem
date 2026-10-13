# Signed 16-bit arithmetic and conversions

The task-local shared compiler adds sint16 globals, inclusive Bounds, checked
addition/subtraction/multiplication, signed comparisons and explicit conversions
to/from uint16. Both buffered and native backends use the same two-byte
little-endian two's-complement layout and existing signed type tag 2. Production
compiler and generic VDPlet runtime/SDK remain unchanged. Qualification and
human review are recorded below.

```golem
sint16 position = -10
sint16 velocity = 2
uint16 screen = 118
bool negative = true
Sub tick
    position = position + velocity
    screen = uint16(position + 128)
    negative = position < 0
EndSub
```

Arithmetic is checked at every intermediate. Addition overflow uses status 1,
subtraction overflow status 2, multiplication overflow status 3. The failed
destination is preserved; earlier effects remain and subsequent statements do
not execute. Full signed range is -32768 through 32767. Bounds admit numeric
signed values, including host-written negative words. Mixed signed/unsigned
arithmetic or comparison requires an explicit conversion. Conversion preserves
the numeric value; negative sint16 cannot convert to uint16, and uint16 above
32767 cannot convert to sint16. Either conversion failure uses status 32.
Same-type conversions are allowed. Boolean conversions are unsupported.

Signed literals include -32768; arithmetic unary minus on variables is deferred.
Use checked `0 - value` for negation. For counters and paired divmod remain
uint16. Signed division and broader integer types are separate work. Constant
invalid arithmetic still fails compilation even in an unreachable branch.

Buffered comparisons map signed ordering to unsigned byte ordering by flipping
the sign bit. Addition/subtraction inspect sign changes; multiplication computes
magnitudes using the qualified unsigned helper, checks the signed result limit
and restores its sign. All carry bytes reside in private scratch. Bounds and
arithmetic failures propagate through logical helper calls before outer writes.
Native intermediates use sufficiently wide int32 arithmetic, with explicit
range checks before packing two-byte results; no signed overflow or wrapping
conversion is used to define the language.

## Visual review

[motion.golem](motion.golem) is the complete initialized program. Golem controls
signed position/velocity, displacement multiplication, reversal, conversion to
screen coordinates and colour selection. Resident VDU draws a square and a gray
vertical zero marker; assembly owns VSYNC pacing and alternates backends every
twelve frames. The square moves across the origin, red at negative positions and
green at zero/positive positions. After 600 calls it stops green to the right of
the origin. State is 18 bytes and must be explicitly installed.

On Oryx, once staging finishes:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-031/.emulator
./fab-agon-emulator
```

Follow the single onscreen dir instruction, close and report whether it worked.
The agent retrieves saved results; no second-terminal verifier is required.

## Verification

[signed.golem](signed.golem) covers arithmetic, all six comparisons, negative
Bounds, conversion failures, same-type casts and conversion inside short-circuit
conditions. Prior unsigned/Boolean/division/loop fixtures remain in host coverage.

```bash
.venv/bin/python docs/tasks/GOLEM-031/host_checks.py
.venv/bin/python docs/tasks/GOLEM-031/fixture.py all
.venv/bin/python docs/tasks/GOLEM-031/stage_oryx.py
```

See host-checks.json and evidence/ for receipts. Actual-target fixtures use
representative cases within finite guest capacity. No physical board is flashed;
emulator timings are functional evidence only.

Host checks pass **14,912 cases, 75 grammar/reference cases and 93 rejections**
with ASan/UBSan, ELF admission and maintained checks. Stock signed qualification
passes 66 calls/67 observations/1,072 bytes; motion logic passes 29 calls/30
observations/540 bytes. Each actual Xtensa engine passes 132 calls/1,068
observations/10,680 bytes with default and relocated IDs. Each 600-frame visual
run passes 481 observations/4,729 bytes, all 56,320 final playfield pixels and
actual MOS return; private cleanup preserves shared state.

Host qualification took 74.6 seconds on the Pi. Oryx stock checks took 2–4 seconds,
ESP32 numeric suites about ten seconds per engine, and visual runs 19–22 seconds.
The dedicated Oryx profile is hash-verified/stopped with stale results removed.
The Author accepted review with “looks good”. Retrieved results match all 481
observations/4,729 bytes and runtime identities pass. Keyboard success was not
separately stated. The Author explicitly approved commit/push on 2026-10-13.
