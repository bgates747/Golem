# GOLEM-002 — Research runtime-loaded native VDP math modules

Determine whether PC-compiled C/C++ modules can be uploaded and invoked on the
Agon's ESP32 VDP at runtime, alongside interpreted buffered commands. The primary
motivation is convenient numeric capabilities beyond the existing buffered
operations, with reusable code resident on the VDP and small wire updates.
Keep the Buffered Commands API unchanged. Research should establish what native
modules add over Rally's already implemented matrix-based floating-point path,
and recommend the smallest useful experiment before implementation.

Recorded 2026-10-11; research has not started. Overall status belongs in
[TODO.md](../../TODO.md). Put reports, exploratory code, build products and other
task artifacts under `docs/tasks/GOLEM-002/`; promote useful artifacts only after
review. This request records a future research task, not authorization to build
or deploy a module loader now.

## Motivation and boundaries

Golem's current purpose remains compilation to the VDP's Buffered Commands API.
Direct C/C++ cross-compilation may be the simpler initial route to native modules;
Golem integration is optional future work, not a prerequisite or a decision to
change Golem's purpose. Compilation happens on a regular PC. Runtime deployment
means loading an already compiled payload, not compiling on the eZ80 or ESP32.

Rally already uses stock matrix commands for scalar binary32 arithmetic through
1x1 matrices, affine inversion for reciprocal/division, and qualified rounding
and integer conversion routines. It supports fixed-point storage/formats, but
does not implement a general fixed-point expression system. Assess the native
option against that actual baseline; do not describe the buffered API as purely
integer arithmetic. The Author recalls proposing the matrix approach and sees
native modules as a complementary extension of the idea.

Keep existing buffered command encodings and semantics intact: no new buffered
subcommand, opcode repurposing or dependency on exploiting undocumented behavior.
If native loading/calling requires firmware support, distinguish that explicitly
from changing the Buffered Commands API and evaluate a separate interface.
Establish whether stock firmware can supply the required route before proposing
a firmware extension. Do not assume a loader exists merely because ordinary
buffer bytes can be uploaded.

Performance optimization is not the current priority. Numeric expressiveness,
interoperation, deployment practicality and wire traffic are the main questions;
executable-memory and execution-time/stack limits remain feasibility constraints.

## Sources and platform baseline

- SRC-001 [ ] Inspect the actual Agon ESP32 target, current VDP build/toolchain,
  Arduino/ESP-IDF versions and memory layout. Record source identities and
  distinguish measured facts from documentation assumptions. Follow the shared
  environment ownership conventions; upstream checkouts remain read-only.
- SRC-002 [ ] Examine Espressif's ELF loader, examples, supported original-ESP32
  target, build requirements, relocations and symbol-resolution facilities.
  Verify compatibility with the VDP's pinned toolchain rather than treating an
  ESP-IDF example as a drop-in Arduino/VDP component.
- SRC-003 [ ] Use GOLEM-001's comparison and the Rally compiler/native evidence
  to inventory existing scalar float, fixed-format, reciprocal and rounding
  support. Read the canonical bespoke-VDP emulator guide: Fab's host-native
  `.so` firmware modules are different from Xtensa payloads loaded on an ESP32.

## Feasibility and architecture

- RES-001 [ ] Identify concrete numeric gaps worth filling: ordinary scalar
  expressions, division domains, fixed-point arithmetic, useful math functions
  and precision/conversion choices. Explain which Rally/stock operations already
  meet each need and what a native module would add. Avoid assuming every C/C++
  math routine or floating-point width has hardware support or negligible cost.
- RES-002 [ ] Describe PC cross-compilation and runtime loading for one small
  C/C++ module, including payload format, code/data/literal sections,
  relocations, imports, initialization and unloading. Compare using Espressif's
  loader with a deliberately restricted module format; explain dependencies and
  compatibility costs before recommending one.
- RES-003 [ ] Establish executable-memory requirements on the original ESP32,
  including instruction/data address mapping, alignment, floating-point data
  access, available executable heap and fragmentation. Distinguish PSRAM data
  capacity from executable capacity. Specify how eventual measurements would
  account for the running VDP's video/audio allocations; do not change the live
  device merely to obtain those measurements during document research.
- RES-004 [ ] Propose a small versioned C ABI for finite native calls and VDP
  services. Cover typed input/output buffers, explicit sizes/formats, ownership,
  lifetime and error reporting. Evaluate direct access through service functions
  versus copying. Avoid binding the module interface to private C++ class layouts
  or firmware addresses. Determine which compiler/runtime/math dependencies must
  be linked into a module or deliberately exported by the host firmware.
- RES-005 [ ] Define how existing buffer operations and native jobs could
  cooperate without modifying the Buffered Commands API: upload inputs using
  existing facilities, invoke a native entry through a separately defined route,
  and leave results available to ordinary buffered commands. Identify required
  firmware changes, wire framing, feature discovery and behavior when the native
  facility is unavailable. Compare a C/C++ caller with an optional later Golem
  foreign-call interface; do not design new Golem syntax as a prerequisite.
- RES-006 [ ] Examine lifecycle constraints: finite return to VDP processing,
  stack budget, graphics/audio context, task/core ownership, shared data and
  module unloading while references exist. State the trust/failure model for
  native code and validate payload bounds, imports and resource limits at the
  proposed loader boundary. Do not present native execution as sandboxed or
  assume a timeout can safely interrupt arbitrary code.
- RES-007 [ ] Account for one-time module installation, persistent data and
  recurring input/call/result bytes. Compare with equivalent interpreted Rally
  operations and direct VDU emission. Keep wire reduction separate from numeric
  capability and speed; optional timing measurements must not displace the
  current motivation or become an unrequested optimization campaign.

## Deliverables and review boundary

- DEL-001 [ ] Write `docs/tasks/GOLEM-002/native-vdp-modules-feasibility.md`,
  marked DRAFT, with an executive summary, sources/versions, supported and unknown
  cases, architecture options, memory/toolchain constraints and recommendation.
  Separate verified platform support from untested Agon integration. Explain
  whether this ultimately belongs in a shared VDP-runtime project rather than
  the Golem compiler repository.
- DEL-002 [ ] Specify one bounded follow-on proof: a PC-compiled C/C++ numeric
  kernel receives changing resident inputs, writes results back to a VDP buffer,
  returns and unloads cleanly. Include an independent numeric reference, repeated
  invocation and ownership checks, exact wire accounting, and original-ESP32
  validation. A Fab host-library test cannot establish execution of Xtensa code;
  describe the appropriate physical or CPU-emulation evidence. Keep all proof
  artifacts in the task silo until promotion is agreed.
- DEL-003 [ ] Stop with the report and proposed experiment for Author review.
  Do not implement or flash loader firmware, alter the buffered API, launch
  emulator/hardware tests, or add a native Golem backend as part of the research
  writeup. If later pursued, staged firmware/emulator changes must follow their
  existing human-validation gates. Record acceptance/rejection and rationale in
  the dated devlog before removing the task from the register.

## Initial references to verify during research

[Espressif ELF loader](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/readme)
documents runtime loading and lists ESP32 among supported targets. Its presence
establishes a plausible foundation, not proven compatibility with the Agon VDP.
[ESP32 heap capabilities](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/mem_alloc.html)
distinguish executable memory from ordinary data allocations.

Local references:

- [GOLEM-001 comparison](GOLEM-001/rally-capability-integration-proposal.md).
- [Rally hosted compiler](/Agon/mystuff/golem-rally19/src/hosted.hpp) and
  [hosted contract](/Agon/mystuff/golem-rally19/docs/design/rally19-hosted-v1.md).
- [VDP build configuration](/Agon/agon-vdp/platformio.ini) and
  [buffered-command implementation](/Agon/agon-vdp/video/vdu_buffered.h).
- [Buffered Commands API](/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md).
- [Bespoke VDP emulator workflow](/Agon/mystuff/agon-dev-env/codex/bespoke-vdp-emulator.md).
