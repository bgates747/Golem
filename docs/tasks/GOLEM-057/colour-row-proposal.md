# Scoped colour rows — first experiment proposal

Expose the current hidden drawing page for one finite native call so a renderer
can store its final pixels there directly. This may reduce temporary colour
storage and copying. It also adds native addressing and synchronization-bit work
to each store, so speed must be measured. Preserve the existing copied path and
all prior services. This is a contract proposal for FSIM review, not a released
ABI or permission to start firmware/consumer implementation.

## Scope and ownership

The Author requested this consultation in FSIM; durable request
2026-10-30_20-08-46_fsim-pingo_pingo-colour-framebuffer-borrow-request-one
is recorded under GOLEM-057/CNS-009. Golem owns generic SDK/runtime/admission
and standalone qualification. FSIM owns an optional Pingo store backend, the
engine-change ledger and matched Corsair comparison. Existing consumer/runtime
proofs and copies remain controls. No async work, raw firmware imports, custom
worker, unbounded lifetime, physical flash or publication follows.

The first experiment accepts an actual SBGR2222/RGB222 native drawing surface
with double buffering, physical coordinates and finite synchronous RUN. MODE136
is a test case. Discover actual properties: a mode request alone proves nothing.
Packed palette surfaces remain unsupported by this first borrowing service.

## Suggested small ABI extension

Prefer one appended optional `color_frame(context, operation, descriptor)`
callback with BEGIN, ROW and END operations. Candidate allocation is ABI1.6,
capability256 and an84-byte table extending the80-byte1.5prefix; these numbers
remain proposed until contract freeze. One checked SDK accessor verifies
major/minor/extent/capability/callback. The package builder admits the new bit
and selects minor6; old module default bytes and old service views stay exact.

Add opt-in `--borrow-rows` only to pixel-enabled feature selections. It is off
by default, including full/base/lean/pixels/pixels-lean defaults. Existing wire
protocols and no-option feature choices retain their behavior. Measure linked
flash/static RAM deltas from matched same-source builds. No scratch allocation
is needed in the service; an application may keep its row-pointer table in its
existing arena, outside the limited native stack.

The descriptor’s exact fields/layout need final source/consumer review. It must
carry struct extent, generation token, physical rectangle, relative row index,
native format, returned row-group base/span, first-pixel bias, address XOR and
colour/preserve masks. A56-byte candidate fits these fields with32-bit pointers.
This describes actual storage rather than pretending it is a contiguous logical
RGBA image. Callers needing general surface dimensions/flags also declare the
existing PIXELS capability and use surface_info (buffers+pixels+rows=273). Do not use reserved fields or widen old descriptors silently.

## Address and lifetime contract

BEGIN validates RUN, declared capability, actual format/buffering, rectangle,
generation admission before granting a lease. Pause/drain existing Canvas work
once, THEN capture/revalidate the actual drawing page and every row mapping:
pending Canvas work can include SwapBuffers. Release the pause if post-drain
validation fails, without exposing a token or writing new pixels. Retain the
resulting stable drawing mappings until END or automatic call cleanup. Reuse
the existing exclusive frame-transaction state: mono and colour cannot overlap.
Add an explicit mono/colour transaction kind; both APIs must reject cross-kind
ROW/END tokens before interpreting a pointer or releasing the other API’s lease. This lease is separate from the four
ordinary buffer leases and creates no second task, ring or scheduler.

ROW uses the active token and a relative row index. Return the base of the
four-pixel storage group containing the rectangle’s left edge, its available
group-rounded span and the requested left-edge bias. For current native layout,
`pixel_xor=2`, `colour_mask=0x3f`, `preserve_mask=0xc0`. A logical pixel at
relative x uses `row[(bias+x)^2]`; retain its current upper bits and replace
only its six colour bits. Pointer alignment and group padding must be explicit.
The group base must be four-byte aligned. Its span is round_up(bias+width,4).
Rows can come from different memory pools: no row0-plus-stride/contiguous-image
promise exists. A240-row pointer table consumes960caller-arena bytes.
Only pixels in the requested rectangle may change; adjacent bytes included by
group alignment remain borrowed context, not writable extra pixels.

ROW pointers may be kept only inside the active lease and current RUN. END
invalidates all pointers/tokens and releases Canvas; automatic normal/error
call cleanup does the same. Stale/double END and stale ROW reject. Shared
monotonic lease generation never silently wraps. No raw pointer survives RUN,
FINI/unload or mode change. Trusted native code still has responsibility to obey
these bounds and preserve sync bits; the API provides no memory sandbox.

While either frame lease is active, VDU submission, endpoint drawing and copied
pixel writes return BUSY before effects. Read-only surface discovery may remain
available. Ordinary buffer access remains available within its existing limits.
No mode change or swap is serviced during finite RUN; the application swaps
only after successful completion. A failed RUN leaves a possibly partial hidden
page; the host must not present it. There is no general pixel rollback.

## Pingo experiment

Preserve clipping, transforms, depth, material and lighting arithmetic. Change
only the optional final-store path plus target binding/clearing as required.
Fetch row pointers once per frame/row, inline native pixel addressing and masks,
and use ordinary arena storage for depth. No callback per shaded pixel and no
hidden framebuffer-sized colour scratch. Record every engine change.

The first result is the existing opaque-on-clear preview. Match RGB output and
depth against copied control for the same flat/textured/mixed Corsair poses.
Clear only the agreed rectangle, preserve its border and all outside pixels,
verify sync bits and hidden-before-swap behavior. Transparency against a live
background, dynamic flight/compositor overlap and other native formats remain
later contracts. Existing Pingo packed colours have six RGB bits plus high
alpha bits; final stores must preserve controller sync bits, not copy alpha.

## Qualification and timing

Standalone SDK controls cover unsupported format/single page, bad extents and
geometry, odd origins/widths and row-group padding, last row/edge, overlapping
leases, blocked old drawing services, stale tokens, failed-call/unload cleanup
and exact old-module compatibility. Compare physical decoded output and raw
sync bits with copied writes, including visible/hidden pages and outside-area
canaries. Qualify the actual target before FSIM relies on the candidate.

Compare identical scenes/mode/clear/presentation boundaries. Separate cold
loads, output extraction/hash/readback, repeated copy calls and target clock
observations from the final raster-plus-presentation measurement. Prefer a
small agreed target-cycle measurement over noisy host-request subtraction.
Report temporary/retained RAM and native text/private stack alongside timing;
no speed gain is presumed. Oryx always requires a fresh central lease; keep a
readable human fixture stopped and leave implementation uncommitted until the
existing human/publication gates are met.

Before code: FSIM and generic-source review must settle descriptor semantics,
actual row mapping/pause order, build flag/capability allocation and a finite
qualification contract. Register the generic task in agon-vdplet and freeze
that contract there; this compiler task remains coordination only.

## Generic source review

Read-only review of pinned vdp-glac2dd598 confirms the mechanism and finds no
implementation blocker for this bounded design. vgabasecontroller.h exposes
row XOR2 and drawing versus visible row arrays; vgabasecontroller.cpp aligns
viewport width to4, allocates multiple row pools and swaps the arrays/DMA lists.
Canvas beginUpdate suspends processing; waitCompletion(false) drains preceding
primitives, which can include SwapBuffers. Capture actual rows afterward.
Suspension disables the EOF interrupt, not DMA scanout, so the first lease must
be hidden-page only. END resumes; it never swaps. Independently scheduled
drawing/sprite/cursor changes are excluded by the host contract during the lease.

Builder capability<=255/minor<=5 and loader minor<=5 admission must be extended
coherently for the proposed capability/minor; package and wire widths already
accommodate them. This is not a wire-format change. No source, build or target
execution occurred for this review.
