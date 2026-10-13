# Rally/Golem planning contract accepted

Rally accepted all eight clauses of R25-G0 v0.2 without amendments. GOLEM-033
negotiation is complete and removed from TODO. The Author requested task documents
then a stop for discussion/review. No compiler/runtime/firmware implementation,
capacity probe or deployment was authorized or performed by this planning work.

Golem proposal: 2026-10-13_15-06-37_golem_rally25-counterproposal-one.
Rally acceptance: 2026-10-13_15-12-37_rally_rally25-contract-v02-accepted-five.
The reconciled record is docs/tasks/GOLEM-033/accepted-contract.md; the original
proposal and acceptance receipt preserve provenance. Rally will update its own
PROTOTYPE-CONTRACT.md; superseded draft wording does not mean disagreement.

Planned work is decomposed into GOLEM-034 (D0 reproducible delivery/root ABI),
GOLEM-035 (D1a bounded typed arrays/SoA), GOLEM-036 (D1b faithful wide integer
primitives), GOLEM-037 (D2 one-module renderer/helper linking) and GOLEM-038 (D3
firmware scheduler/snapshot/event design with owner/release gates). Each plan
contains scope, dependencies, stable subtask IDs, acceptance criteria and a
separate review boundary. TODO.md is the sole overall execution register.

The plans preserve existing world units, explicit widening and checked narrowing,
signed division truncation/remainder rules, index status 5, single drawing owner,
shared source/storage and honest target requirements. Firmware scheduling and
transport are not compiler features. Sparse snapshot alignment, prediction/stale
handling, recovery policy and firmware owner require explicit decisions before
D3. The configured 8 KiB text budget is not a physical ceiling, and changing it
requires separate evidence and authority. All new plans and existing GOLEM-032
work remain uncommitted pending explicit commit/push approval.
