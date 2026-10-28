# Resident integer printing

Golem now converts and prints decimal integers entirely through stock resident
buffered commands. Use literal print and print str(integer-expression);
consecutive statements insert no spaces or newline. No eZ80 numerical readback,
custom firmware, concatenation operator or general string heap is needed.

The [plan](../GOLEM-052.md) records the released scope. Maintained code is in
src/portable/; tests/test_printing.cpp is part of make -C src check. This silo
contains independent decoder/oracle checks and actual-emulator qualification.

## Try the distribution

At MOS, after installing the rebuilt distribution:

```text
cd /golem
golemc --portable examples/counter.golem output/counter-print
golemrun --portable output/counter-print
```

Expect i=10 total=55. Printing makes this a stock-only resident package by
default. Your own ASM/C host can install once and retain state across repeated
calls; the standalone runner installs/calls/cleans up once. Explicit native/both
with print is rejected. No-print default packages retain their existing bytes.

## Human review

The dedicated Oryx profile is separate from the existing walkthrough:

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-052
./flexe-agon-emulator
```

It compiles numbers.golem and counter.golem on the emulated Agon. The C client
supplies inputs and labels; Golem prints the numerical result rows. Those rows
exercise all four integer types, including -32768, -2147483648 and 4294967295.
Two resident calls print i=10 total=55 and then i=10 total=110. The labelled
failure prints only before:, and the conditional loop prints 0x2. A PASS line
means the independent state checks agree. Follow the onscreen dir instruction,
close and report whether it worked; the agent retrieves the results. Rerunning
replaces only this review's result files.

The Author's 2026-10-27 screenshot confirms the expected output, PASS and working
MOS dir. Retrieved results match all 200 expected bytes; the staged compiler
hash matches too. Human review passed. The Author explicitly approved commit
and push on 2026-10-28. No agent graphical launch or hardware flash.

## Verification

1. make -C src check runs maintained contracts and frozen old-package checks.
2. .venv/bin/python docs/tasks/GOLEM-052/check.py decodes emitted bytes with an
   independent stock-command machine: 132 calls, 496 boundary/random integer
   comparisons, state preservation, cleared text temporaries and cleanup.
3. .venv/bin/python docs/tasks/GOLEM-052/qualify.py runs the actual MOS compiler
   and stock Fab/VDP: 66 pixel-identical expected/Golem pairs (20,275,200 pixels),
   1,320 state bytes, repeated calls, faults and normal MOS continuation.
4. Oryx stock ESP32/Flexe additionally mirrors actual printed text using stock
   VDU2 printer output. All 66 framed strings and 1,320 bytes match the independent
   oracle; VDU3 restores normal output. The human mode uses ordinary screen text.
5. Standard C++ and the Agon library adapter pass sanitizer checks; the adapter
   uses its documented detect_stack_use_after_return=0 recovery configuration.
   All ten existing distribution examples still compile on Agon and run as
   appropriate; the counter/array examples now print their results.

The u16-only counter buffered-install.bin is 9,953 bytes; the mixed four-type
fixture is 17,728 bytes. Common state is installed separately. Conversion helpers are shared per width; only used
widths are included. Named resident calls remain six bytes. These are storage/
transport measurements, not speed claims. Generic stored strings, float formatting
and native printing remain deferred.
