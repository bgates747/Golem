# GOLEM-048 — VDPlet native line-batch consumer qualification

Use the unchanged Corsair transform and geometry to qualify faster native
drawing. This is VDPlet runtime work with an isolated C++ consumer, independent
of the deferred portable Golem drawing frontend. Generic implementation and
phase instrumentation belong to agon-vdplet VDPL-004.

Author release: 2026-10-24, "ok, go". Baseline is accepted GOLEM-047/VDPL-003
(consumer2640a70, runtime5232938). Preserve those source/profile/evidence files.

- PLN-001 [x] Freeze the same 2685 vertices/4903 ordered edges, C++ transform, mode20 benchmark and completion boundary; open generic task and durable owner consultation.
- CON-001 [x] Build a separate worker entry using native line batches, sharing the original transform and root/error/lease/edge bindings. Preserve transform admission before clear.
- PRF-001 [x] Drive task-only VDP profiling with console tracing on/off; read counters after drawing drains. Record generation, queue and raster observations without claiming nonoverlapping phases.
- CHK-001 [x] Compare old submitted-byte versus line-batch drawing under both engines, fixed modes/poses/clip/colour/thickness/history and exact pixels; test error cleanup and outer continuation.
- BEN-001 [x] Run uninstrumented matched full MOS comparisons with order reversal and identical wire/fences; retain raw results, pins and clock limitations.
- REV-001 [x] Stage a finite clearly labeled native-batch Corsair animation on Oryx, retrieve saved results and record human acceptance. The Author accepted the visual/keyboard review and explicitly approved commit/push on 2026-10-24; see GOLEM-048/human-review.json.

Task artifacts live in GOLEM-048/. No edits to the wireframe owner's source,
assets, application or existing review profile. No graphical launch or flash.


## Executable checkpoint

[Results, source and review commands](GOLEM-048/README.md) are ready. Matched
production medians are 780→220ms JIT and1,390→360ms interpreter, with identical
final playfields and221wirebytes/frame. Diagnostic counters identify queue/state
reduction while retaining all4903line executions. Both engines pass functional
and pending-path/point-history checks. Production firmware grows624bytes with
unchanged static DRAM; combined module8004bytes.

The120-frame mode19 human fixture passes its complete headless run in30.4seconds
and is staged/stopped on Oryx. REV-001 remains open for human visual/keyboard
acceptance and explicit publication. No graphical launch or physical flash.


## Human acceptance and publication approval — 2026-10-24

The Author reports "worked. a substantial visual improvement." The agent
retrieved and verified all 120 frames, 20 saved bytes, the PASS log, runtime
pins and actual MOS directory listing; the emulator is stopped with no
execution faults. [Receipt](GOLEM-048/human-review.json). The Author then
explicitly instructed "go ahead and commit and push them." REV-001 is complete
and GOLEM-048 is removed from the active register. Earlier pending statements
above describe the preceding checkpoint. No physical qualification or flash.
