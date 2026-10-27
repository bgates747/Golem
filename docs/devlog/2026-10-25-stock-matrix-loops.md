# Compact stock matrix loops

The maintained stock compiler now uses one resident runtime loop for projection.
Both complete mesh matrix sources install in1,047bytes, with the same32-byte
float input block. Bounded unchanged-stock tests match every unrolled output
byte. Consumer scene/timing/MOS/human gates remain with wireframe; implementation
is uncommitted. Compactness is measured, frame speed is not yet established.

1. Mutual v0.2 contract SHA406ffc96 was committed/pushed as89b9670 and verified
   remotely before any implementation. The sent freeze-three receipt and
   consumer verified-five independently close the Author's explicit start gate.
   Pending GOLEM-049 implementation was excluded from the seven-path freeze.
2. New grammar uses explicitly initialized uint16 counters and inclusive literal
   Range with unit step. Indexed offsets use stride*activeCounter plus optional
   literal bias. All spans/alignment/16-bit addresses and source-work limits
   are proved at compile time; no generic runtime-array/error ABI was added.
3. Lowering installs one mutable body, private counter/cursors and a real stock
   conditional backedge. Every entrance resets private state; final iteration
   returns before any advance. Carry output has dedicated storage. No per-vertex
   host arithmetic/traffic, count-unrolled body or growing call stack remains.
4. Actual stock testing exposed an endpoint issue missed by the initial host
   executor: the conditional reader starts with int32(-1), fills only low16bits
   and mistakesFFFF for its error sentinel. The failed65535endpoint run was
   stopped in its owned process and retained as superseded evidence. Corrected
   zero-extended byte probes avoid the sentinel; host full65536iterations and
   actual65534:65535 now terminate without wrap. Firmware/contract unchanged.
5. Maintained tests and25new loop rejections pass, including1024static-body and
   65536runtime-work boundaries, carries, nonzero/singleton ranges, reset and
   continuation. ASan/UBSan passes. All68legacy package files remain identical;
   two real assembler/load-plan consumers check counter2logical/3storage bytes,
   private ownership/relocation and rejected-output preservation.
6. Actual unchanged-stock2.16 JIT runs compare singleton,30/full2685records and
   subrange3:17 twice, including poisoned counter/cursors. All output bytes match
   the retained unrolled compiler, including retained Z/unvisited records and
   borrowed cleanup preservation. Corrected numerical assertions/evidence and
   zero-fault shutdown stats complete; host exit143 during shutdown has an
   unisolated cause and is not reported as normal exit or MOS/human acceptance.
7. Full peer headers converted read-only under the task both compile to1047
   matrix installationbytes,18resources/twoentries,324loop-body bytes and21state
   plus3counter bytes. Mesh/output data and existing edge drawing are separate.
   Old matrix installations3865/291005bytes remain the matched comparison.
8. Compiler-handoff-four supplies fixed production source/executable pins to
   fsim-wireframe. A tests-only follow-up adds capacity boundaries; final pins
   change only the test-file checksum, with compiler/source bytes unchanged.
   Independent73pose/endpoint/pixel/control/MOS/cleanup/matched timing and stopped
   Oryx staging remain consumer-owned. No peer files/profile were mutated.
9. Only the contract freeze is published. Later code publication still requires
   consumer/human acceptance and direct approval. Native-bulk TEST-007 remains
   plan-only; no native service, binding, firmware or physical operation.

[Task](../tasks/GOLEM-050.md), [report/evidence](../tasks/GOLEM-050/README.md),
[accepted immutable contract](../tasks/GOLEM-050/contract.md).
