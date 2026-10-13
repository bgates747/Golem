# Flexe stock Agon baseline

This baseline runs the unchanged ESP32 VDP 2.16.0 image in Flexe before the new
Fab adapter or vdplet firmware is introduced. The pristine interpreter passes;
the pristine Linux AArch64 JIT fails at startup. The adapter-owned host
instruction-cache correction makes the stock VDP gate pass in both engines.
Fourteen independent executable-RAM cases also pass with its ISYNC observer.
These are automated headless checks, separate from the Author's Oryx review.

The final standalone MOS/BASIC gate also passes in both engines: eleven shell
and BASIC commands, a filesystem read, arithmetic and loop output, VSync and
the expected screen digest `a53304ae`. Its earlier failures are retained. The
adapter agent's UART idle-timeout correction resolved the last JIT key stall.

- PRE-001 [x] Read shared environment/emulator instructions and inspect existing
  binaries/images. The Pi has Cargo/CMake/GCC; no Flexe binary exists. Reuse the
  cached official VDP image and existing verified Golem MOS/BASIC images.
- PRE-002 [x] Export Flexe commit `fbda8e905ccff36a090e4e33b0ffc2337dd79c52`
  read-only into this silo's ignored `build/flexe-source` directory. Build the
  targeted Agon runner and position-independent static core on Pi with two
  compiler jobs, Release mode and LTO disabled. Share the core path with the
  Fab adapter agent.
- VDP-001 [x] Run the upstream stock VDP gate in interpreter and JIT using
  its existing assertions. Extract the embedded Python gate without changing
  its logic, and invoke it with Golem's explicit `.venv/bin/python`. Preserve
  the pristine JIT failure and corrected-core success separately.
- MOS-001 [x] Run the upstream pinned Fab eZ80/MOS/BBC BASIC peer gate if a
  compatible peer build is available without conflicting work; otherwise
  record its disposition and let the new adapter's full-machine qualification
  provide fresh MOS integration evidence. The pinned peer was built locally;
  the final corrected-core gate passes in both interpreter and JIT.
- MOS-002 [x] If the upstream key scheduler races MOS output, build an owned
  runner copy whose test bridge waits for the actual next MOS/BASIC prompt
  before sending another command and for each printable key's echo before
  sending another key. Preserve the guest images and every output,
  filesystem, arithmetic, loop, VSync and pixel assertion. Retain the original
  failure before recording a corrected-harness result. The owned scheduler
  makes the interpreter pass every unchanged assertion; the first JIT run
  still waits indefinitely for a key echo and fails the upstream inactivity
  limit. The subsequent UART timeout correction resolves that separate model
  issue with the same harness, input images and assertions.
- EVD-001 [x] Retain machine/source/build/image identities, exact commands,
  fresh gate output, wall durations and coverage limits in a checked-in
  qualification receipt. `qualification-receipt.json` ties the receipts and
  source scripts together; no emulator pass is equated with hardware proof.
- RAM-001 [x] Inspect how Flexe invalidates JIT code for writes to executable
  RAM and send the findings to the vdplet/adapter agents before their proof.
- RAM-002 [x] At the adapter agent's request, add a synthetic IRAM replacement
  regression before its correctness adaptation. Demonstrate stale translation
  after byte/halfword/word stores, a translated guest S32I and direct backing
  memcpy; demonstrate that explicit invalidation then executes replacement
  bytes. All six mutation paths pass. Keep this independent from firmware
  lifecycle proofs.
- RAM-003 [x] Qualify the adapter-owned ISYNC observer with predecoded
  interpreter execution as well as translated stores. Confirm first-load
  invalidation reaches unused IRAM and a predecoded ISYNC cannot bypass the
  callback through the cached interpreter fast path. All eight ISYNC cases
  pass; this review caught both cached-interpreter bypasses before the sibling
  agent corrected them.

Inputs already hash-verified locally:

| Input | SHA-256 |
| --- | --- |
| VDP 2.16.0 `firmware.bin` | `b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f` |
| MOS 3.0.2 `mos_platform.bin` | `d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae` |
| BBC BASIC `bbcbasic24.bin` | `bedbb3e977a0bbea0874a58a450b6155f3a6ea65677a19a1b6f7d415ed31ca26` |

All work is bounded by GOLEM-021's eight-hour goal. No graphical emulator,
physical board or upstream checkout is modified by this baseline.

The first stock run passed in the interpreter and failed in the JIT with
SIGILL at APPCPU startup. `stock-receipt.json` preserves that failure;
`build-receipt.json` identifies the unchanged source and build. The adapter
agent independently reproduced it and added missing instruction-cache
maintenance on Linux AArch64. `stock-host-cache-fixed-receipt.json` records the
subsequent stock VDP passes and an unrelated failure of the original MOS key
scheduler. `stock-mos-echo-receipt.json` retains the later prompt/echo-aware
interpreter pass and JIT key stall. `stock-mos-uart-fixed-receipt.json` records
the final both-engine pass, and `prompt-runner-build-receipt.json` identifies
the exact copied harness, library and four changed core files. The input
firmware is unchanged throughout.

| Final core revision 4 gate | Interpreter | JIT |
| --- | --- | --- |
| Native VDP stage 5 | Pass: video, audio, serial-console key, UART digest; no unhandled/unmapped accesses | Pass: identical digests and output checks |
| MOS/BBC BASIC | 11 commands; filesystem, arithmetic, loop and VGA `a53304ae` | Same 11 commands and expected VGA digest |
| MOS scenario duration | 11.060 seconds | 11.074 seconds |
| Synthetic RAM coherence | Fourteen independent cases pass against this exact library | Includes translated stores and ISYNC fallback |

The native stage 5 records are retained in the final aggregate receipt as
well as the ignored runner logs. Their screen digests are `56769766` at
640×480 and `a2dbd34c` at 320×240; the UART digest is `1dc20a14`.

The UART correction schedules RX idle timeouts on the existing emulated
peripheral clock and rearms after W1C while bytes remain unread. Empty/reset
cancels the timer. It applies to original ESP32 peripherals only. Independent
source review found no blocker for these proven paths; clock gating with
unread bytes and changing CLKDIV/CONF0 during a pending timeout remain outside
this bounded correction's qualification.

`iram-pristine-receipt.json` preserves intermittent wrong native results after
an explicit flush of the pristine core. With the host cache correction, all
six store paths consistently demonstrate stale guest translations until
explicit invalidation. `iram-qualified-receipt.json` retains these six cases
plus eight ISYNC observer cases, including first load into predecoded unused
RAM and a translated store falling through into ISYNC. See
`executable-ram-notes.md` for the findings and scope.

GCC 14 initially rejected upstream's bounded NVS-key `strncpy` under
`-Werror=stringop-truncation`. The successful clean-source retry demotes only
that warning with `-Wno-error=stringop-truncation`; it does not alter execution.

The pinned Rust peer built with `cargo build --release --locked -j1` in 3m15s;
its Fab eZ80 core is `02a23e67049cd68d5da77f0776cb375fa91a5e4a` and its ez80
crate is `13c151e89f520b3bae3343bea9d67d06e8037cfa`. `build-receipt.json`
retains the Cargo.lock and peer executable hashes. The clean PIC static core
is at `build/cmake/libxtensa-emu-lib.a`; the corrected adapter-owned core is
at `../emulator/build/core/libxtensa-emu-lib.a`. Both are AArch64 builds.

Rebuild the owned prompt/echo harness and run the stock MOS gate from the
project root with the explicit project Python. The upstream MOS gate also
executes the native VDP assertions before each MOS session.

```bash
.venv/bin/python docs/tasks/GOLEM-021/baseline/prepare_runner.py
.venv/bin/python docs/tasks/GOLEM-021/baseline/run_stock.py \
  --gate mos \
  --source docs/tasks/GOLEM-021/emulator/build/flexe-source \
  --runner docs/tasks/GOLEM-021/baseline/build/prompt-runner/flexe-agon-vdp-test \
  --vdp /Agon/mystuff/agon-bench/vdp-recovery/2026-08-27_keyboard-clock/release-v2.16.0/firmware.bin \
  --mos .emulator/runtime/fab-1.2.5/firmware/mos_platform.bin \
  --basic .emulator/runtime/fab-1.2.5/sdcard/bin/bbcbasic24.bin \
  --peer docs/tasks/GOLEM-021/baseline/build/agon-mos-peer/release/flexe-agon-mos-peer \
  --artifacts docs/tasks/GOLEM-021/baseline/build/uart-fixed-artifacts \
  --receipt docs/tasks/GOLEM-021/baseline/stock-mos-uart-fixed-receipt.json
.venv/bin/python docs/tasks/GOLEM-021/baseline/run_iram.py
```

Guest output/pixel/audio artifacts and clean source exports remain under the
ignored `build/` tree. JSON receipts retain fresh output, exact file identities,
commands and durations. Timings are reproduction evidence, not a comparative
performance claim. Serial-console input is covered; physical PS/2, hardware
execution and human visual review are not.
