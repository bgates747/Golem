# DRAFT — Smallest physical vdplet proof

Prove one separately built numeric module really executes on Agon's ESP32 after
upload, returns, retains state and unloads. Use no framebuffer API, Golem backend
or game integration. The proof should settle code placement, literals, data,
relocations, ABI and practical memory costs before porting the packed renderer.
It is proposed work after Gate A, not a test already run.

## Proof program and independent result

The module has an explicit C-linkage entry, one initialized scale/bias record and
a zero-initialized persistent call counter. RUN reads one float32 from a resident
input buffer and writes `y = 1.5f * x + 0.25f` and the incremented counter to a
separate resident output buffer. INIT verifies the data record and zero BSS; FINI
returns normally. Use strict floating behavior without fast-math/FMA surprises.
Keep scale/bias addressable in module data and inspect the final disassembly so
they have not simply disappeared into a constant-folded answer.

| Invocation after fresh load | Input x / bits | Expected y / bits | Counter |
| --- | --- | --- | --- |
| First | 2.0 / `0x40000000` | 3.25 / `0x40500000` | 1 |
| Changed input | -4.0 / `0xC0800000` | -5.75 / `0xC0B80000` | 2 |
| Repeated input | -4.0 / `0xC0800000` | -5.75 / `0xC0B80000` | 3 |
| After unload/reload | 0.0 / `0x00000000` | 0.25 / `0x3E800000` | 1 |

These binary-exact cases are independently calculable; the reference checker
must not reuse the module's arithmetic implementation. No timing benchmark or
large randomized suite is required. Optionally verify one defined invalid span
returns a status and leaves the output intact; never execute deliberately broken
machine code to test fault recovery.

## Distinguishing real loading from built-in firmware

- PRF-001: Firmware contains only loader/dispatch and generic buffer services.
  Build it before the module. Retain its map/hash and verify it does not contain
  the numeric kernel or its unique build identity.
- PRF-002: Compile the module as a separate PC artifact using the exact GCC 8.4
  toolchain. Retain unstripped ELF/disassembly for review and a bounded packaged
  image for upload. Inspect every allocated section, undefined symbol, entry,
  literal/data reference and relocation; reject outside the agreed profile.
- PRF-003: Include an address-bearing module data reference to exercise a real
  RELATIVE relocation. Verify its presence in ELF and that the relocated pointer
  lands inside the actual loaded data region. If GCC emits another relocation,
  report it before expanding the adapter.
- PRF-004: Boot the loader firmware with no module installed. Assembly reads the
  separate payload from SD and uploads it **after boot**. Serial diagnostics
  record entry/text/data addresses and build identity; execution entry must lie
  within the new executable allocation.
- PRF-005: Run the table above, then upload a separately built variant with
  scale 2.0, bias 0.5 and a different build identity without rebuilding/reflashing
  firmware. For x=2.0 it must produce 4.5 (`0x40900000`). This makes a built-in
  fixed-answer demonstration inadequate.
- PRF-006: Exercise a second actual placement: a debug-only allocation layout
  hook holds an executable guard allocation spanning the first placement during
  reload, or otherwise obtains a distinct address. Record both actual entry/data
  addresses; do not claim relocation coverage from reload at the same address.
  Never use an unchecked hard-coded address. This hook stays out of normal ABI.

## Memory, execution and parser checks

- CHK-001: Before any flash, compare firmware flash/static DRAM/IRAM with the
  pinned original, and review the module's text/literals/data/BSS/relocation/
  staging sizes against [the planning envelope](native-vdp-modules-feasibility.md#executable-memory-and-stack).
  Report actual sizes; revise an estimate openly if needed.
- CHK-002: On hardware measure executable/internal-data/PSRAM free and largest
  blocks after normal video/audio setup, after staging, after loading, and after
  cleanup. Preserve representative graphics/audio allocations. Capture stack
  high-water in bytes, including loader peak and call-chain compiler stack usage.
- CHK-003: A few host parser checks cover valid image, truncation, wrong target/
  ABI, unsupported relocation/import and simulated allocation failure. Verify
  complete cleanup and no ready handle after rejection. Do not build a broad
  validation framework. Include one truncated/oversized command frame and an
  unknown operation to establish the outer parser's rejection/resynchronization.
- CHK-004: Run serially on the command task. Test changed/repeated inputs, state
  reset after reload, stale handle rejection and a normal VDP print/draw afterward.
  The Author confirms MOS `dir` and keyboard still work. Report clearly which
  output comes from assembly, firmware diagnostics and the uploaded module.
- CHK-005: Account for staged bytes plus upload framing, LOAD, each argument/input
  update, RUN, optional replies, READ16 retrieval and UNLOAD. Compare logical VDU
  counts with captured application bytes. No per-call checksum/readback belongs
  in the proposed normal runtime.

An eight-byte result (float and counter) retrieved as four proposed READ16 queries
costs 4 × (17 outbound + 12 return) = 116 diagnostic bytes, in addition to the
call. Serial debug can avoid that diagnostic UART traffic during the first proof;
either route must be labelled. No result-traffic savings are implied by using
resident outputs.

## Physical gate and stopping point

- RUN-001: Prepare a known-good recovery image and use the canonical hardware
  deployment instructions. Confirm device identity, installed MOS/VDP and bench
  ownership. The existing agon-6502 installation is preserved as the reference;
  no device is touched under the current research authorization.
- RUN-002: At Gate A approve the implementation scope and ownership. Physical
  flashing/deployment needs explicit authorization if the selected tranche does
  not already include it. Emulator behavior cannot substitute for native ESP32
  loading. No GUI emulator is needed for this proof.
- RUN-003: Publish a short receipt: firmware/module/source identities, exact
  outputs, two placements, actual memory/stack deltas, wire counts, cleanup and
  human MOS confirmation. Any missing item is stated as unverified.
- RUN-004: Stop at Gate B. A successful tiny proof permits a decision about the
  packed renderer; it does not authorize changing agon-6502 or adding all numeric
  operations. If the limited ELF profile cannot work within the budget, report
  that failure and the smallest alternative instead of growing a general linker.
