# Loaded display admission: source review

The frozen code can start a partly allocated replacement display before deciding
that its requested mode failed. Its fallback then constructs another controller
while the failed one's memory is still held. This is a credible failure route,
not the demonstrated cause of the recorded stall. The smallest useful repair
would check readiness and safely reconstruct the previous mode after releasing
failed resources. That is stronger than an ISR null guard, but it still cannot
promise an untouched old controller. A true transaction needs staged resources
and has a substantial peak-memory tradeoff.

This is the read-only A2G002 review under GOLEM-057. No implementation, new
instrumentation, emulator, Oryx access, mailbox, publication or hardware action
was performed. Palette21 and the separate CPU/cache controls remain independent.

## 1. Evidence and its limit

1. The inspected image is exact frozen `b42ae55f...`, ELF `a10f7989...`, upstream
   `1056ee38...`, graphics `ac2dd598...`, feature `pixels`. Full identities and
   audited source hashes are in `mode-admission-source-pins.json`. Earlier
   6502/Flexe reports were read before this review.
2. The retained EMPTY run admits mode 20. The LOADED run completes INIT/COLD/64
   original-ROM instructions, requests 20 and receives actual 11/320x240/2 colours.
   Its GP reply exists; the later timeout is specifically fresh VGA frames.
   No Apple RENDER occurs. Its finalized metadata records 137,152,480 unmatched
   reads, with only the first 32 addresses retained: `(0,4,4,0)` repeated eight
   times. No captured PC/core/precise onset or allocation return exists.
3. Flexe's static ELF match to the two 64-bit palette lookups in the VGA2 ISR is
   a strong hypothesis. This report does not relabel those static PCs as an
   observed origin, assert that every omitted read used 0/4, or prove a physical
   out-of-memory condition. The reported allocator pools overlap. The separate
   D/IRAM alias-model issue is not connected to this table by retained pointers.
4. The owner reports module text 54,312 bytes while EMPTY successful20 reports
   executable free 39,052/largest 31,732. In that reported context, merely reversing
   load/mode order is not a demonstrated coexistence solution. Error handling
   cannot manufacture capacity or justify a module-policy increase.

## 2. Where admission loses the old display

All locations below refer to the pinned owned export, not an edited upstream.
Primary files: [screen ownership](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/agon_screen.h:186),
[MODE handler](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/vdu.h:300),
[palette construction](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp:62),
[viewport allocation](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgabasecontroller.cpp:442).

| Point | Actual behavior | Consequence |
| --- | --- | --- |
| `video/vdu.h:300–317`, `vdu_mode` | Clears the old screen and waits; clears teletext/VSYNC registration before attempting the requested mode; retries the old mode and then mode1 | This already changes old image/context before successful admission. The final mode1 result is ignored. |
| `video/agon_screen.h:186–204`, `updateVGAController` | Constructs the new colour-depth object while the old one lives; then updates depth, ends/deletes the old object and begins the candidate | A nonnull C++ object is the only factory check. The old controller is gone before candidate resolution readiness. |
| `agon_screen.h:216–250`, `changeResolution` | Deletes Canvas, calls void `setResolution`, enables background drawing, allocates a new Canvas and publishes dimensions/scales; only then checks full height | Return2 is late. Candidate scanout and derived global state can already be live. |
| `vgabasecontroller.cpp:376–435` | `setResolution` first calls `end()`, even for the same-depth/same-object path; allocates buffers and starts a drawing task without propagating their status | Keeping the old unique_ptr is insufficient: its old rows were freed. There is no status-returning prepare/commit operation. |
| `vgapalettedcontroller.cpp:143–178` | Sets global row pointers, initializes palette/list and starts GPIO/ISR before screen-level height admission | The failure needs to be rejected before activation, not only when the application next renders. |
| `agon_screen.h:459–466` | Updates videoMode only on success but restores palette for every recognized request, even resolution error2 | A fallback report of the old numeric mode does not prove old controller/palette identity or health. |

`vdu_mode` reconstructs a prior mode; it does not roll back to a retained old
controller or recover its pixels. Reconstructing mode 11 while a partial VGA64
candidate still owns its rows is the relevant peak-allocation issue here.

## 3. Concrete unchecked/partial resource paths

1. `vgapalettedcontroller.cpp:62–77` allocates the line-pointer array and palette
   entries, ignores `createPalette(0)`'s result, then creates the signal list.
   For lookup formats, a failed default plane is erased at `:260–275`.
   `createSignalList` at `:361–377` uses `m_signalMaps[0]` when the requested key
   is absent, which can reinsert a null default entry. `setItemInPalette` at
   `:214–227` retries only an absent key, so an existing null entry skips retry.
   That is a real forced-failure hazard; this run did not capture that failure.
2. **Readiness must be format-aware.** Actual 64-colour mode uses
   `VGA64Controller`, a `VGAPalettedController` subclass with SBGR2222 and default
   signalTableSize 0 (`vga64controller.cpp:61–65`). No lookup plane is intended.
   Its ISR at `:706–755` copies raw rows and never reads palette signals.
   Blanket rejection of null default signals in every paletted subclass would
   break successful 64-colour modes. Lookup formats PALETTE2/4/8/16 need nonnull
   lookup planes; raw SBGR2222 needs valid raw rows/line buffers instead. Common
   code currently still needs a nonnull list node because it updates that node.
3. `createSignalList` does not check its allocated node before writing it;
   `updateSignalList` dereferences its incoming node. The line-pointer array and
   each DMA line allocation (`vgapalettedcontroller.cpp:123–129`) also need checked
   status/partial cleanup. A null ISR lookup guard alone misses these earlier
   failure sites and the zero-height modulo in scanout.
4. `vgabasecontroller.cpp:392–435` ignores the bool results from descriptor
   resizing, then fills buffers and starts work. `setDMABuffersCount` at
   `:198–215` dereferences an unchecked head allocation and assigns realloc
   directly over live owners before checking failure. A checked replacement
   must retain old pointers until realloc succeeds. Pool/header/row-array
   allocations at `:442–488` are not all checked before dereference. Reduced
   viewport height is an intentional partial admission path, not whole-mode
   success. The 4000-byte largest-block reserve is a heuristic, not a guarantee
   that later palette, Canvas, task or DMA allocations succeed.
5. `end`/`freeBuffers`/`freeViewPort` (`vgabasecontroller.cpp:127–177`) are tied
   to current DMA/count state; a new partial-construction owner must release
   every allocation even if the normal fully-initialized path was never reached.
   The visible cleanup also does not explicitly release the DMA-head allocation.
   Do not assume a destructor alone implements a complete rollback ledger.
6. Base construction allocates cursor/queue-related state (`displaycontroller.cpp:
   471–501`); task and interrupt creation status is also not propagated through
   the screen API. Factory `new`/Canvas allocation do not provide an explicit
   readiness result. Checking just a nonnull controller address is incomplete.
7. Driver constructors/destructors publish/clear class static instance pointers
   (`vga2controller.cpp:92–103`), and activation publishes global row aliases,
   GPIO/I2S/interrupt state. A future staged factory must defer or safely restore
   that publication; retaining an old object next to a candidate is not enough.

For a new failure check, the right place is before starting GPIO/IRQ or exposing
new Canvas/state. Returning early from an ISR without its acknowledgement can
create an interrupt storm; returning zero for a bad emulated read or mapping
address 0 would conceal the symptom rather than repair admission.

## 4. Recommended bounded first repair, conditional on origin proof

1. Freeze a separate owned candidate only after the independent origin/allocation
   observation identifies the relevant route. Keep b42/a10 and the consumer
   bytes as controls; do not patch the emulator mapping, bypass mode guards,
   change artwork/palette, raise module limits or add a frame loop workaround.
2. Add a format-aware, status-returning readiness path to construction and
   resolution preparation. Check all resources used by the selected scanout;
   initialize nullable owners first and stop on the first failure. Preserve
   successful allocation order/layout where possible. Do not start GPIO/ISR,
   enable background work or publish Canvas/dimensions until readiness is true.
   Palette-map lookup must not create a null entry as a substitute for success.
3. Make partial disposal idempotent and explicit for palette/list, line buffers,
   viewport pools/pointers, DMA arrays/head, queue/task/IRQ and Canvas. Capture
   the previous mode specification before mutation. If the destructive path
   already retired it, stop/release the failed candidate **before** constructing
   the previous mode again, avoiding the currently overlapping fallback peak.
4. Verify previous-mode reconstruction through the same readiness result before
   reporting it usable or resetting contexts/palette against it. Preserve normal
   successful-mode clearing, dimensions, palette, MODE reply and callbacks. A
   failure in the final mode 1 fallback cannot be ignored. If reconstruction also
   fails, keep invalid scanout stopped and report failure rather than running
   through invalid objects; a specific recoverable console/recovery contract
   would need a separately accepted design.

This is **checked refusal and reconstruction**, not a transaction guarantee.
It can preserve a usable previous mode when its reconstruction succeeds; it
cannot promise the old controller, image, palette/custom lists or callbacks were
untouched. The suggested repair is not complete until that distinction and the
failure report/recovery behavior are accepted. A one-branch ISR guard would be
smaller but does not meet even this bounded readiness/cleanup goal.

## 5. What true old-display preservation would require

1. Prepare an inert candidate/resource bundle without publishing driver statics,
   touching shared GPIO/I2S/interrupts, freeing old rows, clearing old pixels or
   resetting application contexts. Validate its full geometry, format, palette/
   signals, scanout buffers and fallible activation prerequisites first.
2. Only commit after validation: drain/quiesce the old controller, switch the
   prepared resources/hardware/aliases in a bounded step, bind the new Canvas,
   then retire old owners. Any fallible activation after that point needs an
   explicit restoration path to still-held old resources. Handle same-depth
   transitions too; current `setResolution` cannot be used as an inert prepare.
3. This requires a library resource/activation split, not just a screen-wrapper
   null check. It retains old and new memory at once unless it implements a
   more complex compatible-block reuse ledger. For mode 20 the new raw payload
   alone is 196,608 internal bytes, plus row pointers, four 512-byte DMA lines,
   blanking/descriptors, task/queue/controller/Canvas. PSRAM is not a substitute
   for allocations with internal/DMA requirements.
4. Consequently a simple old-retaining transaction can refuse a transition
   which currently succeeds after freeing the old display. It cannot silently
   claim identical maximum mode admission. If retaining every historical
   successful transition and preserving the untouched old display on every
   failure are both hard requirements, that is a larger allocator/ownership
   project. A heap-total/largest-block preflight alone cannot guarantee it.

Recommendation: first review the bounded checked-reconstruction contract and
measure it if the suspected route is actually captured. Do not label that a
transactional mode loader. Adopt full staging only with explicit acceptance of
its memory/admission tradeoff or a separately justified reuse design.

## 6. Proportional validation proposal

No test below was executed by this review. A new frozen implementation, source/
artifact pins and an explicit central Oryx grant must precede target work.

1. Host fault injection against the actual extracted implementation: one failed
   default plane for a lookup format; one signal-list node; one line-pointer/
   DMA line; one descriptor realloc; zero/partial viewport or row-pointer failure.
   Prove no activation/published success, no leaked/double-freed partial owners,
   and no accidental rejection of intentionally palette-free SBGR2222. Preserve
   the old owner on pre-commit failure if claiming true staging. Check the
   same-depth path and successful following transition, not only a fresh object.
2. Inject one candidate failure, then permit allocations so the previous mode
   can be reconstructed. Check exact requested/actual mode and usable palette,
   fresh decoded VGA frames, ordinary pixel reply and keyboard. Separately fail
   reconstruction too and prove its declared quiescent/recovery result rather
   than reporting a fictitious healthy mode. Validate task/IRQ/queue creation
   failures only along the chosen bounded implementation's actual status path.
3. Exact target cohort after origin proof: a small successful cross-depth control,
   a same-depth resize control, one deliberately refused allocation/geometry
   admission with the unchanged loaded module, then normal module RUN/FINI/unload
   and an empty-slot check. Retain original ROM/current-state comparisons and
   actual surface/mode/resource snapshots. Separate GP receipt from fresh-frame
   delivery. A fallback mode number alone is not success. Do not repeat known
   mode 20 merely to reproduce its fallback or call a test fixture ROM execution.
4. Use both execution engines on the already qualified cache-control backend,
   retain before/after source/artifact pins and zero unexpected memory events.
   Observe bounded allocator recovery/largest blocks, active tasks and repeated
   failure/recovery without monotonic leakage. Pools overlap; do not add them.
   Use a useful finite repeat count, not an exhaustive mode/performance campaign.
5. Stage a stopped human keyboard/cursor review once automated checks pass.
   Applications restore the visible cursor on exit. Physical hardware needs its
   own release; emulator success does not establish its allocator/scanout result.

A small readiness/checked-reconstruction patch should add mode-change code and
bounded status/ownership metadata, with no per-pixel work, new thread or extra
steady-state framebuffer. Exact flash/DRAM/IRAM cost needs matched builds.
True staging adds the candidate's peak allocations beside the old display even
if steady-state bytes are similar. Preserve actual successful mode behavior;
measure any new conservative refusal rather than silently calling it equivalent.
