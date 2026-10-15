# Flower native mathematics

The complete Flower calculation now compiles from Golem and executes as an
uploaded ESP32 module. Float division, sine, cosine and floor work with existing
globals, conditionals, finite loops and resident integer arrays. Headless JIT
and interpreter checks pass. Drawing integration and the interactive C++ client
are next; visual staging is deferred until the Author wakes.

## What changed

1. `--portable --target native` emits an explicit native-only package using the
   shared frontend/state/root/layout. The default still emits both backends,
   and rejects the new math until stock lowering qualifies. There is no Native
   marker or separate source dialect. Filtered packages contain targets.json,
   native wrappers and library requirements, with no stale buffered artifacts.
2. Float32 `/` shares multiplication precedence and associates left. Its operands
   evaluate once, left to right; zero divisor reports status 4. Sin/cos consume
   radians. Floor returns float32, followed by explicit checked conversion when
   plotting. Builtins are case-insensitive and require float32 operands.
3. Existing finite-input, per-operation rounding, zero/subnormal normalization,
   Bounds, partial effects and error exit semantics remain. No fast-math/FMA,
   host-precomputed curve points or application-level trig tables were added.
4. The SDK's opt-in `--math` statically links pinned libm/libgcc and exports only
   vdplet_entry. Library identities are recorded in each module receipt. Default
   images, runtime/loader/ABI and firmware source remain unchanged. SDK work is
   tracked separately as VDPL-002; the Author accepted and authorized publication
   of both repositories on 2026-10-15.

Read the [actual Flower mathematics source](../flower.golem). `prepare` converts
degrees and initializes the original recurrence; `batch` computes at most 32
connected-segment endpoints into resident arrays. Default parameters produce
396 segments. The client will submit batches and a resident renderer will
consume the arrays; neither role is implemented by this mathematical tranche.

## Evidence

| Check | Result |
| --- | --- |
| Required maintained contracts | 132 legacy / 57 portable rejections; frozen outputs pass |
| C++ ASan/UBSan | Maintained portable contracts pass |
| Sanitized generated C | 615 default + 615 relocated probe cases and 14 complete-default Flower calls |
| Previous outputs | All 55 compared compiler files and all three default SDK images unchanged |
| Prior XOR/array conformance | 468 command-model/generated-C cases and assembler consumer pass |
| Actual assembly consumers | Default/relocated math and Flower named native wrappers assemble |
| Strict SDK loader | Six default/math packages pass sanitizer admission, mutation rejections and cleanup |
| Pi JIT | 278 calls / 27092 replies / 270920 reply bytes; 41.185 seconds |
| Oryx JIT | Same fixture/modules/firmware: 278 calls / 27092 replies; 13.624 seconds |
| Oryx interpreter | Same fixture/modules/firmware: 278 calls / 27092 replies; 25.852 seconds |

Each actual-engine run covers 103 mathematical/error cases at default IDs and
103 at relocated IDs, then 72 Flower calls spanning defaults, 22.5 degrees,
360 degrees, integer step product, zero shrink and full shrink. The integer
product case produces 13 segments; the other five produce 396. There are
4608 array-coordinate observations per run, including unchanged padding slots.
Language statuses 0, 1, 4, 32 and 33, failed-destination preservation, preceding
effects, short-circuiting, native return status and retained root after cleanup
all match. Large finite trig arguments exercise ordinary library reduction.

Division/floor probe outputs are compared exactly with the independent model;
trig outputs permit two float32 ULPs. Full Flower floating state permits eight
ULPs per invocation from the same actual starting state, and integer coordinates
permit one logical unit for a floor-boundary difference. The observed maximum
float separation is one ULP in every run. Actual Oryx JIT/interpreter reply bytes
are identical except their explicit command-task stack high-water diagnostics;
[engine-parity.json](evidence/engine-parity.json) identifies those positions.

Minimum observed command-task free stack is 1844 bytes on Pi JIT, 2120 on Oryx
JIT and 2008 on Oryx interpreter. These are whole-task high-water observations
for the tested paths, not a proof of every possible module/input combination.
No backend faults, unsupported accesses or unresolved instructions occurred.

The [package receipts](evidence/packages.json) report the Flower module at
13668 image / 11336 text / 1244 data bytes, with 15 relative relocations. Both
probe layouts use the identical 7584-byte module image; their supplied state
and argument buffer IDs differ. The unchanged firmware hash matches the
previously accepted hardware-runtime build. No board was flashed.

These runs execute actual ESP32 firmware through the previously qualified
VDP-only adapter and observe state through real wire commands. They do not boot
MOS, qualify the C++ input application or establish physical hardware behavior.
The Oryx adapter is the accepted snapshot; the Pi adapter is rebuilt from the
frozen source export in a task-local ignored cache. No maintained emulator
source, desktop profile, SD mapping or visual launcher changed. Timings include
large diagnostic readback; they are not native curve-rendering benchmarks.
The normal mathematical entry submission remains six outbound wire bytes.

See [host evidence](evidence/host.json), [loader evidence](evidence/loader.json),
[Pi JIT receipt](evidence/pi-jit/receipt.json), [Oryx JIT receipt](evidence/oryx-jit/receipt.json)
and [Oryx interpreter receipt](evidence/oryx-interp/receipt.json). Raw reply
transcripts and backend statistics are retained beside each target receipt.
An initial Oryx run completed its checks but failed to save its receipt because
the evidence directory was missing; that path is fixed and both engines were
rerun to obtain the retained complete evidence.

## Reproduction

From the repository root:

```bash
make -C src check
.venv/bin/python docs/tasks/GOLEM-044/native-math/host_checks.py
.venv/bin/python docs/tasks/GOLEM-044/native-math/build_headless.py
.venv/bin/python docs/tasks/GOLEM-044/native-math/qualify_headless.py --engine jit
.venv/bin/python docs/tasks/GOLEM-044/native-math/qualify_headless.py --engine interp
```

Set EZ80ASM to a native assembler; this run built one from unchanged reference
sources into this task's ignored build/assembler directory. The headless builder
requires the pinned read-only source reference and native CMake/OpenSSL/zlib.
It builds only the VDP adapter and uses the existing SDK firmware, never flashes.
An existing accepted adapter/firmware can be selected with `--backend` and
`--firmware`; `--skip-build` consumes prepared module packages. Use the project
.venv, as in the commands above. No command opens a desktop or audio device.

The Author accepted this implementation and evidence and explicitly authorized
commit/push on 2026-10-15. The Author explicitly
deferred visual staging during sleep. Stock mathematical lowering, the resident
renderer, complete C transport/client, visual review and physical acceptance
remain open under the parent task.
