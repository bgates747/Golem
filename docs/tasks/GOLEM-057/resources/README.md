# Optional pixel-service resource comparison

The independent pixel service adds the exact image and static-memory costs below. Both lean combinations compile successfully from the same frozen source inputs. This is a build/resource result; it does not establish runtime, audio, emulator or hardware behavior.

| Build | Firmware image | Static DRAM | IRAM | App-slot headroom |
| --- | ---: | ---: | ---: | ---: |
| base | 1,090,096 | 45,048 | 94,839 | 220,624 |
| lean | 1,092,304 | 45,136 | 94,843 | 218,416 |
| pixels | 1,092,448 | 45,048 | 94,839 | 218,272 |
| pixels-lean | 1,094,816 | 45,136 | 94,839 | 215,904 |

All values are bytes. Each app slot is 1,310,720 bytes. Exact firmware.bin length includes image metadata, padding and hash. Static DRAM includes initialized data, BSS and noinit; IRAM includes vectors and the end marker. ELF file/debug-section sizes are excluded.

| Matched comparison | Image delta | Static DRAM delta | IRAM delta |
| --- | ---: | ---: | ---: |
| base-to-pixels | +2,352 | +0 | +0 |
| lean-to-pixels-lean | +2,512 | +0 | -4 |

All comparisons match upstream and graphics pins, compiler version/binary, PlatformIO version, board/build configuration, installed package metadata and partition table. All exported headers and firmware artifacts match receipts; all four source receipts are unchanged from the export snapshot. The common headers and builder match across variants. No source drift was detected.

The existing pixels build was preserved unchanged. Fresh base, lean and pixels-lean outputs were built sequentially with two compiler jobs, using the project virtual environment and the owned pinned upstream.git cache. Only pinned library source caches were copied into fresh outputs; no compiled objects were reused. No production source, upstream reference checkout, emulator profile or device was modified.

Application images contain no absolute output-directory path strings. Linked section sizes were independently cross-checked against the matching Xtensa size tool. Dynamic heap, active pixel copies, stack high-water marks, execution performance and physical acceptance are separate qualifications.

[summary.json](summary.json) records artifact hashes, source/header/builder receipts, durations, exact comparisons and the size-tool check. [export-snapshot.json](export-snapshot.json) freezes all export receipts. The four individual scan receipts retain all allocated sections and toolchain identities.

Builder logs are under `/Agon/mystuff/agon-vdplet/build/vdpl-010-{base,lean,pixels-lean}.log`, with PlatformIO logs in each variant output. Build durations below are PlatformIO wall times under shared Pi load, not runtime measurements.

- `base`: 81.95 seconds; SHA-256 `ea08073e64bb74d686c5a2bc8ffe8f9da34925f42e24a7b8fa384bd587f5ce94`.
- `lean`: 69.81 seconds; SHA-256 `c65d56ba02cf66dfec9a0eb0d8cdb3f0db3aa8c8c2831f877d7388ecd2fcd5be`.
- `pixels`: 293.82 seconds; SHA-256 `b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6`.
- `pixels-lean`: 71.81 seconds; SHA-256 `30754052f90fe35d6e480f3f09f8611b7734cf087e8e001b35a9e4b70e8e4a46`.
