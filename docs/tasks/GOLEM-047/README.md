# Native VDU submission: first executable tranche

The native runtime can now consume locally generated move/line command bytes
through the existing VDP drawing interpreter. Old modules still execute
unchanged. The first large consumer draws the complete Corsair wireframe and
returns to MOS. Its matched emulator comparison shows smaller installation
data and faster interpreter execution; JIT whole-frame times are unchanged.
The Author accepted the Oryx visual/keyboard review and explicitly approved
commit/push on 2026-10-24. Retrieved results match all 20 bytes, the completed
application log, runtime pins and an actual MOS directory listing.

This is a C++ VDPlet/runtime proof. It does not add drawing syntax to the
portable Golem compiler. That language/backend integration remains in
[GOLEM-047](../GOLEM-047.md); generic runtime/ABI work belongs to
[VDPL-003](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-003.md).

## Review on Oryx

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-047
./flexe-agon-emulator
```

The dedicated **Flexe** profile uses actual ESP32 firmware and uploaded Xtensa
code. It shows a white Corsair rotating through 24 poses in mode 19 (1024x768),
then returns to MOS. Expect roughly 25 seconds including startup on the tested
Oryx. Follow the single onscreen `dir` instruction, close the emulator, and
report whether the picture and keyboard worked. Rerunning the same launcher
starts a fresh transcript and clears only this fixture's previous results.

The source roles are explicit:

| Source | What executes there |
| --- | --- |
| [worker.cpp](wireframe/worker.cpp) | Native draw entry: read projected vertices/edges, generate complete command chunks, submit them |
| `wireframe/build/transform-source.cpp` | Frozen copy of the wireframe owner's C++ transform, included unchanged by the worker |
| [src/main.cpp](wireframe/src/main.cpp) | AgonDev eZ80 client: load AGNB, admit/bind module, update controls, validate transform, set graphics state, call/fence each frame, unload and return |

Only the new submitted-byte path draws in the human review. There is no hidden
resident comparison scene. The client owns clear/colour/labels/completion; the
module submits the wireframe's move/line bytes. No hardware was flashed and no
interactive emulator was launched by the agent. The agent retrieves saved
results with `wireframe/verify_review.py` after the report; the Author need not
run a second command or keep the emulator open.

## Implemented contract

The optional ABI 1.1 table appends one callback while preserving the original
ABI 1.0 table exactly. New modules request capability 4; default old modules and
their package bytes retain ABI 1.0. The service currently admits complete,
six-byte VDU 25 operations 4/5 only, up to 65,532 bytes per call. It consumes caller
bytes synchronously, preserves the active graphics context and restores the
outer input stream. It does not allocate a temporary buffer ID or process
outer input/events. Caller bytes may be reused after return.

INIT/FINI, open packed-frame transactions, echo buffering and unsupported
commands are rejected. A bad span draws nothing for that call; earlier successful
calls remain visible. Native return is not raster completion. The application
retains the ordered C3/GP boundary and drains drawing before failure cleanup.
See the maintained [API reference](/Agon/mystuff/agon-vdplet/docs/API-reference.md)
for exact state/error/compatibility behavior.

## Qualification and limits

1. SDK host admission/relocation checks and new span/ABI checks pass under
   ASan/UBSan. The pinned firmware rebuild preserves the exact qualified binary.
2. Each actual ESP32 engine passes 19 service cases, 1,179,648 compared pixels
   and 500 observed result bytes. This covers complete/chunked spans, clipping,
   maximum length, malformed/unsupported input before drawing, outer resident
   continuation, legacy table checks, four live leases and partial effects.
   Both finish with 2,180 bytes of free command-task stack; execution fault
   counters remain zero. Old firmware rejects the new image and then executes
   the old numeric module normally.
3. A separate trace compares the original precompiled C++ transform with the
   composed worker over six poses: all 224 root bytes plus 22 XYZ16 vertices
   per pose match exactly (2,136 observed bytes). This is a sampled composition
   check, not a new claim of exhaustive independent transform qualification.
4. Full MOS runs qualify both drawing arms under JIT and the interpreter. All return to MOS, unload the module and preserve
   clean runtime state. Final playfields match across all three runs:
   159,744 pixels each, excluding HUD/MOS text and its blinking cursor.
5. The submitted draw entry uses 384 bytes of scratch (32 edges), 154 submits
   for 4,903 edges, and four leases. Module text/data are 5,580/1,340 bytes; the
   package is 7,784 bytes. Measured full-application command stack has over
   1,800 bytes free. These are fixture measurements, not global edge limits.

Generic receipts live under [qualification/](qualification/); full application
receipts and the reproducible [comparison](wireframe/comparison.json) live under
[wireframe/](wireframe/). Ignored `build/` directories retain raw logs, wire
captures, images, binaries and stack reports.

An early VDP-only large-fixture upload stalled after the transform comparisons;
it was stopped and is not a full-fixture pass. Full MOS transport is the consumer
qualification authority. A separate exploratory attempt to activate a stock echo
callback exposed recursion in the existing echo flush path. No runtime repair
was added: echo-enabled submission is rejected, and qualification separately
checks that a runnable callback registered with echo disabled remains untouched
during submission.

## Measured cost and comparison

The same pinned firmware/toolchain grows from 1,086,192 to 1,086,976 image bytes:
**+784 bytes**, with **+16 bytes of static DRAM**. The mapped flash-use increase is
780 bytes. The configured application partition has 223,744 bytes beyond the new
raw image. Direct native primitive callbacks have not been implemented or
measured for comparison.

The paired Corsair test fixes geometry, the unchanged C++ transform, six poses,
mode 20, colour/clip/HUD, console setting and C3/GP boundaries. Each fresh run
alternates the arm order. Both drawing programs and the edge table are coinstalled
to control allocation layout. The reports retain raw samples; MOS timing has
centisecond units but effective 20 ms quantization. Transform/status and combined
drawing/fence are measured separately; isolated raster time, clear/HUD time and
generation-only time are not measured. Queued drawing overlaps generation.

The JIT complete-frame median is 780 ms for each arm. The interpreter medians
are 1,760 ms resident versus 940 ms submitted (about 1.87 times faster). These
are emulator measurements, with no prediction of physical hardware performance.
The resident drawing program contains 176,508 command bytes installed once; the native path
uses a 19,612-byte retained edge table and generates 58,836 PLOT bytes locally per
frame. Both are called through one six-byte resident frame call. Those installed
resident bytes were never per-frame UART traffic. Both arms send exactly 221
eZ80-to-VDP bytes per timed frame, including controls, checks and HUD. The separate pure scene-container sizes are 216,960
bytes resident versus 60,200 bytes submitted: 156,760 bytes saved (about 72%).
`wireframe/analyze.py` retains the calculation and raw capture hashes.

## Reproduce without human interaction

1. In agon-vdplet, run `make check` and `make firmware`. Receipts in
   `docs/tasks/VDPL-003/` pin the before/after firmware and dependencies.
2. In Golem, run `.venv/bin/python docs/tasks/GOLEM-047/qualification/qualify.py
   --engine jit` (or `interp`) for service-only execution. `check_old.py` uses
   the retained pre-extension firmware for negative compatibility.
3. Run `.venv/bin/python docs/tasks/GOLEM-047/wireframe/build.py` to rebuild
   the isolated consumer from its pinned peer inputs. It never edits those inputs.
4. The full MOS builder/run script is `wireframe/profile.py`. It uses the
   canonical emulator setup tool, requires explicit runtime/variant arguments,
   performs headless runs only, and refuses an occupied/unowned profile.
   Its frozen runtime prerequisites are local qualification inputs, not new
   production runtime dependencies.
5. Retrieve completed images/wire captures and run `wireframe/analyze.py`.
   Stage `--variant review --stage-only` afterward, leaving the profile stopped.

Earlier authorized work and plans were committed/pushed before implementation.
The Author accepted this runtime/consumer/profile tranche and explicitly
authorized publication on 2026-10-24. Physical qualification remains separate.
The Author makes actual VDPlet rendering performance the immediate priority,
independent of implementation language; portable Golem drawing adoption is
secondary. The current comparison does not isolate the pixel renderer.


Accepted generic runtime/SDK pin: `5232938` (agon-vdplet `codex/initial-runtime`).
