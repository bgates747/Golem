# Resident drawing profile

The compiler now supports finite point/line/convex-quad jobs with initialized
packed inputs and named assembly calls. Host, sanitizer, actual assembler and
stock Pi/Oryx headless qualification pass. The Author confirmed MOS keyboard
response and that the expected-shape reference matches the display. Saved
human-run results match; human visual and keyboard review pass.
GOLEM-019 extends the compiler with plain bitmap calls and uint16 resource
fields. Bitmap emission, assembly integration and stock bitmap pixel checks
pass (18,526 checks, 18,869 saved bytes). The Author accepted the corrected static display and then the GOLEM-020 animated
Wolf demo, including MOS keyboard review. Retrieved human runs match their
expected results; commit and push were explicitly approved on 2026-10-12.
Assembly owns graphics setup, installation, updates, invocation and pacing.
See [GOLEM-018](../tasks/GOLEM-018/README.md) for the staged demo and evidence.

## Source and compilation

```golem
uint8 colour Bounds(0,63) = 11
sint16 x = 40
sint16 y = 30
Sub drawDemo
    draw_point(colour, x, y)
    draw_line(colour, 60, 30, 140, 90)
    fill_quad(colour, 180, 40, 260, 40, 280, 100, 160, 100)
EndSub
```

Assembly must establish pixel coordinates, origin (0,0), display/palette and a
valid viewport before calling. The example fits a 320x240 display. Compile with:

```bash
make -C src check
build/golemc --resident --emit-asm docs/tasks/GOLEM-018/geometry.golem build/geometry
```

The standalone drawing profile is selected when resident source begins with
uint8, sint16 or Sub. A source beginning with uint16 selects this profile when
its first Sub statement is a supported drawing call; existing scalar sources
retain their backend. Globals, if present, are uint8/sint16/uint16, explicitly initialized
and optionally bounded by Bounds(low,high), inclusive. Colour fields must have
declared bounds within 0..63; coordinates must be sint16. Matching integer literals
are accepted as operands. A literal-only Sub needs no input buffer. Source admits
one finite Sub containing one or more drawing calls. Keywords and builtin names
are case-insensitive; variable names remain case-sensitive. Statements have no
terminators, and newlines are whitespace. Unknown statements, wrong types, invalid
literals, unsupported mixed profiles and trailing source reject compilation.

The original scalar/table profiles and CLI remain compatible. A drawing package
does not accept --table-data or a scalar example-input argument. --reserve uses
the existing inclusive ID accounting. IDs 0 and 65535 remain unavailable.
Input and program blocks are limited to the stock write-command size, 65535 bytes.
Semantic, allocation and requested assembly-symbol checks precede output writes.
As elsewhere, an I/O failure during publication is not an atomic transaction.

## Drawing behavior

| ID | Call | Stock implementation |
| --- | --- | --- |
| DRW-001 | draw_point(colour,x,y) | GCOL 0,colour then absolute foreground PLOT 69. |
| DRW-002 | draw_line(colour,x1,y1,x2,y2) | GCOL 0,colour; absolute PLOT 4 to first endpoint and PLOT 5 to second, including both endpoints. |
| DRW-003 | fill_quad(colour,x1,y1,x2,y2,x3,y3,x4,y4) | Convex perimeter A,B,C,D maps to PLOT 4,4,85,85 with vertices A,B,D,C, forming triangles A,B,D and B,D,C. |
| DRW-004 | draw_bitmap(bitmapBuffer,x,y) | Select full uint16 bitmap ID with VDU 23,27,32; disable affine transformation, draw at native size with VDU 23,27,3 and disable transformation afterward. |

Coordinates are absolute signed pixels under the required host setup. Negative
and off-screen coordinates use stock clipping. Polygon edge coverage follows
the stock rasterizer, without hidden one-pixel adjustments. The caller supplies
convex perimeter ordering; the compiler does not test topology. Geometry operations
change foreground GCOL and graphics cursor history. No display state is saved
or restored automatically.

Plain bitmap placement uses signed16 screen-pixel coordinates for the top left,
with uint16 IDs in 1..65534. Bitmap fields require Bounds within that ID range;
matching integer literals are accepted. Native bitmap drawing uses stock Set
painting and leaves the current bitmap selected and bitmap transformation
disabled. It does not preserve previous paint/bitmap/transform state. The
existing pixel/origin/full-viewport host setup applies to mixed drawing jobs.
GOLEM-019 verifies signed negative screen clipping, binary transparency over
black/grey and native placement against unchanged Wolf RGBA2 payloads. A prior
affine translation is proved active before the Golem call disables it.

Bitmap packages enable the stock affine feature flag in the bootstrap so the
transform-disable command consumes its operands correctly even when the flag
was initially off. The flag remains enabled after cleanup. Geometry-only
packages retain their prior bootstrap bytes.

## Borrowed bitmaps

Assembly loads and registers bitmap data before calling the entry. AGNB remains
an assembly loading mechanism; Golem source contains no asset filenames or MOS
file requests. Reserve every possible resource ID using --reserve FIRST:LAST
when compiling. The compiler checks literal and initial bitmap IDs against
these reservations before allocating anything. They are excluded from package
installation and cleanup. The manifest publishes initial_bitmap_ids and the
reservation ranges.

For a variable bitmap ID, its numeric Bounds are separate from the set of
reserved, loaded assets. For example, Bounds(1,65534) permits the representation
of any ordinary ID, while the host may only supply IDs reserved by this build
and actually loaded/registered. Later raw host writes must satisfy both
requirements; no per-draw runtime admission or ownership check is emitted.
The assembly application keeps borrowed assets valid until drawing completes.

The [Wolf example](../tasks/GOLEM-019/bitmaps.golem) uses a six-byte input block
for bitmapBuffer, x and y, and draws three further images with literal IDs and
positions. Its build reserves the four AGNB IDs:

```bash
build/golemc --resident --emit-asm \
  --reserve 8195:8195 --reserve 8201:8202 --reserve 8463:8463 \
  docs/tasks/GOLEM-019/bitmaps.golem build/wolf-bitmaps
```

Assembly can call golem_drawWolf repeatedly for six wire bytes per complete
four-image job. Updating the six-byte input block and calling costs 23 bytes;
the bitmap pixels are loaded once. Unchanged calls do not transmit image data.

The compiler validates declared types/bounds and initial/literal values; later
host writes must honor the exported field contract. This profile introduces no
runtime range/topology admission, error-status field, acknowledgement or frame
fence. Calls and updates remain serialized and nonreentrant. Returning from the
assembly wrapper only means the call packet was transmitted; application pacing
and completion mechanisms remain the host's responsibility.

## Package and assembly interface

Outputs are bootstrap.bin, invoke.bin, update-call.bin, cleanup.bin and
interface.json, with optional golem.inc. Globals share one packed buffer in
declaration order: uint8 uses one byte, sint16/uint16 use two little-endian bytes, without
padding. Initializers are installed once, persist through calls and reset on
reinstall. Each operation has a private patchable command template; one longer
entry stream patches and calls these templates. Literal operands are embedded
directly. There is no buffer allocation per individual patch instruction.

Generated call golem_<SubName> transmits the six-byte entry packet. Per-field
_buffer/_offset/_bytes/_type exports identify VDP storage. Type tags preserve
1=uint16, 2=sint16, 3=float32 and 4=packed record; 5 identifies uint8. When inputs
exist, golem_inputs_buffer and golem_inputs_bytes expose the full packed block.
Existing named-call and package payload labels are retained. The JSON manifest
publishes input storage, fields/bounds/initial values, entry, owned IDs and
reservations. A literal-only package has input:null and no packed input exports.

update-call.bin demonstrates restoring the declared input block and invoking;
it is not an automatic per-call requirement. Assembly may update selected fields,
a contiguous range or the whole block using the published offsets. Unchanged
invocation costs six bytes; one N-byte range update plus invocation costs 17+N.
Many freshly uploaded primitive operands can cost more than direct VDU commands;
resident reuse and grouped work provide the savings.

The qualified demo uses 29 input bytes, 13 owned buffers and a 775-byte bootstrap.
Its whole-block update/call is 46 bytes, unchanged call six, cleanup 78. It draws
nine points, a line and a convex quad. These figures exclude display setup,
diagnostic observations and any host completion mechanism. Cleanup clears only
compiler-owned buffers, preserving reservations and leaving graphics state alone.

General plot, arrays/tables driving geometry, assignments/expressions, local
variables, loops, multiple Subs, functions, bitmap transforms and viewport/scroll
operations remain outside this profile. Their design examples do not imply
compiler support.
