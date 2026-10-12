# GOLEM-017 — Typed table job review

Checked whole-record table selection is implemented and callable from assembly.
Host/model, assembler and stock Pi/Oryx headless checks pass. No dummy float
conversion, file-loading source syntax or host acknowledgement is required.
The Author confirmed Oryx human review succeeded; the agent retrieved and
verified all saved results. The Author explicitly approved commit and push.

## Human review on Oryx

Run locally:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-017/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

Allow about 10 seconds. Wait for GOLEM-017: transport complete; results saved.
Follow the onscreen instruction to type dir at the MOS prompt, then close the
emulator and tell the agent whether it worked. The agent retrieves/verifies the
saved observations afterward. No other terminal or verifier command is needed.
Rerunning the launch block removes the previous result and safely repeats the test.
The guest's repeated selected text marks successful calls, not a numerical verdict.
The agent's verifier reports: Review output matches: 873 checks, 3453 bytes.

Preparation preserves the dedicated stock runtime, backs up prior startup under
ignored build/, removes headless results and leaves the emulator stopped. No
GUI or attention alert was launched.

## Supported slice

[table.golem](table.golem) declares a uint16/sint16 record, four-row immutable
table, initialized record destination and uint16 index. [records.dat](records.dat)
contains (1,-2), (2,3), (3,-4), (4,5), packed little-endian. --table-data maps
those bytes at build time; the assembly application sends the resulting package.
The named wrapper is called as call golem_fetch. Field symbols expose the selected
record's member offsets to assembly. The [maintained contract](../../design/resident-table.md)
contains build commands, exports, ownership, grammar and limits.

Invalid index status is 5, reset on each call. Failure preserves all destination
bytes, skips following print statements and resumes normal VDP handling. Correcting
inputs recovers without reinstalling. Index checks happen before address arithmetic
and template patching. The trusted row is copied once; no production byte readback,
checksums or repeated field admission checks are added.

The small package installs 528 bytes in 19 buffers. Recurring input/call traffic
is 19 bytes; unchanged-input invocation is six bytes. Test readback is excluded.

## Evidence

| Qualification | Result |
| --- | --- |
| Independent byte model | 65,872 jobs; every uint16 index for four rows |
| Maintained compiler + ASan/UBSan | Pass; 67 rejection cases |
| Actual ez80asm consumers | Five table layouts; field exports, wrapper and payload bytes match |
| Prepublication failures | 11 cases preserve all six output files |
| Prior scalar compatibility | Ten packages; all five original artifacts match e52a427 |
| Stock Pi Fab 1.2.5 / MOS v3.0.2 | 873 checks, 98 jobs, 3,453 bytes |
| Dedicated Oryx stock runtime | Identical 3,453 bytes; headless pass |
| Oryx human MOS/keyboard review | Passed; saved results independently verified |

Default, relocation across ID 256, 258-row, near-ID-limit and singleton layouts
cover index bounds, high-byte comparisons, signed/full-width field bits, repeated
failure/recovery, immutable table rows, intended template patching, unchanged
other programs, cleanup/reinstallation and external-buffer preservation. The
default guest executes the actual generated eZ80 golem_fetch wrapper; other
layouts send the compiler's invoke packet. GP queries establish parser progress
after valid and failed jobs. No table-index arithmetic runs on the eZ80.

The host model additionally admits a 1,024-row, 32-field schema. That maximum
shape has no stock-emulator or physical-hardware qualification. Tests verify
results; production code does not read back copies.

[Host checks](host-checks.json), [stock checks](emulator-checks.json),
[Oryx preparation](oryx-review.json) and [scalar compatibility/sanitizers](../GOLEM-016/host-checks.json)
retain hashes and timings. Host table checks took 3.151 seconds, the broader
scalar parity/sanitizer checks 34.215 seconds, Pi execution 5.362 seconds and
Oryx execution/verification 7.218 seconds. These are local monotonic measurements,
not calibrated performance comparisons.

Reproduce automated qualification from the development checkout:

```bash
.venv/bin/python docs/tasks/GOLEM-017/run_checks.py
.venv/bin/python docs/tasks/GOLEM-016/run_checks.py
.venv/bin/python docs/tasks/GOLEM-017/qualify.py
```

GOLEM-007 supplies the shared assembler/UART observation transport. Headless
launches use the mandatory profile wrapper with dummy SDL and restore local
startup. Oryx preparation checks accepted runtime hashes before deployment.

Human review passed on 2026-10-12. [Retrieval evidence](human-review.json)
records the verified guest/result identities: 873 checks across 98 jobs, all
3,453 bytes matching. The Author subsequently explicitly approved commit and push.
