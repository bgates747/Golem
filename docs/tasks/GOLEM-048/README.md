# Native line batches: faster Corsair rendering

The native line-batch service makes the unchanged Corsair render about **3.5
times faster** in the matched JIT emulator test: **780 ms → 220 ms per frame**.
Final pictures match exactly. The improvement comes from avoiding repeated
graphics setup and queue traffic; the existing line rasterizer still draws all
4,903 ordered edges. Production firmware grows **624 bytes**, with unchanged
static DRAM. The Author accepted the visual/keyboard review and explicitly
approved commit/push on 2026-10-24; [saved-run receipt](human-review.json).

This is VDPlet runtime work using C++. Golem compiler integration is secondary.
Generic API/SDK changes belong to
[VDPL-004](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-004.md).

## Oryx review

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-048
./flexe-agon-emulator
```

This dedicated **Flexe** profile passed human review and is stopped. It uses the production
firmware, not the diagnostic counter build. Expect two white Corsair rotations
through 120 frames in mode 19 (1024×768), then MOS. The final headless run took
30.4 seconds including startup. Follow the single onscreen keyboard instruction,
close the emulator, and report the result. The agent retrieves saved results;
no second terminal or verification command is required. The launcher clears
only this fixture's old results, so rerunning it is supported.

The human animation uses the batch service throughout. The automated comparison
is separate, with no hidden reference drawing in the animation.

| Source | Role |
| --- | --- |
| [worker.cpp](worker.cpp) | Original C++ transform plus separate byte-submission and native-batch drawing entries; the review calls the batch entry |
| `build/transform-source.cpp` | Frozen owner transform, included unchanged; its inputs remain within the original angle bounds |
| [src/main.cpp](src/main.cpp) | AgonDev client: AGNB loading, module admission/binding, control updates, transform status checks, clear/colour/HUD, calls/fences and cleanup |
| [API reference](/Agon/mystuff/agon-vdplet/docs/API-reference.md) | Public callback, exact version/state/error/lifetime and graphics effects |

Assembly/MOS transport and C++ client responsibilities remain explicit. The
worker reads projected vertices and retained edges using four leases. A batch
contains 32 endpoint records; the last contains seven. No geometry is removed,
reordered or merged. The union workspace remains 384 bytes, with 528 bytes of
declared draw-function stack plus the existing dispatcher/callees.

## Matched production results

The comparison fixes the original 2,685 vertices, 4,903 ordered edges, unchanged
transform, six poses, mode 20, logical 1280×1024 units, colour/clip/HUD, console
setting and C3/GP completion. Both paths are installed in each run and use the
same module and state. Each run reverses arm order. Human mode 19 animation is
a separate visual review surface.

| Engine | VDU-byte path | Native line batch | Ratio | Samples per arm |
| --- | ---: | ---: | ---: | ---: |
| JIT | 780 ms | 220 ms | 3.55× | 24 across two fresh boots |
| Interpreter | 1,390 ms | 360 ms | 3.86× | 12 |

Absolute interpreter timings varied between development runs (an earlier run
was 960→260 ms); their cause was not isolated. The table uses the final retained
paired run. JIT medians repeat at 780→220 ms.

These are uninstrumented whole-application frame medians, including transform,
checks, clear/HUD, drawing and completion. MOS values use centisecond units
with effective 20 ms quantization. They are emulator measurements, not physical
ESP32 speed predictions. Each arm sends exactly **221 eZ80-to-VDP bytes per
timed frame**. The stock line kernel and source geometry are unchanged.

Final VDU/JIT, batch/JIT and batch/interpreter playfields match across all
159,744 pixels per image. The crop excludes HUD/MOS text and its blinking cursor,
and includes the whole final distance-six model. All runs unload the module,
clear owned resources and actually return to MOS, with zero execution faults.
Full-application command stack remains above 1,800 free bytes.

The combined module has 5,800 text bytes, 1,340 data bytes and an 8,004-byte
package. The AGNB scene is 60,484 bytes. Both frame wrappers are 20 bytes with
fresh handles bound explicitly; the client invokes each through the same
six-byte buffered call.

## Why it is faster

Task-only firmware copies measure queue admission, line/state execution,
opcode tracing, entry/submission and completion. Prior HUD work drains before
counter reset; counters are read after C3/GP. These diagnostic runs are distinct
from the production speed comparison.

| Median count per frame | VDU bytes | Line batches |
| --- | ---: | ---: |
| Queued operations | 39,225 | 10,423 |
| Line operations | 4,903 | 4,903 |
| Other operations, including state/cursor moves | 34,322 | 5,520 |
| Native submissions | 154 | 154 |

With console opcode tracing disabled in both draw arms, median queue-admission
elapsed cycles fall from about 156.6 million to 41.8 million. Line execution
remains about 3.6 million cycles for both paths. That supports queue/state work
as the main target here; it does not establish a faster pixel algorithm. Turning
tracing off changes how much time waits for queue space, rather than removing
the observed batching gain.

These are local-core CCOUNT elapsed intervals in emulation. Entry, submission,
queue waiting and work on the other core overlap; **do not add them** or treat
nominal 240 MHz conversion as physical timing. Instrumentation overhead is
disclosed, and no profiling callback or counter is added to the production ABI.
[comparison.json](comparison.json) retains raw phase/count samples and medians.
Ignored `build/` retains raw wire/debug captures, images, binaries and reports.

## Correctness and compatibility

1. SDK admission/count/null checks pass under ASan/UBSan. Both instruction
   engines pass 24 service/context cases and 3,145,728 pixel comparisons each.
   Coverage includes coordinate modes, origin/clip, XOR, thickness, prior line
   patterns, both cursor modes, zero/shared endpoints, whole/chunked input and
   later relative/history-dependent drawing.
2. Separate diagnostic tests prepare a pending path without the normal parser
   lookahead. Both engines pass five comparisons: nonempty whole/chunked batch,
   empty no-op, subsequent ordinary path and final Canvas cursor used by text.
   This tests the required first-point-before-path-commit ordering.
3. Prior ABI1.0/1.1 package bytes remain unchanged; precompiled numeric and VDU
   modules execute. A strict ABI1.1 probe receives its exact old table. ABI1.2
   receives the new table. Old unchanged firmware rejects the new package,
   keeps an empty slot and executes the old numeric module afterward.
4. Null/count/context/undeclared-capability/INIT/FINI/echo/open-frame failures
   are checked. All four ordinary leases remain usable; callbacks remain inert
   during a batch. Caller endpoint bytes may be reused on return. A resident
   wrapper continues afterward with its original input stream. Earlier batches
   remain on later-call failure, with an explicit drawing drain before cleanup.
5. The first animation attempt exceeded the unchanged transform's angle domain
   and was correctly rejected. The finite animation now wraps yaw within
   −180…174 degrees; the accepted transform itself was not broadened or changed.
   The final 120-frame production run passes. This was a fixture correction.

The service uses optional ABI1.2/capability8 and an appended 44-byte table.
ABI1.0/1.1 callers keep exact 36/40-byte tables and masks3/7. A callback accepts
at most 8,191 naturally aligned signed16 endpoint records, preserving active
foreground paint, clipping, thickness and ordinary point history. Caller bytes
are consumed synchronously; return is not raster completion. The application
still owns fences, pacing, error handling and resource cleanup. No new thread,
immediate-mode switch, custom rasterizer or production readback is introduced.

## Reproduction

1. Run `make check` and `make firmware` in agon-vdplet. VDPL-004 retains source,
   size, compatibility and baseline receipts.
2. Run `.venv/bin/python docs/tasks/GOLEM-048/build.py` in Golem for the isolated
   application/module. It checks pinned peer hashes and never edits peer files.
3. `qualify.py --engine jit` (or `interp`) runs service/context checks headlessly.
   `check_old.py` uses the retained old firmware. `check_pending.py` uses the
   diagnostic image's explicit context-preparation hook.
4. `profile_firmware.py` makes owned task-only instrumented copies; initial
   baseline and line-service builds are retained separately. `profile.py` runs
   full MOS variants headlessly, using the canonical setup tool. Diagnostic
   runs use `--label instrumented-off` or `instrumented-on`; production runs
   have no label. `analyze.py` verifies pictures/wire and computes the tables.
5. Restore the production firmware and stage `--variant review --stage-only`
   before human review. `verify_review.py` is agent-only after the report.

Human visual/keyboard review passed: the Author reports "worked. a substantial
visual improvement." All 120 frames, saved results, PASS log, runtime pins and
actual MOS directory listing verify. Commit/push was explicitly approved on
2026-10-24. No physical target was flashed or qualified. Further
queue/scheduling optimization and any pixel-algorithm work require a subsequent
tranche; portable language integration remains secondary.
