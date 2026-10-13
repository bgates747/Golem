# Experimental GOLEM-002 vdplet runtime

The maintained runtime, SDK, examples and build infrastructure have moved to
[agon-vdplet](../../../../../agon-vdplet/README.md). This document and the
existing build snapshots remain historical task evidence. Source paths below
are compatibility links; the firmware builder delegates to the new project's
builder and defaults to that project's `build/firmware/` directory. See its
[hardware usage guide](../../../../../agon-vdplet/docs/STARTHERE.md) for
current build and usage instructions.

The task-owned ESP32 firmware provides one finite runtime-loaded module slot and
a C service table. Both numeric variants now pass post-boot upload/execution
through actual Xtensa interpreter and JIT engines in the hybrid Fab/MOS emulator.
A separately compiled C++ packed renderer also passes the full MOS interpreter
and JIT pixel proof. Physical hardware remains untested; no device has been flashed.

The maintained task plan remains [GOLEM-002](../../GOLEM-002.md). Implementation
progress is tracked in [implementation-plan.md](implementation-plan.md).

## Build and artifacts

Run from the Golem project root:

```bash
.venv/bin/python docs/tasks/GOLEM-002/proof/build_modules.py
# Add --packed to also build the C++ packed renderer.
.venv/bin/python docs/tasks/GOLEM-002/runtime/build_firmware.py
g++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
    docs/tasks/GOLEM-002/runtime/check_image.cpp \
    -o docs/tasks/GOLEM-002/runtime/build/check_image
docs/tasks/GOLEM-002/runtime/build/check_image \
    docs/tasks/GOLEM-002/proof/build/numeric-1.vdpl
docs/tasks/GOLEM-002/runtime/build/check_image \
    docs/tasks/GOLEM-002/proof/build/packed-mono.vdpl 1
```

The installed compiler must identify `esp-2021r2-patch5` / GCC 8.4.0. The builder
exports VDP commit `1056ee38ec1cc68ed327bd32ee47d04b1c30597c` read-only from
Fab's reference submodule; its own export is the only source tree patched.
Graphics is pinned to `ac2dd5986daf496c43ae8e7fe41836274aec54a0`, ESP32Time 2.0.6,
CRC 1.0.4, PlatformIO espressif32 6.6.0, Arduino 2.0.14 / IDF 4.4.6.

Ignored `proof/build/` contains `.vdpl`, original ELF, stripped upload ELF,
disassembly, section/relocation inspection, compiler stack usage and modules.json.
Ignored `runtime/build/firmware-src/.pio/build/esp32dev/` contains firmware.bin,
firmware.elf and firmware.map. Adjacent JSON receipts contain hashes and pins.
There is no numeric kernel or module entry symbol linked into firmware.
Preserved qualifying firmware snapshots are `build/numeric-firmware-v2/` and
`build/packed-firmware-v1/`. Their source/artifact receipts are copied alongside
the binaries. See [qualification.md](qualification.md) for measured results,
hashes and the distinction between these snapshots and later rebuilds.

## Fixed experimental wire contract

These selectors are private to this task-owned firmware, not upstream allocations:

```text
Discovery: 23,0,0xD9
Frame:     23,0,0xD8,length:u16,payload[length]
Payload:   protocol:u8=1,operation:u8,token:u16,flags:u8,reserved:u8=0,args
```

Flag bit 0 requests one reply. No reply is sent for ordinary RUN unless requested.
Integers on the wire are little-endian. Payload length is at most 32; an oversized
complete frame is discarded with a fixed 32-byte scratch buffer. A truncated
frame never invokes native code. This shares the underlying VDP stream timeout
behavior, not a new transport with guaranteed recovery from arbitrary truncation.

| Operation | Value | Argument bytes after common six-byte prefix |
| --- | --- | --- |
| LOAD | 1 | staging buffer:u16, exact package length:u32, reserved:u16=0 |
| RUN | 2 | handle:u16, entry:u16, argument buffer:u16 |
| UNLOAD | 3 | handle:u16 |
| INFO | 4 | field:u16 |
| READ16 | 5 | buffer:u16, byte offset:u32 |

Reply is the existing small keycode packet, code 0x81, payload length 10:
`"VDPL",protocol:u8=1,kind:u8,token:u16,value:u16`. Kinds are IDENT=1,
LOADED=2, DONE=3, VALUE=4, ERROR=5. IDENT's value is capability bits; LOADED's
value is the new handle; DONE's is zero; ERROR's is a status defined in vdplet.h.
Use the MOS keyboard mailbox pattern; modules have no raw serial service.

| INFO field | Value |
| --- | --- |
| 0 | Protocol/ABI identity 0x0100 |
| 1 | Capability bits: contiguous buffers=1; optional packed framebuffer=2 |
| 2 | Current handle or zero |
| 3 | Last LOAD/RUN/UNLOAD status; diagnostics do not clear it |
| 4 / 5 | Actual aligned module text / data+BSS bytes |
| 6 / 7 | Actual entry address low / high word |
| 8 | Successful load count |
| 9 / 10 | Executable free bytes low / high word |
| 11 / 12 | Largest executable block low / high word |
| 13 / 14 | Internal byte-accessible free bytes low / high word |
| 15 / 16 | Largest internal byte-accessible block low / high word |
| 17 | Command task minimum free stack since startup, in bytes (ESP-IDF convention) |

## Module profile and lifecycle

The package has a 64-byte fixed little-endian envelope (builder is authoritative)
and a bounded ELF32 ET_DYN Xtensa image. ABI is classic ESP32's 32-bit windowed
calling convention, exact GCC family, with no imports, constructors, TLS, C++
exceptions, RTTI, heap, callbacks or background execution. Only .text, .rodata,
.data, .bss and .data.rel.ro become resident; all relocations must be symbol-zero
R_XTENSA_RELATIVE with zero explicit addend. Runtime pointers are corrected in
text literals and module data. Initial limits are 16 KiB package, 8 KiB text,
4 KiB data/BSS and a 512-byte declared module stack allowance.

The service header is [vdplet.h](vdplet.h). Assembly pre-creates resident buffers,
uploads the package with a stock one-block WRITE, LOADs it, updates input buffers,
RUNs it, and UNLOADs it in the same ordered stream. Stock six-byte resident calls
can invoke a preinstalled RUN frame. No filesystem names or MOS loader callbacks
are part of native execution.

INIT=1, RUN=2 and FINI=3 are finite native entry operations. A handle is published
only after successful loading and INIT. Each successful load gets a fresh handle;
stale handles fail. Failed allocation/init cleans partial storage. Normal return
releases all outstanding buffer leases. Native faults/infinite loops are ordinary
firmware faults; there is no sandbox, cancellation or rollback guarantee.
Read/write conflicts and argument protection compare the owning block, so a
second VDP buffer ID referring to the same storage cannot bypass these checks.

Buffer services admit one contiguous stock buffer block, with at most four live
leases. Fixed spans check offset and length before returning a pointer.
`VDPL_WHOLE_BUFFER` (`UINT32_MAX`) requests all remaining bytes after the offset;
the sentinel is resolved before arithmetic and the returned `bytes` is the real
span length. Module pointers expire when released or when the entry returns.

The optional packed framebuffer services begin one scoped, single-buffered
PALETTE2 transaction, expose individual writable row spans and end the
transaction. Horizontal position and width must be byte aligned; the rectangle must fit
fully on screen. The host drains pending Canvas work and validates every row
mapping before returning access. It performs no row copy, clipping, pixel
expansion, palette change, swap or vblank wait. Normal return closes a forgotten
transaction. The independently compiled [packed module](../packed-mono/README.md)
contains the actual copy loop.

Numeric entry 0 takes a four-byte argument buffer containing input ID:u16 and
output ID:u16. Input is one binary32 float; output is eight bytes: binary32 result
and persistent invocation counter:u32. Variant 1 calculates `1.5*x+0.25` and
variant 2 executes `(x+x)+0.5`. After a fresh load, x=2 produces 3.25 / counter 1
or 4.5 / counter 1 respectively. Changing x to -4 in variant 1 produces -5.75;
the counter advances. Reload resets BSS. Numeric output uses aligned local float
objects and byte-encoded buffer access, so buffer offsets do not impose float
alignment assumptions.
