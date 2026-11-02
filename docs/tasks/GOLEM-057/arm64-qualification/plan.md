# Matching ARM64 backend: finite mechanism gate

Qualify the consumer-owned native ARM64 build before the Pi-first real-app
checks. The source-equivalent backend has a distinct architecture/artifact pin;
compilation and x86 evidence do not by themselves qualify ARM JIT execution.
No Oryx slot is consumed by these Pi-only processes.

- PRE-001 [x] Prepare without execution: preserve exact SDK rows-r1/immutable
  SDK/worker and original 6502 35,072-byte toy module and 64-byte state oracle.
  No peer source, compiled image or policy change. Freeze any thin harness and
  complete source/input pins before a launch.
- BLD-001 [x] Read FSIM's owner-supplied native build receipt and actual library/
  core/host-check/architecture pins. Stage a new stopped SDK Pi profile through
  the canonical generator, preserving previous profiles and the new immutable
  consumer build. No frontend/firmware rebuild.
- CPU-001 [x] After a separate explicit root Pi-only run permit, run the original
  35,072-byte toy once per engine in fresh processes, mode11, same8steps/29cycles,
  complete64-byte state matching generation21, INIT/FINI/unload/empty slot,
  same-mode executable heap recovery, nonzero stack and zero fault counters.
  Reuse the pinned SDK transport; no new fixture or protocol/lease bypass.
- ROW-001 [x] Then run the frozen rows-r1 generic positive suite once per engine,
  exact capability287 and existing worker/pictures/cross-kind/cleanup/old-module
  controls. The prior x86 no-option control remains separate, without repeating
  an unchanged generic build-selection test solely for the host architecture.
- AUD-001 [x] Stop on the first unexpected failure; preserve source/raw bytes/
  counters and normal shutdown. Audit saved results and record elapsed time,
  target stack/heap/tasks and architecture differences. These are emulator
  mechanism checks, not physical timing or consumer performance.

Scope is four sequential fresh Pi direct-library processes. Preparation is
released now; actual library pin and a later explicit root run permit are
required before any emulator starts. The central Oryx protocol remains intact;
no fabricated Oryx lease, competing launch, GUI, hardware or publication.

All five dispositions complete under G057-ARM-001; see [results](RESULTS.md).
Four fresh processes pass, normal joins/final Pi idle, no retry or source change.
FSIM real application checks were separately released after these results.
