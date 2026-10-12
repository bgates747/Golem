# SYN-001 — Readable drawing proposals

EX-030 through EX-036 in [examples.golem](../GOLEM-003/examples/examples.golem)
cover all five graphics families identified in DRW-001. Propose named geometry
and scrolling operations, retain a general plot escape route, and use full
bitmap buffer IDs. Every sample declares and initializes its own data and states
assembly's required setup. The Author approved this syntax direction after
review. GOLEM-018 subsequently implemented points, lines and convex quads;
their [maintained contract](../../design/resident-drawing.md) records qualification
and the pending human review. Other families remain proposals, and the constraints
and admission decisions below still apply to their later implementation.

## Proposed surface

| ID | Proposed call | Inputs and behavior | Existing Rally coverage |
| --- | --- | --- | --- |
| SYN-F01 | draw_point(colour,x,y) | Logical colour uint8 bounded 0..63, signed16 coordinates; sets foreground paint mode 0 and plots an absolute point. | Friendly specialization of PlotPoint using stock PLOT 69 and explicit GCOL. |
| SYN-F02 | draw_line(colour,x1,y1,x2,y2) | Same colour/coordinate types; absolute move to the first endpoint, then solid foreground line to the second, including both endpoints. | General PlotPoint can express PLOT 4 followed by PLOT 5. Both endpoints are supplied so the call does not depend on prior cursor history. |
| SYN-F03 | plot(code,x,y) | Compile-time byte code plus signed16 coordinates. Preserve stock mode, relative/absolute addressing, cursor-history and current colour semantics. | Rename PlotPoint to reflect its actual general PLOT capability. Named functions handle common operations. |
| SYN-F04 | fill_quad(colour,x1,y1,x2,y2,x3,y3,x4,y4) | Colour as above; four signed16 vertices around a convex perimeter. Set foreground GCOL 0 and fill two triangles sharing a diagonal. | PlotQuad's mechanism, with public perimeter order mapped to triangle-strip order and a proposed ordinary bounded colour value rather than a literal-only colour argument. |
| SYN-F05 | draw_bitmap(bitmapBuffer,x,y[,transform]) | uint16 resource ID 1..65534 and signed16 screen-pixel placement; optional finite nonsingular 3x3 binary32 affine matrix. | DrawBitmap's behavior with a proposed full 16-bit resource selector instead of its legacy 0..255 bitmap index. |
| SYN-F06 | graphics_viewport(left,top,right,bottom) | Nonnegative signed16 pixel bounds, inclusive; order is the programmer's natural left/top/right/bottom. Synchronize Canvas clipping with the required non-drawing absolute PLOT. | GraphicsViewport, including its existing cursor side effect. |
| SYN-F07 | scroll_left(amount), scroll_right(amount), scroll_up(amount), scroll_down(amount) | uint8 pixel count; scroll the current graphics viewport and fill the exposed area using current text background state. Count must fit the region along the chosen axis. | Readable wrappers for ScrollGraphics directions 1,0,3,2 respectively. |

These sample spellings follow the existing snake_case math proposals. Builtin
lookup would follow the language's case-insensitivity convention; identifiers
retain their existing case sensitivity. Ordinary bounded variables and matching
literals are proposed operands; no implicit variable declarations are involved.
The PLOT code remains compile-time because it selects behavior rather than a
runtime coordinate. Full expression support beyond current grammar is not
implied by these samples.

## Coordinates and graphics state

- COORD-001: For this first friendly drawing surface, assembly establishes pixel
  coordinates, origin (0,0), a valid display and viewport. The examples use a
  320x240 display where relevant. Geometry and bitmap placement consequently
  agree on a top-left origin and increasing Y downwards. These calls do not
  secretly switch coordinate systems or reset the origin.
- COORD-002: Named point/line/quad coordinates are absolute signed pixels. A
  line supplies both endpoints; a quad supplies all vertices. There is no
  dependence on cursor history to define their geometry. They still change
  cursor history and foreground drawing state as stock commands do.
- COORD-003: General plot deliberately retains stock semantics. Code 69 is
  merely the demonstrated absolute foreground point; it is not a new point enum
  or a dynamic opcode variable. Assembly sets GCOL explicitly for that sample.
  Other PLOT code groups can require prior cursor positions or other resources;
  the advanced caller supplies those preconditions.
- COORD-004: Viewport bounds are inclusive, but that does not make polygon
  rasterization inclusive on every edge. fill_quad retains stock two-triangle
  rasterization and applies no hidden one-pixel adjustment. For perimeter
  vertices A,B,C,D, emit Rally's PLOT 4,4,85,85 sequence in order A,B,D,C:
  the triangles are A,B,D and B,D,C, sharing diagonal B-D. Passing perimeter
  order directly to Rally's sequence would instead share edge B-C and draw
  the wrong shape. This is a fixed operand permutation, not a geometry search;
  the caller supplies a convex perimeter. A general rectangle or circle API
  is not additionally proposed in this subtask.
- COORD-005: graphics_viewport preserves Rally's clipping synchronization,
  including the extra cursor move. The examples explicitly restore the full
  display viewport; they do not imply a saved-state stack or restoration of an
  arbitrary previous viewport.
- COORD-006: Named operations do not promise to restore all graphics state.
  Bitmap drawing disables affine transformation after the draw; plain drawing
  also disables a previous transform before drawing. It leaves the selected
  bitmap changed. Jobs remain finite and serialized; assembly owns pacing and
  any completion mechanism.

## Initial values, resources and admission

Each block has its own declarations and initializer values; no preceding block
provides variables or required Golem execution. Assembly sends those initializers
and establishes the stated graphics/resource state before invoking the Sub.
Numeric arguments such as full-screen viewport endpoints are literals, not
undeclared variables. Examples are independent source fragments; the combined
examples.golem still intentionally does not compile.

The bitmap examples use ID 50000 as an illustrative build-assigned external
resource. The build reserves that ID from compiler allocation; assembly loads
and registers the bitmap there before invoking the job. A real build can bind
this value from its asset/container metadata and initialize the resident variable
accordingly. No Golem source filename, file loader or runtime name lookup is
introduced. Full-ID selection uses stock VDU 23,27,32,id; this is a deliberate
proposal beyond Rally's legacy selector, useful with AGNB's embedded IDs. It
does not require an AGNB format extension or custom VDP command. The resource
must remain valid until its draws finish, and compiler cleanup does not own it.

EX-034 initializes the affine matrix and rebuilds it from identity on every
invocation. Repeated calls therefore do not compound the scale. Both scale
intervals are positive and exclude zero; the resulting affine bottom row is
(0,0,1), with a nonzero determinant. General unproved rotations, singular
matrices and raw byte mutation of a cached transform are not promised support.
Typed matrix updates must invalidate/rebuild the firmware's inverse cache.

The viewport examples provide ordered, on-screen initial rectangles. Their
individual Bounds alone do not prove runtime left<=right or top<=bottom after
host updates. EX-036's region initially measures 280x60, and its amount is bounded
0..60 so the example is valid in every shown direction. If the host changes the
viewport, it must ensure the new count fits the corresponding inclusive extent;
0..255 storage alone does not prove this. Scrolling fills with the text background
colour, not the quad's foreground colour. Drawing four directions demonstrates
the four names; it is not an undo operation because exposed pixels are replaced.

These are ordinary API/type proposals, not a new error protocol. The eventual
implementation must choose how external inputs are admitted and how invalid
viewport order, scroll extent, resource availability and colour bounds fail.
Current Golem's arithmetic/table status codes do not implement those checks.
No automatic safe clamping, status code, display rollback or host acknowledgement
is claimed. Existing valid/trusted inputs should not receive redundant checks or
copy readback. That policy belongs to later implementation review.

## Coverage and validation

| ID | Inventory family | Example |
| --- | --- | --- |
| COV-001 | General PLOT and its readable point/line cases | EX-030, EX-031 |
| COV-002 | Filled quadrilateral | EX-032 |
| COV-003 | Bitmap, without and with affine transform | EX-033, EX-034 |
| COV-004 | Viewport and clipping synchronization | EX-035; also EX-036 |
| COV-005 | Scrolling in all four directions | EX-036 |

The earlier file contained transform math but no typed drawing calls. The added
examples preserve existing sample IDs and all previously qualified source blocks.
Reviewed each new block for explicit declarations/initializers, independent
assembly preconditions, type/range-fitting values and coverage of the five Rally
families. No compiler, emulator fixture or upstream reference was changed, and
these proposed statements were not compiled or visually qualified. DRW-001's
existing host evidence remains unchanged; rerunning it would not validate these
new spellings.

SYN-001 is complete and the Author approved its syntax direction. WIR-001
traffic/install/invocation analysis and REV-001 implementation options remain
subsequent subtasks. This document does not authorize implementation.
