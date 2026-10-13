# Paired integer division on both backends

After accepting GOLEM-029, the Author authorized committing/pushing all current
work and proceeding with paired divmod. Checkpoint 0cabba3 is pushed. GOLEM-030
adds paired uint16 assignments to both task-local emitters, retaining the common
source and assembly interface. Production compiler and generic runtime/SDK
remain unchanged; human review and explicit commit approval remain pending.

Distinct destinations are explicitly declared uint16 globals. Arguments evaluate
once left to right before output writes, including when arguments alias the
outputs. Division by zero reports status 4; argument arithmetic retains existing
statuses. Both computed results pass destination Bounds before either is written;
status 32 therefore preserves the entire pair if either admission fails. Earlier
source effects remain; subsequent statements do not execute. Writes to active
For counters are rejected. No `/`, `%` or implicit conversions are added.

The buffered emitter reuses the earlier qualified bounded 16-bit division kernel
from maintained ordinary.hpp; native generated C computes quotient and remainder.
Host ASan/UBSan, independent command model/generated-C behavior, ELF admission
and maintained checks pass 9,104 cases, 66 grammar/reference cases and 82
rejections. Earlier arithmetic/control/Boolean fixtures remain covered. Host
qualification took 65.837 seconds on the Pi.

The visual keeps the familiar traffic-light layout. `(phase,withinPhase)` is
computed from frame divided by 60: quotient chooses the lamp phase, remainder
makes green blink during its phase. Source controls all lamp decisions; resident
VDU draws, assembly paces with VSYNC and alternates backends every twelve frames.
State is 24 bytes, buffered installation 9,210 bytes, native package 1,620 bytes
including 708 text bytes and four data bytes. No physical board was flashed;
emulator times are functional evidence only.

Final stock qualification passes 33 division calls/34 observations/612 bytes and
24 lamp calls/25 observations/600 bytes. Each actual Xtensa engine passes 66
calls/607 observations/6,070 bytes at default and relocated IDs. Both visual runs
pass 600 calls/637 observations/6,289 bytes and all 56,320 final pixels, with MOS
return. Literal zero-divisor and literal successful-pair cases are covered.
Oryx stock checks took 2–3 seconds, numeric ESP32 checks about ten seconds per
engine, visual checks 20–24 seconds. Stopped human staging is hash-verified with
stale results removed; human acceptance and commit/push approval remain pending.

The Author accepted visual review with “looks good”. Retrieved results match
637 observations/6,289 bytes and runtime identities pass. Keyboard success was
not separately stated. The Author explicitly approved commit/push on 2026-10-13.

GOLEM-030 is accepted and removed from the authoritative TODO register.
