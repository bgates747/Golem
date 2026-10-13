# Packed monochrome vdplet tranche

After the numeric loading proof passes, add scoped framebuffer services and move
the actual packed row-copy loop into a separately compiled C++ module. Preserve
the agon-6502 renderer's single-block, exact-size PALETTE2 rules and compressed
page pipeline. No existing project is edited, and no physical flash is authorized.

- API-001 [x] Reviewed the pinned physical graphics APIs and agreed whole-buffer borrowing with the integration owner.
- API-002 [x] Added whole-buffer sentinel resolution before arithmetic; fixed numeric spans retain their behavior. Root's short-source rejection passes.
- FRM-001 [x] Added the scoped transaction with row/mode/geometry preflight, immediate Canvas drain and normal-return cleanup. No framebuffer data or complete blit operation is exported as a host implementation.
- MOD-001 [x] C++ module compiles with exact GCC 8.4, one entry export, no imports/exceptions/RTTI/constructors/heap, text572/data4, one RELATIVE fixup, 144-byte compiler stack frame and 1,392-byte package.
- TST-001 [x] Both full Fab/MOS engines pass exact packed pixels, unchanged side margins, repeated resident invocation, invalid source size and unsupported mode; root owns the assembly consumer and independent pixels.
- EVD-001 [x] Preserved numeric firmware v2, published new module/firmware hashes and measured sizes in README.md and ../runtime/qualification.md. Root delivered the service offer to the existing 6502 thread; their owner retains codec work.

The host service table retains its 36-byte ABI v1 layout; optional framebuffer
pointers already exist. Capability bit 2 advertises support. frame_begin returns
format 1 (packed MSB-left one-bit pixels), exact width/height and width/8 row bytes.
frame_row returns one writable row span subordinated to the frame lease; frame_end
invalidates row pointers. Normal entry return closes an outstanding transaction.
The host supplies access and coordination; the module contains the copy loop.

Root will supply recognizable packed game rows and independent image checks.
Two preinstalled argument buffers/resident wrappers can refer to two retained
decoded pages, letting assembly choose a six-byte call without changing args.
This preserves the already compressed wire representation; any traffic saving
must be counted against the actual chosen invocation/update arrangement.

Root's full Fab/MOS interpreter and JIT proofs pass 54 replies / 540 bytes and all 61,440 checked
emulated VGA pixels. Two silent resident calls use six outbound bytes each; INFO
fences are diagnostic traffic. Mode8 returns status9 and a 6,719-byte source
returns status3. Numeric handles1/2/3 and packed handle4 demonstrate replacement
without a firmware rebuild. Root delivered the service offer after reading the
6502 owner's current messages; the owner retains the F2/F3 codec port. Physical
hardware and human visual review remain separate acceptance steps.
