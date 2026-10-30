# VDPlet feature requests — compiled consultation list

Eight approved project sessions supplied nine reports. They yield **15 groups**:
seven shared runtime/SDK/documentation proposals, five application/module
proposals, one emulator diagnostic and two no-change dispositions. Most consumer
work can use existing services. The clearest common request is independently
selectable pixel and PCM support; asynchronous jobs are a separate research item.

These are proposals for the Author's review, with suggested owners rather than
assignments. [TODO.md](../../../TODO.md) and [GOLEM-056](../GOLEM-056.md) remain
the task authority; no implementation or peer assignment is released here.
Stable CNS IDs match the [detailed review](review.md). The original seven-session
audit and later Wolf handoff retain separate provenance.

## Shared runtime, SDK and documentation

| ID | Request | Suggested owner | Next decision / dependency |
| --- | --- | --- | --- |
| **CNS-001** | Offer pixels without the richer audio/queue engine, optionally combined with lean PCM. Supply separate pixel and PCM SDK accessors; preserve the existing helper and old ABI views. | Golem / agon-vdplet | First shared implementation candidate after a bounded plan. Reuse [VDPL-005](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-005.md); measure matched flash, static RAM and allocation costs. Requested by 6502, Rally and Extender; also useful to Wolf/FSIM. A pixels-only build does not exist yet. |
| **CNS-002** | Document existing resource queries and when an ordered screen-pixel query can drain drawing without a vblank wait. | Golem / agon-vdplet | Documentation work: INFO18–21 free/largest PSRAM,22 audio-stack watermark,23 task count. The GP route needs a fresh ordered reply and callback caveats. No new runtime callback requested. |
| **CNS-003** | Let ordinary eZ80 applications use the existing lean copied PCM sink through VDU commands, without loading a module. | Golem / agon-vdplet;6502 owns adoption | Design OPEN/STATE/WRITE/CONTROL/CLOSE, command allocation, generation/one-producer ownership, copied-write acknowledgement and BUSY. Reuse the sink and [VDPL-007](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-007.md) semantics; retained6502 VDP-005 stage6 goal. The larger32-channel scheduler is a different product option. |
| **CNS-014** | Define how native offscreen output becomes a mutable drawable bitmap, including when its storage can be recycled. | Golem generic contract; FSIM consumer integration | First verify whether existing bitmap backing-buffer mutation suffices. Add a checked copy/publication hook only for a demonstrated gap; consolidate with VDPL-005. Screen rectangle writes alone do not define offscreen bitmap publication. Supports CNS-012. |
| **CNS-013** | Research bounded asynchronous native jobs so command processing and other work can continue during long finite computation. | Golem / agon-vdplet; FSIM supplies first use case | Reuse deferred [GOLEM-043](../GOLEM-043.md). Define admission, pinned code/resources, module serialization, worker-safe services, results/backpressure and safe retirement first. Keep output-ready, copy-complete and presented distinct. Separate from synchronous Pingo loading; no worker is implemented or assigned. |
| **CNS-009** | Consider scoped writable colour rows to avoid rectangle-copy overhead. | Golem generic contract with consumers | Conditional research only: first show that the existing copied-pixel route is the bottleneck. Any lease needs native layout/sync rules, bounds, hidden-page ownership, conflicts, release and completion. Preserve existing mono semantics. Requested conditionally by Rally/Extender. |
| **CNS-007** | Assess a P4 RISC-V VDPlet target and discovery of CPU/compiler profiles. | Golem API/package semantics; Extender P4/EMOS integration | Feasibility under existing [PORT-003](/Agon/mystuff/agon-extender/docs/tasks/PORT-003.md) ownership. Needs target compiler/loader/relocations, allocation/cache rules, service adapters and an admitted EMOS reply route. Proposed INFO low/high profile pairs have no allocated IDs. Shared source does not mean shared native binaries. |

## Applications and optional modules using existing services

| ID | Proposal | Suggested owner | Existing work / necessary proof |
| --- | --- | --- | --- |
| **CNS-004** | Reduce native diagnostic readbacks and replies per racing frame. | Rally | Existing INC-001-05 in [Rally TODO](/Agon/mystuff/AgonArcade/TODO.md). Current fixture has six native transactions/replies per frame. Preserve errors, a diagnostic control and final rendering/swap fences; measure wire bytes, waits and complete-race behavior. No new service required. |
| **CNS-005** | Prepare multiple bitmap sizes before racing, selecting sizes by visible error and memory cost. | Rally | Existing [AFF03 proposal](/Agon/mystuff/AgonArcade/docs/tasks/RALLY-28/custom-vdp/prepared-bitmap-set/PROPOSAL.md). Measure setup time, peak/retained RAM, fallback counts and whole-race timing; human review assesses stepping/popping. No native bitmap-creation hook requested. |
| **CNS-012** | Assess a synchronous loadable Pingo renderer with solid, textured and mixed materials. | FSIM engine/arenas/images/presentation; Golem admission/service review | First prove actual relocatable module, dependencies and whole-path stack fit. Use caller-owned scene/work/colour/depth arenas and reconstruct scoped pointers. Resolve CNS-014 publication/lifetimes. Reported 31,555 mapped bytes and 8,192-byte worker stack reservation do not qualify a module under current limits. Async CNS-013 is a possible later step. |
| **CNS-015** | Port Wolf's renderer to a finite module with compact persistent state and an app-owned full-frame workspace. | Wolf3DOrig; Golem generic review | Existing [Wolf TODO8 study](/Agon/mystuff/Wolf3dOrig/agonport/doc/wolf3d-vdplet-migration.md). [Our static review](wolf3d-review.md) identifies no missing Wolf-specific hook. Prove admission/stack, roughly 90 KiB contiguous workspace, full-frame cost, physical pixels/input timing and lifecycle. Preserve the standalone oracle. Technical review is delivered; port implementation remains paused. |
| **CNS-008** | Consider finite flight calculation kernels and engine-sound generation. | FSIM | Optional later consumer work. Existing buffer/math services and stock audio or lean copied PCM suffice. Measure the whole tick, wire traffic and refill timing; preserve current flight/performance work. No additional background callback required for copied playback. |

## Emulator diagnostic and no-change dispositions

| ID | Disposition | Owner / existing work |
| --- | --- | --- |
| **CNS-006** | Diagnose PCM production versus downstream delivery before changing audio behavior. | Flexe, existing FX-062 in [issue log](/Agon/mystuff/flexe-agon-emulator/docs/tasks/FX-026.md). Capture pre/post-ring PCM, counts/rates/depth/drops/underruns/deadlines and clock domains. FX-032 timing is a separate dependency; Golem retains matched controls under VDPL-008. No new firmware ABI requested; the distortion cause is unresolved. |
| **CNS-010** | Wireframe requests no new hook. | Existing buffer/native-line services suffice. Further clipping/visibility/rendering stays in optional consumer algorithms; no permanent scene-renderer ABI. |
| **CNS-011** | Modeler requests no new firmware feature. | Continue offline geometry, UV/texture and aerodynamic-table generation, including existing MODEL-045. No runtime solver/mesh-loader/aircraft-specific service. |

Other explicit already-covered returns:6502's monochrome desktop and native
speaker sink; Rally's line/audio needs; Extender's application-owned controller
mapping, deferred tile-scroller and separately owned P4 audio. None creates an
additional VDPlet feature assignment. FSIM's regional-colour request uses the
existing generic candidate first.

## Recommended selection for review

1. **First shared tranche:** CNS-002 documentation and a bounded CNS-001
   pixel/PCM composition plan. These serve several consumers without adding
   application algorithms to firmware.
2. **Independent owner proposals:** Rally CNS-004, then CNS-005 if selected;
   Flexe CNS-006 using its existing diagnostic task. They need no new runtime
   service and can be reviewed separately.
3. **Next design/research choices:** CNS-003 ordinary PCM access and CNS-012/014
   synchronous Pingo/storage feasibility. Wolf retains its existing reviewed
   migration plan and separate implementation gate.
4. **Conditional later research:** CNS-013 asynchronous jobs, CNS-009 colour
   leases, CNS-007 P4 targeting and CNS-008 flight kernels/audio. Selection
   should follow a bounded benefit/cost case, not a promise of speed.

For any approved runtime change, keep old ABI/service behavior and optional
features compatible, and report matched linked flash/static RAM plus runtime
allocations. Applications retain loading, input, pacing, swaps and cleanup.
Emulator evidence and physical qualification remain distinct. No task launch,
benchmark, deployment or implementation assignment accompanies this catalog.

## Complete source-proposal coverage

| Return | Original proposal IDs and disposition |
| --- | --- |
| [6502](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-25-35_agon-6502_golem056-6502-capabilities-return-one.md) | A2-GFX-001 → CNS-001; A2-FENCE-001 → CNS-002; A2-AUD-002 → CNS-003. A2-BASE-001 / A2-AUD-001 already covered. |
| [Rally](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-20-01_rally_golem056-rally-review-one.md) | RLY056-001 → CNS-004; RLY056-002 → CNS-005; RLY056-003 → CNS-001/009; RLY056-004 → CNS-002; RLY056-005 no additional line/audio hook. |
| [Extender](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-27-33_linux-extender_golem056-extender-capability-return-one.md) | EXT056-001 → CNS-007; EXT056-002 → CNS-001/009; EXT056-003 / EXT056-004 already covered/deferred under existing owners. |
| [FSIM initial](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-19-03_fsim-pingo_golem056-fsim-capability-return-one.md) / [follow-up](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-41-07_fsim-pingo_golem056-pingo-module-and-generic-async-two.md) | FSIM-001 / FSIM-002 → CNS-008; FSIM-003 → existing generic pixels/CNS-001. FSIM-004 → CNS-012/014; FSIM-005 → CNS-013. The follow-up explicitly proposes synchronous Pingo feasibility first. |
| [Flexe](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-13-29_flexe_golem056-flexe-audio-boundary-return-one.md) | FLEXE-AUD-001 → CNS-006. |
| [Wireframe](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-11-45_fsim-wireframe_golem056-wireframe-capability-return-one.md) | WIRE-001 → CNS-010, no useful new hook now. |
| [Modeler](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-39-32_fsim-modeler_golem056-modeler-capability-return-one.md) | MODEL-001 → CNS-011, no useful new firmware feature now. |
| [Wolf3DOrig](../../../../agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-48-39_wolf3dorig_wolf-vdplet-feasibility-requirements-one.md) | Unnumbered requirements/review handoff → CNS-015; generic composition/discovery overlaps CNS-001/002. No Wolf-specific runtime service requested. |

All 22 numbered source proposals and the additional Wolf handoff are represented.
Exact immutable message IDs/hashes remain in [returns.json](returns.json) and
[supplemental-returns.json](supplemental-returns.json). [coordination.md](coordination.md)
tracks later approved assignments; it currently records none.
