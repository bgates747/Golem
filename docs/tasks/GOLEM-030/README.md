# Paired integer division

The task-local compiler adds paired uint16 divmod to buffered and native targets.
It reuses the earlier qualified buffered division kernel. Arguments evaluate
once, left to right; both outputs are computed and checked before either write.
Production compiler and generic VDPlet runtime/SDK remain unchanged. Qualification
and human review status are recorded below.

```golem
uint16 dividend = 100
uint16 divisor = 7
uint16 quotient = 0
uint16 remainder = 0
Sub calculate
    (quotient, remainder) = divmod(dividend, divisor)
EndSub
```

The result is quotient 14, remainder 2. Destinations must be distinct declared
uint16 globals; argument expressions may read either destination. No implicit
conversions, slash operator or remainder operator is introduced by this tranche.
Division by zero reports status 4 and preserves both outputs. Argument arithmetic
retains existing error statuses. If either result exceeds its destination Bounds,
status 32 is reported and both outputs are preserved. Earlier source effects
remain; subsequent statements do not execute. Active loop counters cannot be
outputs. Entry admission still validates all externally supplied fields.

[division.golem](division.golem) exercises ordinary, reversed and expression
aliasing, output Bounds, loop and conditional cases.

## Visual review

[lights.golem](lights.golem) is the complete initialized source. Paired division
of frame by 60 supplies phase and withinPhase. The quotient chooses red, amber
or green; the remainder makes green blink during its phase. Golem sets every
lamp colour; resident VDU draws and assembly owns VSYNC pacing and backend
selection. The same program alternates buffered/native execution every twelve
frames with shared state retained. After 600 calls it stops with only amber lit.
The layout now has 24 explicitly installed state bytes; older state is not
silently migrated.

On Oryx, once staging finishes:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-030/.emulator
./flexe-agon-emulator
```

Follow the onscreen dir instruction, close the emulator and report whether it
worked. The agent retrieves saved results; no second-terminal verifier is needed.

## Verification

```bash
.venv/bin/python docs/tasks/GOLEM-030/host_checks.py
.venv/bin/python docs/tasks/GOLEM-030/fixture.py all
.venv/bin/python docs/tasks/GOLEM-030/stage_oryx.py
```

Host checks retain the full prior arithmetic/condition/loop/Boolean suites and
add division boundaries, input/output aliasing, zero divisors, argument order,
Bounds and skipped/selected failures. Actual-target tapes use representative
cases to fit the finite guest capacity. Native qualification uses actual Xtensa
firmware in both engines. No physical target is flashed and emulator timings
are functional evidence only. See host-checks.json and evidence/ for receipts.

Host qualification passes **9,104 cases, 66 grammar/reference cases and 82
rejections**, with ASan/UBSan, ELF admission and maintained checks. Literal
arguments are covered, including runtime status 4 for divmod(65535,0).

Stock division checks pass 33 calls/34 observations/612 bytes; lamp logic passes
24 calls/25 observations/600 bytes. Each Xtensa engine passes 66 calls/607
observations/6,070 bytes with both default and relocated buffer IDs. Each engine's
600-frame visual run passes 637 observations/6,289 bytes, all 56,320 final
playfield pixels and actual MOS return. Private cleanup preserves shared state.

Host qualification took 65.8 seconds on the Pi. Oryx stock checks took 2–3 seconds,
ESP32 numeric suites about ten seconds per engine, and visual runs 20–24 seconds.
The dedicated profile is hash-verified and stopped with stale results removed.
The Author accepted visual review with “looks good”. Retrieved human results
match all 637 observations/6,289 bytes and runtime identities pass. Keyboard
success was not separately stated. The Author explicitly approved commit/push on 2026-10-13.
