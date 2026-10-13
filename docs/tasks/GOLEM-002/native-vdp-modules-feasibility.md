# DRAFT — Runtime-loaded C/C++ vdplets on physical Agon

**Recommendation:** proceed to a tiny physical loading proof using a restricted
adaptation of Espressif's ELF loader, independently of Golem. The original ESP32
and the existing VDP SDK appear suitable; usable executable heap and the exact
module link output remain unverified. A generic loader requires a custom firmware
installation once. Later C/C++ modules should upload after boot without reflashing.
The packed monochrome renderer is a good second proof, but already has compressed
wire traffic: loading it dynamically does not add compression. **Stop here for
Gate A review before implementation or device work.**

Prepared 2026-10-12 for [GOLEM-002](../GOLEM-002.md), Phases A–C. Supporting
deliverables: [C ABI and assembly protocol](vdplet-abi-draft.md), [physical proof
proposal](physical-proof-proposal.md), and [evidence ledger](research/evidence.md).
These are proposals, not a supported runtime or an adopted language feature.

**Additional resource reviewed:** [Flexe research notes](research/flexe-agon-notes.md).
Flexe emulates Xtensa and has an existing Agon VDP/MOS fixture, making execution
of the actual vdplet payload a possible pre-hardware check. This is distinct from
Fab's host-native VDP route; compatibility with our loader remains untested.

## Decision table

| Question | Finding | Decision |
| --- | --- | --- |
| Can stock buffered commands execute uploaded Xtensa code? | No native entry/loader found in the inspected dispatch. Buffer calls interpret VDU streams. The stock updater writes a firmware image and reboots. | Requires a host firmware addition; OTA is not a vdplet loader. |
| Can an original ESP32 execute dynamically loaded native code? | Espressif supplies an ESP32 ELF loader using executable heap. | Feasible in principle; not yet demonstrated in Agon. |
| Does its SDK requirement fit the existing physical build? | Loader 1.3.4 declares IDF >=4.4.3; inspected build uses 4.4.6. | No SDK upgrade justified yet; Arduino/SCons adaptation still needed. |
| Can Agon's PSRAM hold executable modules? | The loader's PSRAM execution option excludes classic ESP32. | Use internal executable RAM. PSRAM can hold staging/data, subject to ordinary restrictions. |
| Are ordinary arbitrary `.so` files ready to use? | Loader accepts a restricted section/relocation set; original-ESP32 import issue remains open. | Unsuitable as an unrestricted plugin promise. Start without external ELF imports. |
| Is there enough code memory? | Existing ELF leaves 35,868 bytes between `_iram_end` and its IRAM region end. | Encouraging static evidence only. Measure executable free/largest blocks after video/audio setup. |
| Can assembly use this without Golem? | It can upload with stock buffer writes and issue a proposed generic extension request. | Assembly is the first consumer and owns the lifecycle. |
| Can a resident command stream invoke it? | The same VDU dispatcher processes resident streams synchronously. | Feasible with finite calls and no nested interpreter service; qualify later. |
| Does Fab execute the same Xtensa payload? | Fab runs host-native VDP code. | No. A host-native counterpart would be separate work. |
| Could Flexe execute the Xtensa payload? | Its source/docs provide ESP32 emulation and a pinned Agon VDP/MOS fixture. | Candidate preflight; dynamic loading/reloading and our custom image still need qualification. |
| Does this save traffic against the existing custom packed blit? | That renderer already accepts a small request and retains compressed reconstructed pages. | No automatic saving; preserve the pipeline and count added module framing honestly. |

Original-ESP32 support and the version requirement are documented by
[Espressif's component registry](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/readme)
and [dependency manifest](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/dependencies?language=en).
The inspected source package is pinned below; registry “latest” is not a build pin.

## Platform actually inspected

| Item | Identity / evidence |
| --- | --- |
| Target | Classic ESP32, `esp32dev`, Xtensa little-endian, 32-bit pointers, windowed ABI. Physical chip revision and installed firmware still need confirmation at the hardware gate. |
| Reference application | agon-6502 HEAD `f4190ae5c2220269809bcfeb41952e189f4f6b00`; VDP-003 working experiment, not an immutable release. |
| VDP source | Export pin `1056ee38ec1cc68ed327bd32ee47d04b1c30597c`, plus the experiment's inserted packed blit/dispatch. |
| Physical graphics | `AgonPlatform/vdp-gl` commit `ac2dd5986daf496c43ae8e7fe41836274aec54a0`, installed as 1.0.5+sha.ac2dd59. |
| Platform / framework | PlatformIO `espressif32@6.6.0`; Arduino package `3.20014.231204` = Arduino 2.0.14; SDK header identifies ESP-IDF 4.4.6. |
| Compiler | `xtensa-esp32-elf` GCC 8.4.0, `esp-2021r2-patch5`. Confirmed by build log, installed package and ELF `.comment`; the ELF also contains older prebuilt library compiler strings. |
| Flags | `-O2`, GNU C++17, `BOARD_HAS_PSRAM`, `-mfix-esp32-psram-cache-issue`; 240 MHz CPU, 80 MHz flash, QIO. SDK enables PSRAM malloc/cache workaround. |
| Other resolved dependencies | ESP32Time 2.0.6; CRC 1.0.4. |
| Physical firmware image | SHA-256 `c02a35cb5d91da74fb3ff9d06335a7f69f21fdecc5571394aa7302a3ce778ff6`, matches the prior qualification record. No device was queried this tranche. |
| Fab dependencies | Fab 1.2.5, userspace-vdp-gl `cdd57fd4d709427b2e89738ae3b086720d8cf013`; different graphics implementation from physical firmware. |

The Oryx installation also contains an unversioned Arduino 2.0.16/GCC 12.2 pair.
Those are **not** the packages recorded in this firmware's build log/map. Reusing
the first executable found on PATH would select the wrong development baseline.
[Read-only inspection receipt](research/physical-build-inspection.txt) retains the
actual package, map, SDK and compiler observations, including artifact hashes.

Ownership recommendation: a standalone user-owned `vdplet` runtime/SDK project
should eventually own the ABI, loader adapter and examples. No repository has
been created. Keep the first prototype in this task silo, exporting the pinned
VDP into an ignored build tree. Do not patch the active agon-6502/Pingo/Wolf
checkouts. The current user-owned `agon-vdp` checkout has audio work in progress;
it is not a neutral integration destination. Before shared adoption, agree an
owner and use the canonical cross-project contract. This recommendation does not
claim ownership on another agent's behalf.

## Loader choice and required adaptation

The downloaded **elf_loader 1.3.4** package identifies source commit
`58c2f852125cb1606c2137fc9b697efc3ce1c3d0`; its archive SHA-256 is in
[loader-source.json](research/loader-source.json). Apache-2.0 attribution/license
must accompany any later vendoring. Source was downloaded for inspection only;
no component was installed, compiled or executed.

| Option | Cost / limitation | Recommendation |
| --- | --- | --- |
| Adapt pinned Espressif loader, restricted ELF | Existing allocation/section/relocation machinery; needs SCons/config adaptation, bounded validation and narrow fixes. | First choice. |
| General filesystem `dlopen` / arbitrary ELF imports | Extra filesystem, symbols and linkage surface; import issue on original ESP32. | Defer. MOS already owns filenames. |
| Custom two-segment format with word-pointer fixups | Could reduce parser/staging overhead, but needs a PC converter, specified relocation semantics and independently verified runtime linker. | Fallback only if the restricted ELF route proves disproportionate. |
| Fixed-address raw binary in reserved IRAM | Simple copy, but ties module to firmware layout, reserves scarce memory and fails a general relocation claim. | Diagnostic fallback only; not the desired facility. |
| Flash-resident module partitions/MMU mapping | Different persistence, update and recovery problem. | Outside this first proof. |

Source findings requiring explicit treatment:

- ELF-001: `esp_elf_relocate()` takes bytes without a length. Validate a bounded
  image before calling it; reject invalid table/string/section/symbol references.
- ELF-002: The original-ESP32 section path recognizes `.text`, `.data`, `.rodata`,
  `.data.rel.ro`, `.bss`; it does not handle every allocated section.
- ELF-003: The Xtensa backend handles RELATIVE, GLOB_DAT, JMP_SLOT and a no-op
  RTLD case. It is not an arbitrary relocatable `.o` linker.
- ELF-004: Its relocation caller ignores the architecture routine's return value.
  A narrow adaptation must propagate errors, validate mapped locations and free
  partial allocations. An unsupported relocation must never become a ready module.
- ELF-005: `esp_elf_request()` uses an argc/argv entry and discards its result.
  A typed vdplet entry adapter is needed; do not cast the service table into argv.

These observations come from the pinned
[loader](https://raw.githubusercontent.com/espressif/esp-iot-solution/58c2f852125cb1606c2137fc9b697efc3ce1c3d0/components/elf_loader/src/esp_elf.c)
and [Xtensa backend](https://raw.githubusercontent.com/espressif/esp-iot-solution/58c2f852125cb1606c2137fc9b697efc3ce1c3d0/components/elf_loader/src/arch/esp_elf_xtensa.c).
They are inspection findings, not locally reproduced failures.

[Issue 686](https://github.com/espressif/esp-iot-solution/issues/686) reports an
original-ESP32 crash with several imported functions and an omitted `.got.loc`
region. It remains open as inspected. Version 1.3.4 still does not recognize that
section; its build helper also strips it. Do not assume stripping a needed literal
pool repairs a module. Require no undefined ELF symbols, no PLT imports and no
unhandled allocated sections in the first proof. This avoids the reported import
path; it does **not** prove all PIC output safe.

Start with an ELF32 little-endian Xtensa ET_DYN payload, one explicit C-linkage
entry, the recognized sections and **only validated R_XTENSA_RELATIVE records**.
Reject other relocation kinds initially. Inspect the actual GCC 8.4 output before
confirming this profile; if the tiny module needs another kind, stop and explain
that concrete need. Do not silently extend the runtime linker.

Candidate compilation: the exact firmware toolchain, `-O2 -fPIC -mlongcalls
-mtext-section-literals -mfix-esp32-psram-cache-issue`, hidden internal symbols,
no LTO; C++17 plus no exceptions/RTTI/thread-safe local statics. Link without
startup/default libraries, with the explicit entry and section garbage collection;
disable unwind-table generation and reject unexpected unwind/constructor sections.
Final flags/link layout remain a proof deliverable. GCC documents
[literal-pool placement](https://gcc.gnu.org/onlinedocs/gcc-8.4.0/gcc/Xtensa-Options.html);
matching the name of an ELF file is insufficient evidence of loadability.

No STL, constructors/destructors, TLS, module heap, recursion, threads or imports
of private firmware objects in v1. Pure C++ arithmetic/helpers are fine. Pass host
services as function pointers at entry. Compiler helpers such as divide/conversion
or memcpy must either be eliminated, deliberately linked into the module and
accounted for, or given a later explicit service. No implicit libgcc/libc/libm
export. Build rejection should name the unresolved symbol or section.

## Executable memory and stack

Classic ESP32 instructions belong in internal executable memory. The pinned
[allocator adapter](https://raw.githubusercontent.com/espressif/esp-iot-solution/58c2f852125cb1606c2137fc9b697efc3ce1c3d0/components/elf_loader/src/esp_elf_adapter.c)
requests `MALLOC_CAP_EXEC` for code and byte-accessible memory for data. Its PSRAM
execution branch is for other chips. Do not copy S3 cache/MMU recipes onto Agon.

IRAM reads/writes require aligned 32-bit accesses; normal strings, packed pixels
and float data need appropriate data memory. Stage/pad text in aligned storage,
copy/patch through aligned word accesses, and inspect literal access in the
disassembly. Place ordinary constants/float arrays in DRAM, not casually in text.
Use `L32R` literal loads only where the emitted access is valid. In particular,
do not assume floating load/store instructions can address instruction RAM.
[ESP-IDF memory types](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/memory-types.html)
and [external RAM restrictions](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/external-ram.html)
describe the separate buses and PSRAM data mapping. The classic internal-IRAM
loader path has no external-RAM cache-flush step. Verify code visibility on the
chosen core in the proof; do not disable caches/interrupts around normal calls.

| Existing physical ELF allocation | Bytes / address |
| --- | --- |
| Linker IRAM region | `0x40080000..0x400A0000`, 131,072 bytes |
| End of statically assigned IRAM | `0x400973E4` |
| Unassigned region remainder, calculated | 35,868 bytes; **not measured free executable heap** |
| `.dram0.data` + `.dram0.bss` | 19,536 + 25,152 = 44,688 bytes |
| Existing firmware flash use, historical build record | 1,077,813 bytes; not a runtime code budget |
| Command-processing task stack in source | 4,096 bytes; source comment describes words, but ESP-IDF's creation API uses bytes |

Heap fragmentation and shared D/IRAM allocations mean total free bytes alone are
insufficient. Measure `heap_caps_get_free_size()` **and**
`heap_caps_get_largest_free_block()` for executable, internal byte-accessible and
PSRAM heaps after video/audio and representative buffers are live. Do not sum
overlapping capability totals. The relevant APIs are described in
[heap allocation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/mem_alloc.html).
Stack units are specified by
[ESP-IDF FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/system/freertos.html).

Planning envelope, **not a claim of available memory or compiled size**:

| Item | First-proof target / accounting |
| --- | --- |
| Module text plus literals | Aim <=4 KiB; validate against actual largest executable block and preserve ordinary VDP headroom. |
| Module initialized/zero data | Aim <=1 KiB internal DRAM, with exact BSS/alignment accounting. |
| Uploaded envelope + ELF | Aim <=16 KiB, retained once during relocation, preferably PSRAM-backed. |
| Host metadata / service table | Aim <=1 KiB persistent; measure actual adapter allocations. |
| Loader firmware delta | Unmeasured. Review linked flash/static RAM/IRAM differences before a flash; no size estimate can substitute for that build. |
| Extra stack | Aim <=512 bytes for tiny kernel call chain, verified from compiler stack output and runtime high-water observations; loader peak must be measured separately. |

Peak load memory = staged image + aligned executable allocation + aligned
data/BSS allocation + loader/validation metadata and scratch. Relocation records
can remain in the staged image; no second full image is intended. Release staging
only after relocation/init succeed or fail cleanup completes. Steady-state drops
the staging image once assembly clears its owned upload buffer; the loader only
releases its staging lease. Application buffers/framebuffer are additional costs.
For example, a 16 KiB image, 4 KiB text, 1 KiB data and 1 KiB metadata would mean
22 KiB incremental peak, spread across capabilities, and 6 KiB steady-state.
Those are arithmetic examples, not measured module sizes or admission limits.

If that small envelope does not fit comfortably after real graphics/audio setup,
stop. Do not recover memory by silently disabling audio, changing the game mode,
or reserving all remaining IRAM at boot.

## ABI, ordering and transport recommendation

Use one finite C entry with INIT/RUN/FINI operations and an explicitly supplied,
versioned host service table. One module, one active call, no background callback.
Inputs/outputs stay in pre-existing VDP buffers. Scoped buffer views borrow bytes;
the firmware retains ownership and releases them on normal return. No firmware
C++ container/controller layouts or absolute function addresses cross the ABI.
[The ABI draft](vdplet-abi-draft.md) gives the actual proposed signatures, service
lifetimes, states, status policy and symbolic wire fields.

The renderer's later host service should validate and coordinate framebuffer
access, then expose row spans. The **module** must perform the row-copy loop.
Exporting the old complete blit as a service and uploading a one-line caller
would prove loading, but would not deliver a renderer vdplet. Preserve the exact
single-block source size and packed PALETTE2 geometry rules, no vblank wait and
no claim of tear-free scanout.

Assembly reads the payload from storage, uploads it with existing buffered WRITE,
requests LOAD, updates data, requests RUN, then UNLOAD after ordered completion.
The proposed generic extension is separate from Buffered Commands API opcodes;
its numeric allocation remains undecided. Unsupported-firmware discovery must
avoid sending length/body bytes into stock VDU parsing. No private `0xF0`
allocation is adopted here. Native calls from a resident stream may use the same
handler, but module services must not invoke the buffered interpreter recursively.

The command task stays occupied during a native call, so all work must return
promptly. A serial host timeout cannot cancel it. A bad native pointer or infinite
loop can crash/stall the VDP; there is no sandbox or guaranteed rollback. Render
transactions and borrowed storage release on defined error returns; no promise
of recovery after an exception/reset. Unload executes later in the same ordered
stream; an administrative reply or ordinary poll fence can acknowledge progress.
Do not equate MOS transmit completion with VDP execution completion.

For administration, propose the existing small keycode-packet mailbox pattern,
with at most ten payload bytes and explicit opt-in. The inspected MOS parser has
a 16-byte payload buffer, calls the user keyboard vector first and performs normal
keyboard handling afterward. It also has an oversized-discard issue, reinforcing
the small fixed-message restriction. The actual installed MOS version and packet
coexistence remain hardware-gate checks. [Return-hook notes](vdplet-return-channel-notes.md)
describe Pingo/Wolf's worked examples. No reply is compulsory for every RUN;
resident status/results remain usable without one.

## Useful kernels and wire accounting

Rally already implements useful stock float32 scalar work through matrices,
round/truncate/floor, bounded stores/conversions, reciprocal and constrained
division. Main Golem supports checked uint16 arithmetic/divmod and a bounded
float32-floor conversion slice. This project would add an easier native home for
general finite numeric kernels, broader explicit conversions/fixed arithmetic and
selected math functions, rather than establish the first possible VDP float.
See the [Rally audit](../GOLEM-001/rally-capability-integration-proposal.md) and
[maintained scalar contract](../../design/resident-scalar.md).

Choose a tiny float32 scale/add kernel with persistent counter for proof. Then
port the packed renderer. General sqrt/trigonometry/division helpers can follow
only with their domains/dependencies defined; all of libm is unnecessary now.

| Traffic category | Proposed / current cost |
| --- | --- |
| Generic loader firmware | One installation, separately accounted; not per application call. |
| Module upload | Image M bytes + 8 bytes for one stock WRITE in the first <=16 KiB proof, plus a 6-byte clear if needed; no compression assumed. |
| Proposed LOAD request | 19 bytes under the draft symbolic framing; optional 12-byte administrative reply. |
| Proposed direct RUN | 17 bytes, referring to a preinstalled argument block; argument updates counted separately. |
| Resident wrapper invocation | Stock 6-byte buffer call once wrapper/arguments are installed; changed inputs still need their own updates. |
| Current agon-6502 custom blit | 14 request bytes, excluding existing compressed updates. A generic direct RUN is 3 bytes larger before argument updates. |
| Current Golem floor example | 1,008 bootstrap bytes; 6-byte invoke / 21-byte fresh-input-plus-call, excluding result retrieval. |
| Optional completion | 12 return bytes per small administrative packet. No mandatory per-render packet. |
| Result retrieval | Explicit query chunks/diagnostics; entirely additional, not hidden inside the invoke figure. |

These proposed counts derive from [the draft fields](vdplet-abi-draft.md), not an
implemented wire allocation. Reusing resident code/data can reduce recurring
traffic against sending operations or doing round trips from the eZ80. It does
not guarantee fewer bytes than an already resident Golem/Rally job. For the
packed renderer retain XOR/RLE updates, the stock resident page decoder and the
6,720-byte reconstructed page. Module replacement changes the deployment model,
not the image encoding. Benchmarking is not needed to answer this first question.

## Gate A recommendation and remaining unknowns

Authorize only the [tiny physical proof](physical-proof-proposal.md) next, with
the restricted ELF adapter and buffer services. Keep framebuffer services as a
reviewed draft until Gate B. Prototype artifacts stay here; agree eventual
standalone ownership before promotion or changes in other projects.

- DEC-001: Provisional approach: pinned existing loader, zero external imports,
  windowed ABI, internal executable heap, one finite module. Avoid an SDK upgrade
  or a custom general linker until concrete build evidence requires either.
- DEC-002: Decisive unknowns: GCC 8.4 PIC sections/relocations/literal accesses,
  Arduino/SCons integration and loader footprint, actual executable largest block
  with graphics/audio live, stack headroom, physical reply/MOS behavior.
- DEC-003: Effort estimate: roughly 6–16 engineering hours for adapter, tiny
  module, assembly caller and focused checks, then a physical review session.
  Toolchain/relocation surprises may exceed that range; stop at a demonstrated
  incompatibility rather than expand the scope. This is a planning estimate,
  not a measured duration or a promise of unattended completion.
- DEC-004: Do not adopt the packed renderer, contact another agent, flash a device
  or start a Golem native backend on the strength of this report. Gate A selects
  the next implementation tranche; physical flashing still needs authorization
  if not included in that selection.

This tranche performed source review, a pinned source download, and read-only
inspection of existing Oryx artifacts/toolchain. It performed no firmware/module
builds, hardware or emulator execution, flashing, deployment or other-project edits.
