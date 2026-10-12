# Resident drawing wire contract

GOLEM-011 / WIR-001 is complete after the Author authorized the focused traffic
review. Resident drawing earns wire savings through reused inputs and grouped
operations; uploading every operand for one point, line or quad is larger than
direct stock commands. Recommend the first pixel implementation use these three
geometry operations, packed input storage and the existing named assembly call
contract. Stop for review; no drawing compiler support or emulator was changed.

The task-silo drawing-wire-contract.md records exact packet accounting, all five
Rally graphics families, assembly ownership and a small illustrative install.
wire-counts.json records arithmetic with primary source hashes. A call is six
bytes; one contiguous N-byte input update plus call costs 17+N. Copies/patches
inside the installed program add installation bytes, not recurring wire bytes.

For an illustrative point/line/quad job sharing one colour and otherwise using
independent coordinates, direct commands need 45 bytes with shared GCOL. Fully
refreshing 29 input bytes and invoking needs 46; reusing inputs needs six.
A straightforward five-buffer layout projects 423 installation bytes, amortized
after 11 unchanged executions, or 12 including cleanup. These are transparent
source-derived projections, not measured compiler output or runtime performance.

The proposed first geometry contract trusts assembly to honor declared colour
bounds and supply convex perimeter ordering, uses signed coordinates with stock
clipping, and introduces no readback, new acknowledgement, frame fence or custom
VDP command. Assembly owns finite serialized updates/calls and pacing. Later
bitmap/viewport/scroll admission decisions remain for their implementation review.

Packet lengths and formulas were checked against current source and the local
primary API reference; arithmetic was evaluated with the project-local Python.
No compiler tests or emulator runs were needed for this documentation tranche.
Existing source examples and upstream checkouts remain unchanged. WIR-001 is
checked complete; REV-001 remains next. Changes are uncommitted.
