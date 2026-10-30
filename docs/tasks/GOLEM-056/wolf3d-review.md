# Wolf3DOrig VDPlet feasibility review

The proposed renderer is a plausible consumer of the existing buffer and generic
pixel services. No Wolf-specific firmware hook is identified by this static
review. The roughly 90 KiB application workspace is outside the module's static
data allowance, and a 76,800-byte pixel write has no 16-bit size ceiling. Actual
contiguous memory, linked code/data, complete stack usage and frame time remain
unproved. Preserve the standalone renderer until the proposed port passes its
own gates. This is review only; no conversion or deployment is released.

## Source and request

1. Incoming requirements: [immutable Wolf message](/Agon/mystuff/agon-dev-env/cross-agent/agentcoms/messages/2026-10-30_17-48-39_wolf3dorig_wolf-vdplet-feasibility-requirements-one.md),
   thread `wolf3d-vdplet-migration`, branch `feasibility-v1`.
2. Consumer study: [Wolf migration study](/Agon/mystuff/Wolf3dOrig/agonport/doc/wolf3d-vdplet-migration.md),
   Wolf baseline `7e899cf`; standalone renderer baseline `1dc462e`.
3. Shared runtime reviewed at clean commit
   `2541f10a1f8b06688bcecbc6d22eeae607da2b63`. See
   [API reference](/Agon/mystuff/agon-vdplet/docs/API-reference.md),
   [runtime source](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h),
   [pixel wrapper](/Agon/mystuff/agon-vdplet/src/vdplet_color.h) and
   [pixel kernel](/Agon/mystuff/agon-vdplet/src/vdplet_pixels.h).

## ABI, ownership and completion corrections

1. **Capabilities.** Module requirement mask `17` means buffers plus pixels;
   `--abi-minor 3` is correct. Discovery must test `(available & 17) == 17`,
   rather than require the firmware's entire mask to equal 17. The current
   `--features full` build supports these services. Base and lean builds lack
   pixels. The old `vdplet_media_host` helper also requires PCM bits and
   callbacks, so it works on current full firmware but is not an independent
   pixels-only accessor. This existing SDK/build coupling is already CNS-001;
   the module need not declare PCM when it never calls PCM. No new service
   allocation is proposed.
2. **Surface and DONE.** Double-buffered mode 8 is selected as mode `136`;
   use `surface_info` to verify 320x240, direct64 and the double-buffer flag.
   `pixels_write` drains earlier Canvas work, validates the whole source and
   copies to the current drawing rows before returning. It retains no source
   pointer, swaps no page and waits for no new vblank. A successful requested
   RUN/DONE is therefore a copy-completion fence for this finite render path
   if the module propagates all errors and queues no later unfinished drawing.
   It does not mean the frame is visible. The eZ80 still issues C3. Ordinary
   `vdu_submit` and line submission do not generally finish rasterization before
   returning, so do not generalize this DONE interpretation to every module.
3. **Pixel format and bounds.** RGB222 source bytes are `00BBGGRR`, without
   alpha or sync bits; this differs from RGBA2222 bitmap pixels. The service
   handles native sync/permutation itself. For the full frame use x/y zero,
   width 320, height 240, stride 320 and at least 76,800 source bytes. The
   rectangle must fit completely; there is no implicit clipping or origin.
   Every source value is checked before destination pixels change. Keep source
   memory readable, immutable and disjoint from the display until return.
4. **Errors and atomicity.** No runtime rollback exists for module globals,
   buffers or earlier successful calls. Validating a whole dirty batch before
   applying it is Wolf's own algorithm. ERROR can leave the hidden frame or
   render mirror modified; displaying the old frame requires double buffering
   and withholding the swap. A later call must recover from any partial module
   state. Do not describe ERROR as a general transactional guarantee.
5. **Leases and arguments.** Workspace, tilemap and one texture use three
   leases only after the argument descriptor has been copied and its read lease
   released. Offsets/counts are 32-bit; each acquired buffer must be one
   contiguous block, and conflicting overlapping writes/aliases reject. Module
   buffer pointers expire on release or entry return; module-owned globals
   remain until unload. RUN still needs an existing contiguous argument buffer
   of at most 32 bytes. Write acquisition of that storage, including aliases,
   rejects; this is API admission, not memory protection against trusted code.
6. **Lifecycle and responsiveness.** One occupied slot returns BUSY; never
   evict another owner's handle. Native execution is synchronous on the VDP
   command task even while the eZ80 continues independently. Long RUNs do not
   pump the normal VDP event loop. Finite per-frame/fizzle steps are therefore
   important, and Escape latency needs an actual test rather than an assumption
   from eZ80 concurrency. Drain the outstanding request before unloading; keep
   token/operation purpose and stale-handle checks. Module unload preserves
   ordinary app-owned asset/workspace buffers, which the application later
   clears. INIT/FINI cannot call pixel services; do configuration needing them
   through a finite RUN.

## Memory, package and tooling constraints

1. A 90 KiB stock buffer and a 76,800-byte pixel span are not limited by the
   module's 16 KiB resident-data ceiling. Stock wire writes still use u16-sized
   chunks; consolidate the accumulated blocks before acquisition. Consolidation
   may temporarily need both source blocks and a destination, so count peak
   setup RAM as well as retained RAM. Buffer IDs can avoid filenames at runtime;
   ID ownership and installation still belong to the application.
2. The ceilings remain text 65,536, combined aligned rodata/data/BSS/data.rel.ro
   16,384, image 131,072 and declared stack 2,048 bytes. Builder defaults are
   smaller for text/data/image; select allowances explicitly. The 3,604-byte
   quarter-wave table counts in rodata alongside the proposed 5–8 KiB BSS.
   Executable storage and module data are separately allocated internal memory;
   a caller-owned PSRAM workspace does not increase either allowance. Check
   actual free/largest executable, internal and PSRAM blocks, not only totals.
3. The stack field is an admitted declaration, not an enforced isolated stack.
   The command task has 4 KiB total and services use part of it. Source `.su`
   files do not include linked library frames or prove whole-path safety.
   Inspect renderer/callee/service chains and record INFO17 before/after target
   workloads; a high-water sample supports only paths actually exercised.
4. Pin the public headers, `src/vdplet_policy.h`,
   `src/vdplet_image.h`, `tools/build_module.py` and `tools/module.exports` from
   the runtime commit above. Use classic ESP32 GCC 8.4.0
   `esp-2021r2-patch5`, PIC/windowed ABI and one C/C++ translation unit. Only
   `.text`, `.rodata`, `.data`, `.bss`, `.data.rel.ro` become resident. Admitted
   relocations are symbol-zero `R_XTENSA_RELATIVE` with zero explicit addend;
   unresolved imports, arbitrary ELF sections/alignments, constructors, TLS,
   exceptions and runtime dependencies are not supported. No firmware heap or
   private symbol imports exist. Freestanding aggregate operations may require
   local memcpy/memset implementations, as the examples demonstrate. `--math`
   optionally supplies pinned static libm/libgcc, whose size and stack count too.

Example later build command, not executed by this review:

```bash
cd /Agon/mystuff/agon-vdplet
.venv/bin/python tools/build_module.py /path/to/wolf3d.cpp \
    --name wolf3d --capabilities 17 --abi-minor 3 \
    --text-limit 65536 --data-limit 16384 --stack-limit 2048 --image-limit 131072
```

The actual admission implementation is `src/vdplet_image.h::vdpl::load`, then
target LOAD/INIT. There is no separate generic package-info/validator CLI.
`tests/check_media.cpp` is a fixed mask-49 example checker, and
`tests/check_image.cpp` uses the older mask-3 fixture; neither is a drop-in
validator for an arbitrary Wolf mask-17 package. Reuse the maintained loader in
a consumer-owned admission harness when implementation is authorized.

## Existing fixtures and exact evidence boundary

1. Reuse [host pixel tests](/Agon/mystuff/agon-vdplet/tests/test_pixels.cpp)
   and [VDPL-005 qualification](/Agon/mystuff/agon-vdplet/docs/tasks/VDPL-005/qualification/README.md)
   for formats, padded spans, bounds, palette/state preservation and copy
   lifetime. `qualification/target.py::upload` shows chunking plus stock
   consolidation; `worker.c` shows argument copying and lease release. The
   small maintained [media example](/Agon/mystuff/agon-vdplet/examples/media_basic/media_basic.c)
   shows the C lifecycle, but its PCM use and mask 49 are not Wolf requirements.
2. The VDPL-005 Pi JIT/interpreter and Oryx JIT receipts record 1,136,640 pixel
   comparisons per suite, including direct64 mode 8 and hidden-buffer mode 136.
   The double-buffer probe writes a **16x2** rectangle at (3,17), checks that it
   is invisible before C3 and verifies it afterward. This proves that scoped
   emulated path, not a full-frame 320x240 Wolf copy or physical scanout.
   Source: `qualification/qualify.py`; recorded firmware SHA-256
   `dd24e7d5c26697e81598bb2324a2371170b0f7075268569e73023e9dfa6c99ae`.
   Those historical receipts predate current HEAD and must not be relabelled
   as a fresh run of its firmware. Current source preserves the pixel path;
   pin actual new build bytes during any later consumer qualification.
3. [GOLEM-041 guest](/Agon/mystuff/golem/docs/tasks/GOLEM-041/qualification/guest.asm)
   and [GOLEM-042 guest](/Agon/mystuff/golem/docs/tasks/GOLEM-042/qualification/guest.asm)
   provide actual MOS packet/vector/token/error/cleanup patterns. They are
   qualification guests, not a general production client library or proof of
   Wolf's Escape behavior. The VDPL-005 `qualification/src/main.c` provides
   the AgonDev C equivalent; review its finite fixture assumptions before reuse.
4. No physical generic RGB222 mode-136/full-frame result, Wolf frame-time gain,
   90 KiB workspace allocation or complete Wolf module admission is established
   here. Count validation plus full-frame conversion/copy and prior Canvas drain
   in measured render cost. Keep Wolf's four gates: actual module admission,
   physical pixels, matched physical frame/input timing, complete lifecycle.

## Proposed central disposition

CNS-015 records this additional consumer outside the frozen seven-session
invitation set. Wolf owns its compact mirror, arenas, batch semantics, DDA/HUD/
fizzle port and independent parity/performance/lifecycle qualification. Golem
owns shared runtime/API interpretation and any demonstrated generic gap. Reuse
CNS-001 for pixels-only build/accessors and CNS-002 for resource discovery docs;
no extra renderer service or automatic limit increase is proposed. Any later
prototype or hardware work requires the Author's release.
