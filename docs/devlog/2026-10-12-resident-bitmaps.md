# GOLEM-019 — Familiar Wolf assets and plain bitmap compiler

The Author requested recognizable AgonWolf3D bitmaps, then authorized the next
subtask. AST-001 selects four existing images at native size; BMP-001 implements
plain resident draw_bitmap with named assembly exports. Host/sanitizer and real
assembler checks pass. Stock bitmap qualification is the next review tranche;
no new emulator fixture has been staged or launched.

The task silo contains unchanged RGBA2 payloads and a subset AGNB retaining the
original Wolf LIST BUFR records and IDs. The reference decodes these exact pixels:
320×160 splash above a 32×32 BJ face, 64×64 pistol frame and 40×20 inventory pistol.
AgonWolf3D remains unchanged. Source paths, dimensions and hashes are recorded
in assets.json, and prepare_assets.py reproduces the selection.

The drawing compiler accepts initialized uint16 globals as well as existing
uint8/sint16 fields. First-Sub token inspection disambiguates drawing sources
starting with uint16 from the existing scalar backend. Full-ID bitmap selection
uses VDU 23,27,32; signed placement words are patched at native bitmap coordinates.
The 24-byte private template disables any prior affine transform before drawing
and disables it afterward. Bitmap bootstrap enables the stock affine feature:
vdu_sys.h's transform selector otherwise returns without consuming operands.
Geometry-only bootstrap output is unchanged.

Literal/initial IDs require explicit build reservations before allocation.
Dynamic IDs remain host-trusted: numeric Bounds and reserved/loaded asset
membership are distinct requirements. No runtime checks, acknowledgements or
pixel uploads are added to the drawing loop. The manifest publishes the initial
asset IDs; uint16 assembly fields retain type tag 1. Compiler installation and
cleanup exclude borrowed assets, whose load/register/lifetime remains assembly's
responsibility.

The four-image example has six input bytes, a 69-byte entry, four 24-byte bitmap
templates and six owned buffers. Bootstrap is 262 bytes, repeated invocation six,
full-input update/call 23 and cleanup 36. The 57,336-byte AGNB transfer occurs once
outside those recurring counts.

check_bitmap.py passed in 48.364 seconds on this development host. Maintained
contracts cover full-word ID/coordinate patches, literal limits, types, names,
missing reservations and unsupported arguments. Address/undefined sanitizers
pass. Both default and relocated consumers assemble with real ez80asm and verify
field exports, wrapper instructions and embedded payloads. All twelve earlier
geometry artifacts remain byte-for-byte identical; all six prior scalar/table
CLI examples select their original backend. Three publication failures preserve
existing artifacts. bitmap-host-checks.json records source hashes and durations.

BMP-001 is complete and awaiting review. CHK-001 remains pending for stock bitmap
pixels/transparency, persistent updates and borrowed-asset preservation. Previous
uncommitted work is preserved; this turn does not commit, push or launch a GUI.

The Author then authorized CHK-001. A task-silo assembly guest now loads the
57,336-byte AGNB once using an unchanged snapshot of the canonical agon-utils
API. Its local dependencies retain the canonical helper contracts with exact
word writes, and the edited guest/support ADL audit reports 12 wide accesses,
all backed by 24-bit storage, with no definite or unresolved hazards.
The real 127,483-byte executable fits the physical 256 KiB user RAM window.

Stock Fab 1.2.5/MOS 3.0.2 qualification passes 17,663 checks and 18,006 saved
bytes, including 17,613 pixel observations and 5,152 direct/resident pixel
pairs. Default and relocated resident packages execute 15 jobs in total,
covering unchanged repeat, high-byte bitmap-ID changes, position-only updates,
signed negative clipping, binary transparency over grey and prior affine state.
A translated BJ pixel is first checked beyond its ordinary 32-pixel width,
proving the stock transform is active before Golem restores native placement.
Bootstrap is also exercised with affine support initially disabled.

Every resident pixel is checked against independently composed Wolf RGBA2
pixels; the fixture sets an identity palette so logical pixel replies are
physical RGB222. Initial, clipped and grey-background cases also execute direct
stock bitmap commands for comparison. All registered assets still draw after
Golem cleanup; their prefix/suffix samples and unrelated canaries survive, and
reinstallation restores declared inputs. No copy checks or stock pixel queries
are inserted into production programs.

The first headless run took 17.976 seconds. Adding the two active-transform
controls justified one final qualification run, which passed in 11.817 seconds.
Execution timing varies and is not a performance claim. Runtime/module hashes
match the accepted geometry runtime; the exact profile-local launcher runs with
dummy SDL and the original autoexec is restored. expected.dat freezes verified
results; emulator-checks.json records source/runtime identities and observations.
The compiler itself is unchanged since the passing host/sanitizer checks.

CHK-001 is complete and awaiting review. ORY-001, staging the dedicated Oryx
review and its final onscreen presentation, is next. No graphical emulator was
launched and Oryx has not been changed by this subtask. Changes remain uncommitted.

The Author corrected that review boundary: the visual test is part of their
review, so implementation, qualification and Oryx staging must be delivered
together. ORY-001 was completed directly. The final review.golem centres the
same native-size Wolf images on a 512×384 display, preserving a lower text
viewport for the single dir instruction and MOS output. The code verifies the
final centred pixels as well as the original stock qualification cases.

The updated local run passes 18,526 checks/18,869 bytes in 7.299 seconds. The
guest is 132,210 bytes; the local assembly width audit still reports no hazards.
The unchanged AGNB, exact guest and frozen observations were copied to the
dedicated Oryx profile. Remote headless execution matches every observation and
the pinned runtime/module identity. The emulator is stopped and headless results
removed for a fresh human run. README now supplies the expected picture and
single-terminal launch commands; Oryx evidence is in oryx-review.json.
HUM-001 is pending. No graphical emulator was launched, committed or pushed.

On the Author's subsequent status request, the agent found the Oryx emulator
closed and a fresh results.dat present. Retrieved human-results.dat matches all
18,526 checks/18,869 bytes, with unchanged runtime and staged fixture identities.
human-review.json records the numerical pass. The Author has not yet explicitly
confirmed both the visual appearance and MOS dir response; HUM-001 remains open.

The Author confirmed the images looked fine and asked why the display did more
drawing than review.golem's four calls. Visual confirmation is recorded as passed.
The additional scenes are test-tape operations in qualify.py, executed by
guest.asm: parameter changes, clipping/transparency/transform checks and direct
stock comparisons. review.golem describes the final four-image drawing only;
the assembly test application also loads assets and runs/saves diagnostics.

The Author objected that the combined run did not distinguish Golem drawing from
the surrounding test. This was a presentation error: a human could not tell
which scenes came from their short Golem source. The replacement human demo
calls review.golem exactly once. All four bitmap draws are compiled Golem code;
onscreen labels identify that source and assembly's setup/loading role. Direct
stock comparisons and intermediate scenes remain only in qualify.py/guest.asm's
automated run. The existing human-result evidence is retained as superseded
presentation evidence, not discarded.

prepare_demo.py builds the single-call tape and demo_guest.asm, verifies 864
expected pixels and stages that exact binary through prepare_oryx.py. The first
attempt printed labels before drawing, exposing stock Canvas text clipping;
the resulting pixel mismatch was caught headlessly. Labels now print after
drawing/observation, so they cannot change bitmap clipping. The corrected local
run passes in 1.849 seconds. Staging refuses a missing or changed qualified
binary. The local assembly width audit still reports no hazards. The full
automated qualification evidence and compiler sources remain unchanged.

The corrected single-scene demo is staged and headlessly verified on Oryx, left
stopped with fresh human results removed. The usual launch commands now run this
labelled Golem demo. Human review of the revised presentation is pending.

The Author confirmed the corrected static pictures are right, then requested
animation and suggested vblank as the clock. GOLEM-020 implements that request
with the existing drawing backend: assembly owns position arithmetic and vblank
pacing; Golem visibly owns frame clearing and all bitmap drawing. No keyboard
confirmation or commit approval is inferred from acceptance of the pictures.

GOLEM-019 is closed after the Author's successful GOLEM-020 animation/MOS review.
Corrected static visuals and 864 saved bytes had already passed; the separate
static keyboard check is superseded by that later accepted review. It is removed
from TODO without claiming an additional static keyboard test. Commit approval
remains pending.
