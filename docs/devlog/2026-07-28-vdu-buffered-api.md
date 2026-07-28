# `vdu_buffered_api.inc`: an eZ80 wrapper library for the VDP Buffered Commands API

## Summary

Wrote [runtime/legacy-asm/vdu_buffered_api.inc](../../runtime/legacy-asm/vdu_buffered_api.inc)
(moved from `src/runtime/` on 2026-07-28 once it became clear `src/` should
be reserved for the compiler's own C++ source, then into `runtime/legacy-asm/`
alongside a sibling `runtime/agondev/` reserved for the future
agondev-built loader/runtime),
an original (not vendored) eZ80 assembly library providing wrapper routines
for every command documented in
`/home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md`. Its primary
intended use is as a standalone library for hand-written eZ80/MOS assembly
applications that want to use the Buffered Commands API directly; several of
its routines and its calling convention are also intended to double as a
baseline for a future Golem compiler/loader component that constructs and
uploads VDP buffer command blocks from eZ80 SRAM. See "Primary use case"
below for the reasoning behind that framing.

Every byte layout in the file was cross-checked against the firmware source
(`/home/smith/Agon/agon-vdp/video/vdu_buffered.h`), per the project's own
convention (golem/AGENTS.md) of treating firmware as the definitive source of
truth over prose docs. This turned up two places where relying on the docs
(or on the dispatcher `switch` alone) alone would have produced a wrong or
overly-pessimistic implementation - see "Surprises" below.

## Design

- **`vbuf_header`**: the universal 6-byte `VDU 23, 0, &A0, bufferId; command`
  prefix-emitter. Every other routine is built on top of it.
- **`vbuf_raw`**: a generic escape hatch - prefix, then blast a caller-built
  tail of bytes (pointer+length). This can express *any* command, including
  every one classified "needs inspection" or "punted" below; those
  classifications are about whether a *friendly, register-driven* wrapper
  was written, not about whether the command is reachable at all.
- Friendly wrappers stash their register arguments into small per-routine
  static scratch cells (`@name:` labels, scoped between two ordinary global
  labels per ez80asm's local-label rules) before calling `vbuf_header`,
  then replay the scratch cells via `RST.LIL $18`. This mirrors the existing
  `vdu_load_buffer` idiom in `reference/agnb-asm/lib/includes.inc`.
- Calling convention: `hl` = primary bufferId, `a` = operation/options byte,
  `de`/`bc` = secondary numeric args or pointers, `ix`/`iy` spill for the
  few commands needing more than 3 register-pairs. Full convention
  documented in the file's header comment.
- Genuinely variable-shape arguments (terminated buffer-ID lists, advanced
  offset/operand encodings, matrix arguments, bitmap mapping tables) are
  never synthesized internally - callers supply a pointer+length to their
  own pre-built bytes. This is the "bounded dynamic scratch memory" approach
  anticipated when this task was scoped.

## Primary use case

The original framing for this file was "baseline for a future Golem
compiler/loader component." On reflection, that undersells (and misorders)
its actual near-term value.

The Buffered Commands API's central efficiency trick is that a command
sequence, once uploaded to a buffer, can be re-triggered later with a short
"call buffer" (command 1/2, a handful of bytes) instead of re-streaming the
full sequence again. That matters because the eZ80-to-VDP link is a
comparatively slow synchronous UART: every byte sent over it is time the
eZ80 isn't spending on other, possibly time-sensitive, work. For any
hand-written eZ80/MOS assembly program - not just a hypothetical Golem
compiler's output - hand-assembling a non-trivial buffered command (an
affine transform, a multi-field bitmap draw, a conditional/looping
sequence) byte-by-byte from scratch is tedious and error-prone, and doing
it repeatedly just to re-trigger the same operation defeats the purpose of
buffering it in the first place.

So this library's **primary** use case, going forward, is as a standalone
toolkit for regular (non-Golem) eZ80 assembly applications that want to
build and upload buffered command sequences without re-deriving byte
layouts from the docs/firmware every time, and without repeatedly paying
the UART cost of re-sending them. Its **secondary** use case - the original
framing - is that several of its routines and its calling convention are
likely reusable as-is, or as a design reference, for whatever eZ80-side
loader/runtime-shim component Golem's compiler eventually needs (per
golem/AGENTS.md's note that the compiled program itself runs entirely
inside VDP buffers, so *something* still has to construct and upload the
initial buffer blocks from the eZ80 side). That secondary use case doesn't
touch the harder, still-open compiler-architecture questions (host
implementation language, IR design, the block/address allocation model) -
it only de-risks the narrower "get bytes into a VDP buffer correctly"
layer that a compiler backend would need regardless of how the rest of it
is designed.

## Classification

**Straightforward** (full, general implementation; no caller-built tail
needed for any documented variant):
commands 0 (write), 1 (call), 2 (clear), 3 (create), 4 (set output stream),
7 (jump), 13 (copy), 14 (consolidate), 15 (split), 16 (split into), 17
(split from), 18 (split by), 19 (split by into), 20 (split by from), 21
(spread into), 22 (spread from), 23 (reverse blocks), 24 (reverse data - all
options-byte combinations fully handled), 25 (copy ref), 26 (copy
consolidate), 64 (compress), 65 (decompress), 80 (add callback), 81 (remove
callback), 128 (debug info).

**Needs further inspection** (a friendly wrapper is provided for the
common/basic case; less-common modifier-bit combinations require a
caller-built tail via `vbuf_raw`, with the exact byte layout documented in
comments):
- 5 (adjust) - basic single-target/single-operand form only; the
  `&10`/`&20`/`&40`/`&80` advanced modifier bits (advanced offsets,
  buffer-fetched operand, multi-target, multi-operand) are not wrapped.
- 6, 8 (conditional call/jump) - basic buffer-byte-check form only; VDP
  variable checks, buffer-fetched operands, 16-bit comparisons, and advanced
  offsets are not wrapped.
- 9, 11 (jump/call to offset) - these commands *always* use the 24-bit
  "advanced offset" wire format; the wrapper covers the common sub-case
  (no explicit block index, i.e. offset bit 23 clear). The block-index
  variant needs a hand-built 5-byte offset via `vbuf_raw`.
- 10, 12 (conditional jump/call to offset) - offset is handled directly
  (same caveat as 9/11), but the condition tail (operation/checkBufferId/
  checkOffset/operand) is always a caller-built blob via an `ix`/`bc`
  pointer+length pair, since combining the offset and condition arguments
  exhausts the available register pairs.
- 48 (read VDP variable) - basic non-advanced-offset form only; the
  advanced-offset variant needs `vbuf_raw`.
- 72 (expand bitmap) - the fixed `options`/`sourceBufferId`/`width` prefix
  is fully implemented, but the mapping-data tail (either 2^bpp inline
  bytes or a buffer ID, depending on two independent options bits) is
  always a caller-built blob.
- 40 (transformed bitmap) - fully implemented, but flagged here rather than
  "straightforward" only because the command is experimental and gated
  behind the VDP's affine-transforms test flag (`VDU 23, 0, &F8, 1; 1;`).
- 41 (transform data) - only the certain, fixed part of the shape
  (`options, format, transformBufferId;, sourceBufferId;`) is documented;
  the firmware (`bufferTransformData` in vdu_buffered.h) confirmed that each
  of the four optional trailing fields (size/offset/stride/limit) has its
  own byte width *and* can independently be redirected to come from a
  buffer+offset instead of the command stream, which was judged too
  variable to usefully fix in registers. No dedicated wrapper routine is
  provided at all; use `vbuf_raw` directly per the comment above the
  relevant section.

**Totally punted** (constants only, or a small argument-free subset wrapped;
no attempt at the general case):
- 32, 33 (2D/3D affine transform matrix) - only the two argument-free
  operations (`set identity`, `invert`) are wrapped. Every other operation
  needs a floating-point or fixed-point encoded argument, and Golem has not
  yet decided on a numeric/float representation (see the "no general
  floating point initially" note in
  [2026-07-28.md](2026-07-28.md)). Also experimental/test-flag-gated.
- 34 (arbitrary-dimension matrix) - no wrapper at all. Every operation
  requires either a float/fixed argument or a row/column/buffer-ID
  combination; given the same floating-point-representation blocker as
  32/33, this wasn't worth partially wrapping. Constants for the operation
  table are provided for future use.

## Surprises (docs vs. firmware)

1. **Command 40 (transformed bitmap)**: the firmware's `vdu_sys_buffered()`
   dispatch `case` only reads `options`, `transformBufferId`, `bitmapId`
   from the stream before calling `bufferTransformBitmap(...)` - at first
   glance this looked like the documented optional `width; height;` fields
   (sent when the "explicit size" options bit is set) didn't actually exist
   in this firmware version. Reading `bufferTransformBitmap`'s *body*
   (rather than just the dispatch site) showed it reads `width`/`height`
   itself, later, directly from the same live stream - so the documented
   behaviour is correct after all. Lesson reinforced: the dispatch `switch`
   only shows a command's *fixed* prefix reads; variable/conditional reads
   can happen deeper inside the per-command handler function, so verifying
   "does this command have more args" requires reading the handler body,
   not just the dispatch case.
2. **Command 41 (transform data)**: confirmed genuinely more complex than
   a first read of the docs suggested - `bufferTransformData`'s internal
   `getArg` lambda shows every one of the four optional fields can
   independently be 8-bit or 16-bit *and* independently be buffer-fetched,
   which the prose docs describe but easy to under-estimate the combinatorial
   effect of without reading the code.

## Next steps

- Verify the two implemented affine-transform-matrix operations (identity,
  invert) and command 40 against the emulator/hardware once there's a
  runnable harness - all three require the VDP's affine-transforms test
  flag, which nothing in this repo has exercised yet.
- When Golem's numeric/float representation is decided (open item in
  [2026-07-28.md](2026-07-28.md)), revisit commands 32/33/34/41 to add
  proper float/fixed-point argument encoding helpers and, likely, friendlier
  wrappers for the currently-punted matrix operations.
- No assembler has been run against this file yet (no build system exists -
  see golem/AGENTS.md). Once a host toolchain/assembler is chosen, this
  file should be test-assembled (and ideally exercised against the emulator)
  to catch any typos in the register/stack choreography that reading alone
  didn't surface.

## Design note: regression testing is hard because the VDP is write-only from the eZ80's perspective

This is a fundamental platform constraint (deliberate on Bernardo's part, to
keep the hardware architecture simple while still capturing the feel of
older uni-directional-VDU systems like the BBC/Acorn series), not something
specific to this file, but it directly affects how `vdu_buffered_api.inc`
(and any future Golem-compiled output) can ever be regression-tested, so
it's recorded here.

**The constraint, verified against firmware:** there is no generic "dump
buffer bytes back to the eZ80" command anywhere in the Buffered Commands
API. The general (non-buffered) VDU protocol does have a family of fixed,
single-value "send X back to MOS" queries
(`/home/smith/Agon/agon-vdp/video/vdu_sys.h`, roughly lines 389-577):
general poll/echo (`&80`), cursor position (`&82`), screen char
(`&83`/`&93`), screen pixel value (`&84`), colour (`&94`), time, mode info,
keyboard state. None of these read back arbitrary buffer contents - only
specific rendered/system values. So, as things stand, the only way to
inspect what's actually in a VDP buffer is to convert it to a bitmap (or
other renderable form) and dump it to the screen - and even then, "reading"
the result back into the eZ80 programmatically (rather than requiring a
human to look at it) is only possible by querying individual pixel/colour
values back via `&84`/`&94` after rendering, which is slow, indirect, and
limited to data that can be meaningfully rendered.

**A separate, narrower channel that exists but doesn't help production
code:** `debug_log()`/`force_debug_log()` (`video.ino` ~line 178) write to a
completely separate `DBGSerial` port, gated by a `DEBUG == 1` compile flag
(`debug_log` only; `force_debug_log` always fires). This is invisible to
eZ80 runtime code - it's a PC-side/dev channel only (a USB debug port on
real hardware, or the emulator's console/log). `vdu_buffered.h` already logs
rich per-command internal state (bufferId, offset, computed values) on
nearly every command path, which makes it a plausible oracle for a PC-side
test harness driving a DEBUG-build emulator, at the cost of being fragile
(tied to exact log wording, DEBUG-only, and unrelated to what real hardware
or release firmware would confirm).

**Practical takeaway - split testing into two tiers:**

1. *Encoder-correctness tests*: does a routine in this file emit the exact
   documented/firmware-verified byte sequence for a given call. This needs
   no VDP readback at all - capture and compare the raw bytes leaving
   `RST.LIL $18` (e.g. UART capture under the emulator, or a host-side
   stub). Cheap, deterministic, and where most of the near-term test value
   for this specific file lies.
2. *Semantic/execution tests*: did the VDP actually do the right thing to
   its buffer once the command was interpreted. Genuinely constrained by
   the write-only property above. Options, in roughly increasing
   fidelity/cost: (a) manual bitmap dump and visual inspection (the
   baseline, not automatable), (b) automated bitmap-dump-plus-pixel-readback
   using the existing `&84`/`&94` queries, (c) scrape a DEBUG-build
   emulator's `debug_log` output as an oracle, (d) as a last resort, and
   only after discussion, patch a test-only raw-buffer-readback command into
   a local emulator fork - this would give perfect CI fidelity but the
   resulting tests would no longer verify anything a real board could ever
   confirm, so it trades platform-fidelity for convenience and shouldn't be
   done without weighing that trade-off first.

No decision has been made yet on which tier(s) to build tooling for; this
is a design note to revisit once there's an actual test harness (or a
compiler backend generating code to test) to make it concrete.

### Addendum: input from `docs/devlog/from_codex.md`

A cross-project handoff from the Pingo render-completion work
(`docs/devlog/from_codex.md`) independently confirmed the write-only-VDP
finding above (their section 7.3: their hardware-proven completion event
"does not solve Golem's arbitrary VDP-buffer readback problem") and
contributed two things worth folding in here:

- **A missing synchronization primitive.** Neither testing tier above
  answers a more basic question underneath both of them: how does a test
  harness know the VDP has actually finished processing everything sent so
  far, before it goes and inspects a rendered bitmap or reads back a pixel?
  Codex's fixture uses a stock, no-firmware-changes trick for exactly this:
  clear MOS `sysvar_gp` (`IX+&37`), send `VDU 23,0,&80,&A5` (general
  poll/echo with a caller-chosen marker), then busy-wait until `sysvar_gp`
  becomes `&A5`. Because the VDP processes its input queue in order, this
  is a reliable "everything before this point has been consumed" barrier,
  usable by any future Golem test harness (or by hand-written apps) without
  needing a debug build or emulator instrumentation.
- **A third testing axis: timing/ordering correctness.** Codex's postmortem
  (their section 6, item 5) found a real hardware input-queue-ordering bug
  that an emulator pass did not surface - the emulator got the functional
  behaviour right but not the timing. That's distinct from both tiers
  above: it's not about whether our routines emit the right bytes (tier 1),
  and not about whether the VDP's buffer *state* ends up correct (tier 2) -
  it's about whether operations are correctly sequenced/paced relative to
  the VDP's actual, asynchronous processing rate. The mitigation is
  different too: more emulator instrumentation won't necessarily catch this
  class of bug, since Codex's experiment shows the emulator can pass while
  hardware fails. Timing-sensitive Golem-generated code (or runtime-library
  routines) will need to be validated on real hardware at some point, not
  just under the emulator, and the general-poll barrier above is the
  cheapest available tool for imposing determinism where it's needed.
