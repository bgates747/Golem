# Native VDU submission planning

The accepted backlog is pushed, and a new plan explores letting native modules
submit ordinary drawing bytes to the existing firmware parser. The intent is
to reuse rasterizers while avoiding per-edge resident copy/template machinery.
Only consultation and planning are released; no runtime/compiler change or
physical operation was performed.

Publication groups are Golem b48aae7 (stock matrix), ba1511c (accepted streamed
packages/C++ integration), and agon-utils6467b74 on codex/agnb-plain-buffers
(canonical local d337783). The isolated matrix commit and full compiler worktree
pass maintained checks. Unrelated utility history and changes are preserved.

GOLEM-047 owns consumer/language planning; agon-vdplet VDPL-003 owns the generic
service plan and later ABI/firmware work. The Author's direct instruction
supersedes the earlier discussion hold for questions and planning. Questions
and draft paths were mailed to fsim in design-questions-one and draft-ready-two.
No reply has arrived at this checkpoint; PLN-002 remains open in both tasks.

Source review identifies the current36-byte service table, strict package
ABI1.0 admission/capability restrictions, existing nested buffer stream logic,
timed EOF reads, busy guard and Canvas/frame lifetimes as first feasibility
questions. The actual consumer emits two PLOT commands per edge and uses C3
followed by GP; this existing completion boundary must remain matched in any
comparison. Its176508-byte copy/call stream is installed once, not sent every
frame. The draft distinguishes wire traffic, storage, parser time, raster
completion and presentation, and makes no measured speed or firmware-size claim.


## Completed consumer consultation

A direct desktop prompt activated the existing wireframe session after the
Author asked about wake-up support. The peer returned consultation-three in
canonical message2026-10-23_23-02-00_fsim_fsim-vdu-submit-consultation-three.
Both plans now incorporate the six answers: PLOT4/5 only;32-edge/384-byte initial
scratch; retained19612-byte edge pairs and four-lease phase budgeting;
successful-transform-before-clear ordering; RUN-only/open-frame restrictions;
existing echo/callback review; partial-error drawing drain; active context and
outer buffered stream restoration; and matched C3/GP/effective20ms timing.
Eight peer file hashes and deterministic packed XYZ/edge hashes independently
match; the runtime task silo retains fixture-pins.json. No new functional test,
benchmark, code/runtime/profile change, commit, push or physical action was
performed for this planning continuation. The consumer requirements are
incorporated; ABI choices and implementation release remain open.
