# GOLEM-021 — Eight-hour Flexe/Fab integration and vdplet delivery

Build a user-owned experimental emulator combining Flexe's actual ESP32/Xtensa
execution with Fab's eZ80/MOS, SD and desktop features. Establish runtime-loaded
C/C++ vdplets in that environment, then offer the concrete service to the
agon-6502 agent. The Author authorized implementation and that contact on
2026-10-12. Physical flashing is not included; GUI review remains on Oryx and
nothing emulator-related is committed/pushed before explicit human acceptance.

The eight-hour work window begins 2026-10-12 09:37:48 America/New_York
(13:37:48 UTC), ending 17:37:48 local (21:37:48 UTC). This is the goal's work
window, not an assertion that every outcome will succeed. Preserve partial
results and report exact remaining blockers if the window expires.

TODO.md owns overall unfinished work. This task owns the integration plan;
GOLEM-002 owns native loading details. Artifacts stay in GOLEM-021/ and
GOLEM-002/ pending review/promotion. Upstream references remain read-only.

## Architecture and bounded scope

Keep Fab's existing SDL frontend, eZ80/MOS core and host-backed SD card. Supply
a new VDP shared library implementing Fab's existing ABI, backed by Flexe's
execution of actual ESP32 firmware. This preserves firmware UART/CTS and
decoded VGA/VSync/audio without a second desktop frontend or TCP peer in the
delivered runtime. Initial keyboard support uses the firmware's serial console;
physical PS/2 ULP execution is outside Flexe's current model. Pin both source
identities. See GOLEM-021/emulator/PLAN.md for the implementation plan.

## Stable subitems

- PLN-001 [x] Record authorization, deadline, repository boundaries and parallel ownership before implementation.
- ARC-001 [x] Inspect Flexe/Fab integration surfaces, choose and document the minimum useful architecture.
- EMU-001 [x] Build a reproducible user-owned hybrid runtime with pinned sources, preserving upstream trees. Owned Fab frontend plus actual-firmware Flexe adapter build on Pi; Oryx build/staging remains EMU-003.
- EMU-002 [x] Qualify actual MOS boot, SD reads, UART/CTS/VSync, graphics and serial input; report timing/model limits. Both engines pass stock MOS/BASIC, nine video geometries, audio and the full native-loading assembly proof. Actual post-success MOS prompt is observed; physical PS/2 ULP and cycle-accurate timing remain unsupported.
- EMU-003 [x] Stage a dedicated stopped Oryx human review instance with a single clear launcher and recognizable demo. Both native x86_64 engines pass the final 56-reply/560-byte fixture and 61,440 physical-VGA pixel checks, including unchanged pixels after failed source/geometry calls. All source-role labels and instructions are visible; no graphical instance was launched.
- VDP-001 [x] Adapt/build the minimal C/C++ vdplet loader and small versioned service ABI under GOLEM-002.
- VDP-002 [x] Demonstrate postboot upload, native invocation, changed inputs/state, unload/reload and changed code without firmware rebuild. Full Fab/MOS assembly run passes 44 replies/440 bytes; all three native entries reuse 0x40097b64, with three executable-code invalidations. See GOLEM-021/evidence/numeric-jit.json.
- VDP-003 [x] Add the packed monochrome renderer module if the numeric proof succeeds; preserve existing compressed resident data flow. Actual uploaded C++ passes both engines: 54 correlated replies/540 bytes and 61,440 decoded physical-VGA pixel checks per run, including native repeat calls and mode/source rejection. Final Oryx fixture passes 56 replies/560 bytes, adding stack-margin and geometry-preservation observations.
- XAG-001 [x] Identify the agon-6502 agent and publish an evidence-backed service offer through canonical cross-project coordination. Replied to its existing thread with exact ABI, artifacts, proof and codec ownership; message 2026-10-12_10-50-07_golem_vdplet-services-ready-one. Read acknowledgements do not imply acceptance.
- XAG-002 [x] Qualify the codec owner's supplied module if it arrives within this window; keep code ownership separate. The owner supplied its unchanged F2/F3 module; all 36 both-engine Oryx runs pass 32,708 replies and 2,211,840 image/margin pixels, including eight complete retained reads. Evidence returned to the owner. [Report](GOLEM-021/codec/README.md).
- DOC-001 [x] Record reproducible commands, artifacts, exact results, boundaries, source identities and task dispositions. GOLEM-021/README.md is the review/reproduction entry point; emulator/, consumer/ and GOLEM-002/runtime hold detailed source and evidence.
- GOL-001 [x] If primary outcomes are achieved with time remaining, plan and execute one bounded useful Golem tranche. GOLEM-022 implements multiple finite named drawing Subs with shared resident globals and unchanged legacy packages. Host, assembler and focused stock Oryx checks pass; its separate visual review is staged and stopped, awaiting the Author.
- REV-001 [ ] Retrieve and verify the Author's saved main hybrid review, record visual/keyboard acceptance, and obtain explicit commit/push approval. Agent implementation and headless qualification are complete; this human review is not claimed while the Author is AFK.

## Parallel ownership

The root owns task tracking, coordination, assembly consumer/integration and final
review staging. Delegate emulator runtime/architecture and vdplet loader to
separate agents with disjoint silo directories. A third independent worker may
qualify existing Flexe baseline/toolchain and return a receipt. Shared contracts
are agreed in messages before integration; agents do not edit each other's files.

## Completion and review

A native vdplet claim requires executing newly uploaded Xtensa code, not a
host-native .so or a firmware-built equivalent. A functional emulator claim must
include real MOS/VDP integration and observable pixels/inputs, with separate
upstream historical evidence and fresh results. Physical timing/cache/input
parity remains outside a functional test claim. No graphical process is launched
for the Author. Complete and stage the visual review in the same tranche.

## Integration findings

The initial stock interpreter baseline passes UART/VGA/audio/serial input.
Pristine Linux ARM64 JIT failed with SIGILL because upstream code generation
only maintained the host instruction cache on Apple ARM64; the owned core export
now receives a portable cache-maintenance correction. Runtime IRAM replacement
also needs an explicit JIT invalidation boundary and a changed-code regression.
These are emulator correctness changes, not performance optimization work.

The first full frontend run timed out before upload. A direct adapter check then
proved that the loader firmware boots and handles graphics/input. Inspection
identified that Flexe's ELF option activates compatibility function hooks as
well as symbols. The adapter now loads diagnostic symbols independently, with
compatibility hooks disabled. A further CTS deadlock was diagnosed through the
MOS debugger: the adapter stopped at 96 bytes, below the firmware UART's
120-byte interrupt threshold. The bound now follows the modeled 128-byte FIFO.
Full MOS startup, file loading, native module upload and saved-result execution
pass after these corrections. Failed attempts remain under evidence/attempt-*.
