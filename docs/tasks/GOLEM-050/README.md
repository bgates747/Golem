# Compact stock matrix loops

The compiler now installs one runtime loop instead of expanding the vertex
sequence. Full heavy-tank and Corsair matrix sources both install in1,047bytes;
the previous matrix-only installations were3,865/291,005bytes. Their geometry
and drawing storage remain separate. Bounded stock tests match every unrolled
output byte; independent application/timing/human review is with wireframe.
Implementation is uncommitted pending that review and direct publication approval.

1. The [accepted v0.2 contract](contract.md) is immutable at SHA256
   `406ffc969635295448da03f6a7cd7c0815ad092e92ccd67d1c4b08e74b0b694f`.
   Freeze89b9670 was committed/pushed and independently verified BEFORE code
   changes. [freeze.json](freeze.json), [acceptance.json](acceptance.json) and
   [baseline.json](baseline.json) retain authority and the sent release receipt.
2. [example.golem](example.golem) declares every required type and initial state.
   The host installs its exact borrowed single blocks as described in the
   contract. Compile with:

   ```bash
   make -C src check
   build/golemc --stock-matrix --emit-load-plan --reserve 1000:1003 \
     docs/tasks/GOLEM-050/example.golem docs/tasks/GOLEM-050/build/example
   ```

3. For/Range is inclusive, positive unit step, literal bounded and uses an
   explicitly initialized uint16 counter. Typed offsets use stride*activeCounter
   plus optional literal bias. Bounds/alignment are checked at compile time;
   counters/cursors reset on entrance and never advance past the last iteration.
   The host still updates only its float controls and calls one root per frame.
4. The emitted body has a stock conditional backedge. Private counter storage
   reserves command5's carry byte. Termination compares zero-extended byte words
   to avoid the firmware's0xffff/-1conditional sentinel. The first actual-stock
   endpoint attempt exposed that bug and was stopped; the corrected algorithm
   passes the65535endpoint. No firmware or contract change was made.
5. [cli-evidence.json](cli-evidence.json) records two real assembler consumers,
   logical/storage field sizes, reservations/relocation, rejected-output
   preservation,68unchanged legacy files and source-only conversions of both
   peer mesh headers. Their32-byte/8float control block stays unchanged.
6. The complete matrix sources each own18resources/two named entries. The
   seven-operation loop body is324bytes; cursor/bound/scratch data is21bytes and
   counter storage3bytes. These code/state sizes are independent of vertexcount.
   Count-dependent geometry/output data and the existing edge renderer are
   excluded from that constant-footprint claim and must be included in whole
   application totals. Compactness is not a frame-speed measurement.
7. [stock-evidence.json](stock-evidence.json) retains unchanged-stock2.16 JIT
   results for singleton,30/full2685records and subrange3:17. Each runs twice,
   including poisoned counter/cursor state. All output bytes equal the unrolled
   baseline, retained Z/unvisited records/guards and borrowed cleanup pass.
   Numerical assertions/evidence and zero-fault shutdown stats complete, but the
   host exits143 during shutdown; its cause is unisolated. This does not claim
   normal host exit, full MOS/human acceptance or a physical result.
8. Host tests execute the encoded command stream: full2685copies/repeat,
   65536iterations, nonzero/singleton ranges, carries, continuation, source-body
   versus runtime-work caps and25rejection cases. Maintained/sanitizer logs stay
   ignored under build/. Production tests/compilation have no task dependency.
9. `check_cli.py` reproduces metadata/assembler/parity/source checks.
   `qualify_stock.py` uses a hashed immutable task-local copy of the existing
   VDP-only transport, pinned by [target-inputs.json](target-inputs.json).
   `compiler-pins.json` records the delivered executable/source checkpoint;
   `compiler-pins-final.json` updates only the later capacity-test checksum.
10. Wireframe owns the agreed independent full scene/reference/pixel/MOS,
    all73shared-v2 poses, matched completed-frame timings and stopped Oryx visual
    staging. No peer source/profile was mutated by this tranche. The separate
    native-bulk research remains plan-only; no native code or runtime change.
