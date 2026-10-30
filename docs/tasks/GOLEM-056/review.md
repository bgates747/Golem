# C/C++ VDPlet consultation — consolidated review

All seven project sessions have returned eight durable reports, including a
Pingo follow-up; every report has been read. Most improvements can use existing
services and stay in the applications or optional modules.
The strongest shared runtime request is a small pixels-only firmware choice,
with independently checked SDK accessors, rather than adopting the larger audio
candidate to obtain colour drawing. FSIM also proposes assessing a synchronous
loadable Pingo renderer, then separately considering generic asynchronous jobs
to recover overlap. The modeler and wireframe need no new firmware feature now.
The proposals below are ready for joint review; no implementation assignments
have been sent. A later, additional Wolf3DOrig handoff is reviewed separately
below; it does not change the frozen mail window or seven-session roster.

The frozen brief and every original message remain indexed in
[coordination.md](coordination.md) and [returns.json](returns.json).
For the concise list with complete source-proposal coverage, use the
[compiled feature-request catalog](feature-requests.md).
Read acknowledgements and successful chat delivery are not substitutes for a
mailbox return. No implementation has been dispatched by this consultation.

## Candidate groups and suggested ownership

| ID | Candidate | Suggested owner if approved | Source proposals / dependency |
| --- | --- | --- | --- |
| CNS-001 | Make generic colour pixels available without the richer audio/queue engine; allow combination with lean PCM. Add separate pixel/PCM SDK accessors while preserving the original helper and ABI. | Golem / agon-vdplet; consumer-specific checks remain with their owners | A2-GFX-001, RLY056-003, EXT056-002; FSIM-003 is a related later use. Reuse VDPL-005; do not duplicate its surface interface. |
| CNS-002 | Document already-implemented INFO18–23 and the conditional ordered GP draw-fence route. | Golem / agon-vdplet documentation | RLY056-004, A2-FENCE-001. Source confirms fields18/19 free PSRAM,20/21 largest PSRAM block,22 audio-stack watermark,23 task count; specify applicability and fence caveats. No new runtime service implied. |
| CNS-003 | Give ordinary eZ80 applications a small VDU frontend to the same copied PCM sink. | Golem / agon-vdplet core/wire adapter;6502 later adoption | A2-AUD-002; retained product goal in VDP-005 stage6 and lean PCM follow-up. Design command allocation, one producer, generation/status, copied-write acknowledgement and BUSY before implementation. Reuse the sink rather than the32-channel scheduler. |
| CNS-004 | Reduce per-frame native readbacks/replies and keep ordinary draw submissions local. | Rally | RLY056-001 / existing INC-001-05. Current fixture uses six native transactions/replies per frame; preserve a diagnostic control, error handling and final draw/swap fence. Existing services suffice. |
| CNS-005 | Prepare multiple bitmap sizes before racing; select by visual error and total memory cost. | Rally | RLY056-002 / existing AFF03. Count setup and retained/peak RAM separately; preserve actual source artwork. No bitmap-creation firmware hook requested. |
| CNS-006 | Diagnose audio production versus downstream emulator delivery, then assess sample-rate handling. | Flexe, with Golem preserving controls | FLEXE-AUD-001 / existing FX-062. Capture both sides of the host ring, counts/rates/deadlines and clock domains; no waveform workaround or new VDPlet ABI. Existing timing issue FX-032 is a separate dependency. |
| CNS-007 | Assess a source-compatible P4 module target, with RISC-V packages, loader/relocation/cache support and target discovery. | Joint feasibility: Golem API/package semantics; Extender P4 backend and EMOS route | EXT056-001 / coordinate PORT-003. Xtensa packages cannot run on P4. Proposed additive CPU/compiler profile INFO queries are discovery-time work; IDs are not allocated here. No deployment or production replacement. |
| CNS-008 | Consider finite native flight calculation kernels and engine sound. | FSIM, if later selected | FSIM-001/002. Existing buffers/math and stock audio or copied PCM suffice. Measure complete tick/wire/refill cost; no whole-flight-model migration or extra background module. |
| CNS-009 | Consider scoped native colour-row transactions only if the existing rectangle-copy route fails to deliver the needed benefit. | Golem generic contract with Rally and other consumers | RLY056-003, EXT056-002. Existing pixels_write still copies. Any later lease must define layout, sync bits/lane ordering, bounds, hidden-page ownership, conflicts and automatic release. Do not silently change old mono-row semantics. |
| CNS-010 | Keep current native wireframe services and completed culling work; no new hook. | Existing wireframe owner | WIRE-001. More clipping/visibility/solid rendering would be optional module algorithms, not grounds for a permanent scene-renderer ABI. |
| CNS-011 | Keep geometry, texture/UV and aerodynamic table authoring offline; no new firmware feature. | Existing modeler and FSIM consumer owners | MODEL-001. No measured runtime need for a solver/mesh-loader/aircraft-specific service; retain renderer ownership. |
| CNS-012 | Assess a synchronous loadable Pingo renderer, preserving flat/textured/mixed materials, clipping and depth. | FSIM packages the engine/scene adapter and checks image parity; Golem checks admission and generic storage/service gaps | FSIM-004. Measure actual relocatable code/data/dependencies and whole-call-chain stack; use caller-provisioned offset arenas and existing buffer/math services first. No presumed fit, limit increase, production migration or speedup. |
| CNS-013 | Research a bounded generic asynchronous compute-job facility, separately from module loading. | Golem / agon-vdplet generic lifecycle and worker boundary; FSIM supplies the renderer use case | FSIM-005. Reuse deferred GOLEM-043 if selected. Admission, pinned code/resources, serialized instances, worker-safe services, completion, backpressure and safe retirement precede any experiment. Not a second buffered interpreter or unrestricted background callbacks. |
| CNS-014 | Establish how a completed offscreen buffer becomes a drawable mutable bitmap, and when its storage can be reused. | Golem generic storage/lifetime contract with FSIM consumer tests | FSIM-004. First verify/document existing backing-buffer mutation; add a checked copy/publication service only if that cannot meet the need. Consolidate with VDPL-005. Direct screen rectangles alone do not supply an offscreen bitmap contract. |
| CNS-015 | Review Wolf3DOrig's proposed synchronous renderer module and app-owned full-frame workspace. | Wolf owns the renderer port and consumer qualification; Golem owns generic contract interpretation | Additional Wolf handoff, outside the original roster. Existing buffer/pixel services appear sufficient; reuse CNS-001/002. Actual module fit, workspace allocation, full-frame cost and physical/lifecycle gates remain unproved. No new hook or implementation assignment. |

## Current boundary decisions to preserve

1. A pixels-only build does not yet exist. Separate capability bits are present,
   but the full candidate's combined media helper and build composition still
   couple pixel and richer PCM availability. Existing full/lean/base behavior
   must not silently change during a future separation.
2. Upstream Pingo is synchronous; FSIM added its worker/compositor wrapper
   separately. A synchronous loadable renderer is a credible first assessment.
   It does not preserve the current overlap by itself: owned image slots,
   copying and presentation lifetimes still need explicit handling. FSIM's
   follow-up requests generic async research as a separate facility, rather
   than making async a prerequisite for proving the renderer module.
3. The copied PCM sink already covers native continuous synthesis. The new
   request is ordinary VDU access to it, retaining one-producer ownership and
   prompt backpressure; it is not a request to install the full scheduled engine.
4. Framebuffer leases are substantially more invasive than SDK/doc composition.
   Test the existing generic rectangle service in the actual consumer before
   selecting a new writable-row API. Keep interfaces about formats and surfaces,
   not one screen mode or one application's road renderer.
5. Audio service correctness, mixer consumption, I2S output and host listening
   are separate. No response diagnoses the reported distortion as a firmware
   problem or authorizes an audio repair. Flexe's existing diagnostic is the
   proposed next owner action.
6. Every improvement needs matched whole-application measurements where speed
   matters. Existing wireframe/race/flight numbers use different fixtures and
   clocks and are not cross-project rankings or hardware forecasts.

## Pingo feasibility and asynchronous jobs

The [FSIM follow-up](/Agon/mystuff/agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-41-07_fsim-pingo_golem056-pingo-module-and-generic-async-two.md)
adds two distinct decisions to the original return. Preserve the current FSIM
renderer and FLIGHT-065/069 investigation while assessing these options.

1. **Synchronous module first (CNS-012/014).** FSIM reports 31,555 mapped bytes
   for the renderer plus its command owner. This suggests code fit might be
   practical, but is not a relocatable module size or dependency budget. Its
   current worker reserves 8,192 stack bytes; that is not measured peak usage,
   and does not qualify the VDPlet's 2,048-byte stack ceiling. An actual module
   build and whole-call-chain assessment must resolve both before execution or
   any proposed limit change. Keep bulk meshes, textures, prepared state,
   colour/depth and work storage in caller-owned PSRAM-capable offset arenas.
   Reconstruct pointers within each invocation and stay within four leases;
   do not import firmware allocator addresses or retain expired pointers.
2. **Offscreen output and publication (CNS-014).** The renderer fills explicit
   colour/depth targets with dimensions, stride and capacity, returning cropped
   bounds, origin, sequence and status. Scene/HUD policy and page swaps remain
   with the application. First establish whether modifying the host-created
   bitmap's backing buffer provides the required association and lifetime. If
   it does, documentation and consumer adaptation may suffice. Otherwise
   assess a generic checked buffer-to-bitmap copy/publication hook. Define when
   copying finishes and storage can be reused; expose no private graphics
   pointers. Pixel writes to the screen solve a different part of the pipeline.
3. **Optional generic async facility (CNS-013).** A single compute worker and
   bounded queue could let command processing and eZ80 work continue during a
   long finite native job. Proposed SUBMIT/STATE/COLLECT operations are research
   shapes, not allocated ABI calls. Pin code and declared resources until the
   job stops; specify mutation conflicts, serialized module state, copied
   arguments, results and backpressure. Cancel queued jobs or use cooperative
   checkpoints; drain running work before unload. Start with math/buffer work,
   not assumed thread-safe Canvas/VDU/audio calls. Output-ready, copy-complete
   and presented are separate milestones. Native code remains trusted.
4. **Broader value and costs.** The same job model could serve finite image
   decoding/conversion, procedural tiles, numerical analysis, physics,
   pathfinding or emulation slices. PCM playback is already asynchronous; jobs
   would make sample production nonblocking. A long render sharing the worker
   could instead harm audio deadlines. Account for worker stack, queue and
   retained memory, command latency and scheduling; no throughput improvement
   follows merely from adding a worker. Physical timing remains separate.

The first proposed deliverable is a bounded feasibility/contract report with
actual module/storage/stack evidence and a clear remaining-hook list. FSIM owns
engine packaging, scene arenas, image correctness and presentation; Golem owns
generic admission, bitmap storage and any later async lifecycle. Module parity
must include flat, textured and mixed scenes, depth/near-plane/cropping cases,
errors, repeated reset/retirement and matched whole-frame/wire cost. Async
implementation would require its own reviewed contract and release under
[GOLEM-043](../GOLEM-043.md). No such work starts from this report alone.

## Recommended work split and order

1. **Golem / agon-vdplet first:** CNS-002 discovery/fence documentation, then a
   bounded CNS-001 plan for independent pixel/PCM accessors and pixels-only build
   composition. Keep existing helpers/table layouts and base/lean/full behavior
   compatible; measure complete linked image/RAM cost. This addresses three
   independent consumers without installing the heavier audio candidate.
2. **Rally, when dispatched:** CNS-004 round-trip reduction in existing
   INC-001-05, then the separate CNS-005/AFF03 prepared-bitmap experiment if
   selected. These need no new firmware service. Measure actual wire bytes,
   replies, waits and complete-race behavior; retain diagnostics and real fences.
3. **Flexe, when dispatched:** CNS-006 via existing FX-062, diagnosing production
   versus delivery before changing audio behavior. Golem supplies the preserved
   matched controls; no duplicate audio or firmware repair project is needed.
4. **Golem follow-on design:** CNS-003 lean ordinary-VDU PCM frontend. The same
   copied sink and ownership semantics should serve module-free applications;
   command allocation and reply/copy-completion design precede implementation.
   Native6502 speaker adoption already has a suitable sink and is separate.
5. **Joint Pingo feasibility, if selected:** assess CNS-012/014 as a synchronous
   module first, with actual stack/module fit and bitmap lifetime evidence.
   Follow with the separate CNS-013 / GOLEM-043 async design only if useful
   overlap justifies its runtime cost. Preserve current Pingo and its performance
   investigation until an independently qualified alternative exists.
6. **Defer until selected or justified:** P4 RISC-V target feasibility (CNS-007),
   native colour-row leases (CNS-009), and optional FSIM kernels/audio (CNS-008).
   First exercise the existing colour-copy candidate. Preserve current Pingo,
   Extender/P4 and physical qualification gates.
7. **No new work from this consultation:** wireframe and modeler (CNS-010/011).
   Their existing tasks and human reviews continue under their own authority.

This ordering is a recommendation for the Author, not dispatch authority. The
shared pixel work and application/emulator work can be separate approved tranches;
whole-frame timing should avoid contention and use matching fixtures/clocks.

## Additional Wolf3DOrig handoff

After the seven invited sessions returned, Wolf sent an Author-requested
requirements handoff on its own `wolf3d-vdplet-migration` thread. Its
[static runtime review](wolf3d-review.md) confirms the proposed 90 KiB workspace
and 76,800-byte full-frame write have no module-data or 16-bit API size ceiling.
Actual allocation, module fit, stack safety and full-frame time still need proof.
It identifies no Wolf-specific firmware hook.

The review distinguishes synchronous pixel copying from queued VDU drawing,
requires mode136 plus an explicit swap for double buffering, and qualifies ERROR
handling: keeping the displayed frame requires withholding the swap; there is
no rollback of module state or the hidden buffer. Existing target evidence uses
a 16x2 hidden-buffer probe, not physical or full-frame Wolf qualification.

The exact message and review state are indexed in
[supplemental-returns.json](supplemental-returns.json). The Author approved the
technical reply and chat notification, both delivered, and then added Wolf3DOrig
to the [approved consultation list](consultation-authorization.md). This expands
the current participant list without changing the original frozen invitation
set. No conversion, dispatch or deployment is started.

## Dispatch gate and central tracking

All seven sessions have replied; their eight reports include the Pingo follow-up.
There are no unresolved identities or missing
responses. [coordination.md](coordination.md) records their exact message IDs,
content-review state and the empty assignment register. Original messages remain
immutable; [assessments.json](assessments.json) preserves proposal IDs and owners.

The Author and Golem now review this complete set here. Only approved groups
become dispatched assignments. Reuse existing owner tasks where identified,
send substantive assignment/acceptance/completion mail plus a chat notification,
and track acknowledgement, progress, completion evidence and remaining blockers
centrally. Peer TODOs remain authoritative for implementation; the Author does
not need to relay work between sessions.

## Delivery exceptions

Rally and6502 initially reported automatic approval-review rejections of their
local mailbox sends. The Author explicitly approved Rally's draft and then all
seven consultation reports, including relevant project paths and measurements.
Both durable replies have now arrived. [Verifiable authorization](consultation-authorization.md)
records that approval; it grants no implementation or external-publication scope.

The Author later specifically approved publication to both named repositories
after requesting commit/push. That subsequent scope is recorded in
[consultation-authorization.md](consultation-authorization.md); the consultation
and mailbox checkpoints are published. Feature implementation remains gated.
