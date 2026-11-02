# Prepared failed-display release candidate

The narrow fallback candidate builds successfully and adds **176 flashed bytes
and no static RAM** against its matched control. Host tests confirm that a
returned height error releases the failed display before constructing fallback,
including same-depth and repeated failures. It has not run in an emulator or on
hardware; usable restored video, module retention and cleanup remain target gates.

## Behavior and boundary

The actual journal retained by the root task captured the fallback's 2,048-byte
lookup allocation returning null, its null publication, and 32 same-run ISR
origins using that controller. That supports this separate experiment; it does
not establish physical ESP32 memory exhaustion.

Only `candidate/video/vdu.h` changes from frozen original010:

```cpp
static void releaseFailedResolution() {
    canvas.reset();
    if (_VGAController) {
        _VGAController->end();
        _VGAController.reset();
    }
    _VGAColourDepth = uint8_t(-1);
}
```

The existing MODE handler calls it only after `changeMode` returns error 2,
before the existing previous-mode fallback, and again if that fallback returns
2 before the existing final fallback. Canvas dies before its controller; the
controller stops before deletion; cached depth becomes 255 before the next
factory. `videoMode` is preserved for the existing previous-mode retry. No
success/error1/invalid/teletext branch, factory, allocator, ISR, journal,
service, capability, module limit or default SDK image changes.

The plan was frozen before preparation as
`../mode-fallback-release-plan-v1.md`, SHA-256
`ff84eb791917a8a4432187b0035c306c924296d521d2a5e5f29abd04eaebda53`.
The exact patch is `source.patch`, SHA-256
`e73c930084eda9a4062a7a4ff9314412260d103c58a6f4a2682fd12f69dadbb0`.

## Artifacts and actual cost

Both new roots are under
`/Agon/mystuff/golem/docs/tasks/GOLEM-057/build/display-mode-repair/`:
`control00/` and `candidate/` have equal-length names. Artifacts reside at
`.pio/build/esp32dev/` inside each. The build tree is ignored. Original010,
earlier control/journal builds, the maintained SDK and upstream were preserved.

| Variant | Image bytes | Firmware SHA-256 |
| --- | ---: | --- |
| Original010 | 1,092,448 | `b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6` |
| Matched control00 | 1,092,928 | `b3dd3772aceb6bf54d7e959dc1c5790cdaebca352b6a7fcab6e766bc1dfa1f8d` |
| Candidate | 1,093,104 | `f30b299a95d58517be510b7adc4312252265eefd338b03712b91d4bef4394844` |

Candidate ELF SHA-256:
`3ea64b06cd99c7cdf51080551451c3cd3fe80d07e8a9ad0f95a22e5eef5b6a88`.
Candidate map SHA-256:
`0f5dc75c446844a401fd257ea5d720d7408fcafccc891e4f0fd2dd51c706e219`.
All artifact/source/tool hashes and exact paths are in `build-results.json`,
`preparation.json` and `source-manifests.json.gz`.

| Resource | Candidate minus matched control |
| --- | ---: |
| Flashed image file | +176 bytes |
| Static DRAM data/BSS | 0 bytes |
| IRAM text | +12 bytes |
| Flash read-only data | +28 bytes |
| Flash text | +140 bytes |

The private helper is 78 bytes with a 32-byte compiler entry frame. The existing
MODE handler's 32-byte frame and `updateVGAController`'s 48-byte frame are
unchanged. Nested controller `end`/destructor/RTOS stack high-water remains
unmeasured. No new heap allocation, task, object member or steady-state buffer
is introduced; existing cleanup calls are reordered on failure only.

The matched control is not relabeled b42/a10: its embedded tool-package paths
are longer than the original's (read-only data +480 bytes; flash text -8 bytes),
and its image differs. Cost comparisons use the paired equal-length roots and
matching expanded flags, not that original binary. Bootloader/partition files
match the original; no diagnostic journal symbols exist in either new ELF.

## Preparation and checks

`prepare.py` copied the original owned export and cached pinned dependencies;
it did not invoke the evolving SDK exporter or fetch a dependency. The original
configuration and feature header remain. PlatformIO uses task-local state and
read-only links to already installed packages. Both builds ran sequentially
with two jobs; PlatformIO reported 81.79 seconds for control and 95.59 for
candidate. These are build durations, not runtime performance.

`verify_sources.py` checks all 4,755 source files per copy and the original
artifacts. Removing the helper/two calls exactly restores original source.
The unchanged-source and ownership results are retained in
`source-verification.json` and `host-results.json`.

The host harness compiles the actual extracted MODE handler, private helper and
unchanged `updateVGAController` against ownership fakes. Nine scenarios cover
success, invalid/teletext -1, factory1, non-height second error, first/second/
repeated height2, and same-depth height2. Original/control traces match; five
unaffected cases also match candidate traces. The candidate verifies empty
ownership and depth255 at the fallback factory, Canvas→end→delete order,
unchanged previous numeric mode, and safe repeated disposal with empty owners.
Original/control pass 396 assertions each; candidate and ASan/UBSan candidate
pass 424 each. Leak detection is disabled for the tool runner's ptrace limit.
These fakes do not prove real partial-allocation teardown or display recovery.

Repeat source/host checks without executing firmware:

```bash
cd /Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-057/mode-repair/verify_sources.py
.venv/bin/python docs/tasks/GOLEM-057/mode-repair/check_host.py
```

Build commands/expanded flags are recorded in `build-results.json`. Raw
environment dumps stay in the ignored build tree, outside the sealed handoff.

## Remaining gate

Root owns consumer coordination and a separate execution permit. The target
fixture must retain the same original module/client/ROM/history and load order,
observe mode20 refusal with restored11 and three fresh decoded frames, preserve
module state, and complete normal FINI/unload. Both engines and an unchanged
successful mode transition are required, with heap/cleanup status reported
independently. Preserve the first unexpected failure and normal stop/join.

This helper cannot rescue a constructor or `end()` that fails to return. It
does not preserve old pixels, create transactional mode admission, guarantee
arbitrary final-fallback success or prove exact heap recovery. SDK promotion,
human review, flashing and publication remain separate. No target, Oryx,
emulator, mailbox, deployment or physical operation was performed here.
