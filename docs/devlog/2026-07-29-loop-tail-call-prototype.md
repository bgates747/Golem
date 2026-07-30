# 2026-07-29: Hand-prototyped loop + tail-call-jump verification

Addresses AGENTS.md's "Current task" checklist item 2: "Hand-prototype the
'one instruction per block' encoding for a trivial loop; verify tail-call
jump conversion behaves as documented (check against `vdu_buffered.h`, not
just the prose docs)." Both parts of that item are now done, and a real,
previously-undocumented conditional-check gotcha was found and worked
around along the way.

## What was built

[examples/loop_test/loop_test.asm](../../examples/loop_test/loop_test.asm) -
a hand-assembled (not `golemc`-compiled - `golemc` has no loop syntax yet)
eZ80 program, following the same pattern as `examples/hello/hello.asm`: use
`runtime/legacy-asm/vdu_buffered_api.inc` to talk to the VDP's Buffered
Commands API directly. It's a byte-for-byte reproduction of
`Buffered-Commands-API.md`'s "Repeating a command" worked example (three
buffers: a print routine, a 1-byte counter, and a loop-body buffer that
calls the print routine, decrements the counter, then conditionally calls
itself), with the loop count deliberately set to 50 - well past the docs'
own "the \[call] stack depth limit appears to be in the region of 20 calls"
warning - specifically so that a failure to tail-call-optimise would be
obvious (crash/hang) rather than silently passing under a low count.

An `emulator/` profile was added alongside it (symlinks to the canonical
`fab-agon-emulator` binary/firmware + an `sdcard/` with `autoexec.txt`),
matching `examples/hello/emulator/`'s established, uncommitted,
project-local convention.

## Result: tail-call-to-jump conversion is confirmed working exactly as documented

Running under `--verbose` and grepping the log for the relevant debug
lines gives an exact, countable confirmation:

```
bufferCall: buffer 1   (the print routine)   -> 50
bufferCall: buffer 3   (real call + each successful conditional-call attempt) -> 50
bufferJump: buffer 3   (tail-call-converted jumps)                            -> 49
evaluated as true                                                              -> 49
evaluated as false                                                             -> 1
```

That's exactly what the docs describe: 1 real (non-tail) call into buffer 3
from `main`, then 49 successive conditional-calls-that-become-jumps (one
per completed iteration), and a 50th conditional check that correctly
evaluates false and simply falls off the end of the buffer, returning
control to the original caller - with the whole 50-deep logical loop never
growing the VDP's internal call stack past depth 1. No crash, no hang.

This was also cross-checked directly against the firmware source (not just
the prose docs, per the checklist's explicit wording):
`agon-vdp/video/vdu_buffered.h`'s `bufferCall()` (around line 337) contains
exactly:

```cpp
if (id != 65535) {
    if (inputStream->available() == 0) {
        // tail-call optimise - turn the call into a jump
        bufferJump(bufferId, offset);
        return;
    }
    ...
```

i.e. the conversion trigger is purely "is this the last remaining byte in
the current stream" - which matches the mental model used to design the
test (build buffer 3's three command blocks in order, with the
conditional-call block always the final one, and never append anything
after it).

## A real conditional-check gotcha found (and worked around)

The first version of this test used `VBUF_COND_EXISTS` (command 6, basic
form, no modifier flags) to check "is the counter buffer's byte non-zero".
That version ran the loop **51** times instead of 50, and then hit a
`bufferConditional: invalid source or operand value` VDP-side rejection on
what would have been attempt 52 (after the counter byte wrapped from 0 to
255).

Root cause, confirmed by reading `vdu_buffered.h`'s `bufferConditional()`
and `buffers.h`'s `readBufferBytes()`:

- `bufferConditional()` initialises `int32_t sourceValue = -1;` (i.e. all
  four bytes `0xFF`) before reading the checked value.
- For a non-16-bit, non-buffer-value-operand check, it then does
  `readBufferBytes(checkBufferId, offset, &sourceValue, 1)`, which is a
  plain `memcpy(target, ..., 1)` - it overwrites only the **lowest** byte
  of `sourceValue`, leaving the upper three bytes as the `0xFF` from the
  `-1` initialisation.
- So a checked byte of `0` doesn't read back as `sourceValue == 0` - it
  reads back as `0xFFFFFF00` (`-256` as a signed 32-bit int), which is
  **non-zero**. `VBUF_COND_EXISTS`'s check is exactly `sourceValue != 0`,
  so it wrongly evaluates true for a zero byte - one extra iteration.
- The following iteration decrements the counter buffer from `0` to `255`
  (`0 - 1` wraps as an unsigned byte). Checking that byte the same way
  produces `sourceValue == 0xFFFFFFFF` (`-1`) - which collides with the
  sentinel value `bufferConditional()` uses to mean "the read failed",
  so it's rejected outright ("invalid source or operand value") instead
  of being evaluated as a normal (very large, definitely non-zero) value.

This isn't an emulator bug - `vdu_buffered.h`/`buffers.h` are the same
shared firmware source used on real hardware, so this would reproduce
there too. It specifically affects the "basic" single-operand check forms
(`VBUF_COND_EXISTS`/`VBUF_COND_NOT_EXISTS`) and comparisons against a
*literal* operand, because the operand in those cases is read cleanly
(no `-1`-contaminated upper bytes), while the checked buffer byte is not -
so the two sides are never comparable in a way that behaves as naively
expected once the checked byte is exactly `0` (or `255`, by unlucky
sentinel collision).

**Workaround used** (and now the recommended idiom for Golem's own
codegen, once it grows loop support): compare the counter against a
second, permanent buffer holding a constant `0`, using
`VBUF_COND_NE | VBUF_COND_F_BUFVALUE` instead of `VBUF_COND_EXISTS` or a
literal operand. Both sides then go through the exact same
`readBufferBytes`-into-a-`-1`-initialised-int32 path, so their identical
upper-byte contamination cancels out in the comparison
(`0xFFFFFF00 == 0xFFFFFF00` is still true when both real bytes are `0`).
With that fix, the log is exactly the clean 50/50/49/49/1 breakdown shown
above, with zero `invalid source or operand value` occurrences.

See `examples/loop_test/loop_test.asm`'s header comment for the same
writeup in-place next to the working code, and
`/memories/repo/golem-asm-conventions.md` for the condensed note-to-self
version.

## Verification workflow used

Same emulator-testing pattern as previous sessions: swap the built binary
onto `examples/loop_test/emulator/sdcard/`, run
`./fab-agon-emulator --sdcard sdcard --verbose --firmware console8 --zero`
redirected to a log file, then `grep -c` the log for the specific debug
lines above rather than trying to visually inspect the rendered screen
(this is more precise anyway - it gives an exact iteration count, not just
"it looked about right"). Note: once the program finishes and returns to
the MOS prompt, the emulator window stays open (normal interactive-app
behaviour, not a hang) - the process only needs to be given enough time to
run the actual test sequence (a second or two), then can be left running
or killed; no need to wait for it to exit on its own.

## Checklist status

AGENTS.md's checklist item 2 ("Hand-prototype the 'one instruction per
block' encoding for a trivial loop; verify tail-call jump conversion
behaves as documented") is now complete and verified against both the
prose docs and the firmware source.
