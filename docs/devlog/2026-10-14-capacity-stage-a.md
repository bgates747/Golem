# Capacity Stage A — findings returned for review

Author-released Stage A is complete. Real Rally kernels/helper with resource-only
typed-copy/publication scaffold use up to25,620 text/988 data/28,184 image bytes;
resolved module stack path800 before callback. Proposed ceilings are64KiB text,
16KiB data,128KiB image and2KiB module stack, with smaller initial budgets. No
numbers are implemented or target-qualified. Stop before Stage B for Author review.

[The report](../tasks/GOLEM-002/capacity/stage-a/RESULTS.md) retains exact sources,
compiler/SDK/firmware identities, resource/call-path/heap-table evidence and
reproduction commands. All20 route and14 helper manifest entries verified; all21
generated artifacts match per default/relocated layout. Twenty repeated receipts
are identical. The command task has4096 bytes; selected one-wrapper firmware
frames736, loose nonrecursive all-module sum2016. Proposed module2048/firmware-service
2048 split requires final analysis and observed headroom qualification. Actual
heap tables confirm executable D/IRAM aliases share internal data RAM; static IRAM
link remainder is not the entire executable heap or a free-contiguous measurement.

Current SDK still rejects these builds, with no .vdpl emitted. The scaffold has
an intentionally early source-success suffix, reacquired root, placeholder missing
presentation fields and no complete readiness/status gate, so it is not runnable
D2 or a gameplay/renderer qualification. No peer source/helper changes, SDK edits,
firmware build, target/stress launches, hardware probes or commits occurred.
Historical fit receipts and reviewed protocol hash are unchanged. Exact findings
were sent in Golem capacity-stage-a-findings-nineteen (2026-10-14 00:04 local).
