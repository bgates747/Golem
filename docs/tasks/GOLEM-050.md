# GOLEM-050 — Compact stock matrix loops and indexed buffer access

Replace build-time vertex expansion with a finite runtime VDP loop. The same
matrix operations and endpoint bytes should execute from compact reusable
commands, while the eZ80 still sends controls and one root call per frame.
Reduced installation storage is the objective; a frame-speed gain is not
assumed. Golem owns the compiler; wireframe owns its consumer and independent
acceptance.

The Author's 2026-10-25 wireframe instruction was directly verified: register a
task, negotiate a contract, and begin implementation only after mutual agreement
and a scoped contract freeze **commit and push**. Planning does not release
implementation. This freeze must not publish the separately pending GOLEM-049
implementation, emulator changes or another agent's files.

- CON-001 [x] Intake the consumer proposal, verify direct Author authority and freeze baseline/source/fixture hashes without editing the consumer.
- CON-002 [x] Agree the complete revision of GOLEM-050/contract.md with fsim-wireframe through the canonical mailbox; retain acceptance and exact content hash.
- FRZ-001 [ ] Commit and push only this task, its accepted contract/baseline and its task-register entry. Verify the remote freeze and send its receipt before starting implementation.
- CMP-001 [ ] Add the agreed explicitly typed For/Range and bounded index-offset grammar, preserving portable semantics and old stock packages.
- CMP-002 [ ] Emit a reusable body, private bounded iterator/cursor state, offset patching and a stock conditional backedge; no source-count unrolling or growing call stack.
- CHK-001 [ ] Check compile-time bounds/alignment/overflow, rejected syntax, first/last records, single/repeated calls, zero-count rejection and uint16 endpoint termination.
- CHK-002 [ ] Verify maintained regressions, legacy output parity, sanitizers, assembly/load-plan sizes and ID relocation/reservation/cleanup.
- TGT-001 [ ] Qualify the supported compact sequence on unchanged stock2.16 and hand off pinned compiler/packages for independent heavy-tank/Corsair reference/pixel/application acceptance.
- BEN-001 [ ] Record code/helper/state/data/whole-package sizes and matched completed-frame measurements versus the retained unrolled baseline; do not presume a speed gain.
- REV-001 [ ] Reconcile owner evidence, Oryx human review and direct implementation-publication approval; leave affected implementation uncommitted until its gate passes.

## Dependencies and ownership

The mutually accepted v0.2 [contract](GOLEM-050/contract.md) owns the proposed language and runtime
semantics, supported bounds, acceptance and exclusions. [baseline.json](GOLEM-050/baseline.json)
records the current HEAD, locally qualified but unpublished GOLEM-049 compiler
pins and immutable shared comparison input. GOLEM-049's numerical/host-independent
VDP and full MOS consumer gates have passed; its human/publication gates remain
separate. A documentation-only GOLEM-050 freeze does not close or publish it.

Wireframe proposal: mailbox request stock-matrix-loop-contract-one, thread
fsim-stock-matrix-loops, branch contract-v1, recipient fsim-wireframe. Existing
GOLEM-049 projection/rounding/domain and host ownership remain the baseline.
No private compiler fork, native code, firmware service, GUI launch or physical
operation is authorized by this task. Future native-language portability is
outside this bounded stock profile extension.

## Contract acceptance checkpoint — 2026-10-25

Fsim-wireframe accept-four explicitly accepts v0.2 at
406ffc969635295448da03f6a7cd7c0815ad092e92ccd67d1c4b08e74b0b694f.
The contract bytes remain unchanged; its draft status is superseded by
[acceptance.json](GOLEM-050/acceptance.json). The earlier v0.1 chat alert did
not publish a durable acceptance. The65536-operation per-loop cap explicitly
admits2685*7=18795iterations-of-body-operations. This accepted documentation
checkpoint still requires actual freeze commit/push verification and a sent
receipt before CMP-001/CMP-002 may begin.
