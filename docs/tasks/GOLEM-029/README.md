# Short-circuit Boolean expressions

The task-local compiler extends the accepted Boolean slice with Not, And and Or
on both buffered and native targets. Stock and both Xtensa engines pass qualification; the stopped Oryx profile
is ready for human review. Production compiler and generic VDPlet runtime/SDK are unchanged.

## Language

Precedence, highest first: numeric arithmetic, numeric comparison, Not, And, Or.
Parentheses group Boolean expressions or numeric operands. Operators and literals
are case insensitive; identifiers retain case sensitivity. Boolean nesting is
bounded to 16 unary/group levels. Boolean globals still explicitly initialize
from true/false; assignment supports compound expressions. No numeric truthiness,
bitwise meaning, Boolean comparison or new implicit conversion is introduced.

And evaluates its right operand only if the left is true. Or evaluates its right
operand only if the left is false. Not inverts a canonical Boolean. Operands
are evaluated once, left to right. A skipped operand cannot raise an arithmetic
error; a selected failing operand preserves the destination, retains preceding
effects and stops the invocation. Admission still checks every external global,
even if its later use would be skipped. Buffered logical helpers propagate errors
through each calling frame before subsequent writes can resume.

```golem
bool ready = true
bool blocked = false
bool permitted = false
uint16 ticks = 0
Sub tick
    permitted = ready And Not blocked
    If permitted Or ticks == 0 Then
        ticks = ticks + 1
    EndIf
EndSub
```

## Visual review

[lights.golem](lights.golem) is the complete reviewed source. It now computes
amber with a conjunction and green by negating a grouped disjunction. The same
three-lamp traffic-light display is intentional: these syntax changes preserve
its behavior. Golem sets all lamp colours; resident VDU draws the lamps, assembly
chooses the backend and paces 600 calls with VSYNC. The backend alternates every
twelve frames while state persists. Final display has only the amber lamp lit.

On Oryx, once staging is complete:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-029/.emulator
./fab-agon-emulator
```

Follow the onscreen dir instruction, close the emulator and report whether it
worked. The agent retrieves results; no second-terminal verifier is required.

## Verification

The host suite retains earlier arithmetic, comparison, Range and Boolean cases.
New cases cover complete truth tables, precedence and grouping, double negation,
short-circuited overflow/underflow, selected failures and destination preservation.
Both model/generated-C checks and actual stock/Xtensa qualification are required.
Task-local build commands use the project Python environment:

```bash
.venv/bin/python docs/tasks/GOLEM-029/host_checks.py
.venv/bin/python docs/tasks/GOLEM-029/fixture.py all
.venv/bin/python docs/tasks/GOLEM-029/stage_oryx.py
```

See host-checks.json and evidence/ for measured results once qualification finishes.
No physical board is flashed; emulator timing is functional evidence only.

Host ASan/UBSan, command-model/generated-C behavior, ELF admission and maintained
checks pass: **5,284 cases, 59 grammar/reference cases and 74 rejections**.
Stock Boolean checks pass 45 calls/46 observations/828 bytes; lamp logic passes
24 calls/25 observations/500 bytes. Each ESP32 engine passes 90 calls/823
observations/8,230 bytes for both default and relocated IDs. Each engine's
600-frame visual run passes 533 observations/5,249 bytes, all 56,320 final
playfield pixels and actual MOS return. Private cleanup retains shared state.

Host qualification took 61 seconds on the Pi; Oryx stock checks took 2–3 seconds,
ESP32 numeric checks about ten seconds per engine, and visual runs 19–21 seconds.
The dedicated Oryx fixture is verified and stopped with stale results removed.
The Author accepted review with “checks out”. Retrieved results match all 533
observations/5,249 bytes and runtime identities pass. Keyboard success was not
separately stated. The Author explicitly approved commit/push on 2026-10-13.
