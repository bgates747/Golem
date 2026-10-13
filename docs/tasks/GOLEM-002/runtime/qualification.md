# Native vdplet qualification — 2026-10-12

Runtime-loaded native C and C++ execute after boot in the hybrid Fab/Flexe
emulator. This is actual Xtensa execution of separately uploaded module bytes,
including changed executable instructions loaded at the same IRAM address.
Neither module entry nor numeric/renderer kernels are linked into the firmware.
No physical ESP32 has been flashed or tested. Human visual/keyboard review is
owned by the main task and remains separate from these automated results.

## Execution evidence

| Fixture / engine | Checked replies / payload bytes | Checked VGA pixels | Elapsed seconds | Receipt |
| --- | --- | --- | --- | --- |
| Numeric / JIT | 44 / 440 | — | 8.795 | [numeric-jit.json](../../GOLEM-021/evidence/numeric-jit.json) |
| Numeric / interpreter | 44 / 440 | — | 15.723 | [numeric-interp.json](../../GOLEM-021/evidence/numeric-interp.json) |
| Numeric + packed / JIT | 54 / 540 | 61,440 | 9.219 | [packed-jit.json](../../GOLEM-021/evidence/packed-jit.json) |
| Numeric + packed / interpreter | 54 / 540 | 61,440 | 17.164 | [packed-interp.json](../../GOLEM-021/evidence/packed-interp.json) |

All four runs load from the emulated SD card through the real MOS assembly
consumer, discover the runtime, upload modules after boot, execute, unload and
return to the actual MOS prompt. Both engines report zero unresolved MMIO, ROM
or memory accesses. Reply payload SHA-256 is
`e06ad6d9fdb791dbaefc495263fa30d0ffefcc9f159c57eb7e8b1ad36e2413c9`
for numeric and
`576c87e9a933d07d084c0a3d3add7a33a6ca459c7911de3aafcddbd21e9ffbe0`
for the original combined packed test.

Numeric handles 1/2/3 are fresh, and actual entry 0x40097B64 is reused for all
three loads on numeric firmware v2. Input changes, persistent counters, zero BSS
after reload and variant 2's different add/add instructions all match independent
binary-exact expectations. That run observes 22 ISYNC instructions and three
emulator code invalidations. On packed firmware v1, numeric entry 0x40097B8C is
likewise reused; packed handle 4 enters at 0x40097B84. Both combined runs observe
28 ISYNC instructions and five code invalidations.

The packed test verifies the real 280x192 Lode Runner image and unchanged
320x192 surrounding scanout region. It rejects unsupported mode 8 with status 9
and a short 6,719-byte source with status 3, then runs the valid module through
two silent resident six-byte buffered calls. Diagnostic INFO fences read status
without clearing it. All 61,440 checked pixels match the independent packed-bit
oracle. VGA receipt hashes differ between engines because the complete captured
frame also includes timing-sensitive text; the checked game image and margin
match in both.

These original receipts cover INFO fields through 16. The current runtime also
exposes INFO17 for the command task's minimum free stack, in ESP-IDF bytes. The
main integration task owns the final 56-reply qualification and Oryx
staging; those results do not retroactively change these preserved snapshots.

## Reproducible artifacts

Clean source pins are VDP `1056ee38ec1cc68ed327bd32ee47d04b1c30597c` and graphics
`ac2dd5986daf496c43ae8e7fe41836274aec54a0`. PlatformIO espressif32 6.6.0 builds
Arduino 2.0.14 / IDF 4.4.6 with the installed `esp-2021r2-patch5` Xtensa GCC 8.4.0
windowed ABI. ESP32Time is 2.0.6; CRC is 1.0.4. The task exports a clean pinned
source tree and patches only its own copy; upstream and other projects remain
untouched. Build commands and the wire contract are in [README.md](README.md).

| Module | Upload bytes | Text / data+BSS | Entry virtual | Relative relocations | Compiler entry stack |
| --- | --- | --- | --- | --- | --- |
| numeric-1.vdpl | 1,472 | 468 / 20 | 0x174 | 4 | 112 bytes |
| numeric-2.vdpl | 1,460 | 456 / 20 | 0x174 | 4 | 112 bytes |
| packed-mono.vdpl | 1,392 | 572 / 4 | 0xEC | 1 | 144 bytes |

Module package SHA-256 values:

```text
f62999262bcae37afee95ad02e40a4160a3d49c8122f0471ef3a34580e2b004e  numeric-1.vdpl
3486e188b752ce6ddd6d566db91b7605c77b41f9031d5998d318616423036bfa  numeric-2.vdpl
609abaa6c4de6a162d9272c4e43955ce47f59ad0a5d36e0a1c2346fb09ee577d  packed-mono.vdpl
```

Numeric source SHA-256 is
`9f117eaacbf1425516a668d1b81c99ec21e4cd319a8a31de0c5380988afee054`;
packed C++ source SHA-256 is
`3e79056f612720ec7344a9ef4c3df22c7ff6b8707958ba365c6827e7f9dcbd97`.
The ABI header used by all three modules has SHA-256
`2311c7abb3a0470e293ad6c30ef7aefe9aff6375cccc90c181be64c0045a549d`.
Ignored `../proof/build/modules.json` records complete source-to-package mappings,
allocated ELF sections, entry addresses and compiler identity. Original ELF,
stripped upload ELF, disassembly and stack reports remain alongside each package.

Preserved firmware artifacts reside in `build/numeric-firmware-v2/` and
`build/packed-firmware-v1/`, each with firmware.bin, firmware.elf, firmware.map
and copied source/artifact JSON receipts. The paths inside those copied JSON
receipts name the original build output; the hashes identify the snapshot files.

| Firmware snapshot | Binary bytes | Static DRAM | Firmware binary SHA-256 |
| --- | --- | --- | --- |
| numeric-firmware-v2 | 1,085,280 | 45,016 | `2aa6710fd9eab3443a874b19786a5f2efc5562ddc6ab437c33c8e6b4bcf20b5b` |
| packed-firmware-v1 | 1,086,192 | 45,032 | `31d25eec03a531b336d78d453f0720bbd1312c7c8516fdfa80130769ab15e49f` |

ELF SHA-256 values are
`6c90d0bfdb03bc5fd615efb03f51692340dd22b19193e87dda5313aaa86b3fb1`
and `51f196eb7b84ed79394a9fde1dff832ad479417c360525c69582fd093c3a0a3a`
respectively. These snapshot numbers describe complete firmware. A subsequent
[matched size comparison](size-comparison.md) removes only vdplet integration:
the facility adds 8,512 bytes of flash/binary and 320 bytes of static RAM, with
36 additional firmware IRAM instruction bytes. Loaded module code/data and
staging buffers remain separate dynamic costs. Qualified snapshots are unchanged.

## Checks and practical limits

Host ASan/UBSan image checks load both real ELF forms, verify relative mapping
and numeric BSS, and cover twelve malformed/import/relocation/allocation-cleanup
cases. A focused source review found no blocker in lease cleanup, source span
resolution, bounded geometry, transaction cleanup or the module's row-copy loop.
The baseline qualification owner independently reviewed the numeric loader.

The loader is deliberately narrow: one finite synchronous module slot, no
imports, constructors, TLS, background callbacks, C++ runtime, exceptions or
RTTI. Package/text/data and declared stack allowances are bounded; the declared
stack allowance is admission metadata, not runtime stack enforcement. Native
code is trusted firmware code: faults and infinite loops can fail the VDP.
Leases describe validity; they do not sandbox native pointers.

Actual execution validates relocation and BSS through the executed results.
Deliberately forcing different physical allocation placements remains untested;
the API was not expanded solely to create that diagnostic. Six-byte repeated
invocation is proved; asset upload, setup and diagnostics remain additional
traffic. No renderer speed, hardware reliability or compressed-page saving is
inferred from the emulator's whole-test elapsed times.

The root agent delivered the scoped-services offer to the existing 6502 thread;
[service-offer.json](../../GOLEM-021/evidence/service-offer.json) records the
message identity. That agent retains its current F2/F3 codec work.
