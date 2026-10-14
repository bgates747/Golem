# GOLEM-037 — D2 single-module renderer/helper composition

## Accepted delivery — 2026-10-14

The Author accepted the actual native demo and explicitly approved committing
and pushing Golem/SDK changes. This supersedes historical holds and unchanged
budget statements below. Real bindings, enlarged SDK/firmware and the pinned
independent-frame-clock patch are delivered; Rally's saved human run passes
900 frames/3,600 ticks. See [implementation and evidence](GOLEM-037/binding/README.md).
The fixture omits background and does not establish autonomous scheduling,
physical hardware behavior or 30-Hz performance. Broader stress remains GOLEM-002;
scheduler/snapshot/event design remains GOLEM-038.

## Historical planning and gates


Link generated Golem C with Rally-owned renderer/helpers behind a narrow,
reviewed C interface. Prefer one module/SDK dispatcher and one drawing owner;
measure combined resources before choosing any alternative. This follows the
[accepted contract](GOLEM-033/accepted-contract.md), depends on D0/D1 interfaces
and was released for a first implementation tranche by the Author on 2026-10-13.

**Subsequent Author decision, 2026-10-13:** increase the limits; that direction
was already approved and has been reaffirmed. Numerical values remain to be
established. The broader [GOLEM-002 capacity investigation](GOLEM-002.md#further-investigation--substantial-single-module-applications)
records the stress-protocol/Wolf-class application ambition. Current instruction
is to record that investigation only; do not start new builds, stress runs or
runtime changes in this turn. Historical evidence below remains unchanged; its
pending increase-versus-reduce decision is superseded.

Stage A was later explicitly released through Rally twenty-nine and is complete:
[actual-source measurements and proposed budgets](GOLEM-002/capacity/stage-a/RESULTS.md).
Only compile/offline inventory occurred. Proposed ceilings are not implemented;
Stage B SDK changes/real glue/target qualification remain unreleased. Missing
renderer presentation-field maps are identified for owner agreement, not filled
with invented game policy. The measurement scaffold must not be deployed as D2.

The first gate is a reproducible target build of the exact accepted helper and
a minimal single dispatcher, followed by combined generated-source measurement.
Keep the existing 8,192-byte text, 4,096-byte data, 512-byte declared stack and
16,384-byte image limits. If the helper alone or the combined module exceeds
them, retain the exact build, report the measured blocker and stop for review;
do not implement an unapproved capacity workaround.

## Released tranche

- FIT-001 [x] Verify all 14 accepted helper manifest entries; freeze the four unchanged build sources, header and SDK/Golem inputs in GOLEM-037/helper-pin.json.
- FIT-002 [x] Measure helper object, minimal linked dispatcher and tiny actual generated-Golem composition using pinned Xtensa flags. Final single-entry text/data 7,600/864 and 8,224/1,132; measured module stack paths 544 and 576. See GOLEM-037/README.md and evidence/fit.json.
- FIT-003 [x] Resource gate fails under unchanged 8,192-text/512-stack policy. SDK rejects the tiny combined build; helper minimal call path already exceeds declared stack. Return the measured blocker for Author review; ABI/linking/target/visual work below remains pending.

## Scope

Rally owns its renderer, assets and integration logic; Golem owns generated
bindings and compiler code. This agent owns agon-vdplet integration proposals,
with SDK changes separately scoped in that repository. No independently owned
VDP checkout is assigned to this agent by implication.

The retained renderer's recorded 7,016-byte text size is evidence of that build,
not a combined-module forecast. Current 8 KiB admission is a configured policy,
not a measured hardware ceiling. Increased limits are now the Author-selected
direction, with sizing/protocol work recorded under GOLEM-002. No new capacity
probe/run, second slot or forced firmware split is started by this plan update.

- BND-001 [x] Freeze helper symbols, scalar/view arguments and results, finite execution, capabilities, errors and ownership with Rally. Accepted v0.3 header/proposal hashes below; concrete compiler field maps remain ABI-001 work.
- ABI-001 [x] Retain one SDK entry/dispatcher; define language/runtime/helper failure mapping and successful-generation publication.
- LNK-001 [x] Implement reproducible generated-C plus renderer/helper object linking without recursive loader entry or cross-entry cached pointers.
- POR-001 [x] Supply an agreed buffered counterpart or explicit target-requirement rejection for each helper; do not falsely advertise stock compatibility.
- RES-001 [x] Measure combined text/data/rodata/BSS/stack, relocations, leases, installation and steady wire traffic against actual configured admission.
- CHK-001 [x] Qualify inputs, partial failures, lease/lifetime cleanup, retained assets and fresh-handle/state-preserving reload behavior.
- CHK-002 [x] Compare actual target commands/pixels against Rally's independent renderer fixtures; separate submission from completion.
- VIS-001 [x] Stage the combined source-labelled scene on Oryx, stopped, in the same tranche as qualification.
- REV-001 [x] Obtain human acceptance and explicit commit/push approval.

## Acceptance

One finite invocation can execute agreed generated game logic and the retained
renderer through the published interface. No silent SDK admission change or
per-frame unload/reload workaround is accepted. Resource failure is a reviewable
result, followed by an explicitly authorized decision. Keep Golem-owned artifacts
in GOLEM-037/; SDK and Rally implementation artifacts remain with their owners.

First-tranche outcome: [resource report](GOLEM-037/README.md). No real D2 binding,
runtime execution, target pixels or visual candidate is claimed. Recommend a
separately authorized SDK budget review; helper reductions remain Rally-owned.

## Peer renderer proposal received

Rally's R25-RENDER v0.1 proposal is retained at GOLEM-037/rally-helper-proposal.md
(SHA256 4455fba7079ee795c941a3dfa540e4c449691db24e4cd3b800010de99332faef).
Golem agrees with a narrow declared native-only binding, one linked dispatcher,
explicit buffered rejection, separate helper/completion results and suppression
of root success after helper failure. Exact grammar, structs, generated metadata
and helper/SDK error mapping remained D2 agreement work at that proposal checkpoint.
No implementation was released then; the released tranche above supersedes that status.

D0 holds argument AND root leases. If the helper holds asset+output concurrently,
composition would peak at four. Proposed D2 adjustment: release the argument view
after resolving root identity, before helper entry, then scope root+asset+output
to three. D0 does not yet do this; qualify new cleanup/error paths when implemented.
Readiness must be cleared by the invocation owner before attempted RUN, including
rejection before native entry. stop only invalidates helper state after the
firmware drawing/invocation owner has drained; it does not itself implement drain.

Concrete R25-RENDER v0.2 was received/hash verified and retained under GOLEM-037/
(v02-rally_render.h and v02-CONTRACT-PROPOSAL.md). Caller-owned context 84,
bindings 76, typed state 108 and completion 48 replace the earlier opaque functions.
Review of the final compiler binding/header/storage contract remains queued for
D2 before shared integration. Prefer a separate generated application-owned
completion buffer: appending a non-source footer to root would require explicit
agreement with root-v1's globals_bytes meaning and existing adapter shape checks.
No compiler flag, helper hooks or completion marshalling is implemented by D1a.

Rally accepted the separate application-owned completion-buffer refinement in
message 2026-10-13_18-02-47_rally_rally25-completion-buffer-refinement-sixteen.
Completion publication/release can itself fail: skip root-success publication
and require runtime/interface/language/helper results, matched completion and
fresh root success before drawing. Resident ready alone is insufficient. Exact
header/build-hook agreement remains queued; no D2 implementation is released.

## Accepted helper contract v0.3

Exact header SHA256 `2adcc445e416f3dd95db2f52afb7ef8803c67e6fc444556be5e9d1505b133e50`
and proposal SHA256 `0b1c90c8f22233ca7328155537ca9a59cbe817a90e33eab98d558dbc751db885`
are retained as v03-rally_render.h/v03-CONTRACT.md. v0.3 replaces private context 84
with pointer-free context 464 (snapshot/scratch included); bindings 76, state 108,
completion 48 and signatures remain fixed. Golem accepts the helper interface,
error/publication rules and proposed bounded native-bindings spelling for D2
planning, recorded in v03-acceptance-request.json. Concrete generated JSON/source
field maps remain D2 implementation/validation work. The contract acceptance
itself did not release implementation; the Author's subsequent 2026-10-13 go
releases the bounded tranche above. The later increase decision is recorded at
the top; stress execution and numerical changes are not performed by this update.
Composed native stack remains unqualified under the unchanged 512-byte policy.
