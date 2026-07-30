
# Testing/debugging Golem output: what the emulator exposes on the VDP side

Date: 2026-07-29

Prompted by a practical worry: once `golemc` emits real command-5 `Adjust`
sequences (see the golemc milestone devlog and Axis 13), how do we actually
*watch* one run and confirm it did what we think? The eZ80 side already has
obvious answers (`-d`/`--debugger`, `-b`/`--breakpoint`, CPU tracing) - this
is a precis of what exists for looking inside the **VDP** side, where the
real action (buffer contents, command-5 arithmetic) happens.

**Canonicity note, per this session's correction:** `/home/smith/Agon/agon-vdp`
and `/home/smith/Agon/agon-mos` are the official upstream firmware
checkouts and are treated as the canonical reference/validation copies for
anything claimed below. `fab-agon-emulator` bundles its own working copy at
`fab-agon-emulator/src/vdp/vdp-console8` (a fork/mirror, `tomm/agon-vdp` per
`.gitmodules`) that it actually compiles into the `.so` it loads - close to
canonical `agon-vdp`, and cross-checked to agree on every point below, but
it's the copy that would need editing if we ever hack the emulator (see
"Hacking the emulator" at the end).

## The short answer: three independent hooks exist today

1. **`--verbose`** - a whole-run firehose of buffer lifecycle events.
2. **`VDU 23,0,&A0,bufferId; &80`** - an on-demand hex dump of one buffer's
   *actual current contents*, embeddable directly in a compiled program.
3. **Ctrl+M (or Alt+M with `--ralt-hostkey`) at runtime** - a heap-allocator
   stats dump, coarser-grained (not per-buffer).

None of these require rebuilding anything - they're all already wired up in
the emulator as shipped. Details below.

## 1. `--verbose`: buffer lifecycle logging

`fab-agon-emulator --help` documents this only as "includes VDP debug
logs," which undersells it. Tracing it end to end:

- `src/parse_args.rs` parses `--verbose` into `args.verbose` (a plain bool).
- `src/main.rs` calls the VDP shared library's exported
  `setVdpDebugLogging(args.verbose)` once at startup (`vdp_interface.rs`
  declares the FFI symbol).
- `src/vdp/rust_glue.cpp` implements `setVdpDebugLogging` as nothing more
  than `vdp_debug_logging = state;` - a single global bool.
- `src/vdp/vdp.h` (emulator-only glue, not part of canonical `agon-vdp`)
  defines the actual `debug_log()` used throughout the VDP source as a
  `static inline` function that checks that bool and, if true, prints to
  **stdout** with an `"ESP32 DBGSerial: "` prefix (a nod to the fact this
  is normally a physical serial line on hardware).
- Canonical `agon-vdp/video/video.ino` confirms *why* a separate userspace
  `debug_log` exists at all: its own `debug_log()` is compiled out entirely
  under `#ifndef USERSPACE` (hardware-only), and is additionally gated at
  compile time by `#if DEBUG == 1`. `video.ino` forces `DEBUG` to `1`
  whenever `USERSPACE` is defined ("Always enabled on the emulator, to
  support `--verbose` mode"), so the emulator build always *compiles* every
  `debug_log(...)` call site in - `vdp.h`'s runtime bool is what actually
  gates whether each one prints, letting `--verbose` be a live on/off
  switch rather than a rebuild.

What this actually logs, confirmed straight out of canonical
`agon-vdp/video/vdu_buffered.h`: every `bufferWrite` (create/append,
including short-write timeouts and the reserved-buffer-65535 case),
`bufferCall` (including "buffer not found"), `bufferClear`, `bufferCreate`
(including "already exists"/"reserved" rejections), `setOutputStream`, and
`bufferAdjust` failure paths (e.g. "no operand buffer ID"). That's real
visibility into whether a command-5 `Adjust` even *found* its target/source
buffers - useful for catching a wrong buffer-ID computation in `golemc`'s
codegen before worrying about whether the arithmetic itself is right.

Caveat: this is a firehose covering *all* VDP subsystems (audio, fonts,
sprites, contexts, ...), not just buffered commands - expect to grep the
output.

## 2. `BUFFERED_DEBUG_INFO` (command byte `&80`): dump a buffer's real bytes

This is the more interesting find for Golem specifically, because it's not
a `--verbose`-only firehose message - it's a genuine **on-demand
inspection command**, callable exactly like any other Buffered Commands
API command, from anywhere a VDU byte stream can be emitted - including
from inside a compiled Golem program's own output.

Confirmed in canonical `agon-vdp/video/agon.h` and `vdu_buffered.h`: the
buffered-commands sub-command byte space (`VDU 23,0,&A0,bufferId; <cmd>`)
includes `BUFFERED_DEBUG_INFO = 0x80`, alongside the already-known
`BUFFERED_WRITE=0x00`, `BUFFERED_CALL=0x01`, `BUFFERED_CLEAR=0x02`,
`BUFFERED_CREATE=0x03`, `BUFFERED_ADJUST=0x05`. So:

```
VDU 23, 0, &A0, bufferId; &80
```

drives straight into `vdu_buffered.h`'s `case BUFFERED_DEBUG_INFO:`, which
uses `force_debug_log` (not `debug_log`) - correctly unconditional in the
sense that it isn't gated by the `vdp_debug_logging` bool `--verbose`
flips, so no `--verbose` flag is needed to *reach* this code path. It
prints the buffer ID and how many stream blocks are stored in it, a
pretty-printed row/column dump if the buffer is flagged as a matrix, and
otherwise a straight hex dump of every byte currently in that buffer.

**Correction (found 2026-07-29, after this section was first written):
in `fab-agon-emulator` as shipped, none of that output is actually
visible anywhere.** `force_debug_log` (defined in canonical,
emulator-bundled-unmodified `video.ino`) calls `DBGSerial.print(buf)` -
`DBGSerial` being a `HardwareSerial` instance, the real ESP32 UART class
on hardware. The emulator's userspace build supplies its own stand-in
`HardwareSerial` at
`fab-agon-emulator/src/vdp/userspace-vdp-gl/src/userspace-platform/HardwareSerial.h`,
and that class's `print(const char *)` override is a literal no-op -
`void print(const char *) {}` - with `begin()` likewise stubbed to
nothing and no redirection to stdout/stderr/a file set up anywhere at
startup. This is a *different* code path from hook #1's `debug_log()`,
which is an emulator-only reimplementation that explicitly does
`fputs(..., stdout)` - the two only *look* similar ("unconditional
debug print in the VDP source") but do not share an output mechanism, and
only one of them (`debug_log`/`--verbose`) is actually wired to anything
observable in the emulator today.

Practically: `BUFFERED_DEBUG_INFO` is real, documented, and reachable,
but **useless for inspection purposes in the emulator until something is
wired up to receive `DBGSerial`'s output (or the emulator's
`HardwareSerial` stub is patched to forward to stdout like `debug_log`
does).** See "Hacking the emulator" below and the note-to-self recorded
in repo memory - this is now a concrete, scoped motivation for actually
doing that hacking, not just a hypothetical option.

## 3. Ctrl+M: heap stats, not buffer contents

`src/main.rs`'s SDL event loop maps the emulator's host-key modifier
(right-Ctrl by default, or right-Alt with `--ralt-hostkey`) + `M` to the
VDP library's exported `dump_vdp_mem_stats()`, which just calls
`malloc_wrapper_dump_stats()` (`src/vdp/rust_glue.cpp`). This is
allocator-level (how much VDP heap is in use/free), not a way to inspect
any specific buffer's contents - useful for chasing the project's own
recorded "~3MB gets antsy" anecdote (Axis 12) if buffer allocation volume
ever becomes a real concern, but not a substitute for hook #2 above.

## Hacking the emulator: allowed, and probably still worth it later

Per direction this session: modifying `fab-agon-emulator` itself (not just
using its existing switches) is fair game if the built-in hooks above turn
out to be insufficient - the bundled VDP source is a local, buildable
checkout (`fab-agon-emulator/src/vdp/vdp-console8`), not a black box, and
the Rust host (`src/main.rs`) already demonstrates the exact pattern
needed (an FFI-exported function from the VDP `.so`, wired to a host-side
trigger - a keypress, in `dump_vdp_mem_stats`'s case). The same pattern
could plausibly give us, for example, a keybind or CLI flag that dumps
*every* live buffer's contents at once, or a hook fired automatically after
every `bufferCall` returns, without needing a `&80` VDU command baked into
the test program itself.

Codex (the external collaborator already in an ongoing thread via
`docs/devlog/to_codex.md`/`from_codex.md` on the separate Pingo project) is
expected to write up a short note on their own approach to hacking
`fab-agon-emulator` for debug purposes - worth reading once it arrives and
folding in here or into a follow-up devlog, since they may have already
solved problems (build wiring, symbol export mechanics) we'd otherwise
rediscover from scratch.

The `DBGSerial`/`HardwareSerial` finding above (hook #2's output being a
no-op in the emulator) is itself now a first concrete candidate patch:
either redirect the userspace `HardwareSerial::print`/`write` overrides to
stdout (mirroring what `debug_log` already does by hand), or add a small
dedicated FFI-exported hook the way `dump_vdp_mem_stats` does for hook #3.
Either would make hook #2 actually usable instead of merely documented.

## Open items / not yet done

- Hook #1 (`--verbose`) **has** since been exercised successfully against
  real `golemc` output: the live addition-primitive test (see the
  golemc-milestone devlog) confirmed the exact expected
  `bufferWrite`/`bufferAdjust` command sequence in the verbose log,
  matching the Axis 13 design. This devlog originally claimed none of the
  hooks had been tried yet - that's now out of date for hook #1
  specifically.
- Hook #2 (`BUFFERED_DEBUG_INFO`) is now known to be a dead end *as the
  emulator ships today* (see correction above) - its output goes to a
  stubbed `DBGSerial.print()` that discards everything. Not yet
  exercised in a way that could have shown output, because there is
  currently no output to show. Needs an emulator patch before it's worth
  testing at all.
- Hook #3 (Ctrl+M heap stats) still not actually exercised against a real
  `golemc`-compiled program.
- Haven't yet checked whether `BUFFERED_DEBUG_INFO`'s hex dump has any
  practical output-length limit worth knowing about for larger buffers -
  moot until hook #2 is patched to actually emit anything observable.
- Codex's note on emulator-hacking technique is pending (see above) -
  revisit once it arrives.
