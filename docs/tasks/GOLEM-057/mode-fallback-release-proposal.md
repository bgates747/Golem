# Release a failed display before constructing its fallback

Yes: a small change confined to the existing height-failure branch can remove
the fallback construction overlap without changing successful mode transitions.
Destroy its Canvas, stop/dispose the failed controller, invalidate the cached
colour depth, and only then request the existing previous-mode fallback. This
is a plausible remedy for the suspected fallback allocation pressure. The
allocation journal has not established that cause, and this is a proposal,
not an implementation or a general out-of-memory recovery guarantee.

## Exact scope and source route

1. In frozen010 `vdu_mode`, a recognized request returning 2 means
   `changeResolution` reached its final height mismatch check after resolution
   setup/Canvas creation. The requested controller remains installed. Existing
   `changeMode(videoMode)` then constructs the fallback object before
   `updateVGAController` stops/deletes that failed controller. Consequently its
   row pools and other resources still compete with the fallback constructor's
   default palette allocation. The observed request20→previous11 is this
   cross-depth route; the journal must decide whether its lookup allocation
   actually returned null.
2. The cleanup should execute only for this returned height-failure result 2.
   A result of −1 can mean invalid mode or teletext initialization failure;
   factory failure 1 can leave the prior controller alive. Do not dispose it
   indiscriminately.
   Preserve the first successful path exactly, including its allocation order,
   display setup, callbacks and final MODE information.
3. Use the stock `end()` followed by unique_ptr reset, as the normal controller
   replacement already does. `end` stops GPIO/IRQ and the drawing task, frees
   ordinary viewport/DMA state, and virtual `freeViewPort` releases the per-line
   DMA buffers. Destruction then releases palette/list/line-pointer resources.
   This reduces the overlap peak; it does not add a second framebuffer or alter
   any allocator capabilities, aliases, module limits or scanout loop.

The exact source remains pinned by
[the existing review receipt](mode-admission-source-pins.json):
[mode fallback](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/vdu.h:300),
[controller replacement](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/agon_screen.h:186),
[height check](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/video/agon_screen.h:249),
[stop/free](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgabasecontroller.cpp:127),
and [palette destructor](/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp:81).
The original b42/a10 artifacts and both diagnostic arms remain unchanged.

## Proposed code shape

This is an insertion sketch, not a complete replacement MODE handler:

```cpp
// Private helper: only after changeResolution returned its height error.
static void releaseFailedResolution() {
    canvas.reset();
    if (_VGAController) {
        _VGAController->end();
        _VGAController.reset();
    }
    _VGAColourDepth = uint8_t(-1); // Existing "no current depth" sentinel.
}

// Inside the existing errVal != 0 block, before its existing first retry:
if (errVal == 2) releaseFailedResolution();
errVal = changeMode(videoMode);

// If that retry also failed, perform the same release for error 2 before
// the existing mode1 fallback. Keep its current policy separately identified;
// this sketch does not claim arbitrary final-fallback failure is safe.
```

1. **Invalidate depth.** `updateVGAController` immediately returns true when
   requested colours equal `_VGAColourDepth`. Resetting only the pointer would
   make a same-depth fallback skip construction and later dereference null.
   The 255 sentinel forces a fresh object after disposal. Leave `videoMode`
   unchanged: `changeMode` publishes it only on success, so it remains the
   previous successful mode for the existing first retry. A local copy is
   optional but must be captured before any final-fallback assignment.
2. **Dispose Canvas first.** It retains a raw controller pointer; the pinned
   Canvas has no custom destructor action. Removing it first avoids leaving
   that wrapper attached to a deleted controller. Do not call drawing, surface
   queries, palette restoration or the MODE epilogue during the null interval.
   The following successful `changeResolution` replaces dimensions/scales and
   creates a new Canvas as usual.
3. **Stop before deletion; construct afterward.** Shared scanline aliases can
   temporarily reference old storage, so the failed controller's ISR/task must
   be stopped before its memory is freed. Do not manually clear every driver
   static. Existing VGA2/4/8/16 destructors clear their class instance pointer;
   constructing after deletion avoids a destructor clobbering its replacement.
   A later successful resolution repopulates row aliases and normal mouse/
   context binding. No callback or event-pump step belongs between disposal
   and reconstruction.

## Limits and blockers kept open

1. The captured null-palette ISR identifies the bad lookup use; it does not yet
   identify an allocation failure or prove this overlap caused it. Compare the
   pending matched journal arms first. A nonnull allocation disappearing later
   would redirect the investigation; releasing memory is not a remedy for an
   unexplained later pointer loss.
2. This path runs only if resolution returns2. It cannot rescue a constructor
   dereference, zero-height scanout failure or stalled task which prevents that
   return. The existing `end` waits for background work; a bounded target run
   must prove the failed candidate reaches cleanup and teardown completes.
3. Stock cleanup is not a proof of complete disposal under every partial
   allocation failure. Existing DMA-head/early-allocation and cursor ownership
   questions remain. Likewise lookup/list/queue/task/IRQ readiness and arbitrary
   extreme allocation failures remain outside this change.
4. It reconstructs the previous mode after the old controller was already lost.
   It does not preserve old pixels, custom palette/list, contexts, callbacks or
   an untouched prior object. Final-fallback failure still needs an explicit
   recovery contract before making any broad graceful-refusal claim. Do not
   run the existing graphics epilogue against a null/unusable display and call
   that successful recovery.
5. Raw SBGR2222 intentionally lacks a lookup plane. Do not combine this small
   cleanup-order experiment with a blanket nonnull-palette guard or the larger
   readiness/transaction design. No allocation-capability or mapper change is
   part of this proposal.

## Proportional validation before considering adoption

1. A focused lifetime/order test should establish: success never calls the new
   helper; result 2 disposes Canvas→end→controller before any fallback factory;
   depth becomes 255; the old numeric mode remains available for retry; invalid
   mode/factory errors do not destroy a healthy controller. Include same-depth
   height failure so the stale-depth fast path cannot be missed. This can use
   small ownership fakes; no allocator refactor or exhaustive fault campaign.
2. Freeze a separate original010-based repair candidate after the journal result
   supports this route. Keep module/client/backend/mode/load order and the
   original/control/journal evidence intact. In the same bounded failed request,
   require actual restored 11 plus fresh decoded VGA frames, ordinary readback,
   unchanged retained module state and normal FINI/unload. MODE reply alone is
   insufficient. Use both engines and require normal stop/join and no new
   unexpected memory events; root owns the fresh central grant.
3. Retain one existing successful mode transition to confirm its observed pixels,
   mode reply and cleanup remain unchanged. Add one same-depth refusal control
   if it can be admitted through a narrowly frozen fixture. A small repeat
   cohort should check heap recovery rather than monotonic leakage. Measure
   the patch's actual image/static-memory cost; expect no new steady allocation,
   but do not assign unbuilt byte counts.

The small path is worth testing if the journal confirms fallback allocation
pressure. It can remain a narrow failure-order correction while general
readiness and transactional replacement stay deferred. No source change,
build, test run, emulator/Oryx access, mailbox/chat notification, profile edit,
commit or hardware operation was performed for this proposal.
