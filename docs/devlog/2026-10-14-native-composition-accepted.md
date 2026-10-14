# Native composition accepted

The Author accepted GOLEM-037 and explicitly approved commit/push on 2026-10-14.
The single module runs actual Golem simulation and Rally-owned native rendering;
host oracle/sanitizer/failure controls and Rally actual-target qualification pass.
Rally verified the accepted human run: 900 frames, 3,600 ticks, 2,913 retained
replies and final globals/arrays matching its independent model.

GOLEM-037 ABI-001, LNK-001, POR-001, RES-001, CHK-001/002, VIS-001 and REV-001
are complete for the bounded fixture. Binding CFG/GEN/SDK/PKG/CLK/CHK/REV-001
are complete. The native renderer explicitly rejects buffered selection.
Generic admission policy and firmware builds live in agon-vdplet (VDPL-001);
the narrow emulator patch is retained against an exported commit, without
editing concurrent maintained-emulator work. No physical target was flashed.

Viewing throughput remains below 30 Hz. Background/full-game integration belongs
to Rally. GOLEM-038 scheduler/snapshot/events design, GOLEM-002 capacity stress
and physical qualification remain separate tasks.

SDK implementation checkpoint: agon-vdplet `9fbb21d`.
