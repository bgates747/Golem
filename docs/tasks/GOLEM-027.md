# GOLEM-027 — Bounded portable Range loops and batched movement

Add finite For/Next loops to both task-local backends so one host call can do a
batch of work. The Author approved constant inclusive ranges, explicit uint16
counters and safe handling at 65535. The visual review will perform four small
movement steps per Golem call, retaining assembly drawing/backend choice/VSYNC.

Keep artifacts under `docs/tasks/GOLEM-027/`. Fork accepted GOLEM-026 with
provenance; preserve historical tasks, production compiler and generic SDK/runtime.
No hardware flashing or graphical launch by the agent.

## Initial contract

Use `For counter In Range(start:end[:step]) ... Next [counter]`; the name after
Next is optional, matching the maintained language decision. The counter must
already be an initialized uint16 global. Bounds and signed step are compile-time
integer literals; step defaults to 1 and must be nonzero, with magnitude at most
65535. Reject a step pointing away from the endpoint. Inclusive limits need not
be reached exactly: Range(0:9:2) visits 0,2,4,6,8. Equal bounds execute once with
either step sign. No implicit declarations or empty reversed ranges.

Compute the trip count and last visited value on the PC. All visited values must
fit the counter's declared Bounds. On normal completion retain its last visited
value; never increment the shared counter past that value. A failure retains the
current counter and earlier effects and exits the whole invocation. The body may
read but may not assign an active loop counter; nested loops need distinct counters.
Allow eight nested loops, with a per-entry ceiling of 262144 worst-case statement,
condition and loop-iteration visits. This is a finite-work bound, not a timing
guarantee. Keep batches small because buffered execution does not yield.

## Subtasks

- SYN-001 [x] Parse typed bounded loops, validate counter ownership/ranges and
  publish trip/last metadata. Preserve If/ElseIf and existing grammar.
- EMIT-001 [x] Emit finite buffered loops using jumps and checked shared-state
  semantics; emit native loops from the same typed model without code unrolling.
- CHK-001 [x] Verify inclusive/unaligned/descending/singleton/extreme ranges,
  nested loops/branches, selected/skipped failures and retained counters/effects.
  Run sanitizer, rejection, prior-language, ELF and maintained checks.
- CHK-002 [x] Execute stock buffered and actual Xtensa JIT/interpreter paths,
  including full 0:65535 traversal, relocated IDs and state-preserving switching.
- VIS-001 [x] Build and qualify the batched animation, explain the four-to-one
  arithmetic call reduction, deploy readable source and a stopped Oryx review.
- REV-001 [x] Provide launch/source, retrieve human results after review and await
  explicit commit/push approval for this tranche.

## Evidence

See [report/source/review command](GOLEM-027/README.md),
[host receipt](GOLEM-027/host-checks.json) and [target evidence](GOLEM-027/evidence/).
The finite buffered loop and native C loop share the typed model and retain state
through backend changes/cleanup. The Author reports the review worked; retrieved
results identify GOLEM-027 and match all 585 observations/5,769 bytes. REV-001
is complete: the Author explicitly approved the checkpoint before proceeding
on 2026-10-13.
