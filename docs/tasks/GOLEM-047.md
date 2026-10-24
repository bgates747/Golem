# GOLEM-047 — Native drawing through submitted VDU bytes

Keep approachable drawing operations in the source language while letting a
native backend generate ordinary VDU bytes locally and submit them to existing
firmware drawing routines. The stock path remains available. The first
dependency is a correct generic runtime submission service; this is a planning
task, not an implemented language feature or a promised performance gain.

The Author released consultation and planning on2026-10-23, after publication
of the earlier matrix/AGNB work. The prior fsim discussion hold is lifted for
this purpose. No new compiler, SDK or firmware implementation is authorized by
this planning instruction.

## Separation of concerns

Generic service planning, ABI/versioning, interpreter integration, size and
runtime qualification are owned by
[VDPL-003](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-003.md). Its authoritative
register is agon-vdplet/TODO.md; do not duplicate its implementation checklist
here. Fsim owns its wireframe application, fixtures and independent review.

This task owns the eventual language/backend boundary. Preserve the accepted
[backend portability requirements](../design/backend-portability.md): shared
typed source, evaluation order, state/layout, entry/error semantics and explicit
host-controlled backend selection. A VDPlet still requires enabled firmware;
the stock buffered path must remain usable without it. Do not introduce an
application-source Native drawing dialect, automatic flashing or automatic
replay after partial drawing.

## Consumer work and acceptance

- PLN-001 [x] Open the generic runtime task, link ownership and send wireframe questions about commands, fixtures, chunking, context and completion.
- PLN-002 [x] Incorporated consultation-three in VDPL-003 and the requirements below. Fixture hashes independently match unchanged files/data; no API acceptance or implementation release is inferred.
- SEM-001 [ ] After the service contract is agreed, select a small drawing slice that can retain the same source operation and visible effects on both backends. Existing resident drawing syntax is a reference, not evidence it is already accepted by the portable frontend.
- SEM-002 [ ] Specify operand typing, conversions, coordinate units, graphics-state effects, earlier drawing on failure and completion. Treat bitmap/asset lifetimes as separate from simple PLOT coordinates; do not infer full drawing coverage from line success.
- SEM-003 [ ] Publish retained edge-index storage/bindings for the native consumer without changing the source geometry. Keep transform/status admission before clear/labels/draw, so transform failure preserves the previous frame. Fit the draw phase's root/argument/projected-vertex/edge leases within the existing four-lease budget; do not silently request a fifth lease or embed a large edge table in module data.
- CMP-001 [ ] After explicit implementation release, add the supported operation through shared frontend/typed representation and target-specific emission. Stock uses existing resident command mechanisms; native emits complete-command chunks and the negotiated service callback. Keep buffer IDs/layout/entries generated and app-owned installation/pacing/cleanup explicit.
- CHK-001 [ ] Compare the same source, initialized state and graphics context on stock/native backends, including clipping, chunk boundaries, unsupported capabilities and partial-failure behavior. Preserve existing package bytes when the new facility is unused.
- REV-001 [ ] Stage an honest visual review showing the actual source program, source roles and expected scene in the same tranche as qualification. Record retrieved acceptance and applicable publication authority before publishing coupled runtime/profile changes.

No compiler optimizer, buffer-lease optimization, new drawing thread, broad
native primitive API or full wireframe rewrite belongs to this tranche. The
wireframe workload is a useful independent service consumer and benchmark, not
permission to move its application code into Golem.

## Coordination checkpoint

The owned backlog is published: stock matrix compiler `b48aae7`, accepted
streamed packages/C++ host `ba1511c`; generic AGNB C++ reader `6467b74` on
agon-utils `codex/agnb-plain-buffers` (canonical local `d337783`). Both Golem
AGNB integration reviews retain95-byte result receipts; the stock matrix profile's
human visual review remains fsim-owned and is not claimed by publication.

Wireframe questions were sent in canonical mailbox message
`2026-10-23_22-12-16_golem_golem-vdu-submit-design-questions-one`. The current
plan remains provisional at the ABI/implementation boundary. Later task artifacts belong under
`docs/tasks/GOLEM-047/` if multiple files are required.


### Direct session consultation

The Author asked why wireframe had not been awakened. A direct desktop prompt
to its existing thread01a16a53-6d76-72a0-94eb-51a7abded231 was delivered, and
read_thread confirms an active turn reviewing the mailbox questions and both
plans. The earlier mailbox-only wait was insufficient to activate an idle
thread. Peer feedback incorporation is recorded below; no new implementation
was requested.


### Owner consultation incorporated

The final reply is canonical mailbox message
`2026-10-23_23-02-00_fsim_fsim-vdu-submit-consultation-three`. It confirms the
bounded raw-span investigation and separate runtime/language/application
ownership. The generic plan now incorporates all six answers:

- First service: complete PLOT4/5 commands only, RUN-only, no open frame
  transaction, no outer input/event processing; ordinary buffer leases remain
  usable. Existing echo/callback state needs an explicit safe contract.
- Initial consumer target:32 edges /384bytes per chunk (154 calls, final7edges),
  retained19612-byte edge table and draw-phase argument/root/vertex/edge leases
  within the existing four-lease allowance. Verify combined native/parser stack.
- Transform/status success precedes clear/white colour/HUD/native draw/C3+GP.
  A separate drawing entry is the first comparison candidate. Preserve active
  graphics history and the outer resident-wrapper stream. Partial draw failure
  requires a drawing drain before cleanup, with no replay or pixel rollback.
- Freeze the existing C++ transform, ordered geometry, mode20 logical coordinates,
  poses, raster/fence boundaries and raw timing samples in both drawing arms.
  Generation can overlap queued drawing. Distinguish effective20ms MOS timing
  resolution from nominal centisecond units and from physical measurements.
- Fixture hashes and deterministic XYZ/edge bytes were independently checked;
  receipt: agon-vdplet/docs/tasks/VDPL-003/fixture-pins.json. Existing reference
  summaries are peer qualification; deleted temporary images are not retained
  golden binaries and must be regenerated in a later released tranche.

No callback number, capability bit, global length ceiling or ABI version was
selected by the peer. Native/stock language adoption preserves signed16
conversions, units, effects/order and application-owned lifecycle; the current
task-local renderer is not an implemented portable frontend feature.


## Implementation release — 2026-10-23

The Author approved publication of the pending plans, then released the largest
implementation/qualification tranche possible before further human attention.
Wireframe consultation may be awakened directly, with substantive exchange in
the durable mailbox. Generic service correctness is the deliverable; mode20,
edge count/table and scratch sizing are the initial consumer fixture, not global
application restrictions. No physical flash or automatic graphical launch is
released. New emulator-coupled implementation remains uncommitted for review.
