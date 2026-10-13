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
