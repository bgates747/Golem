# GOLEM-004 — Review grammar and the next scalar integration slice

Reconcile the current syntax decisions with the maintained compiler and the
GOLEM-003 scalar prototype, then specify the smallest implementation tranche.
The Author requested this review task and authorized proceeding on 2026-10-11.
Deliver concrete grammar, compatibility and allocation recommendations before
changing either parser or promoting the prototype. This task does not revoke
the earlier deferral of compiler implementation.

Overall status belongs in TODO.md. Put the review, source probes and evidence in
`docs/tasks/GOLEM-004/`; keep experimental files out of production directories.
Treat GOLEM-003 as the numeric behavior baseline, not as a language specification.

## Sources and grammar review

- SRC-001 [ ] Inspect current specification, readable examples, maintained
  parser/emitter and scalar prototype. Identify accepted direction, proposals,
  current executable behavior and stale/conflicting wording.
- GRM-001 [ ] Specify a small grammar without statement terminators or significant
  newlines. Cover existing declarations, assignments, additions, literal print
  and For/Next, plus the scalar example's declarations and named job boundary.
- GRM-002 [ ] Examine adjacent statements, expression/call boundaries, reserved
  words, multiline source, missing operands and nested block ownership. Give
  concrete valid/rejected examples and diagnostics requirements.
- GRM-003 [ ] Reconcile ElseIf, pre/post-condition Do loops, Switch and the selected
  step-1 Range dispatch contract. Keep stepped ranges, membership and Bounds
  naming distinguishable from the minimal implementation target.

## Integration and verification review

- INT-001 [ ] Define how typed scalar operations can join main without conflicting
  buffer IDs, exposing scratch matrices or changing numeric semantics. Separate
  source grammar, allocation, resident packaging and host admission obligations.
- INT-002 [ ] Recommend the next implementation-sized tranche, its compatibility
  policy and explicit exclusions. Identify what can be host-verified and what
  needs real firmware execution before production promotion.
- VER-001 [ ] Run read-only host probes against the current parsers where useful;
  keep generated sources, binaries and results in this silo. Check that reviewed
  source code remains unchanged and avoid emulator/deployment changes.
- DEL-001 [ ] Deliver the review and evidence, reconcile maintained design/handoff
  wording, update task tracking and the dated devlog, then commit and push the
  review result. Do not implement proposed parser/backend changes in this task.
