# GOLEM-038 — Generic runtime support findings

The existing runtime supplies the finite invocation and storage lifetime needed
by a Rally-owned autonomous service. It does **not** supply a scheduler,
cross-task synchronization, coherent player-snapshot publication, cancellation
of running native code, or collision-event delivery. No new compiler primitive
or public module-ABI change is presently demonstrated necessary. Rally should
first draft its serialized service boundary; a firmware-facing invocation adapter
is a possible small follow-on request, not an implementation delivered here.

This is a static review, completed 2026-10-14. No builds, benchmarks, emulator
runs, hardware actions or production edits were performed. Sources are pinned
in [review-inputs.json](review-inputs.json). SDK checkpoint is `9fbb21d`; the
actual composed package is the unchanged GOLEM-037 candidate `48eac7efe8fa51f4`.
Firmware call sites come from the builder's owned export of VDP commit
`1056ee38ec1cc68ed327bd32ee47d04b1c30597c`. Build-export paths are convenient
reading locations, not maintained integration interfaces.

## Services and ownership

| Area | Existing service | Caller obligation / unresolved boundary | Owner |
| --- | --- | --- | --- |
| Invocation | One slot; validated handle and bounded argument buffer; synchronous INIT/RUN/FINI | Serialize every invocation and lifecycle operation; select a supported entry; native code must return. No published firmware adapter yet. | Generic adapter: Golem SDK, if requested. Service scheduling: Rally. |
| Buffer views | Four lease slots; span/access/owner-alias checks; automatic cleanup on return | No cached pointers across calls; leases do not synchronize other tasks or stock buffer writers. | SDK maintains checks; Rally owns snapshot publication and buffer-use ordering. |
| Rendering | Scoped single-buffer PALETTE2 row access; current Rally helper instead prepares retained command lists | Keep commands/data/assets stable until their consumers finish; renderer completion is not display completion. | SDK owns row service; Rally owns drawing and completion policy. |
| Stop/cancel | BUSY rejection; normal-return cleanup; finite StopRace source entry | Stop scheduling, let active call return, drain queued drawing/callback work, then stop/unload. No preemption or timeout recovery service. | Rally service; generic change only on explicit request. |
| Reload/reset | FINI then disposal, fresh nonwrapping module handles; application buffers survive unload | Rebind new handle and module-private helper state. Ordinary buffer clear/reset is not module unload. | SDK lifecycle; Rally sequencing. |
| Events | Command replies through firmware packet sender; no module event callback in host ABI | Define collision mailbox, session/episode/rearm, delivery and stream routing separately. | Rally firmware/game protocol. |
| Clock/deadlines | VSync callbacks polled by the command processor | Polled frame change is not a counted 120-Hz timer; choose clock, backlog and acceptance policy. | Rally service and Author policy review. |

## Invocation and execution context

The installed wire handler calls `install`, `invoke` and `unload` directly.
`invoke` resolves a single-block argument buffer of at most 32 bytes and calls
the loaded native function synchronously. `call` sets `busy`, protects the
argument owner, invokes the function, then closes outstanding frame/buffer
leases and clears `busy` **after normal return**. There is no new task created
for a module and no exception/fault containment around the function call.
Sources: [runtime call and lifecycle](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:107),
[wire dispatcher](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:194).

In the pinned hardware firmware, `processLoop` is a core-0 task with a 4096-byte
stack; it calls `processor->processNext()`. Command dispatch reaches the new
selector through the builder's small integration patch. The native call uses
the caller's stack. The module's admitted stack declaration does not allocate
another stack. INFO 17 reports the current caller task's high-water mark, not a
separately reserved module allowance. Sources:
[task creation and loop](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/video.ino:99),
[builder integration](/Agon/mystuff/agon-vdplet/tools/build_firmware.py:77),
[INFO 17](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:179),
[declaration admission](/Agon/mystuff/agon-vdplet/src/vdplet_image.h:53).

`busy` is an ordinary `bool`, and runtime state/buffer registry access has no
lock in these paths. It guards sequential reentry; it does not authorize calls
from a second task or ISR. The ABI contains neither a scheduling callback nor
a thread-ownership API. Timer/ISR code should signal the serialized owner,
consistent with accepted contract clause 6. Sources:
[runtime state](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:14),
[buffer registry](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/buffers.h:15),
[public host structure](/Agon/mystuff/agon-vdplet/include/vdplet.h:80),
[accepted contract](../GOLEM-033/accepted-contract.md).

The named `Tick120` and `Render30` source entries do not themselves create
those execution rates. Generated assembly wrappers send commands, not wait
for completion. A future firmware caller must preserve the same result and
state contract even if it avoids serial command submission. Sources:
[native entry validation](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:52),
[assembly wrappers](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/calls.inc:20).

## Buffer lifetime and immutable snapshots

Acquisition retains a `shared_ptr` to a single contiguous block, checks spans,
and checks overlapping SDK leases by owning block rather than buffer ID.
Read/read overlap is allowed; overlapping writes conflict. RUN's argument
owner remains protected until entry return, even after generated code releases
its argument view early. All returned pointers expire on release or entry
return. These checks apply during INIT/FINI as well, while `busy` is set.
Sources: [acquire/release](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:36),
[argument-owner cleanup](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:111),
[generated early release](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:58).

Retaining storage does not freeze its bytes or synchronize publication. Stock
buffer writes and clears do not consult the SDK lease table; generated state
accesses are ordinary byte loads/stores. Therefore a simultaneous telemetry
writer could expose mixed old/new fields despite successful acquisition.
Rally must define a complete accepted snapshot and publish/copy it at the
serialized step boundary. A queue or separately synchronized staging storage
is a possible design choice; no specific mechanism or stale-input policy is
selected by this review. Sources:
[stock byte writes](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/buffer_stream.h:92),
[stock clear](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_buffered.h:398),
[generated byte access](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:4).

The composed renderer keeps a value-only private context. It reacquires assets
and outputs per entry; its retained snapshot is not the telemetry publication
mechanism. Preflight explicitly assumes retained allocation identities cannot
change mid-entry. Prepared output lists must also remain stable while the
firmware drawing consumer uses them. Sources:
[helper context](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/helper/rally_render.h:57),
[preflight](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/helper/rally_render.c:79),
[prepare](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/helper/rally_render.c:138).

The packed framebuffer service is limited to single-buffered PALETTE2, with
byte-aligned x/width and bounded rows. It begins a Canvas update and drains
pending work; row views expire at frame end/entry return. It supplies no swap,
general VDU execution, pacing or display-completion callback. It is **not**
Rally's mode-136 command-list renderer API. Source:
[frame services](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:66).

## Cancellation, unload and reset

There is no cancel opcode, cancellation query in the host table, timeout around
the native call, or forced-unload path. A stop packet cannot be dispatched by
the same command task while native code is still running. Returning an error
still reaches runtime lease cleanup; hanging, crashing or deleting the task is
not normal return and has no cleanup guarantee. Source:
[call/unload](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:107),
[execution contract](/Agon/mystuff/agon-vdplet/docs/API-reference.md:272).

`StopRace` is a finite application entry. Its helper hook clears binding state;
it does not cancel timers, stop future submissions or drain graphics. The
generated FINI entry simply returns success. Safe service teardown therefore
needs an application-owned sequence: stop accepting/scheduling work, let the
current entry return, prevent queued callbacks/submissions from restarting it,
finish consuming submitted drawing that uses retained data, run agreed
StopRace, then unload and release application resources. This is a proposed
ordering requirement for Rally's draft, not an implemented shutdown service.
Sources: [stop helper](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/helper/rally_render.c:183),
[FINI](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:55),
[buffer callbacks](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_buffered.h:2748).

UNLOAD calls FINI and frees text/data even if FINI returns an error, provided
FINI returns. LOAD while occupied rejects; successful loads increment a fresh
handle and admission stops rather than wrapping after 65,535 generations.
Native text/data/BSS disappear, but application buffers remain. Reload requires
updating installed RUN handles and reestablishing renderer bindings; do not
cache pointers into the old module. Conversely, clearing all VDP buffers does
not call UNLOAD, and can leave a loaded module with missing application data.
ResetRace is application logic, not a loader reset. Sources:
[install/unload](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:128),
[image disposal](/Agon/mystuff/agon-vdplet/src/vdplet_image.h:31),
[stock clear](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_buffered.h:398).

## Success, events and clock boundary

Keep runtime return, root interface/language status, helper completion and
last-success generation separate. Generated helper execution precedes root
success publication. Earlier source writes can survive a later error; final
root release can fail after success markers were written. Readiness must be
cleared before each attempted invocation, including rejection before entry,
and drawing must require all success channels plus matching session/request/
tick and fresh generation. A direct firmware adapter must preserve these
obligations; calling `invoke` alone does not perform wrapper preparation.
Sources: [root/interface contract](../GOLEM-034/README.md),
[helper marshalling](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:23),
[final publication](../GOLEM-037/binding/delivery/candidate-48eac7efe8fa51f4/native.c:1711),
[binding contract](../GOLEM-037/v03-CONTRACT.md).

The wire handler sends requested VDPL command replies through `send_packet`.
The module host table has no UART or event-send service. `send_packet` uses the
processor's current output stream, respects suppression and invokes sent-packet
buffer callbacks. Reusing it for collision events requires a firmware-owned
encoding/routing/callback policy, not a module writing arbitrary serial bytes.
No delivery/ack/retry/session/episode/rearm mechanism exists in this SDK ABI.
Sources: [VDPL reply sender](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:194),
[packet sender](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_stream_processor.h:517),
[stream output](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_stream_processor.h:286).

Existing VSync buffered callbacks run when `processNext` notices a changed
frame counter. `checkForVSYNC` tests inequality, not the number of elapsed
frames; long finite work can therefore coalesce frame observations. Nested
buffer execution does not process the ordinary event queue in its inner loop.
This is a reusable serialized callback mechanism, not proof of a 120-Hz
scheduler or delivery of every timer occurrence. Sources:
[command/event loop](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/vdu_stream_processor.h:588),
[frame observation](/Agon/mystuff/agon-vdplet/build/firmware/firmware-src/video/context.h:1312).

## Questions for Rally's D3 draft

1. Which exact owned firmware workspace integrates the service? Can the
   command task be the single owner of invocation, drawing, buffer mutation and
   lifecycle, or does the proposed design introduce another writer/task?
2. Will the owner reuse installed resident command wrappers, or require a
   small firmware-facing invocation adapter? For direct calls, identify exact
   validation/result/readiness obligations and requested interfaces first.
3. Where is incoming telemetry staged, and how does one complete accepted
   snapshot become immutable for a step? Define timestamps/staleness and
   backlog behavior in the game/service contract, not in buffer-acquire.
4. What counts as rendering finished: command-list preparation, queued Canvas
   work drained, swap submitted or displayed frame? When may output buffers,
   bitmaps, matrices and mode be reused or destroyed?
5. How are scheduled/queued callbacks quiesced before StopRace/unload? What
   policy applies if finite work overruns, fails, or never returns?
6. Which collision mailbox/event encoding and MOS receiver own session,
   episode, duplicate/stale rejection and explicit recovery rearm?

The immediate next dependency is Rally's design draft and Author policy review.
No generic cancellation primitive, new module ABI, scheduler implementation or
capacity campaign is commissioned by these findings. If the draft needs a
firmware adapter, Golem can review a separately scoped SDK request with exact
repository boundary and acceptance cases. Static evidence cannot establish
deadlines, concurrent integration behavior or hardware performance; those
checks belong to the later explicitly released service qualification.
