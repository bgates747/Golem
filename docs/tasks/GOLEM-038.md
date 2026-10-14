# GOLEM-038 — D3 autonomous scheduler, snapshots and collision-event design

Define the separately owned firmware integration needed for 120 Hz simulation,
30 Hz rendering and sparse collision notifications. This is a gated design and
coordination plan under the [accepted contract](GOLEM-033/accepted-contract.md),
not authorization to implement firmware, probe capacity or deploy hardware.

## Required boundary

One serialized firmware task owns module invocation/drawing and safe load/unload.
Timer/ISR callbacks only signal work. Module entries are finite; no autonomous
loop or ISR module execution. Each simulation invocation reads a complete
immutable player snapshot. Firmware owns parser/publication, time alignment,
deadline/overrun policy, cancellation and event transport.

The game supplies collision events into an owned mailbox. Firmware sends
collision-only notifications, without opponent IDs, with agreed session/episode,
duplicate/stale handling and explicit recovery rearm. Runtime status, language
status and successful tick/render generations remain distinct. Failed work
cannot publish success or claim rollback of earlier source effects.

D0/D1 tick-aligned supplied snapshots and synchronous fixtures do not certify
sparse-telemetry autonomous gameplay. Interpolation/extrapolation/staleness and
recovery behavior remain Author decisions before D3. Exact wire encodings and
render/deadline behavior after failure must be agreed, not inferred here.

- OWN-001 [ ] Name the custom VDP/firmware owner and obtain explicit work/repository authority before any implementation.
- POL-001 [ ] Present Rally's snapshot-alignment and recovery recommendations/options for Author decision.
- ABI-001 [ ] Freeze versioned snapshot/event/session schemas, publication synchronization and failure/success markers.
- SCH-001 [ ] Design fixed 120 Hz stepping, paced 30 Hz deadlines, backlog/overrun handling and one invocation/drawing owner.
- EVT-001 [ ] Specify collision-only delivery, retries/duplicates/stale sessions, episode latching and recovery rearm without raw module UART.
- LIFE-001 [ ] Specify lease lifetime, StopRace, callback cancellation and safe reset/unload/exit behavior.
- REL-001 [ ] Obtain separate implementation release; create owner-local implementation tasks rather than editing firmware from this plan.
- CHK-001 [ ] Define timing, telemetry-during-render, failure, duplicate-event and teardown qualification; distinguish emulator evidence from physical hardware.
- REV-001 [ ] Require applicable Author visual/hardware review and explicit publication approval.

## Acceptance

A named owner and reviewable runtime/game sub-contract exist before autonomous
implementation begins. No benchmark, cadence, hardware memory ceiling or
physical behavior is claimed by this design document. Keep Golem coordination
artifacts in GOLEM-038/ and firmware source in its explicitly owned repository.

## Rally ownership proposal — 2026-10-14

Rally forty-four proposes Rally ownership of application-specific firmware
integration, scheduling, snapshot/events and gameplay/recovery policies. Golem
retains compiler/generated bindings and generic SDK/runtime ownership. This is
a proposed split for Author review, not an implementation delegation. Requested
Golem support is coordination/review of an eventual Rally-led D3 draft: existing
hooks, missing primitives, ABI, lifetime, cancellation and thread restrictions.
No new compiler feature is currently demonstrated necessary. Rally's proposed
design-only tranche also awaits Author release; existing demo authorization does
not release autonomous scheduling.

## Proposed next Golem tranche

[Generic runtime support review](GOLEM-038/runtime-review.md): a bounded static
review of invocation, leases, cancellation and unload, with source-backed
findings and owner-specific questions. Released and completed on 2026-10-14: [source-backed findings](GOLEM-038/runtime-findings.md). No scheduler
implementation, benchmark duplication or new runtime feature is commissioned.
The broader D3 items above are coordination boundaries, not this agent's
implementation assignments.
