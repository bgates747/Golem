# GOLEM-002 research evidence — 2026-10-12

Source and existing-artifact inspection supports a restricted ELF proof on the
original ESP32. No new native execution or hardware measurements occurred.
“Verified” below means inspected in this tranche, not physically qualified here.

## Evidence ledger

| ID | Classification | Evidence / limit |
| --- | --- | --- |
| EVD-001 | Verified source | Espressif elf_loader 1.3.4 registry archive, commit `58c2f852125cb1606c2137fc9b697efc3ce1c3d0`; [download identity/file inventory](loader-source.json). Extracted sources are an ignored inspection cache. |
| EVD-002 | Verified artifact | [Oryx inspection](physical-build-inspection.txt): firmware log/map/ELF identify Arduino 2.0.14, IDF 4.4.6, GCC 8.4, graphics pin ac2dd59; `.bin` hash matches the previous record. |
| EVD-003 | Verified toolchain | Pinned GCC preprocessor reports windowed ABI, little-endian and four-byte pointers. Its unversioned installed neighbour is a different compiler; not used as the proposed module compiler. |
| EVD-004 | Verified source, inferred feasibility | Loader manifest requires IDF >=4.4.3 and supports classic ESP32. Integrating C sources/configuration into Arduino/SCons appears possible, but has not been compiled here. |
| EVD-005 | Verified source | Pinned loader section/relocation handling, missing input length, ignored architecture return, argc/argv wrapper, allocator/PSRAM conditions inspected. These are not new reproductions of a crash. |
| EVD-006 | External report + verified source | [Espressif issue 686](https://github.com/espressif/esp-iot-solution/issues/686) remains open; describes .got.loc/import failure on 1.3.1. 1.3.4 source still lacks a .got.loc section path. No general claim that all 1.3.4 modules fail. |
| EVD-007 | Verified source | [agon-6502 packed_mono.h](/Agon/mystuff/agon-6502/experiments/vdp003/packed_mono.h), build adapter and exported dispatch: compile-time function, private 0xF0, packed rows, single PALETTE2 buffer. No executable loader. Local hashes in [local-source-identities.json](local-source-identities.json). |
| EVD-008 | Historical physical evidence | [VDP-003 qualification](/Agon/mystuff/agon-6502/docs/validation/2026-10-12-vdp003-packed-blit.json) records user-reported physical success. This tranche independently matched the stored firmware hash, not the flashed device. No performance claim made. |
| EVD-009 | Verified source + historical audit | [Rally audit](../../GOLEM-001/rally-capability-integration-proposal.md), current hosted source, [Golem scalar](../../../design/resident-scalar.md): stock float matrix operations already exist; proposed math modules add convenience/domain/capability. No compiler test run here. |
| EVD-010 | Verified source | Stock `vdu_sys.h`, `vdu_buffered.h`, `updater.h`, `video.ino`: interpreted buffer calls, firmware OTA/reboot, synchronous dispatcher, 4,096-byte command task stack. No module entry found. |
| EVD-011 | Verified source | MOS reference commit `8336409351ee5314e02801a7b72a4f1bb5282519`, `src/vdp_protocol.asm` and `equs.inc`: 16-byte payload limit, key callback precedes keyboard handler, oversized-discard issue. The actual installed MOS identity is unverified. |
| EVD-012 | Previously reviewed worked examples | [Pingo/Wolf notes](../vdplet-return-channel-notes.md) record pinned producers/consumers and twelve-byte completion packets. Re-read here; no new end-to-end replay or promotion to a stable protocol. |
| EVD-013 | Verified convention/source distinction | [Bespoke VDP guide](/Agon/mystuff/agon-dev-env/codex/bespoke-vdp-emulator.md): Fab loads host-native shared libraries. Not Xtensa execution. Physical and userspace graphics pins are distinct. |
| EVD-014 | Calculated, not measured | IRAM region remainder 0x400A0000−0x400973E4 = 35,868 bytes; data+BSS = 44,688 bytes. Actual executable heap/largest block/stack margin await physical proof. |
| EVD-015 | Proposed, not implemented | ABI fields, symbolic protocol and wire counts, resource planning envelope, physical kernel cases and effort range. No opcode assigned, module built, allocator trial or firmware footprint measured. |
| EVD-016 | Verified source + upstream documented results | [Flexe review](flexe-agon-notes.md), commit `fbda8e905ccff36a090e4e33b0ffc2337dd79c52`: real Xtensa execution route with existing Agon VDP/MOS test plumbing. No tests repeated here; our dynamic loader/custom image and executable RAM reload behavior remain unverified. |

## Primary external sources

| Source | Use |
| --- | --- |
| [elf_loader registry](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/readme) and [dependencies](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/dependencies?language=en) | Versioned advertised targets and IDF minimum. |
| [Pinned loader](https://github.com/espressif/esp-iot-solution/tree/58c2f852125cb1606c2137fc9b697efc3ce1c3d0/components/elf_loader) | Read README, manifest, Kconfig, CMake, build helper, changelog, public/private headers, main loader, adapter and Xtensa relocation implementation. Download pin wins over moving master. |
| [ESP-IDF 4.4.6 memory types](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/memory-types.html) | Instruction/data buses, aligned accesses, ordinary IRAM region. |
| [ESP-IDF 4.4.6 external RAM](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/external-ram.html) | PSRAM data mapping/cache/stack restrictions and compiler workaround. |
| [ESP-IDF 4.4.7 heap API](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/mem_alloc.html) | Capability allocators and largest-block measurement. Adjacent-version documentation; actual build version is 4.4.6. |
| [ESP-IDF 4.4.6 FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/system/freertos.html) | ESP-IDF task-stack byte units. |
| [GCC 8.4 Xtensa options](https://gcc.gnu.org/onlinedocs/gcc-8.4.0/gcc/Xtensa-Options.html) | Literal placement/compiler options. |

No firmware/module builds, dependency installs, hardware/serial connections,
flashes, deployment, emulator launches, other-project edits, agent messages,
commits or pushes occurred in this research tranche. SSH only read existing
Oryx project/package files and ran read-only binary/compiler identity inspection.
The GCC preprocessor query produced no compiled object. The receipt includes an
initial unsuccessful SDK-config path lookup followed by the successful QIO SDK
path; no toolchain/configuration was changed to resolve it.
