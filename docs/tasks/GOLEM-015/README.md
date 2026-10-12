# GOLEM-015 — Resident uint16 divmod review

Paired uint16 quotient/remainder is implemented. Host/sanitizer and stock Pi/Oryx
headless qualification pass: 2,126 checks across 156 jobs, with identical 12,232
saved bytes. Zero divisor preserves both destinations and ends the job with
status 4. The Author confirmed Oryx human review succeeded; the agent retrieved and
verified the saved results. The Author explicitly approved commit and push.
Recurring new-input/call traffic remains 21 bytes.

## Human review on Oryx

Run locally:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-015/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

Allow roughly 30 seconds. Wait for `GOLEM-015: transport complete; results saved.`
Follow the onscreen instruction to type `dir` at the MOS prompt, then close the
emulator and tell the agent whether it worked. The agent retrieves and checks
the saved results afterward. No second terminal or user-run verifier is needed.
Repeated `completed` text is fixture output, not the numerical verdict. Rerunning
the launch block safely removes the previous results before repeating the test.

The agent's verifier reports only `Review output matches: 2126 checks, 12232 bytes.`
Preparation removed its headless results for a fresh human run and backed up the
previous startup under the ignored task build directory. No graphical emulator
or attention alert was launched.

## Supported behavior

```golem
uint16 numerator = 17
uint16 denominator = 5
uint16 quotient = 0
uint16 remainder = 0
float32 x Range(-32768,32767) = -0.75
sint16 result
Sub compute
    (quotient, remainder) = divmod(numerator, denominator)
    result = sint16(floor(x))
EndSub
```

Quotient becomes 3 and remainder 2. Both destinations must be distinct declared
uint16 globals. Arguments use the current +, -, * expression grammar and evaluate
once, left to right. They are snapshotted privately before outputs commit, so
input/output aliases work, including replacing numerator and denominator.

A zero divisor returns status 4 (`uint16_divide_by_zero`); failures while evaluating
arguments retain existing codes 1/2/3. Either failure preserves both outputs,
skips subsequent statements and resumes normal VDP processing. Earlier writes
remain. Status resets on each invocation; correcting inputs allows recovery
without a reset or acknowledgement. Literal-zero divisors use the same runtime
exit. Invalid constant arithmetic within arguments remains a compile error.

The existing profile still requires a bounded float input and one floor-to-sint16
conversion. Signed divmod, / and % syntax, general functions, parenthesized
arithmetic expressions, handlers and raw-mode divmod remain outside this slice.

## Backend and evidence

A shared finite 16-step restoring division routine computes both results without
host interaction. Conditions compare zero-extended bytes and single-bit masks,
avoiding 0xFFFF conditional operand sentinels. An explicit comparator tail jump
keeps helper depth at most three. The calling job checks zero before entering
the kernel, and copies both outputs only after successful computation. Scratch
state resets per evaluation; no failing operation separates the output copies.

The two-site fixture installs 5,086 bytes across 55 owned buffers. New-input/call
remains 21 bytes, plain call 6 bytes. Numerical readback is test instrumentation
and is excluded from those recurring application traffic figures.

| Qualification | Result |
| --- | --- |
| Maintained checks, warnings as errors; ASan/UBSan | Pass; 43 rejection cases |
| Independent command model | 156 jobs across default/relocated layouts |
| Additional argument/order/alias checks | 16 cases |
| Stock Fab 1.2.5 / MOS v3.0.2 on Pi | 2,126 checks; 156 jobs; 12,232 bytes |
| Dedicated Oryx stock runtime, headless | Identical 12,232 bytes |
| Pure floor compatibility | All five artifacts unchanged |
| Reviewed addition/subtraction/multiplication | Four command artifacts unchanged against 0c311ba |
| Raw compatibility / invalid publication | Four identical outputs / three preserved rejected packages |

Python's independent integer divmod supplies expected results. Cases cover every
numerator bit and power-of-two divisor, full-width boundaries, deterministic
random values, first/second-site zero divisors, argument errors and aliases.
Persistent state, skipped statements, repeated failure/recovery, relocation,
program immutability, cleanup/reinstall and external buffer preservation pass.
GP queries establish VDP parser progress after successful and failed jobs.

[Host evidence](host-checks.json), [argument checks](argument-checks.json),
[stock report](emulator-checks.json), [compatibility](arithmetic-compatibility.json)
and [Oryx preparation](oryx-review.json) retain hashes and timings. Host checks
took 33.404 seconds including rebuild/sanitizers; Pi execution/collection took
15.996 seconds; Oryx execution/collection/verifier took 23.712 seconds. These are
local monotonic measurements, not calibrated performance benchmarks.

Reproduce automated qualification from the development checkout:

```bash
.venv/bin/python docs/tasks/GOLEM-015/run_checks.py
.venv/bin/python docs/tasks/GOLEM-015/argument_checks.py
.venv/bin/python docs/tasks/GOLEM-015/qualify.py
```

The runner reuses GOLEM-007's assembler/UART transport, launches only via the
profile wrapper with dummy SDL video/audio and restores the development startup.
Oryx preparation checks accepted runtime hashes before staging and headless
verification. Physical ESP32 hardware and other firmware versions are unqualified.

Human review passed on 2026-10-12. [Retrieval evidence](human-review.json) records
the exact guest and result hashes; all 2,126 checks across 156 jobs match.
The Author explicitly approved commit and push.
