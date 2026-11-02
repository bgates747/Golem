# VDPL-012 frozen target results

All three approved fresh processes passed. The JIT and interpreter each passed
11 groups and 2,620,416 independently decoded pixel comparisons. The matched
no-option firmware passed three groups and 196,608 comparisons: **25 groups and
5,437,440 comparisons in total**. The frozen qualifier, native modules and runtime
expectations were unchanged throughout execution.

| Case | Firmware capabilities | Groups | Pixels | Actual duration |
| --- | ---: | ---: | ---: | ---: |
| rows-r1 / JIT | 287 | 11 | 2,620,416 | 18.423 s |
| rows-r1 / interpreter | 287 | 11 | 2,620,416 | 20.565 s |
| base-r1 / JIT | 31 | 3 | 196,608 | 4.936 s |

These are direct-library tests of actual ESP32 instructions and decoded VGA,
including boot, transport and frame capture. They are not MOS application tests
or isolated rendering benchmarks. Central generation 20 was released after
normal shutdown and a fresh whole-host inspection found no emulator processes.

## Coverage

- All four X biases, narrow/odd/full widths and the final row/column preserve
  sync bits and surrounding pixels. Direct group-base/XOR2 writes match the
  existing copied-pixel control. Drawn pixels remain hidden until ordinary C3.
- A queued ordinary C3 followed by RUN obtains the newly hidden page. The
  separate host/source test owns injected pending-Canvas-swap ordering; these
  target results do not claim that injection.
- New and old frame ROW/END operations reject each other's tokens. VDU, lines
  and copied drawing refuse BUSY during a row lease. Four ordinary buffer
  leases coexist with the frame transaction.
- Invalid bounds/extents, nonzero input lease, unsupported format, visible
  single page, wrong phase and stale tokens refuse without returning pointers.
  Fresh transactions receive fresh generations.
- Forgotten END is closed after RUN. A failed RUN leaves its partial result
  hidden; existing drawing can replace it. FINI phase rejection, unload and
  reload preserve cleanup behavior.
- ABI minor 0 through 6 retain their exact 36/40/44/72/76/80/84-byte views and
  capability masks. Undeclared row access refuses despite the available
  callback; a required-row image is refused by no-option firmware before INIT.
  Unchanged original numeric and packed-mono packages produce their established
  results on both candidates.

## Raw audit and resources

The two positive engines received a byte-identical 1,724,816-byte fixture. Each
returned 8,825 sequentially tokened VDPL replies, occupying 88,250 bytes. The
negative control returned 953 replies / 9,530 bytes. All emitted, delivered and
received UART byte streams match within each run; UART counters agree with
captured lengths. All three report zero failed, unhandled, unregistered or
unmapped execution faults.

| Observation | rows JIT | rows interpreter | base JIT |
| --- | ---: | ---: | ---: |
| Command stack free, boot → final | 2,760 → 2,100 B | 2,760 → 1,972 B | 2,760 → 2,152 B |
| Internal free, boot → final | 60,796 → 181,508 B | 60,796 → 181,508 B | 60,828 → 181,680 B |
| Internal largest, final | 110,580 B | 110,580 B | 110,580 B |
| PSRAM free, boot → final | 4,167,647 → 4,164,615 B | 4,167,647 → 4,164,615 B | 4,167,647 → 4,164,615 B |
| PSRAM largest, final | 4,128,756 B | 4,128,756 B | 4,063,220 B |
| Audio stack free, boot → final | 1,536 → 1,424 B | 1,536 → 1,424 B | 1,536 → 1,536 B |
| Tasks, boot → final | 12 → 12 | 12 → 12 | 12 → 12 |

The worker's own static call chain is 848 bytes before runtime/Canvas callbacks,
with a declared 2,048-byte module allowance. INFO17 above records actual command
task headroom, rather than a whole-firmware static stack guarantee. Boot and final
screen/buffer states differ; their heap values do not measure leakage or the
resident cost of the feature.

## Frozen candidate paths

Oryx SDK root: `/home/smith/Agon/mystuff/agon-vdplet`.

| Artifact under that root | SHA-256 |
| --- | --- |
| `.emulator/vdpl-012-qualification/esp32-firmware.bin` | `8d980025f891484cf7e6cc5f2992cf517cb80e0123ff7de712ae8d7f2dc1c807` |
| `.emulator/vdpl-012-qualification/esp32-firmware.elf` | `1a7c44482d4ba8c5bfb49976c0bd664b9a9c2a635c7027b30954929ca567f814` |
| `.emulator/vdpl-012-qualification/flexe-vdp.so` | `f7e5bf316258793c32313e2f367e28d09519dda82a72c40e4c3e204b61dab615` |
| `build/vdpl-012/base-r1/firmware.bin` | `48c926160cc0df0a41158e031714272d387aecab6fab087f8ce2f22bbae30ea1` |
| `build/vdpl-012/base-r1/firmware.elf` | `9b74fe7e8be4c7d15a3ca7bd85e43bb6701b530d5eaa6617045e51b899c043dc` |
| `build/vdpl012qualification/modules/rows.vdpl` | `828fa85a934b62f4559d45476bb4ca383cc40581f1db3d0ebc5812c3317f4caf` |

The row firmware/ELF are also copied under `build/vdpl-012/rows-r1/`. Immutable
compiler headers/builder are under `build/vdpl-012/sdk`. Additional native
packages are `build/vdpl012qualification/modules/undeclared.vdpl` and
`view-0.vdpl` through `view-6.vdpl`.

The machine-readable [results](results.json) include full source/module pins,
per-run resource observations and hashes for every raw artifact. Full native
worker output is preserved in the three adjacent `evidence.json` files. Raw UART
logs, audit streams, final captures and backend counters have been retrieved to
`/Agon/mystuff/agon-vdplet/build/vdpl012qualification/oryx/`; the originals remain
in the three corresponding Oryx build directories. `collect_results.py` audits
these saved files without starting an emulator.

One pre-execution manifest verification used the SDK root instead of the profile
as its working directory. It found only missing relative paths, was recorded in
`preflight-cwd-error.json`, and passed after correcting the working directory.
No target retry, firmware/source change or expectation change occurred.
