# Runtime-loaded packed monochrome renderer

[packed_mono.cpp](packed_mono.cpp) is a separately compiled C++ Xtensa module.
The module copies packed one-bit image rows into the VDP's native PALETTE2
framebuffer. The firmware supplies scoped buffer access and row access; the
actual copy loop is in the uploaded module. No renderer is linked into firmware.

Build from the Golem root:

```bash
.venv/bin/python docs/tasks/GOLEM-002/proof/build_modules.py --packed
```

Output is `../proof/build/packed-mono.vdpl`: 1,392 upload bytes, 572 executable
bytes, four initialized data bytes, one R_XTENSA_RELATIVE relocation and a
144-byte compiler-reported entry stack frame. It uses the same GCC 8.4 windowed
ABI as the numeric modules, without imports, constructors, exceptions, RTTI,
heap allocation or C++ runtime support. The anonymous export map retains only
`vdplet_entry` as an exported module symbol. Its ELF entry virtual address is
0xEC; the loader chooses the actual address.

## Entry and storage contract

RUN entry 0 consumes an exact ten-byte, little-endian argument buffer:

| Byte offset | Field | Type |
| --- | --- | --- |
| 0 | Source buffer ID | uint16 |
| 2 | Destination x | sint16 |
| 4 | Destination y | sint16 |
| 6 | Width in pixels | uint16 |
| 8 | Height in pixels | uint16 |

Assembly creates and fills the source and argument buffers, loads the module,
then invokes it. The source must be one contiguous stock buffer block containing
exactly `(width / 8) * height` bytes. Rows are top to bottom; each byte holds eight
left-to-right pixels, most significant bit first. Neither source storage nor
argument storage is owned by the module. Source buffers may remain resident for
repeated calls.

The VDP must be in a single-buffered, two-colour PALETTE2 mode. Width and height
must be positive; x and width must be multiples of eight; the entire rectangle
must fit. Mode 11 (320x240) is used by the qualified 280x192 fixture. The module
has no clipping, transparency, scaling, GCOL processing, palette changes,
decompression or page allocation. Existing compression/decompression can run
before the call and leave its decoded block resident.

The module borrows the whole source block using `VDPL_WHOLE_BUFFER`, checks its
actual exact size, opens a framebuffer transaction and copies each exposed row.
The host validates all row mappings and drains pending Canvas work before
exposing the first row. It neither waits for vblank nor swaps buffers. Assembly
owns pacing and may call the usual VSYNC operation separately. Normal entry
return releases any outstanding leases and ends an outstanding transaction.

Mode 8 is rejected with status 9 (`VDPL_E_UNSUPPORTED`); a 6,719-byte block for
the 280x192 fixture is rejected with status 3 (`VDPL_E_BOUNDS`). These checks
precede any image writes. Unaligned width is status 1 (`VDPL_E_ARGUMENT`), while
a rectangle exceeding the screen is status 3.

## Qualification and traffic

Both actual Xtensa engines pass the full Fab/MOS/SD test: 54 diagnostic replies
(540 payload bytes), all 61,440 independently checked emulated VGA pixels, exact
Lode Runner image pixels and an unchanged black margin to its right. Each run
returns to the real MOS prompt and reports no unresolved MMIO, ROM or memory
access. JIT elapsed time is 9.219 seconds; interpreter time is 17.164 seconds.
Those are whole qualification runs, not renderer benchmarks. Physical hardware
and human visual review remain pending.

The image fixture is the real agon-6502 Lode Runner HGR capture, converted by the
assembly integration build to 6,720 MSB-first packed bytes. Its SHA-256 is
`34baecf6c24a305eb8a4a6fa36976fe190e6b189bb684288add8cf587f87b382`.
Full receipts and artifact hashes are in the [qualification report](../runtime/qualification.md).

Once assembly installs a silent RUN command buffer with the module handle and
argument ID, each ordinary buffered call costs six outbound bytes. Two calls
are exercised by the proof; diagnostic INFO fences are separate test traffic.
The direct framed RUN costs 17 bytes. Source upload, argument changes, module
loading and replies must be counted separately when comparing a real game's
total traffic. The earlier agon-6502 private F0 blit command costs 14 outbound
bytes per call, so a resident wrapper reduces this invocation by eight bytes;
it does not itself improve the compressed page representation.

The integration owner delivered this interface to the existing 6502 agent.
That agent retains its F2/F3 codec port; no active 6502 project files were edited
by this tranche.
