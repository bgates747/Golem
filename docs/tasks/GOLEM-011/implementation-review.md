# REV-001 — First pixel implementation recommendation

Implement draw_point, draw_line and fill_quad as a small standalone resident
drawing profile, with packed typed inputs and the existing named assembly-call
interface. Stage a visible demonstration in the dedicated Oryx emulator after
host and stock-runtime qualification. This is the recommended implementation
scope for Author review; no compiler or emulator changes have been made by
REV-001. The syntax is approved, and the wire review explains both savings and
cases where direct commands remain smaller.

## Scope for the next implementation tranche

| ID | Include | Contract |
| --- | --- | --- |
| GEO-001 | One initialized set of uint8/sint16 globals and one finite Sub | Explicit declarations and initializers. uint8 colour bounds fit 0..63; coordinates are signed words. Accept matching literals as well as declared operands. Keywords/builtins remain case-insensitive. No statement terminators. |
| GEO-002 | draw_point(colour,x,y) | Set foreground GCOL 0,colour; absolute foreground point. |
| GEO-003 | draw_line(colour,x1,y1,x2,y2) | Set foreground GCOL 0,colour; move to the first endpoint and draw the solid line to the second. |
| GEO-004 | fill_quad(colour,x1,y1,x2,y2,x3,y3,x4,y4) | Caller supplies a convex perimeter. Map A,B,C,D to A,B,D,C for the two stock triangles; retain stock edge coverage. |
| GEO-005 | --resident --emit-asm package and named call | One packed input buffer, offsets/widths/type exports, initialized install stream, six-byte invocation, example update/call, cleanup and manifest. Keep prior scalar/table artifacts compatible. A uint8 export needs a distinct type tag; retain existing tags 1..4. |
| GEO-006 | Repeated/partial update demonstration | Assembly installs once, invokes by name, changes selected input bytes and invokes again. Resident inputs persist. Print instructions onscreen and return to a responsive MOS prompt. |

This is geometry support, without requiring a dummy float/floor or table job.
Do not imply general assignments, expressions, loops, functions or composition
with the existing scalar/table profiles. Their broader language designs remain
separate. General plot, bitmaps/affine transforms and viewport/scroll examples
remain approved directions for later slices.

## Compiler and host boundary

Reuse the current token reader, checked ID allocator, buffer emission helpers
and assembly renderer. Add a small drawing frontend/backend rather than import
Rally's whole hosted language. Rally's patch/call mechanisms and verified stock
commands are useful references; Golem keeps its accepted surface syntax.
Profile selection must be unambiguous, and unsupported mixed programs must fail
before package output is mutated. No additional CLI syntax is established by
this review.

Pack globals together and export their actual offsets, including byte/word
fields; assembly can update either a field or a contiguous range. Emit finite
patch/call sequences together in a longer command stream, with private mutable
drawing templates. Literal operands can be embedded directly. No optimizer,
compression scheme, software stack or one-buffer-per-instruction model is needed.

Assembly owns display/palette setup, pixel coordinates, origin (0,0), viewport,
installation, updates, invocation, pacing and cleanup. Compile-time validation
checks source types, literal ranges and declared colour bounds. Later host writes
must honor that contract; do not add a new runtime colour/topology error path.
Signed off-screen coordinates use stock clipping. Calls remain serialized and
nonreentrant, without a new acknowledgement, frame fence or copy readback.

The wire benefit is resident reuse: a call is six bytes, and updating N contiguous
bytes then calling costs 17+N. Actual installation and invocation lengths must
come from the implemented package, not WIR-001's illustrative 423-byte layout.
Preserve the option to issue direct VDU commands from assembly when appropriate.

## Qualification and the visible result

| ID | Evidence required for the implementation | Purpose |
| --- | --- | --- |
| VAL-001 | Maintained compiler checks, warnings/sanitizers, relevant rejection cases and previous scalar/table artifact parity | Catch malformed templates, type/range errors, allocator/export collisions and regressions. |
| VAL-002 | Real assembler consumer using generated install data and call golem_<SubName> | Verify the intended application interface rather than a handwritten replacement call packet. |
| VAL-003 | Stock-runtime drawing comparison with direct VDU geometry | Check point/line placement, convex quad ordering and edge coverage, including representative signed clipping. Reuse existing local capture tooling where available. |
| VAL-004 | Repeat invocation, partial input update, cleanup/reinstall and reserved-buffer preservation | Confirm persistent state, exported offsets, actual wire lengths and ownership without production readback logic. |
| VAL-005 | Staged Oryx human review | Show a visible group of points, a line and a filled trapezoid, with clear onscreen expectations. Return to MOS and print the instruction to type dir. The Author closes the emulator and reports whether it worked; the agent retrieves saved observations afterward. |

Automated observations and any pixel capture belong to the task fixture, not the
production hot loop. The agent leaves the Oryx emulator stopped and supplies the
profile-local launch commands once deployment is verified. The Author does not
need a second terminal or a separate verifier. Emulator-related changes remain
uncommitted until human validation and explicit commit approval, per the
canonical workflow.

REV-001 is complete. Recommended next action: approve this geometry scope, then
implement and qualify it as the next tranche. No intervening design subtask is
needed. Other drawing families remain in this review's coverage for follow-on
implementation, without a new actionable checklist outside TODO.md.
