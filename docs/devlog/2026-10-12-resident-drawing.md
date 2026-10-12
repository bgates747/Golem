# First resident drawing implementation

The Author accepted GOLEM-011's review scope and authorized implementation.
GOLEM-011 is removed from the authoritative TODO as accepted; GOLEM-018 tracks
the implementation and remaining human review. Points, lines and convex quads
now compile to a standalone resident drawing package with packed typed inputs,
private templates and a longer patch/call entry stream. The Oryx pixel demo is
staged and stopped after passing headless qualification. Human pixel/MOS review
and explicit commit approval remain pending; all changes are uncommitted.

The new resident_drawing.hpp reuses the token reader, checked allocator and
existing byte-emission helpers. --resident selects this profile from uint8,
sint16 or Sub source, without a dummy floor operation. Initializers are explicit,
uint8 colour bounds must fit 0..63, signed coordinates use stock clipping and
matching literals embed directly. Globals share one buffer; assembly exports
their byte offsets and type tags, adding uint8 as tag 5 without changing 1..4.
Convex perimeter A,B,C,D becomes A,B,D,C for the stock triangle sequence.

The qualified demo declares 29 input bytes and draws nine points, a line and
a trapezoid. It installs 775 bytes in 13 owned buffers, invokes in six bytes,
uses 46 bytes for a whole-block update/call and cleans up in 78 bytes. Assembly
still owns loading, display setup, serialized updates/calls, pacing and cleanup.
No new production range/topology admission, observation readback, completion
acknowledgement or custom VDP command is introduced.

Maintained compiler checks pass with 92 rejection cases; ASan/UBSan pass. Twelve
prior scalar/table configurations preserve all six artifacts against 29e269e.
Three real assembler consumers cover default, relocated and near-ID-limit IDs,
packed byte/word exports, wrapper instructions and all payloads. Six publication
failure cases preserve all existing files. Host checks including builds took
52.598 seconds on the local monotonic clock.

Stock Fab 1.2.5/MOS 3.0.2 qualification compares 5,704 resident/direct pixel pairs,
with explicit direct triangles and additional expected interior/exterior colours.
It also checks clipped coordinates, repeated unchanged calls, partial input
updates, persistent storage, immutable entry bytes, cleanup/reinstall and reserved
buffers. The saved result contains 11,451 checks in 12,589 bytes. Initial Pi
execution took 6.323 seconds; after shortening onscreen instructions the final
run took 6.419 seconds. The same staged Oryx fixture passed with identical bytes;
its local duration and runtime hashes are in docs/tasks/GOLEM-018/oryx-review.json.
These timings are not physical-hardware performance claims.

The fixture reads pixels using stock VDU 23,0,132 and MOS's POINT response flag.
Stock sendScreenPixel waits for drawing completion, making it suitable for test
observations; production wrappers still transmit only a call and provide no fence.
The human display ends with dots, a diagonal line and a filled trapezoid, then
prints the dir instruction at the MOS prompt. The Author launches/closes the
Oryx emulator and reports whether it worked; the agent retrieves saved results.
No graphical emulator was launched by the agent. No upstream checkout changed.

The Author subsequently confirmed dir worked and reported cyan drawings at the
far upper left, asking for an expected-shape reference. Retrieved human-run
results match all 11,451 checks/12,589 bytes, and Oryx runtime/fixture identities
still match the staged hashes. human-review.json records this evidence. The
cyan upper-left case intentionally supplies negative coordinates to exercise
clipping; the final restored geometry is yellow. A task-silo layout reference
shows the transient clipping case beside the final drawing, omitting text and
not claiming exact stock edge pixels. README guidance now explains both phases.
No guest code, fixture or deployment changed. Keyboard and numerical review
pass; final visual acceptance and commit approval remain pending.

The Author then confirmed the reference matches what they saw. GOLEM-018 is
accepted: human visual and MOS keyboard review pass, backed by the retrieved
matching results. HUM-001 is complete, the maintained contract reflects the
accepted behavior, and GOLEM-018 is removed from the active TODO. The Author
found the test pattern odd; expected pictures should accompany future visual
reviews so transient clipping cases are recognizable. Explicit commit approval
has not been given, so changes remain uncommitted.
