# Default lookup-plane diagnostic proposal

Use one small, format-aware journal to capture the default lookup allocation,
initial list selection and final publication before the candidate display
starts. Keep at most eight records, preserve every existing allocation and
decision, and build a separate copy of the original VDPL-010 firmware. This can
distinguish an allocation returning null from a pointer becoming null later. It
does not repair mode admission or establish physical out-of-memory as the cause.

**Status:** proposal for parent review, 2026-10-30. No source changes, build,
emulator run, hardware action or publication is performed by this document.

## Frozen input and question

Use `/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src`, including its
existing `.pio/libdeps/esp32dev/vdp-gl` copy. The upstream pins are VDP
`1056ee38ec1cc68ed327bd32ee47d04b1c30597c` and graphics
`ac2dd5986daf496c43ae8e7fe41836274aec54a0`.

| Frozen input | SHA-256 |
| --- | --- |
| `firmware.bin` | `b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6` |
| `firmware.elf` | `a10f7989d5c1f9974fc999428865ca74771fb65a0d4d694a2ac2b81c19df9902` |
| `firmware-source.json` | `bfe71a8a2788e54255a1f48a6f8ec5990434b703f908f092a925c65ee225f19c` |
| `platformio.ini` | `7947cbd2e5fcb38e12ca3b8dee44b97700da0ca1d05c19914b100d47885f0bb1` |
| `video/vdu.h` | `24923b53a83d77ebf729795d7ecb7343e6f2fed0e9d748d48fc83c8b75e805cb` |
| GL `dispdrivers/vgapalettedcontroller.cpp` | `5c8f9b6b673d917ed7052a39aaef1f9671b669563f379c40337e3d1a829c2f49` |

The source review and further source pins remain in
[mode-admission-source-review.md](mode-admission-source-review.md) and
[mode-admission-source-pins.json](mode-admission-source-pins.json).

The reconciled Flexe reply
`2026-10-30_21-55-56_flexe_golem057-flexe-origin-reconciled-twentyfive`
now proves the origin of the retained first 32 reads in the fresh interpreter
run: core 1, the frozen VGA2 ISR's lookup instructions, with `a8=a9=a10=0`.
The controller register was `a2=0x3ffb92d4`. That proves a null lookup in that
recorded prefix; it does not capture an allocator return, locate the write that
lost a pointer, or classify the unrecorded suffix. Do not reuse these numerical
PCs or controller addresses as identities in another firmware/run.

## Three event sites, eight records maximum

Arm the journal for the first MODE 20 transaction only. This is a reproduction
filter, not a restriction on supported modes. In `video/vdu.h:vdu_mode`, tag the
three existing `changeMode` call sites (`:307`, `:310`, `:314`) as REQUESTED,
RESTORE_PREVIOUS and FINAL_FALLBACK. Record the original requested mode, the
previous mode captured before the first call, the current attempted mode and
attempt ordinal. Leave the calls, assignments and return handling unchanged.
Disarm after that transaction. No startup or arbitrary later palette tracing.

| Event | Exact source site | Record |
| --- | --- | --- |
| DEFAULT_PLANE | GL `vgapalettedcontroller.cpp:createPalette`, `:263–280`, palette ID 0 only | Format, lookup requirement, table size, pre-existing default-map presence/value, whether allocation was called, its requested bytes/caps and actual returned pointer. Distinguish RAW_SKIP, EXISTING_NONNULL, EXISTING_NULL and ALLOC_RESULT. |
| INITIAL_LIST | `createSignalList`, immediately after the existing node allocation at `:363`, before the unchecked node write | Node request/caps/actual return; initial raw-list palette ID/row/count; nonmutating default-map presence/value and the selection that the existing code will make. This is explicitly a selection **peek**, not a published list. Only the initial one-node default list is recorded. |
| PREACTIVATE | `setResolution`, after `updateSignalList` at `:158` and the existing calculation, immediately before `startGPIOStream` at `:163` | Actual default-map presence/value, list head, current item, current item's signals, table size and viewport dimensions. Append and emit the pending records here, before candidate GPIO/interrupt activation. |

Controller identity is `(request sequence, attempt ordinal, this pointer)`.
Use the constructor's already initialized `m_nativePixelFormat`; do not infer
format from a mode number. PALETTE2/4/8/16 require lookup planes. SBGR2222 with
table size zero deliberately skips lookup allocation. VGA2's default request is
2,048 bytes (`256 * sizeof(uint64_t)`); record the actual request instead of
hard-coding this into the journal.

For DEFAULT_PLANE, retain the single existing `heap_caps_malloc` call. Store its
return in one local pointer, append the journal entry, then perform the original
map assignment and erase/return handling. C++17 already sequences the assignment
RHS before the LHS; preserve that allocation order. Do not retry, guard or alter
any original allocation, return or failure path. Map inspection must use `find`,
never diagnostic `operator[]`: the latter could itself insert the null key
whose origin is under investigation.

INITIAL_LIST records the allocation before the original unchecked dereference,
but does not add a null guard to the driver. PREACTIVATE does not walk a list or
follow next pointers. Read its already selected current node's signals only
when that node is nonnull; record a missing node directly. No new dereference
of an absent map value. An earlier fault that prevents PREACTIVATE also prevents
deferred emission; that result is incomplete, not a reason to add crash recovery
to this experiment.

The usual requested raw controller plus fallback palette controller needs six
records. The hard cap is eight across the entire transaction, including any
extra default-plane call. Never wrap or overwrite. A saturated journal is
conservatively incomplete. Zero unused slots and dispose them without emission;
emit each committed slot at most once. Do not expand coverage automatically.

## Fixed record and perturbation budget

Use sixteen 32-bit words per record, with a compile-time 64-byte size assertion.
All pointers are target 32-bit values. Fill a static slot directly; do not copy
a full record onto the stack.

| Word | Meaning |
| --- | --- |
| 0 | Magic plus schema/version/event kind |
| 1 | Record ordinal plus flags: allocation-called, lookup-required, map-present, raw-skip/existing-null, selected-value-is-peek |
| 2 | Request sequence |
| 3 | Requested mode, prior mode, attempted mode, attempt phase/ordinal (packed bytes; phase/ordinal share the last byte) |
| 4 | Controller `this` pointer |
| 5 | Native pixel-format enum |
| 6 | Signal-table size |
| 7 | Allocation requested bytes, or zero when not called |
| 8 | Allocation capability mask, or zero when not called |
| 9 | Actual allocator return, or zero when not called |
| 10 | Default-map pointer, with presence separately indicated |
| 11 | List head; INITIAL_LIST uses its newly allocated node value |
| 12 | Current-item pointer (PREACTIVATE only) |
| 13 | Selected signals: explicit peek for INITIAL_LIST, actual current-node value for PREACTIVATE |
| 14 | Palette ID and initial list count/row tag; zero for PREACTIVATE |
| 15 | Viewport width/height (PREACTIVATE only), otherwise zero |

Reserve eight records (512 bytes) and eight 32-bit context/control words
(32 bytes), **544 bytes planned static internal RAM**. Context holds request
and attempt sequences, packed mode tags, used/emitted counts, armed/closed/
overflow flags and reserved words. No diagnostic heap, PSRAM, per-controller
member, task, timer or IRQ hook is required. Do not add heap-wide statistics or
allocator interception.

Put private declarations in one new diagnostic header under the cloned GL
`src/`; keep journal storage/helpers in the cloned GL translation unit. Add one
small foreground emission bridge beside `DBGSerial` in cloned `video/video.ino`.
Use a diagnostic-only macro and compile its source hooks out completely in the
matched control. No SDK/service/VDU ABI is added.

Emit fixed binary records through existing UART0 `DBGSerial.write` rather than
the VLA-based formatted logger. Emit only at PREACTIVATE, never in
`getSignalsForScanline`, an ISR or an interrupt-allocation callback. This means
"about to activate", not proof that activation completed. Check that the
existing harness preserves raw UART0 bytes before the eventual run; do not
invent a firmware wire command for collection. Eight records are 512 UART bytes,
about 44.5 ms of 115200-baud 8N1 wire time, not a measured elapsed-time bound.

The static journal, helper code, stack use and serial timing can change the
allocation layout and race timing. Measure added flash/IRAM/DRAM and compiler
stack estimates from matched builds before execution; the actual total cost is
not yet known. A nonreproduction is inconclusive. An allocation-null record is
evidence about this diagnostic run, not proof of physical RAM exhaustion in the
original image.

## Reproducible separate build route

Keep original b42/a10 artifacts and original source/libdeps unchanged. Stage two
new task-local roots under `GOLEM-057/build/display-origin-diagnostic/`: `control`
and `instrumented`. Copy the full original exported project and its exact
`.pio/libdeps` into each, excluding `.pio/build`, stale object/archive/output
files and repository metadata. Hash the copied sources before patching. This is
a source copy, not a newly generated SDK export.

Do **not** invoke the current SDK `tools/build_firmware.py`: it copies current
runtime/SDK headers into the export and would contaminate this original-010
control. Retain original `video/vdplet_build.h` (pixels on, sink off, lean PCM on)
and `platformio.ini` unchanged. Preserve espressif32 6.6.0, Arduino ESP32 2.0.14,
ESP-IDF 4.4.6, Xtensa GCC 8.4.0, `-O2`, C++17 and pinned original dependencies.

After review releases implementation, apply one saved, hash-anchored patch to
the copies. Build the macro-off control and macro-on diagnostic directly with
the SDK's existing PlatformIO installation, equivalent to:

```bash
/Agon/mystuff/agon-vdplet/.venv/bin/python -m platformio run \
  -d /Agon/mystuff/golem/docs/tasks/GOLEM-057/build/display-origin-diagnostic/control \
  -e esp32dev -j 2
/Agon/mystuff/agon-vdplet/.venv/bin/python -m platformio run \
  -d /Agon/mystuff/golem/docs/tasks/GOLEM-057/build/display-origin-diagnostic/instrumented \
  -e esp32dev -j 2
```

Select the macro inside the private diagnostic header in the two clones; do
not replace the original PlatformIO flags. Hash the patch, source manifests,
toolchain/packages, flags, binary, ELF and map. Compare section sizes and stack
estimates. Confirm macro-off journal symbols/hooks are absent. Compare the
macro-off rebuild with the original artifacts and report any nondeterminism;
never label a different rebuild b42/a10. Diagnose new PCs using the new ELF.
No build or run is executed until this proposal is frozen and released.

## Reading the eventual result

Use the same consumer bytes, module, requested mode, launch/load order and
reproduction harness in separate fresh original/control/diagnostic runs. Match
allocation/publication records and captured ISR context within each diagnostic
run by its controller address and attempt identity. No timing or speed claim.

| Diagnostic observations for the lookup-requiring controller | Supported conclusion |
| --- | --- |
| Actual default-plane allocation returns null; later list/publication select null | Allocation-null route is present in this diagnostic run. Map reinsertion can explain why later code sees an existing null default. The allocator/model cause remains open. |
| Allocation returns nonnull; PREACTIVATE map or selected signals is null | Pointer selection/loss occurred before activation; initial allocation failure is not the explanation for that recorded pointer. |
| Allocation and PREACTIVATE selection are nonnull; new retained ISR context for that same controller reads a null table | Failure lies later than this publication snapshot, or involves selection/routing/model behavior; this does not identify the exact write. |
| Raw SBGR2222 skips allocation | Expected format behavior; not a failed lookup. |
| Journal full, missing stage, unmatched controller, no new ISR origin, or nonreproduction | Incomplete or perturbation-sensitive; do not claim a fix or silently broaden the trace. |

This proposal excludes readiness repairs, allocator changes, map cleanup,
lookup guards, mode-order changes, renderer changes, D/IRAM alias repairs and
whole-driver allocation tracing. Review the retained evidence before deciding
whether any repair is justified.
