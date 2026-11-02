# Prepared display-origin diagnostic

The separate control and instrumented firmware builds pass. The instrumented
image adds 1,184 flashed bytes and 544 bytes of static internal RAM against the
matched control. Its journal can distinguish a default lookup allocation
returning null from a pointer disappearing before or after display publication.
It has **not been run**: interpretation and any repair still require a separately
authorized matched experiment.

## Prepared inputs and outputs

The reviewed scope is frozen in
`../display-origin-diagnostic-freeze.json` (proposal SHA-256
`4145de67d31212fa0ce30ce428d31d5f891842dbfffc304576211e760a03f74a`).
`prepare.py` copies original VDPL-010 source and cached dependencies, changes
three existing files, and adds private `g057_diag.h`. The original export,
original b42/a10 artifacts, SDK production source, upstream source, consumers
and unchanged 140e backend remain untouched.

| Variant | Firmware bytes | Firmware SHA-256 | ELF SHA-256 |
| --- | ---: | --- | --- |
| Original010 | 1,092,448 | `b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6` | `a10f7989d5c1f9974fc999428865ca74771fb65a0d4d694a2ac2b81c19df9902` |
| Macro-off control | 1,092,976 | `f55c45588cce080e57c9e7b7ec431cd99a326aed4b3cc65e632e3edb8cafcf4d` | See `build-results.json` |
| Instrumented | 1,094,160 | `6284230990419db851139657618d92cafd13d52b9af4cd197293144fac1998cf` | `7d90f167e102896e7f2e5544b3b5442c290903407b85a802f7321a4d72b9b801` |

Source copies and build artifacts are under
`/Agon/mystuff/golem/docs/tasks/GOLEM-057/build/display-origin-diagnostic/`:
`control/` and `instrumented/`, each with artifacts at
`.pio/build/esp32dev/{firmware.bin,firmware.elf,firmware.map,bootloader.bin,partitions.bin}`.
That build directory is ignored. Exact artifact paths, hashes and section
sizes are in `build-results.json`; every copied/prepared source hash is in
`source-manifests.json.gz`, with the compact index in `preparation.json`.
The saved macro-on patch is `source.patch`; macro-off differs only by setting
the new private header's `G057_DIAG_ENABLED` to zero.

The control is **not** relabeled b42/a10. Its eight embedded SDK assert/source
paths have a 66-byte longer task-local package prefix, accounting for the
528-byte `.flash.rodata` increase against the original. Flash text also differs
by -8 bytes and relocated data differs; byte equivalence is not claimed.
The three macro-off edited sources match original text after whitespace removal,
and both expanded build-flag/define sets match (`tool-flags.json`). No diagnostic
symbols survive in the control ELF. Bootloader and partition binaries match the
original in both builds.

## Cost and checks

| Resource | Instrumented minus matched control |
| --- | ---: |
| Flashed image file | +1,184 bytes |
| `.dram0.bss` | +544 bytes: 512-byte journal plus 32-byte context |
| `.dram0.data` | 0 bytes |
| `.iram0.text` | +4 bytes |
| `.flash.rodata` | +56 bytes |
| `.flash.text` | +1,128 bytes |

All six emitted diagnostic helper/bridge entry frames are 32 bytes in Xtensa
disassembly. The existing `createSignalList` frame grows from 48 to 64 bytes;
`createPalette` stays 64, `setResolution` stays 48 and `vdu_mode` stays 32.
The explicit flush→UART bridge contributes two nested 32-byte frames before
Arduino/UART calls. These are compiler frame observations, **not** measured
whole-call-chain stack high-water. Nested UART/RTOS stack and actual timing
remain unmeasured. There is no new diagnostic heap allocation, task, timer or
controller member. Renamed `_GLOBAL__sub_I/_D_g057_records` symbols are the
translation unit's existing initialization/destruction functions, not journal
object constructors; the record/context types are plain static zeroed storage.

The static journal and changed code/stack/serial timing can change allocation
layout or reproduction. A clean diagnostic run would not establish a fix.
An observed null allocator return would describe that run, not prove physical
ESP32 exhaustion or connect the separate D/IRAM alias question.

Host checks pass for classification (raw skip, allocation call, existing null,
existing nonnull), context/attempt identity, one-shot capture, no early or
duplicate emission, zero unused slots, saturation/no overwrite and binary
decoder handling. The C++ checks ran with ASan/UBSan; leak detection was disabled
because LeakSanitizer cannot operate under this tool runner's ptrace.
`source-verification.json` confirms all 4,756 prepared file hashes in each copy,
unchanged originals, macro-off source equivalence, exactly one active allocation
call per recorded allocation site, emission before GPIO/interrupt allocation,
and no hooks in the VGA2 ISR or `getSignalsForScanline`.

Both builds used two jobs. PlatformIO reported 70.41 seconds for control and
67.87 seconds for instrumented. These are build times, not runtime performance.
The original `platformio.ini` and original feature header are unchanged.
The current SDK exporter was never invoked.

## Reproduction and retained setup failures

`prepare.py` refuses to overwrite any existing preparation. It preserves the
original cached GL `.git` metadata solely so PlatformIO recognizes its pinned
installed dependency offline; source inventories exclude that metadata.
PlatformIO uses task-local state with `packages` and `platforms` symlinks to the
already installed, read-only toolchain. Exact build commands are recorded in
`build-results.json`. Expanded flags/defines are in `tool-flags.json`; raw
environment dumps remain only in the ignored build directory and are not part
of the distributable evidence.

One incomplete initial preparation is retained as
`build/display-origin-diagnostic-preparation-attempt1`. Two setup failures are
preserved in `control-build-attempt1.log` (default PlatformIO lock outside the
writable workspace) and `control-build-attempt2.log` (excluded GL metadata made
PlatformIO attempt dependency resolution; network name resolution failed).
Neither compiled firmware or changed the original. Retaining copied GL metadata
and using task-local PlatformIO state resolved them. No source compilation
failure or firmware repair was necessary.

Repeat host checks without any emulator:

```bash
cd /Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-057/display-diagnostic/verify_preparation.py
.venv/bin/python docs/tasks/GOLEM-057/display-diagnostic/check_parser.py
ASAN_OPTIONS=detect_leaks=0 docs/tasks/GOLEM-057/build/display-origin-diagnostic/check-journal
```

## Per-run journal contract for the consumer owner

This is preparation guidance, **not permission to execute**. Use the final
firmware and its matching ELF hashes above. Keep original/control/instrumented
runs separate with identical consumer/module/source, mode request and load
order. Retain normal consumer completion/mode-result evidence and independently
captured ISR origin under the new ELF. Do not reuse old numerical PCs or pointer
addresses as identities.

Capture complete binary UART0 in a fresh existing artifact directory, stop/join
normally, then retain/hash `esp32-uart0.log`. Source proof and I/O limits for the
exact unchanged 140e backend are in [UART-source-proof.md](UART-source-proof.md).
Decode with:

```bash
.venv/bin/python docs/tasks/GOLEM-057/display-diagnostic/parse_journal.py /absolute/path/esp32-uart0.log
```

Records are 64 bytes, sixteen little-endian unsigned 32-bit words, embedded at
arbitrary byte offsets among normal UART text. No text decoding or line splitting.

| Word | Meaning |
| --- | --- |
| 0 | `0x47570100 | event`: 1 DEFAULT_PLANE, 2 INITIAL_LIST peek, 3 PREACTIVATE |
| 1 | Ordinal 1–8 in low byte. Bits 8 allocation-called; 9 lookup-required; 10 map-present; 11 raw-skip; 12 existing-null; 13 existing-nonnull; 14 selection-peek; 15 saturated |
| 2 | Request sequence |
| 3 | Bytes: requested mode, previous mode, attempted mode, tag. Tag low 2 bits phase (1 requested, 2 restore previous, 3 final fallback); high 6 bits attempt ordinal |
| 4 | Controller pointer: correlate with this run's ISR controller register |
| 5 | Native format: 0 Mono, 1 SBGR2222, 2 RGB565BE, 3 PALETTE2, 4 PALETTE4, 5 PALETTE8, 6 PALETTE16 |
| 6 | Configured signal-table bytes |
| 7–9 | Allocation requested bytes, caps, actual return; allocation-called distinguishes a null return from no call |
| 10 | Default-map pointer; map-present distinguishes absent from existing-null |
| 11 | List head, or newly allocated node in INITIAL_LIST |
| 12 | Current node, PREACTIVATE only |
| 13 | Selected signals peek in INITIAL_LIST; actual current-node signals in PREACTIVATE |
| 14 | Palette ID low 16, initial list count next 8, row last 8; zero for PREACTIVATE |
| 15 | Viewport width low 16, height high 16, PREACTIVATE only |

Only the first MODE 20 transaction arms capture. Controller identity is
request sequence + attempt ordinal + pointer; address reuse across attempts is
allowed. The normal requested/fallback case produces six records; never assume
that count. Eight records, a saturated flag, missing/truncated stages or a
controller mismatch mean incomplete coverage. No footer exists: even fewer than
eight records require independent proof that the MODE transaction ended and no
later relevant event was omitted. The parser never promotes UART prefix length
to transaction completion. Deferred records are emitted only before activation;
a fault before that point may leave no emitted diagnostic record.

Raw SBGR2222 with no lookup plane is expected. For a lookup format, compare the
actual allocation return to the list peek and PREACTIVATE pointers. Nonnull
publication followed by a null table in independently captured ISR context
narrows the time window; it does not identify an exact write. No readiness
check, retry, allocator repair, null-ISR guard or recovery is included.
