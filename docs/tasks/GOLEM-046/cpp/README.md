# C++ streamed-package review

The reusable C++ adapter now loads the same large Golem program through generic
AGNB. No whole package is embedded in eZ80 RAM. Golem still computes the result
and draws the pyramid; C++ supplies file loading, calls and native lifecycle.
The previously accepted assembly fixture/profile remains intact.

1. Maintained generic reader: agon-utils `examples/agnb/api/agnb_buffers.hpp`,
   `agnb_stdio.hpp`, `agnb_agondev.hpp`; formal task AGNB-002. Its workspace is
   exactly8,192bytes, caller-owned, with checked framing/metadata and no implicit
   execution, setup, asset creation or numeric ID allocation.
2. Maintained Golem hooks: `runtime/agondev/agnb_host.cpp` and its public header.
   `src/main.cpp` uses the generated large-program header; `src/visual.cpp` puts
   the second generated drawing header in its own translation unit. The Makefile
   compiles the reusable shim once, without copying it or embedding installations.
3. `build_app.py` compiles the unchanged parent `large.golem` and `review.golem`,
   optionally builds its SDK module, generates headers/error fixtures, and builds
   the ordinary AgonDev application. `stock-build-evidence.json` and
   `native-build-evidence.json` record byte hashes, static RAM and source pins.
   Generated files/headers/objects stay ignored in this silo.
4. `qualify.py` reuses the parent task's qualified deployment/headless machinery
   with **separate** root `.emulator/golem-046-cpp`. It tests two load/call/call/
   cleanup cycles, then missing/malformed/truncated files, completed-record
   retention on late failure, and a successful load after errors. The guest
   checks its observed values before writing a successful result.
5. Stock Fab runs on Pi/Oryx; native actual-Xtensa Flexe runs through both engines
   on Oryx. Native checks also verify real MOS return, image samples and execution
   fault counters. No SDK/firmware or compiler semantics were changed. The
   qualification reads results; the production reader never reads VDP data back.
6. Application failure diagnostics are isolated to `cpp.log`/`failure.dat`; the
   review's only geometry is `review.golem`. The loader reports local parser/file
   and submission success, not a stock VDP allocation acknowledgement. Host owns
   completion and cleanup, and an uncertain unresponsive native execution still
   requires recovery/reset rather than freeing in-flight storage.

For an agent build/run, use the project Python and `--runtime` canonical Fab
runtime; add `--native` to build/stage native, `--no-build` to consume prebuilt
Oryx fixtures, or `--stage-only` to leave it stopped. Never prepare concurrently
with a running profile. The latest native rebuild is compared byte-for-byte with
the qualified JIT/interpreter image before final staging.

The stopped Oryx human review uses **Flexe**:

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-046-cpp
./flexe-agon-emulator
```

Expect the same white pyramid, labelled **C++ AGNB loader**, and the successful
C++ test message. Follow the single onscreen MOS keyboard instruction, close and
report. Each launch clears old results/logs. The agent retrieves and verifies
`results.dat` and `cpp.log`; no extra terminal or user-run verifier is needed.
The Author accepted the C++ review on2026-10-23 ("works."). All95 saved
result bytes and the completed application log verify; runtime pins match,
no failure file exists and the profile is stopped. See `human-review.json`.
The Author explicitly approved commit/push on2026-10-23.
