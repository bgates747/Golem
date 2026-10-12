# Rally inventory reconciliation — GOLEM-009 / INV-001

The Author authorized committing/pushing the task queue, then executing one
subtask and stopping for review. Queue checkpoint `1affbc5` was pushed.
INV-001 is complete; its inventory awaits review, and no later subtask has begun.

`docs/tasks/GOLEM-009/inventory.md` reconciles GOLEM-001's historical comparison
with the maintained compiler after GOLEM-008. It distinguishes implemented
support, existing proposed syntax and remaining capabilities. It preserves
comparators/conditionals as the design baseline and records GOLEM-010/011
deferrals for assets/indexing/checked copying and typed drawing.

All 44 Rally statement operations map to inventory entries. A read-only audit
matches all 17 sources hashed in Rally's historical 14-suite host report.
`inventory-audit.json` records hashes and the duration of that validation only.
This did not rerun the test suites, emulator or hardware, change examples or
compiler code, or select new syntax. INV-002's detailed example coverage audit
and subsequent additions remain pending Author review/instruction.
