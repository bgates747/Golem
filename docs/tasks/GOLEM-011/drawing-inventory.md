# DRW-001 — Rally typed drawing inventory

Rally implements five graphics intrinsics: general PLOT, a filled quadrilateral,
bitmap drawing, graphics viewport selection and scrolling. They already patch
and execute resident command templates using typed fields. The useful work for
Golem is readable operations and explicit graphics/resource contracts. Existing
Golem has no typed drawing parser or backend. This inventory completes DRW-001;
syntax examples and wire-traffic accounting remain the next subtasks.

## Implemented operation surface

These are existing Rally signatures, not new Golem syntax proposals. Rally s16,
u16, u8 and f32 correspond to signed/unsigned 16-bit, unsigned byte and binary32
storage. Coordinates must be named fields; arbitrary expressions and direct
coordinate literals are not accepted by these drawing intrinsics. Constant PLOT
codes and quadrilateral colours are exceptions, as shown below.

| ID | Existing Rally call | Accepted operands and generated action |
| --- | --- | --- |
| DRW-F01 | PlotPoint(mode,x,y) | Compile-time integer mode 0..255; x/y s16 fields. Patch two words into a private six-byte VDU 25 template, then call it. Despite its name, this exposes general PLOT codes, not just points. |
| DRW-F02 | PlotQuad(colour,x1,y1,x2,y2,x3,y3,x4,y4) | Constant logical colour 0..63 and eight s16 fields. Private 27-byte template: GCOL 0,colour, then absolute PLOT codes 4,4,85,85. Eight word patches and a call. Two filled triangles share the middle vertex pair, which must form the quad's diagonal; no hidden geometric adjustment. |
| DRW-F03 | DrawBitmap(bitmap,x,y[,matrix]) | Bitmap index in a u8/u16 field whose declared range fits 0..255; x/y s16. Optional initialized finite nonsingular 3x3 affine matrix. Patch index plus x/y into a private 23-byte select/affine/draw/reset template, then call it. |
| DRW-F04 | GraphicsViewport(left,top,right,bottom) | Four s16 fields with declared lower bounds at least zero. Patch VDU 24 in left,bottom,right,top wire order. Its private 15-byte template follows this with absolute PLOT 4 at (0,0) to synchronize Canvas clipping. |
| DRW-F05 | ScrollGraphics(direction,amount) | u8/u16 fields bounded within 0..3 and 0..255 respectively. Patch two bytes into VDU 23,7,2,direction,amount and call the five-byte template. Direction 0=right, 1=left, 2=down, 3=up; extent 2 selects the graphics viewport. |

Source: [hosted.hpp](/Agon/mystuff/golem-rally19/src/hosted.hpp), dispatch branches
1522–1653. [Rally's design](/Agon/mystuff/golem-rally19/docs/design/rally19-hosted-v1.md)
explains its primitive qualification and additional contracts. Template lengths
are installed VDP payload sizes, not recurring eZ80 traffic measurements.

## Graphics state and observable effects

| ID | Requirement or effect | Consequence for a readable interface |
| --- | --- | --- |
| STA-001 | PLOT uses the current logical/pixel coordinate system, graphics origin, viewport, paint mode/colour and cursor history. PlotPoint does not set these. Mode is a raw byte, including values with no defined drawing operation. | A friendly point/line/shape operation must state its coordinate/state contract. The name PlotPoint currently conceals a general command interface. |
| STA-002 | PlotQuad sets foreground GCOL paint mode 0 and the selected logical colour. Its absolute point sequence supplies all four vertices; it leaves graphics colour, mode and cursor history changed. | Colour is a palette index rather than an RGB value. Do not promise preservation of previous graphics state. |
| STA-003 | Filled polygons follow rasterizer edge rules. Rally records flat-top and right-edge exclusion and bottom-edge inclusion for its native polygon cases; filled rectangles differ. | Do not silently add/subtract a pixel to arbitrary geometry. Golem's inclusive array/range bounds do not define polygon raster coverage. |
| STA-004 | DrawBitmap uses direct bitmap placement in screen pixels; its draw command does not apply PLOT's logical coordinate scaling or origin conversion. Bitmap placement uses the top-left basis. | An apparently identical coordinate pair can differ between PLOT and raw bitmap placement unless the caller establishes the intended state. The future syntax must make this distinction understandable. |
| STA-005 | DrawBitmap selects the bitmap, selects/resets the affine transform, draws in Set paint mode, then disables the transform using ID 65535. Plain drawing disables a previously active transform before drawing as well. It does not restore the previous bitmap or transform. | Each draw has an explicit transform choice. The sentinel is an internal stock command operand, not an allocatable resource ID. |
| STA-006 | VDU 24 converts coordinates through the current mode/origin, changes the Context viewport, and does not itself update Canvas clipping. The extra non-drawing PLOT synchronizes it and pushes a new graphics cursor position. | Rally's viewport operation assumes the caller has established pixel coordinates and the intended origin. The cursor move is required behavior, including full-view restoration. |
| STA-007 | Out-of-screen or reversed viewport endpoints are ignored by firmware and preserve the old viewport. The extra PLOT still happens. Rally checks types/nonnegative bounds, not current screen dimensions or rectangle ordering. | This is existing stock behavior, not Golem's status-5 table-error behavior. A future drawing error contract requires an explicit decision. |
| STA-008 | Scroll uses the active graphics viewport and text background colour/paint options for the exposed area. Amount must fit the viewport's size on the selected axis. | A byte-fitting amount alone is insufficient. Rally documents a native harness crash for an oversized vertical move; do not infer clamping or a safe failure exit. |
| STA-009 | These private patch/call sequences contain no user callback/yield. They use mutable templates and shared VDP graphics state. Returning from the interpreter job does not itself prove scanout/drawing completion. | Preserve serialized finite jobs and assembly-owned pacing. A graphics function is not an automatic host acknowledgement or frame fence. |

The viewport and raw bitmap distinction is confirmed by the stock runtime paths:
VDU 24 calls toScreenCoordinates; PLOT calls pushPoint and setGraphicsOptions;
raw bitmap command 3 calls drawBitmap directly with compensateHeight=false and
forceSet=true. drawBitmap does not perform toScreenCoordinates or set a clipping
rectangle. Context::scrollRegion explicitly uses tbg/tpo.

Inspected primary runtime sources are the pinned Fab 1.2.4 tree referenced by
Rally and Golem's local Fab 1.2.5 tree. The pertinent 1.2.5 files are
[viewport.h](/Agon/mystuff/golem/.emulator/runtime/fab-1.2.5/src/vdp/vdp-console8/video/context/viewport.h),
[graphics.h](/Agon/mystuff/golem/.emulator/runtime/fab-1.2.5/src/vdp/vdp-console8/video/context/graphics.h),
[vdu.h](/Agon/mystuff/golem/.emulator/runtime/fab-1.2.5/src/vdp/vdp-console8/video/vdu.h)
and [vdu_sprites.h](/Agon/mystuff/golem/.emulator/runtime/fab-1.2.5/src/vdp/vdp-console8/video/vdu_sprites.h).
This source inspection is not a new visual qualification on either version.

## Resources and operand admission

| ID | Resource or operand | Existing Rally contract |
| --- | --- | --- |
| RES-001 | Input fields | Typed storage exists before the drawing job. Declared bounds support compile-time checks; these five operations do not validate arbitrary host writes against those bounds at each call. In particular, a bounded u16 bitmap/scroll operand is copied as one byte. |
| RES-002 | Internal templates | Compiler-owned mutable buffers, patched before each execution and included in package installation/cleanup. They are distinct from the instruction stream that patches them. |
| RES-003 | Bitmaps | Loaded and registered externally before use. The legacy byte selector maps n to resource ID 64000+n. Every ID allowed by the declared bitmap range must avoid compiler-owned IDs. External bitmap resources are excluded from compiler cleanup. No existence, dimensions or pixel-format check occurs in DrawBitmap. |
| RES-004 | Optional affine matrix | Owned matrix buffer with nine binary32 elements (36 bytes). Bounds must prove finite entries, bottom row exactly (0,0,1), a determinant interval excluding zero, and a finite inverse. Wrong shape, uninitialized/stale bounds or insufficient proof reject compilation. |
| RES-005 | Matrix inverse cache | Stock checkTransformBuffer may append a second 36-byte block for a cached inverse. Matrix updates must invalidate/rebuild that cache; Rally uses typed stock matrix operations for this. Raw byte patching of a live cached transform is unsupported. |
| RES-006 | Host graphics setup | Assembly must establish display mode, coordinate system/origin, viewport, palette/paint state and externally loaded resources. Relevant state may persist or be changed by the job; cleanup of Golem buffers does not restore display state. |

Rally's DrawBitmap is a byte-index API even though its field can be u16. Stock
VDP additionally provides a 16-bit bitmap selector; Rally does not expose that
through this intrinsic. This matters for AGNB: embedded buffer IDs must be
coordinated with the resource selector, rather than assumed to be legacy byte
indices. No AGNB format change or loader implementation belongs in DRW-001.

Affine setup is supported by Rally's existing Identity, Translate, Scale and
RotateDegrees matrix operations, plus matrix load/arithmetic and checked signed
stores. These are math dependencies rather than additional drawing operations.
RotateDegrees interval analysis may fail to prove DrawBitmap nonsingularity even
when a particular rotation is mathematically valid. Do not promise general
rotation support from the existence of both names.

No standalone typed intrinsic was found for setting mode/origin/palette/GCOL,
clearing graphics, sprite lifecycle, line thickness, frame swapping or completion
fencing. Stock PLOT can encode several shapes through PlotPoint and prior cursor
positions, but that does not provide named, self-contained line/circle/rectangle
functions. Bytes only initializes storage at bootstrap, and Call requires a
declared Program; Call does not directly accept a raw Buffer. Additional stock
VDU commands issued by the assembly host are not typed compiler intrinsics.

## Comparison with current Golem

| ID | Current Golem position | Gap identified by this inventory |
| --- | --- | --- |
| GAP-001 | Named assembly wrappers and owned/reserved buffer accounting now exist. | Reuse those contracts for future drawing jobs instead of introducing a new runtime owner or filename lookup. |
| GAP-002 | Scalar jobs implement a bounded float floor conversion and checked uint16 arithmetic. The separate table job stores/copies uint16/sint16 records. | No typed drawing statements, general standalone signed-coordinate job or composition of table access and drawing exists yet. A floor-to-integer implementation is not a drawing frontend. |
| GAP-003 | Current examples contain transform math and proposed storage/access operations. | Readable drawing examples remain SYN-001. This subtask leaves examples.golem untouched. |
| GAP-004 | Golem runtime errors currently preserve a failed assignment and end the job. | Rally's draw paths mostly rely on declared input validity and firmware behavior. Viewport/scroll/bitmap failures cannot be described as existing checked Golem errors. |
| GAP-005 | Assembly owns loading, invocation, pacing, results and cleanup. | The same boundary fits these five operations. Resource IDs and graphics-state preconditions must be exported/documented before implementation. |

The natural grouping for the later syntax discussion is PLOT/geometry,
bitmap-with-optional-transform, and viewport/scroll state. No new spellings or
implementation order are approved by this inventory. Wire savings depend on
which inputs are already resident and how many drawing operations a single job
performs; that accounting belongs to WIR-001.

## Evidence and review boundary

[drawing-audit.json](drawing-audit.json) records the exact Rally commit, source
hashes, both runtime-source identities, four existing host regression commands,
sanitizer results and timings. All four tests pass; build plus execution took
173.796 seconds on this host (local monotonic time, not a drawing benchmark). [audit_drawing.py](audit_drawing.py) builds into
this task's ignored build/ directory and does not alter Rally or upstream code.
The targeted tests cover PlotPoint via test_hosted, PlotQuad via test_elements,
DrawBitmap via test_bitmap, and viewport/scroll via test_viewport. They check
emitted templates, operand constraints, ownership and rejection behavior.

Rally's design separately reports earlier native comparisons: 24 bitmap image
pairs and 30 viewport/scroll image pairs. Those are historical primitive evidence,
not executions performed by this inventory. No emulator, hardware or graphical
window was launched here. No timing/visual claim about Golem's future drawing
implementation follows from host regression tests.

DRW-001 stops here for Author review. SYN-001 remains the next subtask: audit the
examples and propose readable syntax with all declarations/initial state shown.
WIR-001 and REV-001 remain outstanding in the authoritative task document.
