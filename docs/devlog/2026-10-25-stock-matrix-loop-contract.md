# Compact stock matrix loop contract

Golem and wireframe agreed a bounded runtime-loop extension that reduces
resident matrix code without changing projection math or per-frame host
traffic. The Author explicitly requires a scoped contract commit and push
before implementation. This checkpoint publishes the agreed plan only; pending
GOLEM-049 implementation and other project work remain excluded.

1. GOLEM-050 registers the task. Direct Author authority was independently
   verified in the wireframe chat. The consumer proposal requests real stock
   conditional backedges and indexed reads/copies, not build-time unrolling.
2. Contract v0.2 is mutually accepted through mailbox draft-two/accept-four
   at SHA256406ffc969635295448da03f6a7cd7c0815ad092e92ccd67d1c4b08e74b0b694f.
   Its bytes are preserved; acceptance metadata supersedes its draft heading.
   The earlier chat-only v0.1 alert was rejected by a hash guard and is not
   acceptance. GOLEM-050/acceptance.json retains the durable provenance.
3. Scope: declared uint16 loop counters; inclusive positive-unit-step literal
   Range; stride*activeCounter[+bias] typed offsets; deterministic entry reset;
   no one-past update. No nested/body Sub calls or general dynamic expressions.
   Static spans/alignment/overflow prove admission without a runtime-array ABI.
4. At most1024 static body operations and65536primitive body executions per
   lexical loop admit the current2685*7=18795Corsair sequence. Code/helpers/
   private state stay independent of N; data and the existing edge renderer
   remain separately counted. No frame-speed gain is presumed.
5. Golem owns compiler tests and bounded stock qualification. Wireframe owns
   independent exact unrolled/loop endpoints,73pose coverage, whole-frame cost,
   MOS/cleanup and Oryx human staging. Firmware remains unchanged stock2.16.
6. The freeze excludes unreviewed GOLEM-049 source/docs, runtime/emulator work,
   all fsim files and the separately requested native-bulk research. That later
   research is plan-only; no native hot loop/service/binding is implemented.
7. CMP-001/CMP-002 remain blocked at this documentation checkpoint. Start is
   released only after the freeze is committed AND pushed, remote hash verified
   and receipt sent. Later implementation publication retains its own gate.

[Task](../tasks/GOLEM-050.md), [contract](../tasks/GOLEM-050/contract.md),
[acceptance](../tasks/GOLEM-050/acceptance.json).
