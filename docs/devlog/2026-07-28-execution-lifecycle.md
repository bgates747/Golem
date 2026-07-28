# Execution/lifecycle model correction: `bufferCall()` is synchronous and blocks the VDP's outer event loop

Date: 2026-07-28

Prompted by Codex's response to `docs/devlog/to_codex.md` (section 10.2 of
`docs/devlog/from_codex.md`). This is a genuine correction to an assumption
implicit in [2026-07-28.md](2026-07-28.md)'s original language sketch, not
just toolchain bookkeeping - it belongs with the core design, so it gets its
own entry rather than living inside the toolchain devlog.

## The finding

Verified against firmware by Codex, sources cited below:

1. `VDUStreamProcessor::bufferCall()` swaps in the selected buffer and calls
   `processAllAvailable()` **synchronously**.
2. That inner loop explicitly does **not** call `processEventQueue()`.
3. VSYNC callbacks and keyboard/mouse collection are dispatched only from
   the *outer* `processNext()` loop, after the current buffer call returns.

```text
~/Agon/agon-vdp/video/vdu_buffered.h
    VDUStreamProcessor::bufferCall()

~/Agon/agon-vdp/video/vdu_stream_processor.h
    VDUStreamProcessor::processAllAvailable()
    VDUStreamProcessor::processNext()
    VDUStreamProcessor::handleKeyboardAndMouse()

~/Agon/agon-vdp/video/vdu.h
    VDUStreamProcessor::vdu_mode()

~/Agon/agon-vdp/video/buffers.h
    callbackBuffers
```

## Why this matters

[2026-07-28.md](2026-07-28.md)'s original language sketch says "loops
preferred over recursion (zero stack cost, maps to conditional jump)" and
"tail calls are free ... should be used for recursion-shaped code wherever
possible." Read naively, that suggests a Golem `main` could just be one
big, permanently-running conditional-jump loop resident in a buffer -
exactly the shape you'd want for a "game loop," for instance.

That shape is now known to be actively harmful: **a non-returning Golem
`main` starves the VDP's outer event loop entirely.** While such a loop
runs, queued UART commands, general-poll markers, and outer-loop
VSYNC/physical-input processing (and any callbacks attached to them) never
get a turn, because they're only serviced by `processNext()`, which never
regains control while a buffer call is still executing inside it. This
isn't a performance concern, it's a correctness one - a Golem program
written the "obvious" way (per the original sketch) would silently freeze
its own input handling and any VSYNC-driven animation/timing.

Buffered callbacks (commands 80/81) are **cooperative synchronous calls,
not interrupts or concurrent tasks** - the same constraint applies
recursively: a callback handler that doesn't return promptly wedges the
same VDP loop it was supposed to be called back into.

Two further lifecycle rules follow from current firmware behaviour:

- **A mode change clears all VSYNC registrations.** A loader or mode-change
  path must re-register Golem's VSYNC dispatcher afterward, or VSYNC-driven
  code silently goes dead after any mode change.
- **Callback buffers are stored in an `unordered_set`** - multiple
  registered handlers for the same event are not guaranteed to run in
  registration order. Where order matters, register one Golem dispatcher
  buffer per event type and impose order inside it, rather than relying on
  registration order across multiple independently-registered buffers.

## Corrected execution model: three lifecycle profiles

Per Codex's proposal, adopted here:

1. **`batch`** - the loader calls a finite entrypoint, which returns.
2. **`event-resident`** - the loader installs bounded VDP-side VSYNC/input
   handlers (via command 80); no continuing eZ80 controller is required.
   This is the "clean pure-VDP interactive model": persistent code and
   state live in buffers, but as a set of short, returning handlers
   dispatched by the VDP's own outer loop - not as one permanently-running
   foreground loop.
3. **`hosted/co-processor`** (deferred, optional) - an eZ80 host owns
   foreground state and invokes finite exported Golem jobs. Not part of the
   core language; see the "host-extension seam" note below.

This supersedes the "one big loop, zero stack cost" framing for anything
that needs to coexist with VSYNC/input/other-buffer-callback processing.
Straight-line loops (`while`/`for` compiled to conditional jumps) are still
the right tool *within* a bounded, promptly-returning handler or batch
entrypoint - the correction is about *unbounded, non-returning* top-level
control flow, not about jumps/loops as a compilation strategy in general.

## What stays unchanged

Per Codex's explicit framing ("reserve an interface, not resources"):
Golem's core language and ordinary runtime remain pure-VDP. The only
*required* eZ80-side component is the loader/bootstrap that installs the
compiled buffers - no mandatory eZ80 controller, no MOS/UART/`mos_setkbvector`
assumptions baked into core semantics. A **host-extension seam** should be
reserved for the future `hosted/co-processor` profile (symbolic entrypoints
and lifecycle metadata that a future eZ80 host could invoke), but without
allocating any MOS vector, packet magic, or fixed buffer-ID range for it
yet - that's a "maybe later" concern per the Author's own "start simple,
probe boundaries" stance recorded in
[2026-07-28-compiler-toolchain.md](2026-07-28-compiler-toolchain.md).

The general-poll/`sysvar_gp` barrier technique (see the addendum in
[2026-07-28-vdu-buffered-api.md](2026-07-28-vdu-buffered-api.md)) remains
directly useful today, but only around a **finite** Golem buffer call: it
proves the serial parser reached the marker after the call returned and all
earlier input was consumed. It does not prove completion of arbitrary async
side effects, and it cannot arrive at all behind a non-returning Golem
program - another concrete illustration of why `batch`/`event-resident`
must be the default shape.

## Next steps

- Update the "Suggested language shape" section of
  [2026-07-28.md](2026-07-28.md) to state the `batch`/`event-resident`/
  `hosted` distinction explicitly once the design doc work in
  `docs/design/` begins, rather than leaving the superseded "one big loop"
  framing as the only stated model.
- Decide (not yet urgent, per the Author's "start simple" stance) whether
  Golem's first working example targets `batch` or `event-resident` first.
  `batch` is the smaller thing to build and matches "start with the
  simplest case."
