# ARM64 mechanism gate results

All four fresh sequential Pi processes passed under explicit permit
`G057-ARM-001`. The original 6502 toy matches all 64 saved state bytes in both
engines, with 8 steps and 29 cycles, successful INIT/FINI/unload, an empty slot
and same-mode executable heap recovery. The unchanged colour-row suite passes
**22 groups and 5,240,832 independently decoded pixel comparisons** across the
two engines. Every process shut down/joined normally; final host inspection was
empty. There were no target retries or source/expectation changes.

| Case | Duration | Result | Final command stack free |
| --- | ---: | --- | ---: |
| Original toy / JIT | 4.996 s | Complete state and lifecycle pass | 2,136 B |
| Original toy / interpreter | 8.734 s | Complete state and lifecycle pass | 2,136 B |
| Frozen rows-r1 / JIT | 29.616 s | 11 groups / 2,620,416 pixels | 2,152 B |
| Frozen rows-r1 / interpreter | 44.910 s | 11 groups / 2,620,416 pixels | 1,844 B |

Total process time was 88.256 seconds. This includes boot, transport and capture;
it is not a renderer or physical speed benchmark. These direct-library CPU/row
mechanism checks do not qualify a real application, MOS, human review or hardware.
The earlier [x86 generic matrix](../colour-row-qualification/RESULTS.md), including no-option refusal,
remains independent and unchanged. No additional no-option sweep was needed for
this bounded host-architecture gate.

## Architecture and pinned inputs

This is an actual native AArch64 backend: ELF machine 183, 16,384-byte host pages,
GCC 14.2.0 targeting `aarch64-linux-gnu`, native tuning. Owner r02 source is
equivalent to maintained x86 backend `0866265b…`; it is not the same artifact.
The earlier generic x86 row report used `f7e5bf31…` and is not relabelled here.
Owner r01's compile failure remains preserved. Approved r02 records a visible
`rom_stubs.c` string-truncation warning severity exception for that translation
unit only; all other `-Werror` settings remain. Repeated links match, diagnostic
symbols are absent, and the four required owner host checks passed.

| Input | SHA-256 |
| --- | --- |
| ARM backend | `af0ccc012bed9184acadadf684fd00cbd78aae4cc0460660a454c609d0291caf` |
| ARM core archive | `7618f338cc0106a36657b64f62421ef29d19743a596acd222292a0a39a9097e2` |
| Owner build receipt | `4a55dfd2909b57aba4c4f6a3da98bb220ece570aeca0415084eb8a8c80724fa3` |
| Original toy package | `7113d107a7c91fc56c6e65169c1d3a17fc7cc7771a125373505c421cf46f68ef` |
| Complete toy state oracle | `ab5a0785dc71ee37fbc2e7af8fbe75fb55fe207a09801f518945f93a52a9e9e2` |
| Toy firmware / ELF | `b42ae55f69900c44acacfbf0d665ef278e87b277b3b9fd4208e17ebf378f74b6` / `a10f7989d5c1f9974fc999428865ca74771fb65a0d4d694a2ac2b81c19df9902` |
| rows-r1 firmware / ELF | `8d980025f891484cf7e6cc5f2992cf517cb80e0123ff7de712ae8d7f2dc1c807` / `1a7c44482d4ba8c5bfb49976c0bd664b9a9c2a635c7027b30954929ca567f814` |

The toy package is 35,876 bytes, with 35,072 bytes of executable text and 4 bytes
of resident data. It was neither rebuilt nor patched. The thin harness uses the
unchanged frozen SDK transport and the original four-buffer/32-byte-argument
fixture. The row qualifier, worker, modules, SDK and picture expectations are
also unchanged. The new stopped Pi SDK profile is
`.emulator/vdpl012-arm64-qualification`; previous SDK and Oryx profiles were not
changed. No Oryx session or lease was used.

## Raw audit and resources

Each engine pair received a byte-identical fixture. Toy runs each sent 103,050
bytes and observed 72 sequential VDPL replies / 720 bytes. Row runs each sent
1,724,816 bytes and observed 8,825 replies / 88,250 bytes. Emitted, delivered and
received UART streams match in every run; backend counters match captured
lengths. Every failed/unhandled/unregistered/unmapped execution counter is zero.

| Final observation | Toy, both engines | Rows JIT | Rows interpreter |
| --- | ---: | ---: | ---: |
| Executable free, before → after | 161,476 → 161,476 B | Not a gate criterion | Not a gate criterion |
| Internal free, before → after | 205,252 → 205,252 B | 60,796 → 181,508 B | 60,796 → 181,508 B |
| PSRAM free, before → after | 4,167,647 → 4,167,603 B | 4,167,647 → 4,164,615 B | 4,167,647 → 4,164,615 B |
| Internal largest | 110,580 B | 110,580 B | 110,580 B |
| PSRAM largest | 4,128,756 B | 4,128,756 B | 4,128,756 B |
| Audio stack free | 1,536 B | 1,392 B | 1,456 B |
| Tasks, before → after | 12 → 12 | 12 → 12 | 12 → 12 |

The toy observes 44 fewer PSRAM bytes after buffer cleanup; no complete PSRAM
recovery or leak conclusion is claimed. Rows use different boot/final screen
states, so those heap deltas do not measure feature cost or leakage. Command
stack values are actual minimum headroom observations, not a static whole-call
bound. The row suite exercises ordinary C3/RUN rebinding; injected pending-Canvas
swap ordering remains separate host/source evidence.

The [machine-readable audit](results.json) preserves source/module/
runtime pins, resources, counters and every raw file hash. Raw SDK build outputs
are retained under `build/vdpl012arm64qualification/{toy-jit-r01,toy-interp-r01,
rows-r1-jit,rows-r1-interp}/`, including native diagnostic results, full toy state,
transport streams, UART logs, final decoded captures and backend statistics.
Consumer application, whole-call stack, performance and human review remain
separate work for their owners.
