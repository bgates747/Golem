# R25-G0 v0.2 — Accepted planning contract

Golem and Rally mutually accepted all eight clauses below without amendments.
This freezes planning requirements and ownership, not implementation authority.
The Author requested task documents followed by discussion/review; no feature,
firmware, capacity-probe or deployment work is released by this document.

Provenance: Golem counterproposal
`2026-10-13_15-06-37_golem_rally25-counterproposal-one`, accepted by Rally in
`2026-10-13_15-12-37_rally_rally25-contract-v02-accepted-five`.
[Original proposal](counterproposal.md), [immutable acceptance receipt](acceptance-receipt.json).
The Rally-owned PROTOTYPE-CONTRACT.md may still say draft v0.1; it is superseded
for this planning revision by these two immutable messages. Rally owns updating
that file. No other project source or game rules are changed here.

## Subsequent baseline/release amendments

The Author released GOLEM-034/D0 implementation and qualification, then explicitly
requested committing/pushing accepted GOLEM-032 for that pin. It is committed at
25901e52eb80a4d66847c11fbc70d1bac68ee633. Rally verified and accepted that
baseline correction in message 2026-10-13_15-59-07_rally_rally25-float-baseline-accepted-eight.
This supersedes the historical uncommitted/031-only wording in clause 1 below.
Rally also accepted concrete root-v1 unchanged; see GOLEM-034/README.md. These
amendments release D0 only, retaining later feature/firmware and human review gates.

The Author subsequently released GOLEM-035/D1a after accepting and publishing D0.
Rally accepted the concrete storage profile in message
2026-10-13_17-26-57_rally_rally25-d1a-profile-accepted-thirteen; see
[the typed-array contract/report](../GOLEM-035/README.md). D1b/D2/firmware remain
separate release boundaries. Rally accepted language status 6 for minimum
signed/-1 division overflow (zero divisor 4, conversion/Bounds 32), paired outputs
unchanged and earlier effects retained, in message
2026-10-13_17-18-34_rally_rally25-d0-received-division-six-eleven. This resolves
clause 3's pending numeric mapping without implementing signed division.

## Accepted clauses

1. **Baseline.** GOLEM-031 is committed/pushed at b37aa3c. GOLEM-032 float32
   review is now accepted: retrieved human run passes 637 observations/6,289
   bytes, 24,494 host cases and stock/actual-Xtensa qualification. It remains
   uncommitted pending explicit commit approval. Pin 031 for reproducible
   integer contract work; do not build against a moving dirty tree or production
   src/. A delivered compiler snapshot/promotion must identify commit or immutable
   source hashes, shared interface/layout/initial bytes, named entries, toolchain,
   SDK ABI, native ELF/package and buffered artifacts. Promotion is a separate
   Golem-owned delivery step, not a claim that src/ already has these features.
2. **Storage first.** First extension is bounded one-dimensional typed arrays/
   SoA, mutable opponent state and immutable route/sine/track data. Use the
   existing agreed language direction Array sint16 lateral(0:5), inclusive
   explicit bounds, and lateral(index) access. uint16/sint16 first; wider element
   types follow the numeric milestone. No heterogeneous records, heap or general
   pointers are prerequisites. Buffer IDs, descriptors, lengths, access modes
   and field offsets are generated; assembly/build installs assets and state.
   Data stays in retained buffers, not hundreds of embedded C constants. Index
   failure uses existing status 5 and preserves the failed destination/element
   and adjacent storage, retaining earlier effects. Both backends share syntax,
   storage and semantics; a missing buffered operation must reject explicitly,
   never silently use a native-only dialect. Six opponents and 257 sine entries
   are required fixtures, not global language-size limits.
3. **Faithful numeric milestone.** Agree uint32/sint32 state, checked operations
   and explicit narrowing are needed. Add/subtract/compare/multiply and signed
   division must preserve existing units and evaluation order. Signed quotient
   truncates toward zero; remainder is n-q*d, has numerator's sign or zero,
   and abs(r)<abs(d). Zero divisor is status 4; minimum signed value divided by
   -1 is an explicit overflow error, with no output writes. Reuse existing
   operator status families and document the division-overflow code before
   implementation. No float substitution or implicit widening of existing
   16-bit expressions: programmers explicitly form wide operands before a
   product. Rally supplies pinned formulas, operand bounds and reference vectors.
   Where a 32-bit intermediate is insufficient, specify a checked mul-div helper
   with an exact wider intermediate and defined rounding, or an explicitly
   reviewed 64-bit requirement. Do not promise arbitrary 64-bit language support
   before that audit. Outputs/argument-failure behavior follows current checked
   semantics, including all-or-neither stores for paired quotient/remainder.
4. **Composition.** Prefer one linked native module: generated Golem C plus
   Rally-owned renderer/helper objects behind explicit bounded C bindings.
   There is one SDK entry/dispatcher and one drawing owner; helpers do not
   recursively enter the loader or load a second module. Define binding names,
   supported argument/result types, buffer views, capability needs and error
   mapping before adapters use them. Helper bindings require an agreed buffered
   implementation or an explicit target-requirement rejection; stock portability
   is preserved where supported, not falsely advertised for native-only renderer
   bindings. Measure combined text/data/rodata/stack/relocations, lease counts,
   installation and steady traffic. 8 KiB is current admission policy, not a
   physical ceiling; no capacity probe, limit increase or forced firmware renderer
   split is implied by this contract.
5. **Invocation/state ABI.** Retain vdplet_host_v1/request_v1 and the two-byte
   shared-root-buffer argument. Keep bounded parameterless InitRace, Tick120,
   Render30, ResetRace and StopRace names, with generated numeric IDs. Root
   descriptors carry schema/session/byte lengths and input/state/event/table
   ownership. Unknown schema/length/entry rejects before game effects. Separate
   runtime status, language status and last-success generation. Successful
   generation/event publication occurs only after the corresponding job succeeds;
   failure does not claim rollback of earlier writes. Host acquisition of a
   buffer is not snapshot synchronization. Views/leases expire before return;
   generated helpers must not cache pointers across calls.
6. **Scheduling/transport fence.** Agree one serialized firmware task owns
   invocation/drawing, takes a complete immutable player snapshot per step,
   invokes finite Tick120 at 120 Hz and Render30 every fourth successful step.
   Timer/ISR only signals work; no ISR module execution or unbounded resident loop.
   Firmware owns publication, deadline/overrun handling, cancellation, event
   transport and safe load/unload; compiler primitives do not imply those services
   exist. A module publishes collision into an owned mailbox; firmware sends a
   collision-only notification with session/episode identity, no opponent ID.
   Require duplicate/stale rejection and explicit recovery rearm. Exact wire
   encoding, delivery/retry and recovery policy are frozen in the firmware/game
   sub-contract before autonomous work, not guessed by compiler code.
7. **Time alignment.** D0/D1 use complete, explicitly tick-aligned supplied player
   snapshots, or labelled synchronous fixtures. They do not certify autonomous
   sparse-telemetry collisions. Interpolation/extrapolation/stale-snapshot and
   recovery behavior remain an explicit Author decision before D3. Please supply
   a short recommendation/options for those game choices; we can negotiate the
   engineering mechanism without silently changing physics.
8. **Milestones/ownership.** D0 pins the contract/baseline and reproducible scalar
   artifact; D1 delivers indexed storage and faithful numeric primitives against
   independent Rally oracles with eZ80-triggered bounded entries; D2 links the
   existing renderer and qualifies combined resources/state/pixels; D3 adds the
   separately approved firmware scheduler/snapshot/event implementation and
   timebase/teardown qualification. Rally owns game models, renderer, host adapter,
   telemetry/recovery code and fixtures. Golem owns compiler/semantics/artifacts.
   This agent also maintains agon-vdplet, so I can own SDK/runtime integration
   proposals there; no other custom VDP checkout ownership is assumed. Firmware
   integration requires an explicitly named owner and work authorization. Each
   side promotes its implementation tasks only after the applicable release;
   emulator changes still receive Author visual review before commit/push.


## Work decomposition

| Stage | Golem task | Scope |
| --- | --- | --- |
| D0 | [GOLEM-034](../GOLEM-034.md) | Pinned compiler/artifact and root-interface delivery contract |
| D1a | [GOLEM-035](../GOLEM-035.md) | Bounded typed arrays and retained data |
| D1b | [GOLEM-036](../GOLEM-036.md) | Faithful wide integer primitives and narrowing |
| D2 | [GOLEM-037](../GOLEM-037.md) | One-module renderer/helper composition |
| D3 | [GOLEM-038](../GOLEM-038.md) | Scheduler/snapshot/event design and owner/release gates |

TODO.md owns overall execution status; these links describe dependencies only.
Array and numeric grammar/layout details still need implementation-sized design
reviews. Sparse-player alignment, recovery behavior, firmware ownership and any
capacity-policy change remain explicit unresolved decisions.

## Renderer helper agreement — R25-RENDER v0.3

For D2 planning, the mutually proposed concrete helper ABI/storage/publication
contract is accepted as recorded in [v03-CONTRACT.md](../GOLEM-037/v03-CONTRACT.md)
and exact [header](../GOLEM-037/v03-rally_render.h), with SHA256 pins in
[GOLEM-037](../GOLEM-037.md). Private context 464 holds value-only scratch; bindings 76,
state 108 and completion 48 retain their fixed layouts. Generated glue owns typed
marshalling, early argument-view release and a separate application-owned retained
completion range, preserving root-v1 globals_bytes. Any helper or publication
failure suppresses root success. Drawing requires matching session/request/tick,
all success channels and fresh root success after explicit readiness preclear.
The bounded native-bindings schema/profile/hook spelling is agreed for future
implementation; it is not an available compiler flag. Actual combined SDK
admission, cleanup and Author visual review remain later gates. This contract
agreement releases no D2 implementation or capacity change.
