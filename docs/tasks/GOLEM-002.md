# GOLEM-002 — Research and develop runtime-loaded C/C++ vdplets

## Executive summary

Research a small VDP facility that loads PC-compiled C/C++ modules onto physical
Agon at runtime. Establish executable loading and a versioned C service interface
first; use agon-6502's packed monochrome renderer as the first substantial
application. Numeric modules remain an intended use. Golem may call vdplets later,
but neither a native Golem compiler nor emulator support is a prerequisite.

**Current authorization:** the Author's 2026-10-12 eight-hour goal authorizes
implementation of the loader/modules, emulator integration and a service offer to
the agon-6502 agent. GOLEM-021 coordinates that work. Earlier Gate A research-only
restriction is superseded by this explicit instruction. Physical flashing remains
outside scope; human emulator validation/commit rules still apply. Plans precede
each implementation tranche and subitems are checked as evidence is obtained.

This revises the original 2026-10-11 native-math research task following the
2026-10-12 discussion of agon-6502. Existing SRC, RES and DEL identifiers are
retained with their original research purposes. [TODO.md](../../TODO.md) owns
overall status; this document owns the detailed plan. Reports, prototypes,
receipts and other task artifacts stay under `docs/tasks/GOLEM-002/` until an
explicitly agreed promotion or project handoff.

## Historical research tranche result

Phases A–C research is complete. Review the [feasibility report](GOLEM-002/native-vdp-modules-feasibility.md),
[C ABI draft](GOLEM-002/vdplet-abi-draft.md), [physical proof proposal](GOLEM-002/physical-proof-proposal.md)
and [evidence ledger](GOLEM-002/research/evidence.md). Recommendation: adapt pinned
Espressif ELF loading for one finite, zero-import module before the packed renderer.
The Author's additional [Flexe resource](GOLEM-002/research/flexe-agon-notes.md)
provides a candidate for testing the actual Xtensa payload before hardware;
that route is source-reviewed, not run or adopted yet.
At that checkpoint, executable heap, module link output and integration footprint remained
proof questions. ORG-001 was disposed for the research tranche by the ownership
recommendation; an eventual owner/shared integration agreement remains a Gate A
condition. No runtime had been built or deployed at that checkpoint.

## Implemented emulator proof

GOLEM-021 now supplies a working owned Fab/Flexe emulator and separately uploaded
numeric and packed-renderer Xtensa modules. Both engines boot real MOS/VDP,
load/execute/reload changed code, return to MOS and match independent numeric
and physical-VGA expectations. Oryx's final fixture passes 56 replies/560 bytes
and 61,440 pixels per engine. See the [integration report](GOLEM-021/README.md)
and [native qualification](GOLEM-002/runtime/qualification.md). The generic
loader/service firmware is required once; subsequent module changes need no
firmware rebuild. Stock VDP alone does not provide this loader.

The agon-6502 owner accepted the services and supplied its own F2/F3 codec for
independent qualification. Its source remains in that project. Physical heap,
timing/cache and hardware acceptance remain open; no device was flashed.
The earlier research-only Gates A/B do not block the implementation explicitly
authorized through GOLEM-021. Physical adoption/promotion gates remain in force.

## Direction and success criteria

The intended result is one generic loader/service addition to a user-owned VDP
firmware, if required, followed by application-specific modules that can change
without reflashing that firmware. A stock-firmware loading route has not been
established. Research must determine whether one exists; if it does not, explain
the required custom-firmware installation honestly. Avoiding a separate firmware
build/flash for each subsequent function is the deployment goal.

C/C++ is the module implementation language. Assembly applications can load,
update and invoke modules independently of Golem. PC tools perform compilation,
linking and packaging. The eZ80 application owns file loading and transfer; the
VDP does not ask MOS to find files. Normal input/output data should remain in VDP
buffers where useful, with small recurring requests and optional small replies.

A successful first implementation must demonstrate a separately built module
uploaded after firmware boot, actually executing as native code on the ESP32,
using the agreed services, returning finite results and unloading safely. Merely
calling a function compiled into firmware, uploading data bytes, or loading a
host-native Fab `.so` does not establish that capability.

Keep this proportionate to a hobby project: one loaded module and one finite call
at a time are sufficient for the first proof. No general plugin marketplace,
security sandbox, dependency manager, parallel job scheduler or extensive
optimization framework is required.

## Existing evidence and limits

| Evidence | What it establishes | What remains open |
| --- | --- | --- |
| [agon-6502 packed blit](../../../agon-6502/experiments/vdp003/packed_mono.h) and [VDP-003](../../../agon-6502/docs/tasks/VDP-003.md) | A small C++ kernel can copy an existing packed monochrome buffer into physical framebuffer rows; project records include physical Choplifter acceptance. | It is currently compiled into custom firmware and invoked through private buffered opcode 0xF0. It is not a runtime-loaded module. |
| [Pingo/Wolf return-channel notes](GOLEM-002/vdplet-return-channel-notes.md) | Existing custom firmware and MOS applications exchange small completion records for pacing. | Reusable module services, stock availability, MOS compatibility and return framing need review. |
| [Rally capability comparison](GOLEM-001/rally-capability-integration-proposal.md) | Stock matrix operations already support useful scalar floating-point work, rounding and conversions; fixed formats exist. | Native modules must be justified against those capabilities, rather than treating the buffered API as integer-only. |
| [Canonical Fab integration guide](/Agon/mystuff/agon-dev-env/codex/bespoke-vdp-emulator.md) | Fab runs host-native VDP code through a shared-library interface. | It does not execute Xtensa payloads. Any future host-native vdplet counterpart is a separate implementation/test route. |

The current agon-6502 pipeline has three distinct responsibilities: eZ80 packs
and sends compressed XOR changes; stock resident buffered commands reconstruct
packed VDP pages; the custom native kernel copies the selected page to framebuffer
rows. The new kernel itself does not perform the XOR/RLE decoding or expand pixels
to RGBA. Its representative image is 280x192, 35 bytes per row, 6,720 bytes total.

Its present contract requires one exact-sized source block, MSB-left packing,
byte-aligned X/width, a wholly in-bounds rectangle and a single-buffered PALETTE2
controller. It drains earlier Canvas work without a vblank wait, copies through
actual scanline pointers and allocates no per-blit scratch image. Preserve these
semantics in the proposed port. Autonomous sprites, vblank notifications,
behind-the-beam rendering and changing game pacing remain separate work.

## Phase A — Establish the platform and ownership

**Scope:** read-only research after the Author approves this tranche. Do not
modify the running device or another project's active work to obtain answers.

- ORG-001 [x] Identify the user-owned firmware and eventual standalone runtime
  project that should own the loader and services. Golem hosts this initial plan,
  not necessarily the resulting subsystem. Inventory active agon-6502/Pingo/Wolf
  integration surfaces and agree ownership before any shared-source change.
  Coordinate through the canonical cross-project contract when implementation
  spans independently owned projects; do not silently alter the other agent's
  working tree or claim its uncommitted implementation as a stable release.
- SRC-001 [x] Pin the actual Agon ESP32 target, VDP source, physical graphics
  library, Arduino/ESP-IDF/toolchain versions, compiler flags and memory layout.
  Distinguish physical firmware dependencies from userspace Fab dependencies.
  Inspect ELF/map/configuration evidence for available executable-memory routes;
  list later on-device measurements separately. Do not assume another ESP32
  family or an ESP-IDF example matches the original Agon target.
- SRC-002 [x] Examine Espressif's ELF loader and its current primary sources for
  original-ESP32 support, relocations, imports, memory allocation and dependencies.
  Check compatibility with the actual VDP toolchain. Compare its integration cost
  with a deliberately restricted module format. Prefer an existing maintained
  implementation if it fits; do not start a general ELF linker unnecessarily.
- SRC-003 [x] Reconcile the existing Rally numeric capabilities and Golem's
  supported profiles with the original motivation. Review the actual packed
  renderer and return hooks as separate evidence. Read the canonical bespoke-VDP
  guide and explicitly separate Xtensa execution, ordinary host unit tests and
  any optional Fab-native module scheme.

**Output:** a source/version inventory, evidence ledger (verified, inferred,
unverified) and short ownership recommendation. Primary-source links and local
commit/file identities should make every feasibility conclusion traceable.

## Phase B — Specify the smallest viable module contract

**Scope:** propose choices and costs, not firmware implementation. Resolve the
following together so the proposed loader and module can actually interoperate.

- RES-001 [x] Identify concrete useful kernels and the numeric gaps they fill.
  Retain float32/fixed-format arithmetic, division, conversion and useful math
  functions as candidates. Explain what stock/Rally already supplies. Recommend
  a trivial data/numeric kernel for loading proof, then the packed blit for a
  useful application. No new Golem numeric semantics are decided here.
- RES-002 [x] Specify PC compilation/linking and the proposed payload: target and
  ABI identity, entry points, code/literals/read-only data, writable data/BSS,
  alignments, relocation records, imports and initialization/teardown. Establish
  Xtensa calling convention compatibility, including windowed versus call0 where
  relevant, literal access and compiler helper dependencies. Initially restrict
  C++ modules to C-linkage entry points; assess excluding exceptions, RTTI, STL,
  global constructors and module heap allocation. Explain any required libgcc,
  libc or maths support rather than inheriting it accidentally.
- RES-003 [x] Establish where code and data can reside and how the loader obtains
  executable memory on the actual ESP32/VDP build. Investigate instruction/data
  mappings, alignment, literal/float access, cache synchronization and PSRAM
  limitations. Estimate loader footprint and peak module/code/data/relocation
  memory, including temporary upload storage. Define a small later hardware
  measurement under representative video/audio allocations. Recommend limits
  from evidence; do not invent a convenient RAM budget or assume free flash is
  runtime executable heap.
- RES-004 [x] Draft a small versioned C ABI passed explicitly at module entry,
  with fixed-width fields, structure sizes, capability checks and clear ownership.
  Cover bounded input/output buffers, access lifetimes and finite init/call/fini.
  Avoid private C++ object layouts, hard-coded firmware addresses and unrestricted
  firmware symbol export. Specify whether modules copy data or borrow scoped
  buffer views; borrowed storage must remain valid until release/return. Inspect
  error/status and optional completion services using the existing hook notes.
- RES-005 [x] Specify discovery, upload, load, invoke and unload from an assembly
  application. Candidate transport: use existing buffer writes to stage module
  bytes, then a separately framed generic module interface to load/call them.
  Verify that such dispatch is possible in the chosen firmware; do not allocate
  an opcode now. Preserve existing Buffered Commands API encodings and semantics.
  agon-6502's private 0xF0 is historical application machinery, not an adopted
  vdplet command allocation. Define unsupported-feature behavior, bounded command
  lengths, module/entry identification and stale-handle handling. Determine
  whether a call can later execute from a resident command stream without unsafe
  nested interpreter activity. Normal module invocation must not require MOS
  filenames, file-request callbacks or a mandatory completion reply.
- RES-006 [x] Define lifecycle and failure behavior for one serialized finite
  call. Account for the VDP event loop, graphics/audio state, stack, unloading
  during execution, borrowed-buffer mutation and partial initialization failure.
  Loader rejection must release its temporary allocations and preserve existing
  buffers/modules. Treat native modules as trusted development code: format and
  range checks do not sandbox execution, and a host timeout does not stop native
  code. Do not promise arbitrary cancellation, fault recovery or hot replacement
  while calls are active. Establish how the application knows it is safe to
  unload, using ordering and an administrative completion mechanism if needed.
- RES-007 [x] Account separately for loader firmware size, module image upload,
  one-time linking/install traffic, persistent data, invocation bytes, changing
  inputs, optional replies and result retrieval. Compare with the current packed
  renderer and relevant Rally/stock numeric paths. Loading once must permit
  reuse without repeating code or full image data. For agon-6502, do not claim
  extra image compression from replacing its renderer: the existing packed
  XOR/RLE transport is already retained. Speed is a secondary benefit to measure
  only for a justified question; capability and reduced recurring traffic lead.

For renderer services, evaluate scoped access to packed source bytes and writable
framebuffer rows, with explicit pixel format, dimensions and row-byte capacity.
The firmware must own coordination with queued Canvas work and the release of
any drawing lock. Direct module access to `_VGAController`, `canvas`, the C++
`buffers` map or private FabGL classes is not a stable ABI. Conversely, exporting
the entire packed-blit implementation as a firmware service would not prove that
the rendering kernel runs in the loaded module. Show where that boundary lies.

For return traffic, distinguish module load/status, call completion and result
values. Results can stay resident. Administrative acknowledgements or an explicit
optional diagnostic reply may be useful without making every draw chatty. Review
actual MOS packet parsing and keyboard coexistence before proposing reuse of
Pingo/Wolf's keycode-packet transport. No wire format is adopted by this plan.

## Phase C — Feasibility report and first development decision

- DEL-001 [x] Write `GOLEM-002/native-vdp-modules-feasibility.md`, marked DRAFT,
  with executive summary, pinned sources, loader options, required firmware
  changes, ABI outline, memory/stack budgets, unresolved risks and a recommended
  minimum scope. Include a decision table: supported now, feasible with a host
  addition, unverified, or unsuitable. A conclusion that the loader costs too
  much for the available memory or development budget is a valid result.
- DEL-002 [x] Specify one bounded physical proof: a separately compiled C/C++
  numeric/data kernel reads resident inputs, produces independently checkable
  resident results, returns and unloads. Include executable/literal/data/import
  evidence, repeat invocation, fresh input, memory accounting and exact wire
  accounting. Propose a second load address or layout to exercise relocations
  where the selected format requires them. Describe how the proof distinguishes
  uploaded native code from an equivalent built-in firmware function.
- DEL-003 [x] Stop with the report, proposed service interface and hardware proof
  for Author review. Present a recommended decision, effort range and decisive
  unknowns. Do not build/flash loader firmware, alter either application's
  runtime, deploy a fixture or create a native Golem backend in this tranche.

**Gate A:** the Author reviews feasibility and selects ownership, loader approach,
initial ABI scope and the physical proof. Approval here defines the next concrete
tranche; it does not silently approve renderer adoption or all optional work.

## Phase D — Minimal physical loading proof, after Gate A

- LOD-001 [x] Implement only the selected loader and minimal C service table in
  the agreed user-owned project. Keep upstream references read-only. Add checks
  for image boundaries, target/ABI mismatch, required relocations/imports and
  configured memory limits. No new compiler backend or game integration.
- LOD-002 [x] Add a reproducible PC build/package command for one tiny module and
  an assembly consumer. Declare exact toolchain/firmware identities and all
  required sections/imports. Build the module separately from firmware; ensure
  the kernel is absent from the host firmware image. File lookup/loading belongs
  to the assembly-side application, with fixed build-resolved buffer/entry data
  where practical.
- LOD-003 [x] Perform small, targeted host checks of packaging and loader parsing:
  one valid image plus representative truncated, wrong-target/ABI, unsupported
  relocation/import and allocation-failure cases. Inspect actual module ELF/map
  output and loader footprint. Do not substitute a host `.so` run for ESP32
  execution or create a large validation framework before the first proof.
- LOD-004 [ ] Prepare a physical test package, deployment instructions and a
  known-good recovery image/procedure under the canonical hardware workflow.
  Inspect bench ownership and identify the actual target before use. Stop for
  explicit flashing/deployment authorization if it is not already included in
  the approved tranche. Preserve the Author's current agon-6502 installation.
- LOD-005 [ ] On the approved physical target, boot the loader firmware and upload
  the separately built module afterward. Verify finite execution against an
  independent reference, changed inputs, repeated invocation and unload/reload.
  Exercise distinct placement if relocation is part of the design. Record
  executable/data memory before and after, module/image identities and return to
  ordinary VDP operation. Use a small serial diagnostic or explicit status query
  for proof; keep instrumentation out of normal per-call operation.
- LOD-006 [x] Publish a concise proof receipt with actual results, memory/stack
  observations, limitations and loader cleanup behavior. Record failures honestly;
  do not test recovery by executing arbitrary crashing native code. Explain any
  required ABI change before proceeding. Completed for actual Xtensa emulation
  in runtime/qualification.md; physical LOD-005 remains unchecked.

**Gate B:** review the real ESP32 loading proof and costs. Stop if the result
requires an impractical firmware/toolchain dependency or consumes too much VDP
memory. Otherwise authorize the packed renderer as the next bounded tranche.

## Phase E — Packed monochrome renderer vdplet, after Gate B

- BLT-001 [x] Pin an agreed agon-6502 VDP-003 source version and capture its current
  contracts before porting. Coordinate with its owner; keep current custom
  firmware/game binaries available as the reference. Preserve eZ80 packing,
  XOR/RLE transport and stock resident page decoding.
- BLT-002 [x] Move the row-copy kernel into a separately built C/C++ module using
  the agreed services. Preserve exact-sized single-block input, MSB-left bits,
  byte-aligned X/width, wholly in-bounds PALETTE2 single-buffer drawing and opaque
  overwrite of both zero and one bits. Use actual row mappings, not an assumed
  contiguous framebuffer. Add no per-frame allocation, RGBA expansion, pixel
  setter loop, mandatory vblank wait or compulsory per-draw reply.
- BLT-003 [x] Build a small assembly caller that loads the renderer once and
  submits repeated buffer/rectangle jobs. Keep the first test independent of game
  emulation so failures can be attributed. Compare known patterns and erasure
  against independently expected pixels; verify representative invalid requests
  preserve the destination and unrelated buffers. Confirm queued drawing order
  and that all scoped views/locks are released on every defined exit.
- BLT-004 [ ] Qualify the loaded renderer on physical Agon, then prepare an
  explicitly authorized agon-6502 application trial. Retain recognizable latest
  game names and its owner's deployment conventions. Human rendering/keyboard
  review is part of that tranche; do not claim tear-free output or change pacing.
  Any timing comparison must use the owning project's native-game pacing and
  fixed-baseline reporting contract; no automatic benchmark campaign.
- BLT-005 [x] Report recurring wire cost, upload/load costs, code/data memory and
  the extent of shared host services. Verify module unload/reload between finite
  jobs and normal graphics operation afterward. Recommend adoption, revision or
  retention as a prototype, including any residual dependency on custom firmware.
  Emulator proof is complete; retain as a prototype pending BLT-004 physical review.

**Gate C:** the Author reviews the useful physical application and decides whether
to promote the loader/ABI and replace the application-specific firmware command.
Neither this plan nor the successful tiny-kernel proof authorizes that replacement.

## Phase F — Packaging and additional uses, separately selectable

- PKG-001 [ ] After acceptance, promote reusable loader/services into the selected
  standalone project. Provide a small SDK header, linker/build instructions,
  module manifest/example, assembly integration example and supported host/target
  version policy. Record dependency licenses and pinned versions. Define how an
  application detects missing/incompatible support and exits clearly. Document
  firmware installation once versus subsequent module replacement without flash.
- NUM-001 [ ] If selected, implement one useful numeric module chosen from the
  original research: explicit float32/fixed-point operations or a small math
  kernel. Define its numeric domains, conversions and finite error behavior;
  compare with stock/Rally capability. Results remain resident unless requested.
  Do not let this broaden into all of libm or a new language type system.
- EMU-001 [x] Disposed by GOLEM-021: execute actual Xtensa through Flexe in Fab
  instead of maintaining a separate host-native module format. Original option:
  if selected, design a separate Fab-native counterpart compiled from
  the same kernel source and matching public service contract where practical.
  Identify ARM64/x86-64 artifacts, ABI/lifecycle differences and qualification
  boundaries. Host parser/kernel tests may be useful; host module loading does
  not qualify Xtensa relocations, ESP32 executable memory or hardware scheduling.
  No ESP32 CPU emulation or Fab loader change is required by the physical plan.
  Before choosing that counterpart, evaluate the Author-supplied Flexe resource:
  its existing Agon VDP/MOS fixture may run the actual Xtensa payload and remove
  the need for a second module format. A bounded preflight could be selected at
  Gate A; the resource review alone did not authorize setup or execution.
- GOL-001 [ ] If selected after a stable runtime exists, research an optional
  Golem foreign-call interface using installed module/entry identities and
  resident buffers. Demonstrate safe finite dispatch from a command stream and
  account for recurring bytes. Keep the native module written in C/C++; generating
  Xtensa from Golem is a separate, unapproved project. Assembly remains a fully
  supported caller without Golem.

Autonomous sprite motion remains a separately recorded, deferred design idea.
Do not add scheduling hooks, background jobs or vblank events to this loader
project merely to accommodate that idea.

## Deliverable placement and verification policy

The research report, ABI draft, evidence ledger and proof proposal initially live
in this task silo. If development is authorized here before a project handoff,
use its own subfolders such as `research/`, `proof/` and `packed-mono/`, with
build products ignored. Agree the destination before promoting shared runtime
source; do not clutter production Golem code with experimental loaders/scripts.

Use checks that resolve actual unknowns: primary source and binary inspection for
research, a few meaningful format/failure tests for the loader, real execution
for native feasibility, and focused pixel/ordering tests for the renderer. No
per-copy readback/checksum machinery belongs in the rendering hot loop. Retain
source/artifact identities and distinguish historical evidence from new runs.

A future emulator review, if selected, uses the canonical dedicated Oryx profile
workflow and a clearly labelled demo. No graphical instance is launched for the
Author. Physical deployment, flashing, shared-source changes and emulator-related
commit/push follow their respective explicit authorization/review gates. The
GOLEM-021 instruction explicitly authorizes the agon-6502 service offer;
it does not transfer that project's source or hardware ownership.

## Historical first tranche disposition

The Author authorized **Phases A–C only** on 2026-10-12. Research deliverables
are complete; stop at Gate A. The report recommends the next implementation scope
and gives a provisional effort range. The subsequent eight-hour GOLEM-021
instruction authorized the implementation and coordination now recorded above.
LOD-004/005, BLT-004 and packaging/adoption remain physical or later-review work.

## Research leads to verify, not feasibility conclusions

- [Espressif ELF loader](https://components.espressif.com/components/espressif/elf_loader/versions/1.3.4/readme): the earlier plan identified this as a candidate; verify the appropriate version and original-ESP32/VDP toolchain fit during research.
- [ESP-IDF ESP32 heap capabilities](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/mem_alloc.html): check against the actual pinned firmware version and allocation APIs.
- [Current VDP build configuration](/Agon/agon-vdp/platformio.ini) and [buffer dispatch](/Agon/agon-vdp/video/vdu_buffered.h): source references, not authority to modify the upstream checkout.
- [Packed renderer build adapter](/Agon/mystuff/agon-6502/experiments/vdp003/build.py): shows current compilation into firmware and private dispatch insertion.
- [Packed renderer qualification record](/Agon/mystuff/agon-6502/docs/validation/2026-10-12-vdp003-packed-blit.json): separates native-library probes and user-reported physical evidence.
- [Rally hosted source](/Agon/mystuff/golem-rally19/src/hosted.hpp) and [hosted contract](/Agon/mystuff/golem-rally19/docs/design/rally19-hosted-v1.md): actual existing numeric capabilities.
- [Buffered Commands API](/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md) and [return-channel notes](GOLEM-002/vdplet-return-channel-notes.md): existing buffer/transport context.
