# Fab/Flexe integration, native vdplets and shared drawing entries

The Author's eight-hour authorization produced a working owned emulator,
postboot-loaded native Xtensa modules and a successful service handoff to
agon-6502. Its supplied F2/F3 codec also passes actual execution and pixel checks.
The optional Golem tranche adds multiple named drawing Subs with shared globals.
Both human review instances are staged/stopped on Oryx; human acceptance and
explicit commit approval remain pending. No physical device was touched.

## Integration and proof

GOLEM-021 combines Fab 1.2.5's eZ80/MOS/SD/SDL with Flexe's execution of real ESP32
VDP firmware. Owned source exports preserve upstream checkouts. Focused fixes
cover Linux ARM64 host instruction-cache maintenance, architectural ISYNC
invalidation for replaced executable IRAM, UART idle timeout and FIFO/CTS
admission, native frame counting and orderly frontend cleanup. Diagnostic ELF
symbols do not enable Flexe's firmware compatibility hooks. Both engines pass
stock MOS/BASIC, nine video geometries, glyph pixels, serial-console input and
audio. Timing remains functional; physical PS/2 ULP is unmodeled.

The GOLEM-002 loader admits one finite trusted zero-import C/C++ module with a
versioned buffer/frame service ABI. Numeric code demonstrates new inputs,
persistent BSS, reload/reset and changed instructions at reused IRAM. The packed
module draws a recognizable Lode Runner image from resident bytes. The final
Oryx main fixture passes 56 replies/560 bytes and 61,440 physical-VGA image/margin
pixels in each engine, including preservation after invalid source/geometry.
No module kernel is built into firmware or replaced by a host implementation.

A generic loader/service firmware addition is required once. New modules then
load without reflashing/rebuilding that firmware. A direct silent RUN is 17 wire
bytes; a preinstalled RUN packet is invoked with six. Upload, changed input and
test-only diagnostics remain additional traffic.

## Codec owner handoff

The user explicitly authorized contact. The canonical immutable mailbox thread
`agon6502-golem-vdplets` records the offer, acceptance, supplied package and
returned proof. The owner retains its source and hardware. Package
`apple-codecs.vdpl`, SHA-256 fd1aa6a5d1c4d51f0f27fd191de896cee4c7e93480f8a611e6e96ac798fd6092,
is 2,288 bytes with a 192-byte module frame and no imports.

All 36 Oryx runs pass across JIT/interpreter: 32,708 checked replies,
2,211,840 image/margin pixels and eight complete 7,680-byte retained reads.
Intermediate calls sample 20 retained bytes; expected intermediate hashes are
not claimed as whole-buffer observations. Actual complete-read hashes are
reconstructed offline from replies. Mode/source/coverage/geometry rejection,
phase preservation, repeated XOR and missing-source zero-length redraw pass.
Minimum observed free command-task stack is 2,196 bytes. Execution totals 348.130 seconds,
including boot/upload/diagnostics; this is a test estimate, not a game benchmark.
The codec profile remains isolated from the unchanged main review.

## Golem tranche

GOLEM-022 implements multiple finite drawing Subs over one initialized global
block. Assembly selects named wrappers; the first remains the legacy default.
Private entry/templates are source ordered, later-only bitmaps are reserved and
affine-enabled, and assembly symbols are validated together before publication.
No new optimizer, stack/local system, inter-Sub calls or scheduler was added.

Maintained contracts, 132 rejection cases, ASan/UBSan and two real ADL consumers
pass. All 120 compared prior package files and two raw payloads are byte-identical.
Focused stock Oryx default and relocated runs each pass 370 checks/415 bytes;
the separate human Wolf/cyan-line scene passes 62 checks/99 bytes headlessly.

## Review boundary

Detailed plans were written before implementation and stable subitems ticked
from evidence. TODO.md retains GOLEM-002 physical work and GOLEM-021/022 review
pending acceptance; nothing is removed as though accepted. Source and receipts
are linked from [GOLEM-021](../tasks/GOLEM-021/README.md),
[codec qualification](../tasks/GOLEM-021/codec/README.md) and
[GOLEM-022](../tasks/GOLEM-022.md). No graphical emulator was launched, no shared
upstream or codec source was edited, and no commit/push was performed.
