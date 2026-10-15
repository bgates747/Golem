# Flower resident batch renderer

Flower now draws its calculated curve without sending each point from the eZ80.
The existing Golem program computes up to 32 endpoints into VDP buffers; this
small stock-command renderer reads those buffers and connects only the valid
endpoints. Pi headless checks exercise actual uploaded Xtensa mathematics and
the VDP's line drawing. The interactive C++ application is the next tranche.

## Source and ownership

1. [flower.golem](../flower.golem) owns the curve mathematics. It remains
   unchanged from the accepted native mathematical tranche. `prepare` initializes
   the recurrence; `batch` advances it and publishes `startX`, `startY`,
   `batchCount`, `pointsX` and `pointsY`. The default curve has 396 segments.
2. [build_renderer.py](build_renderer.py) generates the resident drawing bridge
   from the compiler's interface/root/backend metadata. It is task-local tooling,
   not new Golem drawing syntax or a production compiler change. It emits command
   installation/cleanup/call bytes, a manifest and a symbolic assembly include.
3. The application owns loading, invocation, display setup and completion.
   The renderer owns four disjoint reserved buffers: its finite command sequence,
   six-byte move and line templates, and constants/padded comparison scratch.
   Defaults use 60002..60005; the relocated check uses 60020..60023 and reserves
   compiler IDs 1..4095. Shared mathematical state and arrays are borrowed.
4. Installation sends 2,243 bytes once, retaining 2,187 bytes of command/data.
   Rendering submits a six-byte buffer call. Cleanup clears only the renderer's
   four buffers, preserving the mathematical state, arrays and existing picture.
   The installed native module and SDK firmware are unchanged by this tranche.

## Drawing and failure contract

The display is modern screen mode 0, verified as 640x480, with the original
1280x1024 logical units and origin (640,512). Positive Y points upwards. The
physical origin is (320,239); stock scaling truncates each signed displacement
towards zero before adding the origin. Reset viewports **before** setting the
origin: VDU 26 also resets the graphics origin. The bridge does not change the
separate resident drawing profile's pixel-coordinate contract.

For a successful batch, the bridge patches/calls the initial absolute move,
then patches/calls one absolute line for each valid endpoint. Every coordinate
copy stays inside the VDP. It does not clear the screen, consume unused array
slots, close the curve automatically or alter mathematical state. The final
default batch draws 12 segments after twelve batches of 32.

The resident guards suppress drawing when either root status is nonzero, the
host-owned runtime-result field is nonzero, the requested/successful entry is
not `batch`, or the count is zero or outside 1..32. A failed mathematical batch
may retain partial state effects; it draws none of that batch and leaves the
preceding picture intact. There is no rollback, checksum or post-copy admission
check in the renderer.

Stock conditionals preserve upper sentinel bytes when reading buffer operands;
literal comparisons can consequently differ. A raw all-ones fetched word is
also an invalid-value sentinel. The bridge uses buffer-backed constants and
zero-padded individual-byte checks for external counts/statuses. Actual checks
include 256/65535 counts and all-ones statuses/entry IDs. This works with the
existing firmware, without weakening admission or changing VDP code.

The guards cannot detect a native invocation that never entered the module and
left an old successful root untouched. **The host must await/check the runtime
result before submitting the renderer.** A six-byte submission is not completion.
The tested sequence is:

1. Submit the native `batch` wrapper; request/check runtime INFO(3).
2. On success, submit the resident renderer and read the retained remaining count.
   On failure, skip rendering and handle the error without replaying the batch.
3. Issue a stock screen-pixel query when a plotting-completion barrier is needed.
   Its firmware handler calls `waitPlotCompletion()`. A GP echo orders command
   interpretation but does not establish Canvas plotting completion.

That sequence sends **49 outbound bytes per batch**, including status, remaining
count and plotting completion, and receives 30 reply bytes. Of the outbound
total, compute/render submissions use 12 bytes. Qualification's full state and
array readback is additional diagnostic traffic, excluded from that measurement;
the application does not need those point reads. Finite calls return between
batches so a future client can service Escape. The actual keyboard/cancellation
and completion-before-unload lifecycle remain application work.

## Qualification

[qualify.py](qualify.py) runs the unchanged actual firmware through the frozen
VDP-only adapter, silently and without a window, on the Pi 5 only. It tests:

1. Counts 0/1/31/32/33/256/65535, distinct unused endpoints, repeated calls,
   signed truncation, clipped/cardinal/zero-length lines and error guards.
   An independent exact cardinal-line model checks complete 640x480 images.
2. Default, 22.5-degree, 360-degree, integer-product, zero/full-shrink and
   zero-radius curves, plus default rendering with relocated state/array/private
   buffers. Actual state/coordinates are checked against the independent
   mathematical model using the accepted ULP/logical-unit tolerances.
3. Every full-curve pixel against direct stock PLOT commands from the observed
   coordinates. Those direct diagnostic drawings are qualification only;
   retained curve PNGs contain the resident path before that comparison.
4. A real native overflow after one point was committed: language status 33,
   batchCount 1, no new drawn segment, preceding picture and partial state kept.
   Renderer/module cleanup preserves the borrowed state, arrays and picture.
5. Two real assembler consumers and sanitized SDK-loader admission for the
   default/relocated module packages. The default module retains the accepted
   mathematical image hash; relocation changes embedded array buffer IDs.

The final Pi runs pass with identical reply bytes and curve images:

| Check | JIT | Interpreter |
| --- | --- | --- |
| Complete-image pixel comparisons | 24,883,200 | 24,883,200 |
| Full curves / measured batches | 8 / 92 | 8 / 92 |
| Retained VDPlet reply bytes | 200,610 | 200,610 |
| Minimum free command-task stack | 2,120 bytes | 2,120 bytes |
| Batch guest duration, including completion | 2.83..7.79 ms | 2.79..7.71 ms |
| Complete run, including diagnostics/images | 66.70 s | 103.66 s |

See [packages.json](evidence/packages.json), [engine parity](evidence/parity.json)
and the Pi JIT/interpreter receipts and reply transcripts in [evidence](evidence/).
Emulator cycle durations are
functional measurements, not physical UART/cancellation guarantees. Diagnostic
traffic, image capture and model comparisons are excluded from individual batch
timings; the complete run includes them. The ESP32 clock is modeled at 240 MHz;
Pi wall times additionally reflect emulation and host scheduling.

These checks do not boot MOS, run the C++ application, restore a saved screen
mode or establish physical hardware behavior. Oryx was left untouched during
the Author's thermal tests. No visual profile, desktop instance or firmware
flash was created. Visual staging remains deferred, and the new renderer work
is uncommitted pending review/publication approval.

## Reproduction

From the repository root, with the pinned Xtensa SDK/compiler and native
assembler installed:

```bash
# Only needed if the accepted Pi adapter cache is absent:
.venv/bin/python docs/tasks/GOLEM-044/native-math/build_headless.py

.venv/bin/python docs/tasks/GOLEM-044/renderer/qualify.py --engine jit
.venv/bin/python docs/tasks/GOLEM-044/renderer/qualify.py --skip-build --engine interp
.venv/bin/python docs/tasks/GOLEM-044/renderer/compare_runs.py
```

Set `EZ80ASM` if the native assembler is outside the preceding tranche's ignored
build/assembler directory. `--build-only` prepares the default/relocated packages
and actual assembly consumers. Generated assembly exposes `flower_render`,
installation/cleanup labels and lengths; the manifest publishes all owned and
borrowed IDs. All build/export/trace storage remains ignored beneath this silo.

The Author subsequently accepted retaining the transparent 32-point batching
and explicitly approved commit/push on 2026-10-15. The C++ client tranche is
released, with headless checks and a stopped Oryx review profile; Oryx is now
available and the Author will launch the interactive review themselves.
