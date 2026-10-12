# GOLEM-012 — Checked resident addition review

Resident uint16 addition now detects overflow, preserves the failed assignment's
destination and ends the job, allowing normal VDP processing to resume. Maintained
host checks, sanitizers and stock-emulator qualification pass. The dedicated
Oryx fixture also passes headless verification, and the Author confirmed human
review worked. Its completion message now prints the `dir` instructions directly.
Fresh-input/call traffic remains 21 bytes. The Author subsequently approved commit and push.

## Human review on Oryx Linux Lite

Run these commands in a local Oryx Bash terminal:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-012/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

Wait for `GOLEM-012: transport complete; host verifies results.dat` and a MOS
prompt. Repeated `completed` text is fixture output; numerical checks are saved
in `results.dat`. The automated runs took approximately 6 seconds on the Pi and
8 seconds on Oryx; allow longer for a graphical run.

The test program also prints the keyboard step directly in the emulator:

```text
Type dir at the MOS prompt below.
If it works, tell the agent: dir works.
```

In another local Oryx Bash terminal:

```bash
cd ~/Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-012/verify_review.py
```

Expected verification:

```text
Review output matches: 1238 checks, 4022 bytes.
Type dir at the MOS prompt in the emulator window and report to the agent that it works.
```

Type `dir` at the MOS prompt **inside the emulator window**. Report the verifier
result and whether `dir` works. Close the emulator after review. This tests that
normal MOS/VDP interaction remains usable after the resident success/error jobs.

The staged SD startup selects `/golem-012/probe.bin`; the preceding startup is
backed up locally under this task's `build/previous-autoexec.txt`. No graphical
instance or attention alert was launched by the agent. No fresh results file is
left from the automated Oryx run.

## Implemented behavior

- ADD-001: Every step of a runtime uint16 sum checks carry. This catches overflow
  in an early term even when later additions could otherwise hide it.
- ADD-002: The complete assignment commits only after success. Self-reference
  follows the same rule. Previous successful statements remain committed.
- ERR-001: Overflow sets resident error code 1 and jumps to a finite error-exit
  buffer. The jump discards the current job stream; later statements do not run.
  End of that buffer returns to normal VDP processing.
- ERR-002: Status resets to 0 on each invocation. A subsequent valid call can
  succeed without a separate host status-reset command. The optional manifest
  `runtime_error` field publishes its location and meaning, without introducing
  a MOS acknowledgement or return-channel protocol.
- CMP-001: Carry is compared as an owned uint16 word against an owned zero word,
  using buffer-fetched operands. Checks never depend on the firmware's 0xFFFF
  conditional sentinel or an inline literal operand.
- INI-001: All-constant sums fold safely; compile-time overflow is rejected.
  Variable additions in global initializers are rejected with a diagnostic to
  move the sum into the job. Simple initializer copies remain supported.

See the maintained [resident contract](../../design/resident-scalar.md).

## Evidence and costs

| Evidence | Result |
| --- | --- |
| Maintained compiler checks, warnings treated as errors | Pass; 29 rejection cases |
| AddressSanitizer and UndefinedBehaviorSanitizer | Pass |
| Independent emitted-command model | 88 jobs across default/relocated layouts |
| Stock Fab 1.2.5 / MOS v3.0.2, Pi headless | 1,238 checks; 88 jobs; 4,022 observed bytes |
| Dedicated Oryx stock runtime, headless | Identical 4,022 bytes; pinned runtime hashes match earlier accepted Oryx profile |
| Pure floor compatibility | All five package artifacts unchanged |
| Legacy raw-mode compatibility | Four representative outputs byte-identical to c83463b |
| Invalid output publication | Three failed compilations preserve existing outputs |

Boundary cases include 65535, byte carry, early-chain overflow, self-reference,
failure before/after floor conversion, destination preservation, skipped writes,
repeated failures and recovery. Tests also cover relocation across reserved
ranges, unchanged installed program bytes, owned cleanup, reinstall reset and
preservation of unrelated buffers. Stock qualification uses an eZ80 UART transport
and buffer readback; arithmetic expectations come from an independent integer
oracle. Follow-up GP queries establish VDP parser progress after job exit.

| Package | Bootstrap bytes | Owned buffers | Fresh input + call |
| --- | ---: | ---: | ---: |
| Pure floor | 1008 | 25 | 21 bytes |
| Counted floor with checked addition | 1238 | 32 | 21 bytes |
| Qualification fixture, either layout | 1593 | 40 | 21 bytes |

The previous counted-floor installation was 1120 bytes and 28 buffers. Checks
increase installation size and local VDP work; they add no recurring wire traffic.
A call without an input update remains 6 bytes. Qualification readback traffic is
test instrumentation and is excluded from these application traffic figures.

[Host report](host-checks.json), [stock-emulator report](emulator-checks.json) and
[Oryx preparation report](oryx-review.json) retain hashes, layouts and timings.
Host qualification took 22.213 seconds including sanitizer compilation. Pi
fixture preparation took 0.027 seconds; execution/collection took 5.925 seconds.
Oryx execution/collection/verifier took 8.327 seconds. These are monotonic local
measurements, not calibrated cross-machine benchmarks or fixed review timeouts.

## Reproduction and limits

From the development checkout:

```bash
.venv/bin/python docs/tasks/GOLEM-012/run_checks.py
.venv/bin/python docs/tasks/GOLEM-012/qualify.py
```

The stock runner requires the already prepared project-local emulator and the
GOLEM-007 assembler. It launches only with dummy SDL video/audio, restores the
previous development-profile startup and records executed binary/module identity.
`prepare_oryx.py` copies this fixture into the existing dedicated Oryx profile,
checks its accepted runtime hashes, runs headlessly and removes its results.
Machine access follows the canonical environment connection instructions.

Physical ESP32 hardware and other firmware versions are not qualified. This is
the existing single finite resident job, with one required floor conversion;
source calls, loops, broader conversions, subtraction/multiplication/power,
division/divmod, handlers, Resume/retry and ByRef remain future work. Raw-mode
uint16 addition still wraps. Float admission remains the published host contract.
The Author confirmed human review of the preceding guest worked. The instruction
message revision has automated qualification; the Author subsequently approved commit and push.
