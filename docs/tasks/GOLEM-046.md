# GOLEM-046 — Streamed AGNB program packages

Use generic AGNB plain-buffer loading so generated Golem programs need not be
embedded wholesale in eZ80 application RAM. AGNB owns format/reader/writer and
bounded transfer; Golem owns compiled layout, named APIs and bootstrap behavior.

Author approved this separation and assigned generic AGNB ownership on2026-10-23.
Generic dependency: agon-utils/examples/agnb/docs/tasks/AGNB-001.md.

- SEP-001 [x] Record cross-project ownership, version and dependency boundary; preserve ongoing GOLEM-045 and unrelated agon-utils work.
- PKG-001 [ ] Emit explicit compiler-assigned buffer records from structured package data, using the generic AGNB writer. Keep symbol/header/manifest IDs derived from the same layout, with no application-source numeric bookkeeping.
- BOOT-001 [ ] Separate uploadable ordinary buffers from post-load setup commands such as typed matrix creation, native module admission/fresh-handle binding and activation. Never infer a general installer by stripping arbitrary VDU bytes. Retain existing embedded output as an option.
- HOST-001 [ ] Supply generated assembly/C integration and a small reusable host-side loading adapter around the generic loader. Host owns file I/O, successful completion and entry activation; stock VDP never asks MOS to load files.
- CHK-001 [ ] Compare package contents/layout/setup behavior to accepted embedded output; prove bounded host memory and loading of aggregate packages larger than application RAM, using actual stock and applicable native target checks.
- REV-001 [ ] Stage applicable stopped Oryx review, obtain human acceptance where required, document and publish only authorized owned changes.

Ordinary command/data records contain opaque bytes, not implicit executable
entries. Buffer/block boundaries are compiler contracts; file read windows do
not define them. Matrices may require API creation rather than raw64-byte
uploads. Native module image loading still obtains a fresh runtime handle;
container IDs cannot encode that dynamic handle. Loading does not change
runtime wire costs of the resident algorithm. Template reuse is separate work.


Generic first tranche is implemented in agon-utils: provisional0.3 plain
BHDR/RAWD/DATA records, shared RIFF framing, explicit-ID writer/reader/manifest
CLI, additive agnb_load_buffers/callback runtime entry points. Host tools and
formal task published as3edf8bc on codex/agnb-plain-buffers; canonical local
record is e6f1bcc. Divergent unrelated agon-utils history was preserved by
publishing a cherry-pick on a clean upstream worktree. Actual stock Pi loading
of an aggregate package above0x70000 passes; runtime/profile publication is
held for review. Golem producer/bootstrap integration is not implemented yet.


### Generic dependency accepted — 2026-10-23

The Author reports AGNB's Oryx stock-Fab review worked. Agent retrieval matches
all66 expected result bytes and pinned runtime identities. Generic AGNB-001 is
complete and published on agon-utils codex/agnb-plain-buffers: host tools3edf8bc,
qualified loader35dd8ef (canonical local records e6f1bcc/24596fc). The existing
image/audio behavior is preserved. Generic dependency entry predicate is now
satisfied; PKG-001/BOOT-001/HOST-001 remain Golem-owned implementation work.
No current Golem package silently acquires AGNB output from this dependency.
