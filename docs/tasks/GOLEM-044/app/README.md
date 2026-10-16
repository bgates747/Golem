# Interactive Flower

The ordinary AgonDev C++ application is ready for emulator review. It asks for
seven values, draws Flower with the native Golem program, and keeps the exact
accepted input strings for the next drawing. The application and uploaded
module fit in a single executable; no separate asset/module filenames are
needed at runtime. This remains a task-local application awaiting human review.

## Review on Oryx

The dedicated profile is staged and stopped. On Oryx, run:

```bash
cd ~/Agon/mystuff/golem/docs/tasks/GOLEM-044/.emulator
./fab-agon-emulator
```

1. Press Enter through the seven prompts to keep the defaults and draw the
   original 396-segment curve. Each prompt shows its current value in brackets.
2. Press any key to return to the prompts. The existing curve remains visible
   outside the four-row prompt pane. Enter keeps the current string; typing
   supplies a replacement, and Backspace corrects it.
3. Try `3.0300` for Petals and `22.5` for Angle. Those exact strings should
   appear when returning to the prompts again. Angles are decimal degrees,
   inclusive from 0 through 360. No batch-size setting is exposed.
4. Escape exits from input, drawing or the completed picture and restores the
   original screen mode. Clearing on exit is the approved exception. Follow
   the application's single onscreen MOS keyboard instruction, then close
   the emulator and report what worked or differed. The agent retrieves and
   checks the saved results; no second terminal or verifier command is needed.

The fixture begins in mode 8, so the application visibly changes to 1024x768
and returns to the original 320x240 mode on exit. The application itself queries
the actual original mode; it does not hardcode mode 8 as the restoration target.
The generated launcher checks the executable, adapter, actual firmware, current
tagged MOS and source identities, and refuses stock fallback or runtime overrides.
Rerunning the same command starts fresh application state and overwrites its
review log. No interactive session was launched by the agent.

## Which code runs

1. [flower.golem](../flower.golem) contains all curve mathematics: native float32
   division, radians conversion, sin/cos, floor, original recurrence and finite
   32-point batches. It is unchanged from the accepted mathematical slice.
2. [src/main.cpp](src/main.cpp) contains the real application: prompts, input
   validation, loading, named-entry submission, completion, error handling and
   cleanup. It computes no curve coordinates. `parameters.hpp` keeps text and
   parsed numeric values separately; accepted text is never rebuilt with float
   formatting. Field values are packed explicitly in little-endian order.
3. The [resident renderer](../renderer/README.md) patches/calls stock PLOT move
   and line commands from Golem's resident arrays. It draws only their valid
   prefix. The host transfers seven inputs and submits calls, never points.
4. [generate_header.py](../generate_header.py) derives the C interface from
   compiler metadata. Native-only named `golem_prepare()`/`golem_batch()` calls
   are available; the application's checked invocation helper uses the same
   generated named entry IDs. These submit commands to the VDP rather than
   executing ESP32 code on the eZ80. The earlier dual-target ABI proof's two
   generated headers retain identical bytes.
5. [build_app.py](build_app.py) resolves the compiler/renderer layouts and embeds
   the common/private installation, cleanup and module bytes into the executable.
   It also creates a qualification-only occupied-slot fixture. That helper is
   removed from the final human-review SD directory.

The review screen labels those roles. There are no extra diagnostic shapes or
direct-reference drawings hidden in the interactive application. Automated
framebuffer comparisons happen outside the guest application.

## Input and display behavior

| Field | Initial text | Accepted numeric domain |
| --- | --- | --- |
| Petals | `3.03` | Positive finite normal float32 |
| Vectors | `1.98` | Positive finite normal float32 |
| Depth | `0.6` | Greater than -1 after float32 conversion |
| Periods | `66` | Positive; derived raw step count must be below 65535 |
| Shrink | `0.8` | Finite; values outside 0..1 are allowed |
| Radius | `512` | Nonnegative, in the original logical graphics units |
| Angle | `0` | Decimal degrees, 0..360 inclusive |

Replacement strings have a 31-character limit and support signs, decimal points
and decimal exponents. Invalid syntax, nonfinite/subnormal conversion, field
domain failures and overlong entry are rejected without replacing the previous
accepted string. The editor leaves rejected replacement text available for
correction. A blank replacement retains the current text/value. Vendor parsing
is checked on the actual eZ80 as well as the host; it is not assumed to be a
correctly rounded desktop parser. The tested `3.0300` conversion is one float32
ULP above the initial compiled `3.03f` value. Displayed text still remains exact.
Zero mantissas with large exponents do not enter the vendor's exponent loop;
nonzero exponents are bounded before calling its signed 24-bit parser.

Golem preparation validates derived quantities before the screen is cleared.
A set whose raw count exceeds the bound returns to the editor with its strings
and previous picture intact. A later failed batch is not rendered or replayed.
The display uses mode 19: 1024x768, four colours, single buffered. Text and
curves are pure white on black; dimensions and mode selection are checked.
The existing legacy mode family is preserved.
The complete original mode byte, including double-buffer flag, is restored.

The keyboard callback separates VDPlet reply records from a fixed key ring. It
does no libc, filesystem or drawing work. Ordinary character input is withheld
from the MOS editor while physical key/modifier/release state continues updating.
The key which reopens the editor and its repeats are consumed until release;
function keys also count as keys. Escape has a separate latch, even if the ring
were full. At exit the callback is removed and normal MOS input resumes.

## Completion and ownership

Before installation the client safely probes discovery and version/capabilities,
then refuses an occupied module slot. Stock VDP receives only the unknown
discovery selector and a normal ordered poll; no native payload is uploaded.
There is no flashing, eviction, host-calculated fallback or background worker.

The client checks runtime status after each native call, renders only on success,
reads the remaining count and awaits a stock screen-pixel reply as a plotting
barrier. A normal batch uses 49 outbound / 30 reply bytes, including completion;
core compute/render calls are 12 bytes. One extra remaining-count read starts
each complete drawing. Review logging adds MOS filesystem work, not point data
on the VDP wire. Drawing remains finite, giving Escape a boundary between batches.

Cleanup confirms unloading before removing owned native/renderer/common/staging
buffers, then restores the saved mode and cursor/input. Missing replies have
bounded waits. An ordered echo may establish that a finite call completed after
a lost reply. Unconfirmed load/unload ownership, or an unresponsive VDP, retains
resources and requires a reset instead of freeing potentially active code or
claiming a successful MOS return. Close/relaunch the emulator in that exceptional
case; physical hardware requires a reset. Deliberately hung modules, missing-ACK
injection and physical timeout recovery are **not qualified** by the normal
application tests and remain a follow-up before production promotion.

## Evidence and build

The final target fixture covers both actual Xtensa engines for the complete
edit/redraw/error workflow. Additional headless scenarios cover all seven prompt
exits, early-startup Escape, Escape during a 60001-segment drawing, a 31-character
edit, full-circle input, relocated buffers, legacy mode selection, double-buffered
restoration, an occupied slot and absent runtime. Native cases execute a real
MOS `dir` after application exit. The stock control checks controlled exit and
the unchanged original mode, with a captured stock screen; it has no wire-trace
hook and does not claim an independent scripted `dir` witness.

The final executable is **34,760 bytes**, including the unchanged 13,668-byte
native module. All **17 scenarios** pass; the original 640x480 receipts record 2,580,480 exact
playfield comparisons and 95 complete 49-byte batch sequences. The two complete
workflows have identical settled pictures and application logs. Final workflow
runs take 20.37 seconds JIT and 24.42 seconds interpreter, including boot,
scripted typing, screenshot settling and MOS return; these are not rendering
benchmarks. See [summary.json](evidence/summary.json).

Exact pixel comparisons check the default against the accepted renderer and
preservation outside the prompt pane on re-entry and failed preparation. JIT
and interpreter pictures/text are compared separately. Host ASan/UBSan tests
cover shared lexical/domain/retained-text/editor contracts and native C packing/
dispatch. All raw receipts, logs and wire observations live under [evidence](evidence/).
These are emulator checks, not physical hardware acceptance or a promise of
BBC-FPP/float64 precision on very long curves.

From the Golem root, with AgonDev, the pinned Xtensa SDK and native assembler:

```bash
.venv/bin/python docs/tasks/GOLEM-044/app/build_app.py
.venv/bin/python docs/tasks/GOLEM-044/app/host_checks.py
# Optional independent relocation consumer:
.venv/bin/python docs/tasks/GOLEM-044/app/build_app.py --relocated
```

The normal executable is `app/bin/flower.bin`. Layout-specific copies and
generated build storage remain ignored under `app/build/`. Use `EZ80ASM` when
the native assembler is outside the preceding mathematical tranche's cache.
The task's Oryx profile uses copied accepted frontend/adapter bytes and the
same actual firmware as the native mathematical/renderer tranches; it does
not consume another emulator project's current unreviewed build.

For headless reproduction on Oryx, after staging the inputs, use the Golem
.venv and `app/qualify.py --help`. It always sets dummy video/audio drivers,
injects real SDL key events through PS/2, saves receipts and shuts down cleanly.
`profile.py` refreshes only this task's stopped instance and refuses an active
profile. The agent-only `verify_review.py` reads saved human results without
repeating the guest's keyboard instruction. New application/profile work remains
uncommitted pending the Author's human review and explicit publication approval.


## Mode 19 revision — 2026-10-15

The Author requested the highest available resolution and white throughout.
The application now selects mode 19 (1024x768, four colours), maps palette
entry 1 to RGB 255/255/255, and uses it for both text and graphics on black.
The logical 1280x1024 coordinates and Golem mathematics are unchanged; stock
VDP scaling supplies the higher-resolution lines. The four-row input pane
adapts to the reported text dimensions. Legacy-family mode 19 is also checked.
The original 640x480 evidence remains historical; current qualification is in
[evidence/mode19/summary.json](evidence/mode19/summary.json). Its two engines
must agree on all four complete pictures and the application log; relocated
buffers, the legacy family and angle 360 must match the default picture.
The canonical Flexe launcher is regenerated through the shared naming helper.
All new client/profile changes remain uncommitted for review and publication
approval.


Mode 19 qualification passes all 17 Oryx headless scenarios. Both engines
produce identical complete workflow pictures and application logs. There are
3,014,656 exact playfield comparisons and 94 observed complete 49-byte batches
(the cancellation test may stop at a different batch). Default pictures also
match with relocated IDs, the legacy family and angle 360. All captured
workflow text and curves are pure white on black. The mathematical module and
resident renderer bytes remain identical to the earlier accepted renderer.
Host sanitizer/interface checks pass. The final default 34,669-byte executable
is staged with checked runtime identities on the stopped Oryx Flexe profile.
Human mode 19 review and explicit publication approval remain pending.


### Acceptance and publication — 2026-10-15

The Author accepted the mode 19 white-on-black review and explicitly requested
commit and push of all pending work. REV-001 is satisfied. Retrieval of the
saved human log found no controlled Escape exit, so no separate human exit or
MOS keyboard pass is claimed; the 17 passing headless scenarios retain the
independent lifecycle evidence. Earlier pending-publication statements above
are historical and superseded by this approval. Controlled fault injection
and physical qualification remain open.
