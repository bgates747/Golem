# Shared scalar frontend and data-layout foundation

GOLEM-024 delivers a task-local typed C++ frontend and state planner as the first
implementation of the shared-source direction. Buffered/native selections emit
identical common artifacts; only their honest requirements descriptors differ.
No executable backend, production compiler change or target qualification is
claimed. The prototype is ready for Author review.

The initialized two-Sub example combines uint16 state with float32 expressions in
a 24-byte packed block. Shared storage is allocated before target-private code;
external reservations, shared ownership, source positions, logical entries and
assembly layout exports are published. Host reference execution checks arithmetic,
admission, Bounds, failed destinations, earlier committed effects and repeated
invocation. It is not execution of either VDP target.

Fresh evidence passes 44 ASan/UBSan checks and 42 rejection cases, eight identical
common artifact pairs at default/relocated IDs, ten CLI rejection cases preserving
outputs, and two real ez80asm consumers checking 33 equates each. Maintained
compiler checks pass, including its existing 132 rejection cases. The receipt
records source/dependency hashes and build/check duration; details are in
[the task report](../tasks/GOLEM-024/README.md).

Requirements, roadmap and earlier research were committed/pushed first at
3e0251e as authorized. This subsequent prototype remains uncommitted for review.
No emulator was launched, firmware flashed or sibling runtime code changed.
The next proposed implementation is an actual checked-uint16 emitter pair using
this shared model and storage, followed by appropriate execution qualification.
