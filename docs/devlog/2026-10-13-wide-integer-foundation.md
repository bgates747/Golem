# Wide integer foundation — 2026-10-13

The first GOLEM-036 tranche now executes the same uint32/sint32 source on stock
buffered commands and native VDPlets. Checked addition/subtraction, comparisons,
integer conversions and retained four-byte arrays pass host/stock/actual-Xtensa
qualification. The Author accepted the source-labelled Oryx animation; saved results were
verified and commit/push explicitly approved. GOLEM-035 was committed/pushed
as 93bf1e4 before this work. GOLEM-036 remains active for subsequent numeric work.

1. Same 32-bit packed layout and semantic errors across both targets; no implicit
   widening of narrow intermediates. Old GOLEM-035 executable/native-C/layout files
   remain identical. Source multiplication/division/shifts and wide float casts
   are explicitly excluded from this first tranche.
2. Host 3872 cases, maximum-array offsets through 65528 and direct published Rally
   eight add/sub plus 29 conversion vectors pass. Values outside source 32-bit
   domains/full route/contact/projection replay remain deferred. Detailed evidence
   and resource limits are in [the tranche report](../tasks/GOLEM-036/README.md).
3. Both actual Xtensa engines/default-relocated layouts pass 2311 observations
   each; stock probes/motion also pass.600-frame reviews pass 590 observations/
   5846 bytes,56320 pixels and MOS return. The animation uses station 3.2 million
   and signed Q8±38400, with host drawing/VSYNC only. Ordinary frame wire remains 15
   bytes; installation/diagnostics are separately accounted. No hardware flashed.
4. R25-RENDER v0.3 contract-only acceptance is sent to Rally. Context 464 moves
   scratch out of helper stack; separate completion 48/publication failure rules
   preserve root-v1. Native-only binding spelling is planned, unimplemented; D2
   linking and actual composed resource/stack qualification are not released.

[Task plan](../tasks/GOLEM-036.md) retains stable subitems and review boundaries;
TODO.md owns overall status. Immutable delivery pointer resides beside the report.

GOLEM-036 foundation human review subsequently passed: the Author reported 'good test.' Retrieved results match all 590 observations/5,846 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/evidence/human-review.json. The foundation tranche is accepted; the Author subsequently explicitly approved commit/push. Multiplication/division and remaining GOLEM-036 work are still open.
