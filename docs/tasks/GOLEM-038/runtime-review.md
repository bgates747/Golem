# GOLEM-038 — Generic runtime support review tranche

Review existing VDPlet services to identify what Rally can reuse for autonomous
execution and what requires a separate interface proposal. Deliver a short,
source-backed account of invocation, buffer lifetime, cancellation and unload.
The Author released this review on 2026-10-14 after committing/pushing the plan.
Static review is complete; findings are in runtime-findings.md. No implementation
or runtime qualification was performed.

## Scope and ownership

Golem owns compiler/generated bindings and generic agon-vdplet runtime/SDK work.
Rally owns application-specific scheduling, snapshot/event integration and game
policies under its proposed split. This tranche supports that boundary; it does
not select gameplay timing, stale-input, catch-up or collision-recovery policies.
Existing demo authorization does not release autonomous implementation.

Read maintained SDK/runtime sources in `/Agon/mystuff/agon-vdplet`, relevant
generated binding contracts and the accepted cross-agent contract. Inspect
firmware call sites only as needed to establish execution context. Distinguish
source-proven behavior, caller obligations and unanswered questions. Do not
infer thread safety or interrupt suitability from a successful synchronous demo.

## Review steps

- INV-001 [x] Locate invocation entry points and their actual callers; document execution context, serialization/reentrancy assumptions, finite-return requirement and result channels.
- BUF-001 [x] Trace buffer acquisition/release, owner protection and cleanup; identify lifetime rules across source/helper calls and what a future snapshot publisher must preserve.
- CAN-001 [x] Establish whether cancellation exists, where work can safely stop, and whether active native calls can be interrupted; distinguish module return, host stop requests and callback cancellation.
- LIFE-001 [x] Trace unload/reset/replacement behavior and handle validity; identify required ordering with active calls, leases and application-owned drawing.
- GAP-001 [x] List concrete missing generic primitives only where supported by a specific proposed use; otherwise record questions for Rally's draft rather than inventing requirements.
- DEL-001 [x] Produce a concise findings document with source references and a table of existing service, caller obligation, unresolved issue and responsible owner. Identify any necessary follow-on proposal separately from implementation.
- REV-002 [x] Return findings to the Author for review; do not implement changes or expand the tranche automatically.

## Deliverables and acceptance

Keep findings and any supporting notes in `docs/tasks/GOLEM-038/`. Each conclusion
must cite the relevant maintained source or contract. Clearly distinguish what
already works from what remains unspecified. The report should let Rally draft
its service boundary without treating this agent as the scheduler owner.

No benchmarks, emulator launches, firmware builds, hardware actions, capacity
campaign, game replay or production-code changes are needed. If static evidence
cannot settle a question, record it explicitly with the smallest proposed next
check; do not run that check as an implicit scope expansion. No commit/push is
part of this planning request. Cross-agent delivery of findings follows Author
review or a subsequent explicit instruction.

The completed findings are returned for Author review and sent to Rally under the released coordination tranche. No follow-on implementation is implied.

The Author explicitly approved committing/pushing the completed review on 2026-10-14.
