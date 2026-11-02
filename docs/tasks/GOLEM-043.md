# GOLEM-043 — Concurrent VDPlet computation and buffered commands

Investigate whether a native worker could compute while the normal VDP task
continues interpreting buffered commands. It might overlap simulation with
drawing, but shared buffers, graphics state and module lifetime could make it
more trouble than it is worth. The Author requested a future research item on
2026-10-15, explicitly recognizing both possibilities. No concurrency feature
or implementation experiment is authorized by filing this task.

## Starting point

1. Stock buffered execution is serialized. Our current VDPlet runtime calls
   native entries synchronously on the command task and requires them to return.
   The public module ABI exposes buffer/frame services, not task creation.
2. Runtime `busy` is an ordinary Boolean guarding sequential reentry. Existing
   leases track native views and retain storage; they do not freeze its bytes
   or make the buffer registry, command interpreter or Canvas thread-safe.
   Stock buffer writes/clears do not consult the native lease table. Native
   pointers expire on release or entry return.
3. The ESP32/FreeRTOS platform supports dual-core tasks. That does not establish
   an idle second core, safe concurrent VDP service use or a useful speedup.
4. The existing GOLEM-038/Rally contract specifies serialized invocation and
   drawing ownership. This separate research must not silently change that
   contract, delegate peer work or claim existing qualification covers threads.

Sources: [current runtime](/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h:107),
[execution contract](/Agon/mystuff/agon-vdplet/docs/API-reference.md:274),
[public ABI](/Agon/mystuff/agon-vdplet/include/vdplet.h:69),
[prior ownership/lifetime findings](GOLEM-038/runtime-findings.md),
[D3 ownership boundary](GOLEM-038.md), and
[Espressif task/synchronization documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/freertos-smp.html).
Recheck the actual firmware/SDK versions when the research is taken up.

## Research questions

- BASE-001 [x] Inventory actual command, graphics, audio and other task/core ownership, synchronization and available public services. Distinguish asynchronous drawing already queued by firmware from a second buffered-command interpreter.
- OPT-001 [x] Compare retaining sequential calls, smaller cooperative batches, and a firmware-managed native computation worker. Evaluate computing state N+1 while the command owner draws completed state N; also explain why arbitrary parallel buffered execution or concurrent drawing is harder. Do not presume the worker is the right answer.
- OWN-001 [x] Define candidate input/output ownership: private snapshots, separate result storage, handoff synchronization and buffer reuse. Examine writes, clear/replace/consolidate, aliases, leases, mode changes and drawing/frame access. Identify exactly where present protections are insufficient; merely adding a lock or another task is not a complete design.
- LIFE-001 [x] Investigate submission, completion/publication, backpressure, failure, stop/reset and safe unload/reload. Preserve code/data until work finishes and avoid pointers surviving their permitted lifetime. Separate job completion, successful result publication and displayed-frame completion. Examine hangs, deadlocks, priority inversion and cancellation limits of trusted native code.
- RES-001 [x] Assess extra task stacks, retained snapshots/results, IRAM/DRAM/PSRAM pressure, shared-memory contention and effects on video/audio, command latency and VSYNC pacing. Consider floating-point task-affinity constraints under the actual SDK. Do not assume parallelism doubles throughput or that emulator timing predicts hardware timing.
- LANG-001 [x] Determine whether Golem needs any new syntax or whether an application/runtime scheduling option suffices. Explore a sequential stock-VDP fallback using the same computational source, with explicit ordering/error/publication semantics; do not promise transparent equivalence for code that observes concurrent shared mutation.
- CHK-001 [x] Propose a bounded matched-fixture protocol: identical inputs, work counts, drawing and pacing for sequential/worker cases, separate throughput and latency measurements, deterministic final results, and focused ownership/teardown failure cases. Check emulator dual-core/synchronization fidelity and distinguish emulated evidence from separately authorized physical qualification.
- REC-001 [x] Deliver a source-backed benefit/risk/complexity recommendation. Recommend either a small next experiment, retaining the sequential design, or deferral/rejection. Name ownership fences, missing services and measurable acceptance conditions before suggesting implementation.

## Scope and deliverable

GOLEM-057 RES-001 releases the bounded static review on 2026-10-30 under the
Author's coordinated implementation goal. It supersedes the earlier filing-only
hold for source inspection and research notes. The first deliverable remains
an ownership/cost recommendation; no worker implementation, emulator run,
service allocation or consumer source change is released by this research pass.
Priority synchronous FSIM/6502 qualification continues independently.

The first future tranche is a static feasibility/design review with a concise
recommendation, not a scheduler implementation or benchmark campaign. Favor
simple ownership and a bounded useful workload over general-purpose parallel
language machinery; this remains a hobby project. A decision against adding
concurrency is a valid result.

Keep any multifile notes, prototypes, fixtures or reports under
`docs/tasks/GOLEM-043/`. Generic runtime/SDK work, if separately released later,
belongs in `agon-vdplet`; compiler/language work belongs in Golem. Application
integration remains with its owning project. No firmware changes, peer messages,
new emulator profiles, deployments, physical flashing or task creation in the
runtime follow from this documentation request.

Acceptance for research is a clear account of what can overlap safely, what
would need to change, likely costs and how to test a selected small experiment.
Unmeasured benefits and unresolved risks must remain explicit. Any executable
experiment requires a later scoped release and the applicable human review.

## Static review delivered — 2026-10-30

[Recommendation and source-backed ownership/resource review](GOLEM-043/README.md)
answers the research questions above. [Exact source pins](GOLEM-043/source-inputs.json)
distinguish the uncommitted SDK/consumer candidates from the frozen owned pixels
export. Retain synchronous priority consumers; a later one-job pure-computation
experiment would need private snapshots/results, pinned image lifetime and
foreground publication. No idle core, measured speedup or worker-safe public
service table is established. Implementation and physical qualification remain
unreleased by this review; completed research checkboxes do not claim either.
