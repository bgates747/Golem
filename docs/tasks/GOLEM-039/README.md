# GOLEM-039 — Full-scene source package for Rally

Build a separate real native package for Rally's supplied full-scene source,
using accepted GOLEM-037 bindings/helper and SDK9fbb21d. Rally forty-eight
relays the Author's full-scene release and requests this owner-local build.
Rally owns source/driver/assets/world verification/headless/human deployment.
Preserve prior candidate, firmware, snapshots and shared semantics.

- PIN-001 [x] Verify source c62aee2851a3ac39c859a61bc45684a1ce9e384aab623b3c7730a0f97d39d6b4 and unchanged accepted build inputs.
- BLD-001 [x] Generate actual C/layout/assembly and Xtensa package with existing compiler, bindings, helper and SDK; retain receipts and sizes.
- CHK-001 [x] Check loader admission, interface/layout continuity and a bounded host supplied-player/lifecycle smoke; leave independent game/scene validation to Rally.
- DEL-001 [x] Export a new immutable package with source/config/helper/identity manifest, make available on Oryx and notify Rally promptly.
- REV-001 [ ] Record peer qualification and applicable human review before any new commit/push.

No new SDK/glue semantics, scheduler, capacity campaign, graphical launch or
physical flash is requested. Report any concrete shared semantic issue rather
than silently changing it. Reuse firmware8da2ebf... without rebuild if compatible.

## Delivery — 2026-10-14

New immutable candidate: [candidate-5a3ef7b3e8c3bf6a](delivery/candidate-5a3ef7b3e8c3bf6a/).
Export manifest SHA256 `f12f915170c07bc8da646dbc9bc21a578e6203fe1fdb5738562c66045f2ff6b5`.
All 33 payload files are hash verified on Oryx at the same home-relative path.
Rally received delivery in Golem twenty-eight and owns subsequent integration,
headless/human staging and independent scene/world qualification.

Module SHA256 `5a3ef7b3e8c3bf6a040eb2e55c605fce83e93f3868b296b245738f14a1e69400`:
30,240 code bytes, 980 private-data bytes, 32,776 package bytes, 43 relative
relocations. Existing SDK image checker admits the actual package. No firmware
rebuild was needed; existing firmware binary and ELF hashes remain verified.

Root/array layouts, entry IDs, assembly wrappers, install/cleanup commands and
handle offset 37 remain unchanged. Interface visit bounds change with source;
initial state and array installation bytes deliberately change with new initial
values. An initial parity assertion incorrectly included array installation
bytes; it was corrected to compare layout/ABI rather than source initial data.
No source or compiler semantic change was needed. All 32 old candidate payload
hashes still match the immutable prior receipt.

[Bounded host smoke](host_smoke.py) passes 16 calls and eight supplied-player
ticks with peak three leases: external player fields remain unchanged by
Tick120, worldTick increments, matching render completion is published,
stop/reset/INIT/FINI behavior succeeds, and canaries/leases remain valid.
[Summary](host-smoke-result.json). This does not duplicate Rally's independent
world/reference simulation or establish actual target/full-scene performance.

No production compiler/SDK source, emulator profile or firmware was changed.
New artifacts remain uncommitted pending applicable review/publication approval.

## Publication checkpoint

The Author explicitly requested commit/push of all current Golem work on
2026-10-14. This approves publishing this package and its evidence without
claiming acceptance of Rally's still-pending full-scene visual review. Rally
fifty independently reports a byte-identical client-built module and a passing
60-frame/240-tick Oryx smoke. Full scene/human qualification remains Rally-owned.
