# Mutable stock bitmap backing for a finite VDPlet

A host-created bitmap already points at its stock buffer's pixel bytes. A finite
module can update those bytes through the existing checked buffer service and
draw the same bitmap through coverage profile 4. This provides the synchronous
Pingo publication path without another firmware callback. The application must
keep that storage unchanged until every consuming draw finishes. These are
source-backed findings; the small execution probe below remains to be run.

## Audited baseline

1. Runtime/SDK: `agon-vdplet` commit
   `2541f10a1f8b06688bcecbc6d22eeae607da2b63`.
2. Stock firmware: `tomm/agon-vdp`
   `1056ee38ec1cc68ed327bd32ee47d04b1c30597c`.
3. Stock graphics library: `AgonPlatform/vdp-gl`
   `ac2dd5986daf496c43ae8e7fe41836274aec54a0`. This is the actual ESP32 library
   in the pinned firmware export, not Fab's userspace replacement.
4. Exact local paths, hashes and stock-source identity checks are in
   [sources.json](sources.json). No source was modified and no emulator launched
   for this investigation.

## Association, layout and format

| ID | Existing behavior and implication |
| --- | --- |
| BMP-F01 | Host selects ID with `23,27,32,id:u16`, then creates its bitmap with `23,27,33,width:u16,height:u16,format:u8`. Creation uses the **same ID's** stock buffer, at byte offset zero. Format 1 is RGBA2222. It requires exactly one block and exactly `width*height` bytes for that format. There is no independent bitmap-source ID, offset or row stride. [VDP creation](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_sprites.h:188), [validation/association](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_sprites.h:288). |
| BMP-F02 | The `Bitmap` constructor defaults `copy=false`; it stores `data_` directly and sets `dataAllocated=false`. Stock creation does not override that default. There is no pixel snapshot or owning `BufferStream` reference inside the bitmap. Mutating the original block changes what future draws read. [Constructor declaration](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.h:530), [implementation](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:252). |
| BMP-F03 | `buffer_acquire(..., VDPL_WRITE, ...)` exposes that same block's bytes through a checked span. It allows one contiguous block, at most four live leases and disjoint read/write rules; the argument block cannot be written. It checks **native leases**, not pending Canvas bitmap consumers. Lease release/native return expires the module's pointer, while the application buffer remains installed. [Acquire/release](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:56), [return cleanup](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:249). |
| BMP-F04 | RGBA2222 bytes are `AABBGGRR`, one byte per pixel, row-major, stride equal to registered width. Alpha zero skips the pixel; any nonzero alpha draws it, rather than performing fractional blending. Use `0xC0 | rgb222` for opaque pixels and zero for transparent pixels. The pixels service's RGB222 is instead `00BBGGRR`: treating those bytes as RGBA2222 makes them transparent. [Layout](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.h:365), [alpha test/row indexing](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.h:2690), [RGB222 contract](/Agon/mystuff/agon-vdplet/src/vdplet_pixels.h:20). |
| BMP-F05 | Multi-block upload is valid preparation, but both native acquisition and bitmap creation require consolidation **before** use. Consolidation of several blocks allocates and copies a new contiguous block and removes existing bitmap users; consolidation of one block is a no-op. Recreate the bitmap after any actual replacement. Individual write/create wire lengths are 16-bit; this is not a 65,535-byte limit on the final consolidated bitmap. [Wire lengths](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_buffered.h:32), [consolidation](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_buffered.h:1242), [copy allocation](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/buffers.h:107). |
| BMP-F06 | Appending another upload does not mutate the established block: it adds another block. Ordinary buffer-copy replaces storage with new allocations and removes bitmap users. Clear/recreate/reconsolidate must never race a pending draw; removing an ID or installing another bitmap does not retarget a queued primitive's old raw pointers. In-place native mutation avoids all of this. [Append](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_buffered.h:300), [removal](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_buffered.h:385), [copy/replacement](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_buffered.h:1208). |

For a renderer's larger arena or padded output, either render directly into the
separate installed bitmap buffer using its registered width as pitch, or copy
the desired rows into it inside the same finite module. A subrectangle of an
arena cannot be registered by passing its arbitrary pointer or offset through
these existing commands. A fixed-size transparent target avoids changing the
bitmap object for every cropped frame. Clearing only the relevant previous and
new bounds is a consumer choice, requiring its own image-parity checks.

## Drawing, consumption and reuse

1. **Write before draw.** Host creates and registers the stable bitmap once.
   Before a new render, finish all older draws using that output slot. RUN
   acquires its backing block for WRITE, produces pixels, releases the lease,
   then can submit select/draw bytes through profile 4. Required module
   capabilities are buffers + VDU (`5`); colour-screen-copy capability `16`
   is not required for this bitmap path.
2. **Queueing is borrowing.** `Canvas::drawBitmap` queues a raw `Bitmap*`, whose
   data is also a raw pointer. Queue insertion copies neither object nor pixels.
   Matrix parameters for transformed draws are copied, but the bitmap remains
   borrowed. Single-buffer background drawing may execute later. Double-buffer
   ordinary bitmap draws currently execute immediately, but the common
   application contract should still use an explicit consumption boundary.
   [Canvas submission](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/canvas.cpp:604),
   [queue/direct selection](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:525),
   [matrix snapshots only](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:547).
3. **A stock non-vblank drain already exists.** Ordinary host-issued
   `VDU 23,0,&CA` calls `waitPlotCompletion()` with its default `false` argument,
   hence `Canvas::waitCompletion(false)` and `processPrimitives()`. That
   suspends background execution, consumes queued drawing synchronously, then
   resumes it. The trailing Refresh primitive is not a renewed borrow of this
   bitmap. The foreground command interpreter can next process a RUN that
   overwrites the same slot. `&CA` is **not** admitted by current native
   `vdu_submit`; send it from the eZ80 application.
   [CA dispatch](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_sys.h:366),
   [default argument](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/agon_screen.h:475),
   [drain](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/canvas.cpp:85),
   [queue processing](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:638).
4. **Ordered consumption is different from an observed reply.** Sending CA
   alone produces no completion packet. `draw → CA → next RUN` is ordered on
   that same processor/command stream, so reuse inside that next RUN follows
   the drain. A fresh following RUN reply or ordered screen-pixel query gives
   the host an observable boundary when needed. The latter itself drains
   drawing, reads the pixel and then runs relevant callbacks; keep those
   callbacks from scheduling new uses of the source. Do not confuse an old
   packet/stale MOS readiness flag or the preceding draw's RUN reply with
   completion of subsequent CA. [Pixel-query order](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/vdu_sys.h:446).
5. **Copy completion is different from presentation.** Once the last draw
   has consumed the bitmap, its pixels have been copied/rasterized into the
   display's drawing page and the offscreen slot can be reused. The visible
   scanout need not have reached those pixels. C3 performs the ordinary VSYNC
   swap in double-buffer mode and waits for a VSYNC processing boundary in
   single-buffer mode; it is a possible existing pacing/consumption boundary,
   not a reason to hold every offscreen buffer until physical display scanout.
   No native RUN return alone proves consumption of queued bitmap drawing.
   [C3 behavior](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/agon_screen.h:481),
   [swap](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/canvas.cpp:647).
6. **Use a finite ownership interval.** Keep the bitmap and backing block
   stable from draw submission through its consumption boundary; do not
   mutate, replace, consolidate, clear or recycle them during that interval.
   Native lease release/unload is not that boundary. This simple contract
   assumes no sprites, mapped characters, mouse cursor, callbacks, other
   processors or native/background jobs independently keep reusing the
   same bitmap. Active sprite scanout can keep reading its source indefinitely;
   detach/deactivate such consumers and finish their drawing before recycling.
   No lease tracker presently enforces Canvas ownership on the application's
   behalf. [Sprite source reads](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp:451).

Direct bitmap command 3 forces Set paint but inherits the current Canvas clip.
Bitmap PLOT establishes graphics clipping and its paint action; logical-coordinate
PLOT can compensate the bitmap's height. Stock drawing crops against the current
clip and screen. This is destination clipping, not a source-stride API. Keep
Pingo's output bounds/origin and presentation policy explicit, and disable any
inherited affine transform when testing plain publication. RGBA2222 supports
stock affine drawing; Native-format transformed drawing is not implemented.
[Context drawing](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/context/graphics.h:857),
[graphics clipping](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/video/context/graphics.h:38),
[destination clipping](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:1626),
[affine formats](/Agon/mystuff/agon-vdplet/build/vdpl-009d/firmware/firmware-src/.pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp:1820).

## Smallest meaningful execution probe

1. Host allocates one 117-byte block, registers it once as a 13×9 format-1
   bitmap and installs a separate bounded argument block. Use a patterned
   background, physical coordinates, known clipping and disabled affine state.
2. A real module WRITE-leases the backing block, fills asymmetric RGBA2222
   rows with transparent holes, releases it, then selects/draws the installed
   bitmap through profile 4. Host sends CA and obtains fresh pixel evidence.
   Check colours, holes, odd row width and one clipped draw independently.
3. Without recreating the bitmap, order CA before the second RUN, then replace
   the pixels with a visibly different pattern and draw again. Check the new
   result and unchanged surrounding desktop. Compare single- and double-buffer
   variants without claiming presentation until the host's appropriate swap.
4. Repeat from a two-upload preparation consolidated before registration.
   Verify native acquisition refuses a still-multi-block buffer; do not test
   unsafe clearing/replacement of an in-flight borrowed source. After a real
   replacement and re-registration **following the drain**, confirm the new
   association works. Finish with drain, unload, asset cleanup, cursor restored
   and an actual MOS return.

This probe qualifies mutable association and the application's lifetime order.
It does not qualify asynchronous jobs, every renderer layout, performance gains
or physical hardware. FSIM can build against the source-backed synchronous
contract now, retaining its production path until that execution evidence passes.
