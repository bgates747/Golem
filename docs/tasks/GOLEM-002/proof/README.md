# Separately compiled numeric vdplet proof

Two native Xtensa modules now compile with the pinned classic ESP32 toolchain.
They exercise floating arithmetic, persistent BSS, an address-bearing initialized
data reference and four real relative relocations. Host image checks pass.
Actual post-boot upload/execution passes through Fab/MOS and both hybrid Xtensa
engines: 44 replies / 440 bytes match, changed instructions execute at reused
IRAM addresses, and the guest returns to the MOS prompt. Physical hardware
remains untested and unflashed. See the [qualification report](../runtime/qualification.md).

Source is [numeric.c](numeric.c); build with [build_modules.py](build_modules.py)
using Golem's `.venv/bin/python`. Runtime/header/build details are in the
[runtime README](../runtime/README.md). All generated artifacts remain ignored
under build/; modules.json contains compiler/source/header identities, hashes,
entry virtual addresses and actual section sizes.

| Artifact | Upload bytes | Executable bytes | Data + BSS | Relative fixups |
| --- | --- | --- | --- | --- |
| numeric-1.vdpl | 1,472 | 468 | 20 | 4 |
| numeric-2.vdpl | 1,460 | 456 | 20 | 4 |

Both entries begin at ELF virtual address 0x174, sixteen bytes into .text. The
loader chooses actual executable/data storage and patches all address literals
and the initialized settings pointer. Module compiler stack usage is 112 bytes
for the single entry, excluding host services and the loader. The two variants
share the ABI and argument format but contain different floating instructions:
variant 1 uses multiply/add, variant 2 uses add/add.

The module consumes a four-byte argument block with input buffer ID:u16 and
output buffer ID:u16. Input is binary32; output is binary32 followed by a u32
invocation counter. No source/readback implementation is reused to derive these
independent, binary-exact expectations:

| Fresh-load variant / call | Input bits | Result bits | Counter |
| --- | --- | --- | --- |
| 1 / first | 0x40000000 (2) | 0x40500000 (3.25) | 1 |
| 1 / changed | 0xC0800000 (-4) | 0xC0B80000 (-5.75) | 2 |
| 1 / repeated | 0xC0800000 (-4) | 0xC0B80000 (-5.75) | 3 |
| 1 / after reload | 0x00000000 (0) | 0x3E800000 (0.25) | 1 |
| 2 / replacement code | 0x40000000 (2) | 0x40900000 (4.5) | 1 |

INIT checks zero BSS, the relocated pointer and the variant build identity.
No numeric kernel is linked into firmware. The consumer may include these bytes
in its separately built assembly application; the VDP still receives and loads
them after boot. Changed module instructions require no firmware rebuild.

The runtime snapshots numeric-firmware and numeric-firmware-v2 are preserved
under ../runtime/build/. **Use v2**: it includes owner-based protection against
different buffer IDs sharing one backing block. Reused IRAM with changed code is
the key emulator invalidation regression. Deliberately forcing another allocation
placement is deferred; the prototype adds no special wire operation for it.

Normal runtime calls can leave results resident and request no response. This
proof's diagnostic READ16 requests and administrative replies are additional
traffic, not a proposed per-frame requirement. One stock WRITE costs package
bytes plus eight framing bytes; LOAD costs 19, direct RUN 17, UNLOAD 13 and each
requested reply 12 return bytes. A preinstalled RUN wrapper is invoked through
the ordinary six-byte buffered call.
