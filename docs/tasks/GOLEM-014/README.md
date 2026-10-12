# GOLEM-014 — Checked resident multiplication review

Checked uint16 multiplication is implemented with ordinary precedence and
destination preservation on overflow. Host/sanitizer and stock Fab/MOS checks
pass. The Author confirmed Oryx human review worked; the agent retrieved and verified
all 8,016 saved bytes. Shared commands keep its installation to 3344 bytes; new-input calls
remain 21 bytes. The Author subsequently explicitly approved commit and push.

## Human review on Oryx

Run locally:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-014/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

Wait for `GOLEM-014: transport complete; results saved.` The guest prints:

```text
Type dir at the MOS prompt below.
Then close this emulator and tell the agent if dir worked.
The agent will collect and check the saved results.
```

Follow those instructions. No second terminal or user-run verifier is needed.
The emulator can be closed before verification; the agent checks saved SD results
afterward. Repeating the launch commands safely reruns the fixture and removes
stale results first. Allow roughly 20 seconds for completion. Repeated `completed`
text is fixture output, not the numerical verdict.

The verifier reports only `Review output matches: 1670 checks, 8016 bytes.`
Automated preparation removed its results and left the emulator stopped. The
previous startup is backed up under the task's local ignored build directory.
No graphical emulator or attention alert was launched.

## Behavior

```golem
uint16 a = 2
uint16 b = 3
uint16 c = 4
uint16 total = 0
float32 x Range(-32768,32767) = -0.75
sint16 result
Sub compute
    total = a + b * c
    result = sint16(floor(x))
EndSub
```

This assigns 14. Multiplication has higher precedence than + / -. Operators at
each level evaluate left to right and check each intermediate result. Thus
`a * b * 0` fails if a * b overflows; `a * 0 * b` succeeds. Self-references read
the original destination until the complete assignment commits.

Error code 3 (`uint16_mul_overflow`) preserves the failed destination, stops later
statements and resumes normal VDP processing. Earlier successful writes remain.
Status resets each call; a later valid invocation can recover without a host
reset or acknowledgement. Existing codes 1/2 retain their meanings.

Constant products fold safely; invalid intermediate products reject compilation.
Variable initializer arithmetic must move into the job. The existing profile
still requires one bounded float input and one floor-to-sint16 conversion.
General/parenthesized expressions, signed multiplication, division, power,
handlers and raw-mode multiplication remain outside this tranche.

## Backend and evidence

A shared finite 16-step kernel computes the exact uint32 product using masked
multiplier bits and add-with-carry. The calling job checks the upper bytes after
the kernel returns, keeping error exit outside nested helper calls. Scratch state
resets per product. The conservative helper depth is at most three. Conditions
use single-bit or zero-extended byte values, avoiding 0xFFFF sentinel operands.

The five-site fixture installs 3344 bytes across 50 buffers. Sharing reduced the
preliminary inline installation by 3608 bytes, from 6952. New-input/call remains
21 bytes; plain call remains 6 bytes. Numerical readback is test instrumentation
and is excluded from application traffic figures.

| Qualification | Result |
| --- | --- |
| Maintained checks, warnings as errors; ASan/UBSan | Pass; 36 rejection cases |
| Independent command model | 120 jobs across default/relocated layouts |
| Additional precedence/order/aliasing checks | 14 cases |
| Stock Fab 1.2.5 / MOS v3.0.2 on Pi | 1,670 checks; 120 jobs; 8,016 bytes |
| Dedicated Oryx stock runtime, headless | Identical 8,016 bytes |
| Pure floor compatibility | All five artifacts unchanged |
| Reviewed addition/subtraction compatibility | All four command artifacts unchanged against 964d546 |
| Raw compatibility / invalid publication | Four identical outputs / three preserved rejected packages |

Cases cover every multiplier bit, zero/one/full-width values, 255*257, 256*256,
65535*65535, mixed/chained arithmetic, self-reference, early/late errors, preserved
state, skipped statements, repeated failure and recovery. Ownership, relocation,
program immutability, cleanup/reinstall and external-buffer preservation pass.
GP queries confirm parser progress after success/error. Exact integer expectations
are independent of the emitted bitwise routine.

[Host evidence](host-checks.json), [expression checks](expression-checks.json),
[stock report](emulator-checks.json), [compatibility](arithmetic-compatibility.json)
and [Oryx preparation](oryx-review.json) retain hashes/timings. Host qualification
took 36.908 seconds including rebuild and sanitizers. Pi setup took 0.031 seconds,
execution/collection 10.963 seconds. Oryx execution/collection/verifier took
15.596 seconds. These are local monotonic measurements, not calibrated benchmarks.

Reproduce from the development checkout:

```bash
.venv/bin/python docs/tasks/GOLEM-014/run_checks.py
.venv/bin/python docs/tasks/GOLEM-014/expression_checks.py
.venv/bin/python docs/tasks/GOLEM-014/qualify.py
```

The stock runner uses the existing profile/GOLEM-007 assembler, launches only
with dummy SDL video/audio and restores the development startup. `prepare_oryx.py`
checks accepted runtime hashes before staging/headless verification. Physical
ESP32 hardware and other firmware versions remain unqualified.

Human review passed on 2026-10-12. [Retrieval evidence](human-review.json) records
the exact guest/result hashes; all 1,670 checks match across 120 jobs.

The Author approved commit/push after reviewing the retrieved results.
