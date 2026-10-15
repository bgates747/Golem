# Flower interaction and lifetime contract

The client asks for seven values one at a time, keeps their exact accepted text
and redraws only after the whole set is valid. Returning from a drawing to the
prompts preserves the curve. Escape restores the original screen mode and may
clear the display under the Author's explicit exception.

## Input and screen states

| State | Work | Next state |
| --- | --- | --- |
| Capture | Query fresh mode information; save full mode byte and cursor/input ownership | Set up |
| Set up | Establish mode 0, verify 640x480 and prepare owned resources | Edit field 1 |
| Edit field | Show name/current string; collect bounded replacement | Enter: validate/advance; Escape: exit |
| Prepare | Upload the accepted numeric inputs once; run Golem preparation and validate completion/status | Success: clear/draw; failure: edit with error |
| Draw | Submit one bounded compute/render batch, await completion, poll Escape | More work: next batch; finished: wait |
| Wait | Leave curve visible; consume one key-down event | Escape: exit; other key: edit field 1 |
| Exit | Stop new work; finish pending bounded work, clean owned resources, restore saved mode and usable MOS input | Return to MOS |

Capture the original mode before any mode command. Do not use the original
demo's 1024x768 mode or restore its hardcoded exit mode. A repeated mode command
also clears the screen, so returning to the editor must never send one. Save
the full mode byte, including double-buffer flags. Exit clearing is permitted;
snapshotting/redrawing the departing curve is unnecessary.

`sendModeInformation()` reports that complete mode byte. The VDP also has a
legacy mode family in which mode 0 means 1024x768, so dimension verification is
essential. Keep the mode family unchanged: if mode 0 reports those legacy
dimensions, mode 3 is the corresponding 640x480 choice and must also be verified.
An unsupported/failing setup restores the saved mode and reports the error;
never continue with an incorrectly sized display or silently toggle the family.

Use a fixed four-row prompt pane, positioned explicitly with cursor commands,
without line feeds that scroll the whole screen. It can cover part of the old
curve. Show the field number/name, retained value, replacement text and a short
status/error message on separate rows. Do not clear the graphics viewport when
re-entering it. The next full clear happens only after Golem preparation accepts
the completed set, immediately before the new curve's first move/line.

## Retained strings and validation

1. Initialize the seven strings once from `3.03`, `1.98`, `0.6`, `66`, `0.8`,
   `512`, `0`, in that order. Store each as a bounded ASCII string of at most
   31 characters plus NUL. A replacement uses a separate scratch buffer.
2. Accept decimal syntax with optional sign, decimal point and exponent, with
   at least one mantissa digit. Examples: `3.0300`, `.6`, `0.600`, `2e1`.
   Reject whitespace, NaN/infinity, trailing garbage, unsupported input and
   overlong strings. Preserve accepted characters, including exponent spelling
   and trailing zeroes; never round-trip through printf to reconstruct them.
3. Blank Enter keeps the current string and advances. Backspace removes the
   previous replacement character; at its start it does nothing. Delete may
   share backspace behavior initially. Other navigation/editing extensions are
   optional. Invalid replacements leave the last accepted string/value intact.
4. Validate lexical syntax, complete parsing, finite binary32 conversion and
   field domain before accepting the replacement. Check the rounded binary32
   domain too: decimal depth just above -1 may round to -1 and must reject.
   Current subnormal-input restrictions still apply. Retain strings and parsed
   values separately; parser support must be proved on AgonDev, not inferred
   from host libc. Do not add numeric value computation to the prompt printer.
5. After field 7, Golem computes/checks Q, divisors, initial coordinates and
   derived state. A derived-set error returns to the editor with the candidate
   strings available for correction. Preserve the previous curve throughout.
   If an error occurs after drawing starts, stop the current curve, leave its
   partial image and offer correction; do not replay failed drawing.
6. Angle text `360` or `360.000` remains unchanged in the editor while Golem
   normalizes its numeric initial angle to zero. Input angles otherwise lie
   within 0..360 inclusive; no silent degree/radian ambiguity or extra wrapping.

## Keyboard, completion and failure handling

1. Use nonblocking key-down events, including Escape ASCII 27; ignore key-up
   events. Consuming the wait-state trigger must not insert it in field 1.
   Discard its queued repeats until release so a held trigger key cannot become
   accidental replacement text. During drawing consume/ignore other keys.
2. Install one small assembly callback that demultiplexes VDPlet reply records
   and ordinary key events into separate fixed storage. Do not install AgonDev
   `kbuf_init()` independently: it and the current VDPlet transport both replace
   MOS's single keyboard callback. Preserve ISR registers and never call libc
   or draw from the callback. Restore/disable the owned callback on exit.
3. Start with 32 segments per batch, one outstanding request and explicit
   completion. Measure real target batch duration before promising an Escape
   latency; reduce batch size if necessary. No long top-level resident loop,
   background thread or VSYNC wait per curve segment is required.
4. Submission return is not completion. Match native replies to the outstanding
   operation/handle and check transport, interface and language status. Reuse
   the already-qualified stock general-poll/owned-buffer query technique where
   needed; do not make the client depend on emulator debug ports.
5. A general-poll echo after a finite call is a command-stream barrier, not a
   guarantee that Canvas has drained. Use the stock pixel query's documented
   implementation wait as a final graphics barrier before declaring the curve
   ready or performing exit restoration. Poll keyboard while waiting.
6. Capability discovery, loads, calls and screen queries need finite waits.
   Escape latches an exit request and prevents further submissions while a
   pending batch finishes. Timeout is an unresolved in-flight operation: do not
   unload/free its buffers or assume it stopped. Report the failure and use a
   documented recovery path; native code is trusted and cannot be forcibly
   cancelled by the current runtime. Qualifying that exceptional path remains
   required implementation work, not a promise of safe arbitrary module hangs.
7. If no compatible runtime is available, report that Flower's native math path
   is unavailable. Do not flash firmware, evict another application's module or
   silently substitute host mathematics. The future stock path must be admitted
   explicitly once its new math operations qualify.

The client owns loading, state updates, submission, keyboard, mode and cleanup.
Golem owns the curve arithmetic; the VDP-resident renderer owns connected lines.
This division remains visible in the eventual application's source and labels.
