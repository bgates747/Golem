# GOLEM-034 — D0 pinned compiler and integration delivery

Deliver one reproducible compiler/interface baseline for Rally, avoiding accidental
use of the smaller production src compiler or a moving task tree. This plan
implements D0 of the [accepted R25-G0 contract](GOLEM-033/accepted-contract.md).
The Author released implementation and qualification on 2026-10-13; human review
and explicit publication approval are still required.

## Scope and dependencies

The Author explicitly requested committing the accepted float tranche for this
work. GOLEM-032/25901e5 is now the committed/pushed scalar baseline; 031/b37aa3c
remains the historical integer reference.
Choose a pinned task export or agreed production promotion. Do not silently
promote historical artifacts or alter the agreed language semantics.

Retain SDK v1, the two-byte root-buffer argument and bounded named entries
InitRace/Tick120/Render30/ResetRace/StopRace. Define version/length/session,
retained-buffer descriptor ownership, access rights and separate language/runtime/
last-success state before parallel adapters depend on offsets. Entry IDs and
buffer assignments come from generated manifests. Game snapshot fields are
requirements until agreed, not an invented wire packet.

- PIN-001 [x] Inventory the committed/accepted compiler, SDK, toolchain and runtime identities; choose the delivered baseline.
- ABI-001 [x] Agree root schema, ownership, lengths, entry mapping, initialization and separate status/success semantics with Rally.
- ABI-002 [x] Specify host installation invariants for stock execution; distinguish declared shape from native lease validation of actual buffers.
- ART-001 [x] Deliver immutable source/build receipts, layout/initial bytes, assembly exports, buffered artifacts and native ELF/package identities.
- CHK-001 [x] Exercise a bounded scalar fixture on stock/native paths with fresh handles, retained state and invalid interface rejection before game effects.
- REV-001 [x] Complete any required Oryx staging/human review and obtain explicit commit/push approval.

## Acceptance and ownership

Rally can build a labelled adapter/stub against the delivered schema without
reading compiler internals. Stub success is not feature qualification. Unknown
schema/length/entry must reject before game mutation, with the limits of stock
buffer admission stated truthfully. No transaction rollback, scheduler, arrays,
wide math or renderer support is implied by D0. Golem owns compiler/receipts;
Rally owns its adapter/oracle. Useful multi-file artifacts stay in GOLEM-034/.

## Delivered evidence

[Source/usage/qualification report](GOLEM-034/README.md),
[immutable delivery pointer](GOLEM-034/delivery.json),
[root contract](GOLEM-034/root-contract.md).
The Author explicitly authorized the preceding GOLEM-032 commit/push, completed
as 25901e5. New D0 work remains uncommitted pending the staged human review and
explicit publication approval. Rally owns independent adapter consumption.

The Author accepted human review and explicitly authorized committing/pushing
all pending work and proceeding to GOLEM-035 on 2026-10-13.
