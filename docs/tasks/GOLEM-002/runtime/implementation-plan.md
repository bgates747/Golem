# Vdplet runtime implementation tranche

Implement one bounded, serialized Xtensa module slot in a task-owned firmware
export and prove that separately compiled code is uploaded after boot. Keep all
artifacts in GOLEM-002; do not flash physical hardware or edit other projects.
The user authorized this implementation as part of the eight-hour goal.

- ABI-001 [x] Audited D8/D9 against the pinned and current upstream video-system dispatch, then fixed these experimental selectors with the integration owner. See vdplet.h.
- ELF-001 [x] Pinned GCC 8.4 emits four RELATIVE relocations, no imports and compact code/data; variant 2 changes instructions as well as coefficients/build identity.
- ELF-002 [x] Implemented the narrow independent section loader. Native ASan/UBSan checks cover the actual image and twelve rejection/allocation-cleanup cases.
- SDK-001 [x] Implemented fixed C ABI, finite INIT/RUN/FINI, scoped contiguous buffer access and one generation handle. Actual native execution passes PRF-001.
- VDU-001 [x] Added bounded framed dispatcher, optional administrative reply, INFO and READ16. Live stream qualification remains part of PRF-001.
- BLD-001 [x] Pinned clean firmware builds with the installed exact physical toolchain; 45,016 bytes static DRAM, 1,084,825 flash. First build took 127.50 seconds on Pi.
- PRF-001 [x] Root qualified both actual Xtensa engines through owned Fab/MOS/SD: 44 replies / 440 bytes match independent expectations, fresh handles 1/2/3, reused actual IRAM entry 0x40097b64, persistent counter, BSS reset and changed executed instructions. Numeric v2 is preserved.
- FRM-001 [x] Added scoped PALETTE2 frame services and a separately compiled C++ packed-rowcopy module. Root's full Fab/MOS interpreter and JIT proof each pass 54 replies / 540 bytes and 61,440 independent emulated VGA pixels.
- EVD-001 [x] Recorded artifacts, source pins, memory sizes, unsupported cases and fresh execution evidence in qualification.md; full tracked engine receipts are linked there. Root delivered the service offer to the existing 6502 thread.

The candidate loader intentionally accepts only ELF32 little-endian ET_DYN
Xtensa, named .text/.rodata/.data/.bss, and R_XTENSA_RELATIVE relocations, with
no unresolved imports or dynamic constructors. The first compiler inspection
determines whether that profile is sufficient. A change of profile is recorded
before implementation rather than silently broadening into a general linker.

First compiler inspection showed that the numeric module needs only four
R_XTENSA_RELATIVE fixups. The implementation consequently uses a small independent
section loader instead of compiling Espressif's filesystem/import/dlfcn component.
The package removes unused .got/.got.loc/.dynamic metadata and nulls its stale
PT_DYNAMIC descriptor; no PLT/import support is added. Executable bytes are copied
with aligned 32-bit writes because classic ESP32 IRAM is not byte accessible.
Use memw/isync before entry; this internal IRAM is uncached. Flexe must invalidate
its own predecode/JIT state for writes to executable RAM.
