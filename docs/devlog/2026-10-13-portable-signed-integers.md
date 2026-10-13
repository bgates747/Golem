# Signed 16-bit arithmetic on both backends

The Author accepted GOLEM-030 and authorized committing/pushing all current work
before the signed tranche. Checkpoint 3a1c9c8 is pushed. GOLEM-031 adds signed
16-bit declarations, Bounds, arithmetic, comparisons and explicit conversions to
both task-local emitters. Host qualification passes; human review and explicit
commit/push approval remain pending. Production compiler and generic SDK/runtime
are unchanged.

Two-byte little-endian two's-complement storage uses existing type tag 2. The
signed range is -32768 through 32767. Same-type addition/subtraction/multiplication
check each intermediate and use operator error statuses 1/2/3. Numeric conversions
to/from uint16 admit values rather than wrapping or reinterpreting bits; failures
use status 32. Same-type casts are allowed. Mixed arithmetic/comparisons require
an explicit conversion. For counters and paired divmod remain uint16; unary
variable minus is deferred, with checked 0 - value available.

The stock implementation maps signed order to unsigned byte order by flipping
the sign bit. Addition/subtraction check sign changes using private masks and
carry scratch. Multiplication reuses the qualified unsigned helper on magnitudes,
checks positive/negative signed limits and restores the sign. Bounds admission
uses biased signed ordering. Private status now also records Bounds failures so
conversion errors inside short-circuit helpers propagate before any outer write.
Native int32 intermediates cover every product of two signed words without C
signed overflow; result checks precede packing.

ASan/UBSan, independent command model/generated-C behavior, ELF admission and
maintained checks pass 14,912 cases, 75 grammar/reference cases and 93 rejections.
The full earlier arithmetic, comparisons, Boolean, division and Range fixtures
remain covered. Host qualification took 74.6 seconds on the Pi.

The source motion.golem controls velocity, displacement multiplication, signed
position, reversal, numeric screen conversion and red/green colour. Resident VDU
draws the square and gray zero marker; assembly owns VSYNC pacing and alternates
backends every twelve frames. The square moves through negative and positive
positions, ending green on the right after 600 calls. State is 18 bytes, buffered
installation is 8,096 bytes and the native package is 1,732 bytes including 820
text bytes and four data bytes. No board was flashed; emulator timing remains
functional evidence only.

Stock signed checks pass 66 calls/67 observations/1,072 bytes; stock motion logic
passes 29 calls/30 observations/540 bytes. Both actual Xtensa engines pass 132
calls/1,068 observations/10,680 bytes at default and relocated IDs, and 600-frame
visual runs with 481 observations/4,729 bytes and 56,320 final pixels. MOS return
and shared-state retention after cleanup pass. Oryx stock checks took 2–4 seconds,
numeric ESP32 suites about ten seconds per engine, visual checks 19–22 seconds.
Human staging is hash-verified/stopped with stale results removed. Acceptance
and explicit commit/push approval remain pending.

The Author accepted review with “looks good”. Retrieved results match all 481
observations/4,729 bytes and runtime identities pass. Keyboard success was not
separately stated. The Author explicitly approved commit/push on 2026-10-13.

GOLEM-031 is accepted and removed from the authoritative TODO register.
