# GOLEM-037 — D2 single-module renderer/helper composition

Link generated Golem C with Rally-owned renderer/helpers behind a narrow,
reviewed C interface. Prefer one module/SDK dispatcher and one drawing owner;
measure combined resources before choosing any alternative. This follows the
[accepted contract](GOLEM-033/accepted-contract.md), depends on D0/D1 interfaces
and awaits Author implementation release.

## Scope

Rally owns its renderer, assets and integration logic; Golem owns generated
bindings and compiler code. This agent owns agon-vdplet integration proposals,
with SDK changes separately scoped in that repository. No independently owned
VDP checkout is assigned to this agent by implication.

The retained renderer's recorded 7,016-byte text size is evidence of that build,
not a combined-module forecast. Current 8 KiB admission is a configured policy,
not a measured hardware ceiling. Capacity probing, limit changes, a second slot
or a forced firmware renderer split are not authorized by this task plan.

- BND-001 [ ] Freeze helper symbols, scalar/view arguments and results, finite execution, capabilities, errors and ownership with Rally.
- ABI-001 [ ] Retain one SDK entry/dispatcher; define language/runtime/helper failure mapping and successful-generation publication.
- LNK-001 [ ] Implement reproducible generated-C plus renderer/helper object linking without recursive loader entry or cross-entry cached pointers.
- POR-001 [ ] Supply an agreed buffered counterpart or explicit target-requirement rejection for each helper; do not falsely advertise stock compatibility.
- RES-001 [ ] Measure combined text/data/rodata/BSS/stack, relocations, leases, installation and steady wire traffic against actual configured admission.
- CHK-001 [ ] Qualify inputs, partial failures, lease/lifetime cleanup, retained assets and fresh-handle/state-preserving reload behavior.
- CHK-002 [ ] Compare actual target commands/pixels against Rally's independent renderer fixtures; separate submission from completion.
- VIS-001 [ ] Stage the combined source-labelled scene on Oryx, stopped, in the same tranche as qualification.
- REV-001 [ ] Obtain human acceptance and explicit commit/push approval.

## Acceptance

One finite invocation can execute agreed generated game logic and the retained
renderer through the published interface. No silent SDK admission change or
per-frame unload/reload workaround is accepted. Resource failure is a reviewable
result, followed by an explicitly authorized decision. Keep Golem-owned artifacts
in GOLEM-037/; SDK and Rally implementation artifacts remain with their owners.

## Peer renderer proposal received

Rally's R25-RENDER v0.1 proposal is retained at GOLEM-037/rally-helper-proposal.md
(SHA256 4455fba7079ee795c941a3dfa540e4c449691db24e4cd3b800010de99332faef).
Golem agrees with a narrow declared native-only binding, one linked dispatcher,
explicit buffered rejection, separate helper/completion results and suppression
of root success after helper failure. Exact grammar, structs, generated metadata
and helper/SDK error mapping remain D2 agreement work. No implementation is released.

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
