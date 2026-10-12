# WIR-001 — Resident drawing traffic and host contract

Keep geometry and inputs on the VDP, and invoke a complete drawing job with one
six-byte call. That reduces recurring wire traffic when inputs are reused or
shared across enough operations. Uploading every operand for one primitive can
cost more than sending the primitive directly. Recommend a first pixel tranche
with points, lines and convex quads, one packed input buffer, generated assembly
symbols and a finite named Sub. Bitmap/transform/viewport/scroll implementation
can follow. These are byte-count projections, not a compiled drawing backend or
measured performance result. WIR-001 ends at review; REV-001 remains next.

## What crosses the wire

| ID | Packet or quantity | Bytes | Basis |
| --- | --- | --- | --- |
| CNT-001 | Call installed entry | 6 | Buffered command 1: 23,0,160,id16,1. |
| CNT-002 | Set N contiguous input bytes | 11+N | Command 5, Set with multiple targets/operands: six-byte prefix, operation byte, offset16, count16, N literal bytes. |
| CNT-003 | Update one contiguous range, then invoke | 17+N | CNT-002 plus CNT-001. |
| CNT-004 | Update K separate ranges, then invoke | 6+11K+sum(N) | Every range incurs its own header; ranges need not be resent if unchanged. |
| CNT-005 | Install one initialized buffer of L bytes | 14+L | Clear command 2 (6), then write command 0 (8+L), one block. |
| CNT-006 | Copy between installed buffers inside a job | 15 stored command bytes | Existing copy helper: prefix6, operation1, destination offset2, count2, source ID2, source offset2. This is installation cost, not per-invocation wire traffic. |

Counts use ordinary 16-bit offsets and current multi-byte Set encoding, not
advanced block offsets, compression or a new protocol. Assembly instructions
that transmit the packets are not additional VDP wire bytes. Display setup,
asset upload, debug observation and cleanup must be accounted for separately.

## Direct commands versus resident calls

Each row compares the same one operation with all its dynamic operands refreshed.
Direct named geometry includes GCOL 0,colour; general plot uses existing state.
Viewport includes Rally's clipping synchronization PLOT. Bitmap includes full-ID
selection, transform selection/disable, draw and final transform disable, with
the transform ID fixed by the installed program. Matrix creation is excluded.

| ID | Operation | Direct VDU bytes | Packed operand bytes N | One update plus call |
| --- | --- | --- | --- | --- |
| CMP-001 | draw_point | 9 | 5 (colour + two signed words) | 22 |
| CMP-002 | draw_line | 15 | 9 (colour + four signed words) | 26 |
| CMP-003 | fill_quad | 27 | 17 (colour + eight signed words) | 34 |
| CMP-004 | plot, fixed code | 6 | 4 (two signed words) | 21 |
| CMP-005 | draw_bitmap, fixed transform choice | 24 | 6 (buffer ID + two signed words) | 23 |
| CMP-006 | graphics_viewport | 15 | 8 (four signed words) | 25 |
| CMP-007 | One named scroll direction | 5 | 1 (amount) | 18 |

An unchanged resident job always needs only its six-byte entry call, regardless
of its internal patch/call count. A lone unchanged general plot ties its direct
six-byte command. Splitting every geometry field into a separate update makes
fresh point/line/quad calls 44/70/122 bytes respectively. Packing is useful but
does not create a saving for those isolated fully refreshed primitives.

For EX-036, one job sets the viewport, performs four scrolls using the same amount,
then restores the full viewport. Direct commands total 50 bytes. A packed update
of its four bounds and amount is 9 bytes of operands: 26 bytes including update
header and entry call, or six bytes if unchanged. This is an accounting example;
viewport/scroll support is not required for the first pixel tranche.

For a point/line/quad job sharing one colour but using independent coordinates,
inputs occupy 29 bytes. Direct emission can share GCOL once: 3+6+12+24 = 45 bytes.
A full input update plus call costs 46; an unchanged call costs six. Updating
only one contiguous coordinate pair costs 21 including the call, while all other
inputs remain resident. Compare equivalent requested drawing work: a host that
only needs one line can simply issue that line rather than redraw the whole job.
Later resident calculations or table-selected geometry can reduce host operands
further, but current table and scalar profiles do not yet compose with drawing.

## Installation and invocation boundary

1. HOST-001: The PC build allocates compiler-owned IDs, packs initialized input
   fields and emits offsets/widths/types plus a named entry and install/cleanup
   payloads. Reuse --resident --emit-asm and the existing package conventions.
   A new drawing profile may pack fields together without changing existing
   scalar/table layouts. No dummy floor computation is needed.
2. HOST-002: Assembly sends installation once, then establishes pixel
   coordinates, origin (0,0), display/palette and a valid viewport. The first
   geometry tranche uses no bitmap resources. Later borrowed bitmap IDs must
   be reserved by the build, installed/registered by assembly and excluded from
   compiler cleanup. AGNB/container loading stays in assembly.
3. HOST-003: Assembly updates only inputs that changed, using generated buffer
   IDs and offsets. A whole packed block is convenient when needed; a partial
   contiguous update is also valid. Initializers persist until changed or
   reinstalled. Signed words use little-endian two's complement; colour is one
   byte. Packing does not mean copying the block on every invocation.
4. HOST-004: call golem_<SubName> transmits the entry call. A finite resident
   command stream copies current fields into private drawing templates and
   executes them. Keep sequences together in a longer stream; do not assign
   every patch an individual buffer. Convex quad perimeter order maps to the
   triangle order documented in SYN-001 without runtime geometry analysis.
5. HOST-005: Updates and calls remain serialized and nonreentrant. A transmitted
   call is not a completion acknowledgement or frame fence. Assembly owns
   pacing and any externally supplied completion mechanism. No polling,
   readback or custom VDP command is added by this proposal.
6. HOST-006: Reject unsupported source, wrong types and invalid literals at
   compile time. For initial geometry, declared colour bounds must fit 0..63;
   assembly must honor them when updating. Signed coordinates may be off-screen
   and use stock clipping. The caller supplies a convex perimeter. No new
   runtime colour/topology checking or drawing status code is promised here.
   Later resource/viewport/scroll admission decisions remain explicit.
7. HOST-007: Assembly cleans up compiler-owned buffers when finished; reinstall
   restores initial values. Cleanup does not restore graphics state or remove
   borrowed assets. The pixel fixture must return to MOS, show its keyboard
   check instruction onscreen and leave saved observations for agent retrieval.

## Installation example and evidence limits

For the illustrative three-operation job above, use one 29-byte input buffer,
three templates (9+15+27 = 51 bytes) and one entry stream. Patching every field
separately takes (3+5+9)*15 bytes plus three six-byte template calls: 273 bytes.
Five buffers therefore contain 353 payload bytes and install in 423 bytes under
CNT-005, with a 30-byte eventual cleanup. At unchanged inputs, 11 executions
amortize that installation against repeated 45-byte direct jobs:
423+11*6 = 489 versus 11*45 = 495. If every input changes, recurring 46-byte
calls never repay installation against that baseline. Display setup is common
and excluded; including cleanup moves the break-even point to 12 executions.
Literal folding, shared patches, admission code, feature setup or extra metadata
could change installation size. These figures specify an illustrative layout,
not an emitted artifact size or a required optimizer.

Verified packet lengths against [scalar_backend.hpp](/Agon/mystuff/golem/src/scalar_backend.hpp)
(command/literal/copy/writeBlock), [assembly_output.hpp](/Agon/mystuff/golem/src/assembly_output.hpp)
(named call wrapper) and the primary [Buffered Commands API](/home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md).
Primitive templates follow the [drawing inventory](drawing-inventory.md) and
approved [syntax contract](drawing-syntax.md); full-ID bitmap selection adds one
byte to Rally's 23-byte legacy template. [wire-counts.json](wire-counts.json)
records arithmetic and source fingerprints. No compiler, emulator configuration,
deployment, upstream checkout or existing example was changed by WIR-001.
