# Native line batches: rendering throughput checkpoint

Native line batches make the unchanged Corsair render about 3.5 times faster
in matched emulation. Final pictures match exactly. Profiling identifies queue
and repeated state work as the improvement, with the same line rasterizer and
all 4,903 edges. The Author accepted the production animation and explicitly
approved commit/push on 2026-10-24. Physical speed remains unmeasured.

1. The Author released this tranche after assigning generic runtime profiling
   and native line batches to the VDPlet owner. Portable Golem integration is
   expressly secondary. Plans precede implementation: generic VDPL-004 and
   consumer GOLEM-048. Wireframe retains its application/source ownership.
2. Wireframe consultation-ten confirms the finite endpoint scope and requires
   exact point/relative/history, cursor, pending path, empty/XOR/shared endpoint,
   paint/thickness/clip/origin and lifecycle behavior. Contract-eleven records
   the selected optional ABI1.2/capability8 extension and preserved old tables.
3. The batch method sets graphics state once, pushes both endpoint histories
   and queues copied move/line primitives. Pending path commitment follows
   the first incoming point, matching ordinary PLOT. No new thread, immediate
   mode, simplified geometry or custom pixel kernel. Production code has no
   task implementation dependency or profiling-counter ABI.
4. Host ASan/UBSan and both actual instruction engines pass. Each engine compares
   3,145,728 service/context pixels; separate pending-path/Canvas tests compare
   five cases each. Prior package bytes stay unchanged; actual numeric/VDU
   modules and strict ABI1.1 view pass. Old firmware rejects the new package
   cleanly, then executes the old numeric module.
5. Production full MOS medians: JIT 780→220 ms (24 samples per arm across two
   fresh boots), interpreter 1,390→360 ms (12 per arm). Same geometry, six poses,
   mode20/logical coordinates, HUD, console and C3/GP. All three final playfields
   match across 159,744 pixels each; each arm sends 221 bytes/frame. Cleanup,
   native unload and actual MOS return pass with zero execution faults.
6. Separate task-only firmware counters show queued operations 39,225→10,423,
   retaining 4,903 line executions and essentially equal line-execution cycles.
   With opcode tracing off, queue-admission elapsed cycles fall about156.6M→41.8M.
   The gain survives trace removal; intervals overlap across cores and must
   not be added or converted into a physical speed claim.
7. Production image +624 bytes over5232938; static DRAM unchanged. Combined
   consumer package8,004bytes/text5,800/data1,340; full application has over1,800
   free command-stack bytes. The corrected finite120-frame mode19 animation
   wraps the owner's unchanged angle domain and passes in30.4seconds headlessly.
8. Dedicated Oryx `.emulator/golem-048` uses the production firmware and Flexe
   launcher, leaving accepted GOLEM-047 untouched. The guest supplies the sole
   keyboard instruction. Human acceptance/publication remain pending.

[Task](../tasks/GOLEM-048.md), [results/source/launch guide](../tasks/GOLEM-048/README.md).

## Human acceptance and publication approval

The Author reports "worked. a substantial visual improvement." The saved run
verifies 120 frames, 20 result bytes, PASS log, all runtime pins and actual MOS
`dir` output. The emulator is stopped; execution statistics report zero faults
and the application retains 1,956 free command-stack bytes.
[human-review.json](../tasks/GOLEM-048/human-review.json) preserves hashes and
acceptance. The subsequent instruction "go ahead and commit and push them"
explicitly releases publication. GOLEM-048/REV-001 is complete and removed from
the active register; generic runtime acceptance is recorded under VDPL-004.
No additional implementation, physical run or flash is implied.
