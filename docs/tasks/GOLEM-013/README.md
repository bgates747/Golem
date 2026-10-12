# GOLEM-013 — Checked resident subtraction review

Resident uint16 subtraction now checks underflow and preserves the failed
assignment's destination. Mixed + / - chains evaluate left to right and check
every intermediate result. Host/sanitizer checks and stock Fab/MOS execution
pass. The Author confirmed human Oryx review worked, and the agent retrieved
matching saved results. This tranche was approved, committed and pushed as
964d546. Fresh-input/call traffic remains 21 bytes.

## Oryx human review

Run locally on Oryx Linux Lite:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-013/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

Wait for the transport-complete message and MOS prompt. The guest itself prints:

```text
Type dir at the MOS prompt below.
If it works, tell the agent: dir works.
```

Type `dir` at the emulator's MOS prompt, close the emulator and tell the agent
whether it worked. The agent retrieves and verifies the saved numerical results
from Oryx afterward. No second terminal or user-run verifier is needed. The
emulator need not remain running. Repeating the launch commands safely reruns
the fixture and removes stale results first.

Automated execution took approximately 6 seconds on the Pi and 8 seconds on
Oryx; allow longer for a graphical run.

The profile retains its previously accepted local runtime/MOS identity. Its SD
startup now runs `/golem-013/probe.bin`; the preceding startup is backed up under
this task's local ignored `build/previous-autoexec.txt`. Automated observations
were removed so human review produces a fresh result. No graphical instance or
attention alert was launched by the agent.

## Supported behavior

The existing resident scalar profile still requires initialized uint16 globals,
one bounded float32 input and one floor-to-sint16 conversion in a finite Sub.
Its job now accepts expressions such as:

```golem
uint16 a = 256
uint16 b = 1
uint16 c = 2
uint16 total = 0
float32 x Range(-32768,32767) = -0.75
sint16 result
Sub compute
    total = a - b + c
    result = sint16(floor(x))
EndSub
```

The sum is 257. Binary + and - have equal precedence and evaluate left to right.
An early underflow fails even if a later addition would restore the final value
to range. Self-reference follows the same whole-assignment commit rule.
Earlier successful statements remain committed; later statements are skipped.

Resident error status resets on every call. Code 1 remains addition overflow;
code 2, named `uint16_sub_underflow` in the manifest, means subtraction underflow.
A later valid invocation succeeds without an extra host status-reset command.
No automatic acknowledgement or return-channel protocol is introduced.

Constant expressions fold safely, checking intermediate bounds. Constant
underflow is rejected before output publication. Variable arithmetic in global
initializers must move into the job. Raw-mode subtraction, signed arithmetic,
parenthesized/general expressions, division, handlers and source routine calls
remain outside this tranche.

## Implementation and evidence

The VDP has no direct integer subtract operation. The compiler copies the
subtrahend into owned scratch storage, XORs with 65535 to complement its bytes,
then computes `a + complement(b) + 1` using add-with-carry. It retains the carry
from both steps: either carry means no borrow; neither means underflow. Only
after all expression operations succeed does it copy the accumulator into the
destination. Failure conditionally jumps to a finite helper that sets code 2
and returns to normal VDP processing. Conditions compare owned 0/1 words, avoiding
the firmware's 0xFFFF conditional sentinel.

| Qualification | Result |
| --- | --- |
| Maintained compiler checks with warnings treated as errors | Pass; 32 rejection cases |
| AddressSanitizer / UndefinedBehaviorSanitizer | Pass |
| Independent command model | 88 jobs, default and relocated layouts |
| Stock Fab 1.2.5 / MOS v3.0.2, Pi headless | 1,248 checks; 88 jobs; 4,392 observed bytes |
| Dedicated Oryx stock profile, headless | Same 4,392 bytes |
| Pure floor compatibility | All five package artifacts unchanged |
| Existing counted-floor compatibility | All four command artifacts identical to ddb5e87 |
| Legacy raw compatibility | Four representative outputs identical to ddb5e87 |
| Rejected packages | Three failed compilations preserve existing output |

Cases cover zero, 65535, borrow across a byte boundary, equality, early underflow
concealed by a later addition, addition overflow after a valid subtraction,
self-reference, failure before/after floor conversion, repeated failures and
recovery. Qualification checks unchanged installed program bytes, ownership,
relocation, cleanup, reinstall reset and unrelated-buffer preservation. Follow-up
GP queries establish VDP parser progress after both success and failure.

The fixture installs 1804 bytes across 42 buffers in either layout. Fresh-input
updates plus call remain 21 bytes, and a call alone remains 6 bytes. Additional
work stays inside installed VDP commands; numerical test readback is instrumentation
and is excluded from the application wire cost. Counted-floor still installs
1238 bytes across 32 buffers with unchanged command artifacts; only its manifest
adds the underflow error code.

[Host evidence](host-checks.json), [stock evidence](emulator-checks.json),
[addition compatibility](addition-compatibility.json) and
[Oryx preparation](oryx-review.json) retain hashes and timings. Host qualification
took 23.673 seconds including sanitizer compilation. Pi setup took 0.037 seconds,
then execution/collection 6.435 seconds. Oryx execution/collection/verifier took
8.380 seconds. These are local monotonic measurements, not calibrated benchmarks.

## Reproduction

From the development checkout:

```bash
.venv/bin/python docs/tasks/GOLEM-013/run_checks.py
.venv/bin/python docs/tasks/GOLEM-013/qualify.py
```

The stock runner uses the existing project-local profile and GOLEM-007 assembler,
launches only with dummy SDL video/audio and restores the development-profile
startup. `prepare_oryx.py` stages the guest and verifier into the existing dedicated
Oryx profile, checks accepted runtime hashes, runs headlessly and removes its result.
The command model and UART transport are task-local copies of the preceding
qualification harness; expected arithmetic is independently computed in `cases.py`.
Physical ESP32 hardware and other firmware versions remain unqualified.

The Author confirmed human review worked; the agent retrieved matching results.
Commit and push were explicitly approved on 2026-10-12.
