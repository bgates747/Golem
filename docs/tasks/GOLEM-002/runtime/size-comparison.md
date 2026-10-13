# Vdplet firmware size comparison

The vdplet facility adds **8,512 bytes of flash and 320 bytes of static RAM**.
The firmware binary also grows by exactly 8,512 bytes. This is a matched
build comparison; loaded module code/data and staging buffers are additional
runtime allocations, not included in these static figures.

Measure the net flash and static RAM addition against the same pinned firmware,
toolchain, graphics library and build configuration with only vdplet integration
removed. Build only an ignored task-local copy; preserve qualified firmware and
staged profiles. Native module payloads and runtime heap are separate costs.

- SIZ-001 [x] Copy the qualified build, restore the two upstream dispatch headers,
  remove loader headers, and record the exact source differences and identities.
- SIZ-002 [x] Build the baseline with the same PlatformIO environment and record
  flash sections, static RAM, binary sizes, elapsed time and artifact hashes.
- SIZ-003 [x] Publish the net differences and distinguish static footprint from
  executable/module heap and upload storage.

| Measure | Matching stock | With vdplets | Added |
| --- | ---: | ---: | ---: |
| PlatformIO flash sections | 1,077,305 | 1,085,817 | 8,512 |
| firmware.bin | 1,077,680 | 1,086,192 | 8,512 |
| Static RAM | 44,712 | 45,032 | 320 |

All sizes are bytes. Flash includes the initialized RAM data image. Section
deltas are .flash.text +7,628, .flash.rodata +528, .dram0.data +320 and
.iram0.text +36; BSS and RTC sections are unchanged. Thus 36 of the additional
instruction bytes also occupy firmware IRAM at runtime.

[size-comparison.json](size-comparison.json) records artifact/compiler hashes,
all allocated sections and the five exact source differences. The comparison
uses VDP pin 1056ee38ec1cc68ed327bd32ee47d04b1c30597c, pinned graphics/dependencies
and the existing Xtensa GCC 8.4.0 toolchain. Only vdu_sys.h and
vdu_stream_processor.h were restored from the upstream pin, and the three
vdplet headers removed from an ignored copy. All other application source
and build configuration match. Baseline build took 67.707 seconds including
PlatformIO startup. Qualified custom firmware hashes remain unchanged; no
staged profile, emulator runtime or physical target was changed.

Generated baseline source, build log and setup receipt remain under
build/size-comparison/. The baseline command is:

```bash
.venv/bin/python -m platformio run \
  -d docs/tasks/GOLEM-002/runtime/build/size-comparison/stock/firmware-src -j 2
```
