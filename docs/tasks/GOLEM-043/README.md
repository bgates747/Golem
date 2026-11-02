# Concurrent native work: static feasibility review

Keep the current synchronous renderer path for the priority consumers. A small
background computation worker is feasible in principle, but moving today's RUN
onto another task would make buffer and module lifetimes unsafe. The first
useful future experiment would run one finite pure computation against
private input/output copies while the command task continues handling ordinary
commands. It needs no Golem syntax change and must not move graphics or audio
control onto the worker. No speed benefit or spare ESP32 core is established.

This is the bounded research released by GOLEM-057 on 2026-10-30. It performs no
build, emulator or hardware run, runtime/consumer edit, ABI allocation or peer
message. The task's research questions are answered below; an executable
experiment remains a separate decision. TODO.md remains the work register.

## 1. Sources and applicability

The [source manifest](source-inputs.json) records 49 inspected files, their byte
hashes, checkout heads and dirty status. The runtime checkout is at `2541f10`,
with ongoing uncommitted VDPL-010/011 changes. Its HEAD is **not** a publication
pin for those working bytes. The owned pixels export is separately pinned by
its [build receipt](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-source.json):
VDP `1056ee38`, graphics `ac2dd598`, exported runtime hash `c472db2d…` and image
`b42ae55f…`. Generated build paths are reading locations, not maintained APIs.
The installed framework identifies Arduino ESP32 2.0.14 and ESP-IDF 4.4.6;
PlatformIO selects espressif32 6.6.0. These pins supersede the task's earlier
4.3.4 documentation reference.

FSIM's current module adapter and its accepted compositor are additional
read-only examples. The module is still an owner-controlled candidate, not a
new concurrency qualification. Later source changes require checking the
manifest and these ownership conclusions again.

## 2. What already runs concurrently

These are source configurations, not a live task trace or CPU-load measurement.
The selected ESP-IDF task-stack units are bytes. The old firmware comment that
priority 3 is the highest is not the installed configuration: `configMAX_PRIORITIES`
is 25.

| Owner | Core/priority/stack in the inspected sources | Consequence |
| --- | --- | --- |
| VDU command processor, buffered programs and native entries | `processLoop`: core 0, priority 3, 4,096-byte stack | A native entry occupies this task until it returns. It does not acquire a separate stack from its module declaration. |
| Stock channel/envelope driver | `audioDriver`: core 0, priority 3, 2,048-byte stack; iterates enabled channels and delays 1 ms | Audio processing already has an independent task. Moving computation cannot presume that core 0 is free. |
| Queued graphics primitives | Graphics task: `CoreUsage::quietCore()`, priority 5, 3,000-byte stack | In this configuration the selected quiet core is 0. Queued rasterization is already separate from interpreting commands. |
| VGA paletted-controller interrupt/DMA | I2S1 interrupt on graphics' selected intensive core, 1 here | Core 1 already performs video work; its name is not a reservation for a new renderer. |
| Stock sound DMA refill interrupt | I2S0 interrupt on the selected quiet core, 0 here | A new worker must preserve audio refill opportunities and short critical sections. |
| Arduino setup/loop task | core 1, priority 1; configured loop stack 8,192 bytes | VDP's loop sleeps after setup; this does not establish total core 1 idle capacity. |
| Keyboard/mouse support | Optional virtual-key conversion: unpinned priority 5, 1,500 bytes; mouse task: unpinned priority 5, 1,600 bytes | These and RTOS idle/system tasks also need scheduling time. Terminal support may create further tasks. |

Sources: [command task and watchdog setup](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/video.ino:99),
[audio task](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/agon_audio.h:39),
[audio configuration](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/agon.h:105),
[graphics task](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgabasecontroller.cpp:434),
[VGA interrupt](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp:167)
and the framework/keyboard/mouse files in the manifest.

`processNext()` handles ordinary events, keyboard/mouse, cursor state and VSYNC
callbacks on the command owner. Nested buffered execution's
`processAllAvailable()` deliberately skips the event queue. A second buffered
interpreter would therefore duplicate shared contexts, streams, callback state
and registry access, not merely add a computation task. Sources:
[processing loops](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/vdu_stream_processor.h:588).

## 3. Existing services are foreground services

| Area | Present guarantee | Worker restriction |
| --- | --- | --- |
| Module INIT/RUN/FINI and loader | One installed image, synchronous calls and sequential `busy` guard | No concurrent entry into that module, its globals or lifecycle. `busy` is an ordinary Boolean, not a cross-task lock. |
| Buffer acquire/release | Four scoped views; retain the owning block; reject conflicting overlapping SDK leases | Do not call against the global registry from a second task. Map mutation and byte mutation are not protected by those leases. |
| Mono rows, colour surface/pixels, VDU and native lines | Operate on the current Canvas, display/context or processor | Foreground only. Mode, palette, current bitmap and page can change while a worker runs. |
| PCM, copied sink and queue control | Existing task/ISR synchronization inside selected audio components | Foreground control remains the contract. A ring's internal lock does not make the runtime service table or mixer lifecycle a worker API. |
| INFO, READ16 and packet/event output | Queried/sent from the serialized command path | Poll and publish from the owner; no worker writes into the command stream or invokes buffered callbacks. |
| Pure numerical or image computation | Self-contained code can work on private bytes | Candidate worker class, after separate admission and ownership support. No existing ABI advertises worker-safe service calls. |

`call()` sets global processor/argument/operation state, calls module code and
then closes all frame/buffer views **on return**. Starting a task and returning
from this function would immediately invalidate those views. `install()` copies
the admitted ELF text/data into runtime allocations; `unload()` calls FINI then
disposes them. The module's source upload buffer and its executable image are
different lifetimes. Sources:
[runtime call/lifecycle](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/vdplet_runtime.h:247),
[image allocations and disposal](/Agon/mystuff/agon-vdplet/src/vdplet_image.h:31),
[public host table](/Agon/mystuff/agon-vdplet/include/vdplet.h:33).

Holding a `shared_ptr` keeps an allocation alive; it does not freeze its bytes.
Stock buffer writes/adjustments, clear-all, replacement, consolidation and
aliases do not obey the native lease table. Even mutation of an unrelated ID
can modify the shared registry. Protecting only a list of IDs, or putting a
mutex around `buffer_acquire`, is insufficient. Sources:
[registry](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/buffers.h:15),
[byte writes](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/buffer_stream.h:92),
[clear](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/vdu_buffered.h:398).

Canvas update sections suspend background primitive processing and can deadlock
if another caller fills the queue without draining it. `pixels_write` obtains
current controller mappings and pauses/drains that queue; it is a copied-pixel
operation, not a thread-safe external framebuffer lease. Sources:
[Canvas update semantics](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/canvas.cpp:85),
[pixel copy](/Agon/mystuff/agon-vdplet/src/vdplet_color.h:43).

## 4. Small ownership model for a possible experiment

This is a proposed boundary, not allocated wire commands or an implemented API.

1. Admit at most **one outstanding job** initially. The command owner validates
   the module/generation, finite entry, argument sizes and all input/output
   spans before admission. The private job budget is explicit and admission
   can return BUSY or no-memory. No unbounded queue or silent replacement.
2. Resolve the stock buffers and copy immutable inputs on the command owner.
   Copy the short argument descriptor too. Give the worker separate private
   result/work storage; never return a current buffer/frame lease early and
   continue using its pointer. Alias validation uses allocation/span identity,
   not just differing IDs. For the first test, snapshot all live inputs rather
   than add guards to every stock mutator.
3. Retain the admitted module's text, data/BSS and entry address until the job
   has returned and its result is retired. Serialize synchronous module RUN,
   INIT/FINI, LOAD/UNLOAD and rebind against that job; ordinary unrelated VDU and
   buffered commands may continue. A module with static workspace is still
   one instance even when it exports multiple entry IDs.
4. Use job-local control and views. Today's global `Runtime`/service callbacks
   cannot simply be reused from the worker. A separately reviewed private
   adapter could expose only the copied arguments and job-owned spans; it
   must never look up the live buffer map or call graphics/audio services.
   Cached foreground host callbacks must not become an off-thread escape.
   Public compatibility/discovery needs definition before general exposure.
5. Publish output bytes, status, caller sequence and module/job generation
   through a short synchronized handoff. Use proper acquire/release or RTOS
   synchronization; `volatile` and an unsynchronized ready flag are insufficient.
   Keep control/lock objects internal; bulk copies stay outside spinlocks.
6. The foreground owner collects a complete successful result and validates
   any destination against its **current** mode, palette and dimensions before
   copying/publishing. Private results contain offsets and counts, not borrowed
   pointers into a module. Failed results never authorize drawing stale bytes.
   Mode changes during independent computation can cause publication to reject
   or discard an obsolete result without freeing a still-running job.
7. Retire only after all consumers release the output. A copied-pixel call
   consumes its source by return; queued bitmap drawing can retain source
   storage until the drawing fence. Neither boundary proves visible scanout.
   Pinning a private result does not automatically pin an installed bitmap's
   backing or solve its reuse rules.

Logical progression is idle → running → succeeded/failed → collected/held →
retired. Status polling is sufficient; terminal status stays observable until
retirement, and stale/duplicate identities cannot refer to a newer job. There
is no free queue slot merely because computation ended while output is held.
A later compute-N+1/copy-N pipeline would require a second independently owned
result slot. It is deliberately outside the smallest one-job experiment.

## 5. Lifetime, stop and failure limits

1. Stop admission first. Let a running entry return; do not block the command
   task while holding a lock the worker needs. Then consume or discard output,
   retire resources, call FINI on the owner and release the image. LOAD or
   UNLOAD must return BUSY while the image remains pinned. Holding a shared
   lock across computation or a drawing wait would introduce priority-inversion
   or deadlock paths; the private worker should need only short control handoffs.
2. A timeout may report a stalled job, but cannot safely free its code, stack,
   workspace or inputs. Arbitrary native execution is trusted. Forced task
   deletion, cancellation inside libm, or disposing executable memory under a
   running instruction is not a recovery mechanism. The current firmware also
   disables its core watchdogs unless explicitly configured otherwise.
3. Initially omit running cancellation. A later cooperative stop requires
   explicit kernel checkpoints and acknowledgement before reuse. No callback,
   process isolation, rollback or hard execution deadline is supplied by the
   existing runtime. A fault may still stop the firmware.
4. Separate compute return, successful result publication, copied/drawn source
   consumption, and presented frame. Audio enqueue/PLAY acknowledgement is a
   separate boundary again; playback already continues after foreground calls.
5. An application reset must quiesce/retire its job before calling any reset
   entry that changes the same module state. A firmware reset discards the
   entire runtime; prior handles/results cannot be resumed. Rediscovery and
   reload belong to the application, rather than treating reset as a successful
   job completion or a timed unload.

The version-matched [ESP-IDF task guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/freertos-smp.html#deletion)
also warns against deleting a task running on another core with unknown held
resources. No forced-unload promise follows from ordinary RTOS preemption.

## 6. Resource and scheduling implications

1. A worker adds a task stack, TCB, synchronization/control state and retained
   job arenas. The default internal stack competes with command, audio, DMA
   and display allocations; moving snapshots to PSRAM does not remove that
   cost. Module text remains executable internal memory and module data remains
   internal byte-accessible memory under the current loader. Existing text,
   data and 2-KiB module stack admission limits do not automatically increase.
2. For one job, budget `stack + TCB/control + copied inputs + work + result`.
   Two-result overlap adds another result allocation and longer retention.
   A 4-KiB task stack is only an illustrative starting reservation, not a proven
   safe stack or measured implementation cost. No flash/DRAM delta is measured
   because no worker is implemented. Prefer an opt-in, lazily allocated
   experiment rather than a permanent task in the lean base firmware.
3. The pinned FSIM adapter illustrates bulk cost: work requires
   `8192 + align4(width*height) + 4*width*height` bytes, plus a 64-byte output
   header and cropped colour/depth. A 320×240 work arena is 392,192 bytes before
   scene, textures and outputs. These are source-derived extents, not a measured
   peak. Copying large immutable scenes or double-buffering depth can outweigh
   a small scheduling gain. No unrestricted zero-copy claim is appropriate.
4. Measure free **and largest** internal/executable/PSRAM blocks before admission,
   after warm operation and after retirement. Existing INFO 9–23 supplies heap
   classes, command/audio stack watermarks and task count. It does not measure
   a future worker's stack or per-core utilization; that needs separate
   task-local evidence without guessing new INFO IDs.
5. A worker on core 0 competes with command/audio and higher-priority drawing.
   The hardware command loop polls without an unconditional task delay, so a
   lower-priority worker on core 0 can starve while that owner remains ready.
   Core 1 competes with video interrupts and other tasks. Choose affinity and
   priority from measured interference; raising priority cannot create CPU
   capacity. Shared PSRAM/cache/memory bandwidth also limits overlap. Keep
   job control critical sections short and never copy a frame while holding
   an interrupt-disabling spinlock.
6. ESP-IDF 4.4.6 automatically pins a task that uses the hardware `float` FPU;
   float use in interrupts is unsupported by default. Choose worker affinity
   explicitly before admitting float kernels. The current IRAM load/call path
   orders instruction fetch with `memw/isync`; an executing worker needs the
   corresponding publication/first-call ordering on its core too.
   [Versioned FPU guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/freertos-smp.html#floating-point-usage).

The [versioned heap guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/system/mem_alloc.html)
supports capability-specific allocation and distinguishes D/IRAM from ordinary
byte memory. Its allocator being thread-safe does not make VDP registries or
Canvas thread-safe. [External-RAM restrictions](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/external-ram.html#restrictions)
are another reason to keep task/control storage internal and qualify actual
PSRAM behavior instead of treating memory classes as interchangeable.

## 7. Sequential options and language effect

| Option | Benefit | Cost or limit |
| --- | --- | --- |
| Keep finite synchronous RUN | Existing contracts, storage and consumers remain usable; queued graphics and copied audio already run separately | Long calls delay further command/event processing. Current priority. |
| Smaller cooperative entries | Return command ownership between bounded chunks with existing ABI and application pacing | More calls/state bookkeeping; does not create simultaneous native computation. Consider first if latency becomes a problem. |
| One firmware-managed pure-compute worker | Can keep command intake responsive during a finite independent job | New private job/lifetime/service boundary, stack/snapshots and qualification; benefit unmeasured. |
| Arbitrary parallel native/VDU/buffered execution | No justified first use case here | Shared registry, context, assets, callbacks and presentation would need a much broader redesign. Defer. |

No new Golem syntax is needed to assess the first three options. An application
can schedule the same pure computational kernel sequentially on stock buffered
commands or, where supported, through a native job adapter. Explicit snapshots,
result/error publication and ordering are still necessary; source portability
does not promise identical behavior for code that reads simultaneous shared
mutation. The experiment should begin with C and the runtime boundary, leaving
the maintained compiler's semantics and resident package contract intact.

FSIM's current candidate has static `pg_active`/`pg_views` and erases pointers
before returning. It supports offscreen rendering plus foreground screen/copy
entries. Rendering one entry while invoking its bitmap-copy entry would race
the same static state. Its accepted integrated worker/compositor is an example
of an application-owned pipeline, not evidence that this module or public
service table is reentrant. Sources:
[module state](/Agon/mystuff/agon-fsim/utilities/pingo-vdplet/module.c:30),
[module dispatch/cleanup](/Agon/mystuff/agon-fsim/utilities/pingo-vdplet/module.c:160),
[existing application worker](/Agon/mystuff/agon-fsim/utilities/corsair-backdrop-target/scanout.h:132).

## 8. Bounded experiment and matched test protocol

Recommend **deferring worker implementation until synchronous FSIM/6502
qualification and a measured command-latency need are available**. The smallest
later experiment should isolate one finite numeric/colour-conversion kernel
over a modest private arena, using one loaded module and one outstanding job.
Exclude direct framebuffer access, audio control, general callbacks, recursive
tasks and zero-copy shared-buffer publication. Keep the experiment opt-in.

1. Freeze one kernel, source/module, all input snapshots, work counts, buffer
   extents and the same complete foreground drawing/audio workload. Compare
   sequential whole-call, cooperative chunks and worker invocation without
   changing precision, mesh/material content, frame count or completion fences.
2. Run correctness first: identical result bytes, statuses, caller sequences,
   iteration counts and final pixels. Record submit/admission, queue wait,
   compute, collection/copy, drawing consumption and presentation independently.
   Compare both unpaced throughput and identically paced end-to-end runs;
   never compare one paced path to another unpaced path.
3. Measure command-response latency during work, not just kernel execution.
   Separate guest monotonic time from host-wall/audio delivery and physical
   scanout. Record command/audio/worker stack watermarks, task count, heap
   free/largest blocks, held snapshots/results and retirement recovery.
4. Exercise BUSY/backpressure, stale module/job handles, duplicate collection,
   invalid spans and aliases, failure after partial private output, mode changes
   before publication, delayed result retirement and attempts to unload while
   running. In snapshot mode, changing/clearing/consolidating original input IDs
   after admission must leave accepted private input bytes unchanged. A held
   result cannot be overwritten by a second submission.
5. Verify that no global service is called off-owner and that normal command
   replies/input events remain ordered. Stop during work must retain resources
   until a normal return. Test any intentional hang only in a later disposable
   setup with explicit reset recovery; do not describe a timeout as cleanup.
6. Use host race/sanitizer checks for the private state machine, then actual
   Xtensa/RTOS functional checks. The inspected Flexe adapter enables native
   FreeRTOS and both cores but runs serial host instruction batches. Its
   interpreter treats acquire/release memory ordering as no-ops, and its JIT
   has a conditional-store helper. These can test handoffs and refusals, not
   prove simultaneous silicon races, cache contention or hardware speed. Fab's
   native firmware module is a different host-thread execution environment.
   Sources: [Flexe core loop](/Agon/mystuff/flexe-agon-emulator/adapter/flexe_vdp.c:288),
   [instruction semantics](/Agon/mystuff/flexe-agon-emulator/vendor/flexe/src/xtensa.c:2505).
7. A separately authorized hardware comparison must establish responsiveness,
   audio/video stability and a useful whole-workload benefit for the extra RAM.
   Reject or defer the worker if ownership cannot stay this narrow, actual
   stacks/storage do not fit, or coordination/copy costs remove its value.
   No present evidence establishes physical timing or a net speedup.

This review answers CNS-013's first design questions. Golem/agon-vdplet would
own any later scheduler, admission, worker service boundary and image lifetime.
FSIM/6502 retain their kernels, arenas and presentation policy. No asynchronous
wire/service IDs, delegated implementation or consumer migration is created
by this research.
