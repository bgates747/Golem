# GOLEM-023 — Friendly Golem programs with native VDPlet execution

Research how Golem can provide an accessible, explicitly typed language while
VDPlets execute substantial routines natively on the ESP32. The preferred
direction to investigate is an explicit native compilation boundary with C as
an intermediate output, using the existing Xtensa compiler and VDPlet SDK.
Complete native source modules may offer a simpler first implementation than
mixed native/buffered procedures. This records options and a research plan;
no syntax, backend or implementation is approved by creating this task.

`TODO.md` owns overall status. This document owns scope, design questions and
acceptance criteria. Future reports, prototypes and fixtures belong under
`docs/tasks/GOLEM-023/`; creating this task does not authorize implementation,
hardware flashing or changes to the sibling runtime repository.

## Motivation and intended experience

The programmer should write ordinary typed declarations, expressions, loops and
procedures rather than C/C++, buffer arithmetic or manual command patching.
Native execution could remove much of the buffered API's arithmetic and
control-flow overhead for non-trivial jobs. Keep the original wire-traffic
benefit: install code/data once, update small inputs and invoke finite resident
work. Compute savings and wire savings must be assessed separately; neither
implies a measured frame-rate improvement.

PC compilation remains the intended workflow. Assembly owns file loading,
installation, updates, invocation, pacing and cleanup. Golem should publish
layouts and named assembly wrappers, preserving the existing integration style.
There is no requirement to compile on the eZ80 or ESP32, or to build an
interactive Golem application first.

## Options to compare

| Option | Author-facing experience | Relative effort | Principal limitation |
| --- | --- | --- | --- |
| Native library functions | Golem invokes supplied native transforms, decoders or drawing operations | Smallest | Only supplied operations gain native execution; surrounding buffered work remains |
| Explicit native procedures | Selected Golem Subs/Functions compile into a VDPlet | Moderate with a restricted first grammar | Requires an explicit supported feature set and clear native/buffered boundaries |
| Whole-module native compilation | Every procedure in a designated Golem source module compiles natively | Moderate initially; grows with language coverage | Module size, services and storage rules constrain source programs |
| Automatic mixed compilation | Compiler selects buffered or native execution for portions of a program | Largest | Adds placement policy, data movement and cross-backend execution complexity |

Native library calls are useful as an incremental bridge, but do not satisfy
the whole aim of allowing users to author their own efficient routines.
Explicit native procedures provide a useful long-term direction. A complete
native source module is a strong first-delivery candidate because it can share
native state and ordinary internal calls without introducing mixed execution
inside the same file. These are recommendations to evaluate, not decisions.

Defer automatic backend selection unless the research finds a compelling small
case. Explicit placement lets programmers understand costs and resource limits
without requiring an elaborate optimizer.

## Proposed compilation route

Translate a supported Golem subset into generated C, then use the existing
Xtensa toolchain and package builder to produce a VDPlet:

```text
Golem source
    -> parsing and type/semantic checks
    -> generated C and VDPlet entry/service adapter
    -> Xtensa compilation and restricted ELF packaging
    -> .vdpl package, layout manifest and assembly integration
```

Users should neither maintain nor need to understand the generated C. It can
remain available as a diagnostic artifact. The existing C compiler supplies
instruction selection, register allocation and optimization; Golem owns the
correct translation of its language semantics and the runtime boundary.

A direct Xtensa machine-code generator is unnecessary for the proposed first
route and would consume a substantially larger development budget. Using C
does not by itself settle integer overflow, rounding, aliasing or evaluation
order: generated code must preserve the selected Golem rules explicitly.

## Illustrative source, not implemented syntax

This independently initialized example records the explicit-procedure option:

```golem
float32 scale = 1.5
float32 bias = 0.25
float32 input = 2.0
float32 output = 0.0

Native Sub compute
    output = input * scale + bias
EndSub
```

`Native` is a proposed marker only. Compare it with selecting a whole native
source module through a compiler option, which could leave source syntax
unchanged. Neither mechanism currently compiles. Initial native research should
cover typed numeric values, expressions, conditionals, bounded loops and
Subs/Functions; that is a candidate subset, not an implemented feature claim.

## Storage and execution choices

For whole-module compilation, examine persistent native globals with explicit
host-visible input/output storage and ordinary calls between native procedures.
Define what assembly may update, what persists and how results are published.
Do not imply that an assembly wrapper can directly access arbitrary native
addresses or receive a native function result in eZ80 registers.

For mixed compilation, buffered procedures could invoke native workers through
resident RUN frames and share VDP buffers. Define ownership, serialization,
copying/synchronization and errors at each boundary. A native call must not hide
large repeated transfers or silently create a second divergent copy of state.
Function arguments, return values and object support require an explicit scope
decision; the first slice need not implement every proposed language feature.

Existing named assembly exports are the model for author-facing calls. Fresh
VDPlet handles after reload mean resident RUN packets must be patched or rebuilt
accordingly. Keep all calls finite and serialized; no scheduler, background
execution or automatic vblank wait is implied.

## VDP service and graphics boundary

Native arithmetic and loops are suitable initial candidates. Drawing requires
supported services or a command buffer that the normal VDP dispatcher executes.
Compiling a drawing statement into native code does not automatically supply a
native graphics implementation.

The current runtime provides contiguous buffer access and scoped packed
monochrome framebuffer rows. Direct framebuffer access is restricted to
single-buffered PALETTE2; it does not cover a colour double-buffered renderer.
An alternative is native generation of a resident VDU command stream followed
by a stock buffer call. That removes some computation/patching costs while
retaining ordinary drawing dispatch, which must be counted separately.

Identify any additional services before proposing a graphics milestone. Keep
generic loader/services in the sibling runtime repository and language/backend
work here. The standalone runtime's hardware-only documentation should not gain
language-project or emulator branding from this research.

## Existing runtime limits and evidence

The current classic-ESP32 profile admits one active module, a 16 KiB package,
8 KiB executable text and 4 KiB combined read-only/initialized data and BSS.
The declared stack allowance is at most 512 bytes and is not runtime stack
enforcement. RUN argument buffers are at most 32 bytes; larger application data
can reside in separately referenced buffers. Packages may remain in VDP buffers
and be loaded interchangeably.

The toolchain uses the windowed Xtensa GCC 8.4.0 `esp-2021r2-patch5` ABI. Modules
cannot rely on arbitrary firmware imports, a C++ runtime or background work.
Generated C that introduces compiler/library helper calls needs a compatible,
self-contained solution within the admitted image, rather than assuming libc
or libm is available. Native modules are trusted and must return; a fault or
infinite loop can stop firmware processing.

A direct silent RUN is 17 wire bytes. A preinstalled RUN packet can be invoked
through a six-byte stock buffer call. Installation, changing inputs, requested
acknowledgements and result reads are additional traffic. The new runtime
repository builds successfully and its host loader sanitizer checks pass;
physical execution and performance qualification remain pending. Earlier
experimental execution evidence must not be presented as hardware measurement.

## Research subtasks

- OPT-001 [ ] Compare the four integration options against approachable source,
  development budget, supported language coverage, recurring wire traffic and
  expected sources of execution overhead. Recommend a starting option and a
  bounded migration path.
- LNG-001 [ ] Specify a candidate native subset with complete initialized
  examples, including expressions, conditionals, finite loops, Subs/Functions
  and calls. Distinguish existing syntax, proposed syntax and deferred features.
- GEN-001 [ ] Describe the Golem-to-C translation and packaging interfaces;
  account for integer checks, conversions, float behavior, evaluation order,
  unsupported imports and compiler-generated helper dependencies.
- ABI-001 [ ] Propose inputs, outputs, persistent state, arguments/results,
  exported entry names, assembly wrappers and fresh-handle updates. Compare
  whole-module storage against shared native/buffered storage and quantify any
  recurring copies or transfers.
- SVC-001 [ ] Map desired operations to current runtime services. Identify
  graphics restrictions and compare command-buffer generation with any proposed
  service extension; keep repository ownership boundaries explicit.
- RES-001 [ ] Assess text/data/stack and argument-size limits for representative
  small routines. Explain unsupported programs and diagnostics without adding
  an automatic partitioner, heap or general scheduler as prerequisites.
- MEA-001 [ ] Propose a recognizable, finite first experiment and fair buffered
  versus native comparison. Separate installation/load time, repeated compute,
  wire bytes, copies, graphics dispatch and presentation. Define correctness
  checks and physical timing evidence needed to support a performance claim.
- REC-001 [ ] Deliver a recommendation with phased implementation scope,
  acceptance criteria and unresolved author decisions. Stop for review before
  compiler/runtime implementation or physical deployment.

## Acceptance criteria for the research

The report must explain what a less-technical programmer writes, what the PC
toolchain emits and what assembly/firmware each do. It must recommend a small
first milestone, identify its unsupported cases and distinguish measured facts
from expected benefits. Include enough initialized examples and interface detail
for the Author to review the proposed experience without reading generated C.

Research is complete when those decisions are reviewable; a new backend or a
running hardware prototype is not required to complete this task. No changes to
current buffered-mode behavior, new native syntax or service extensions are
authorized by this document alone.

## References

- [Main language proposals](../design/language-type-proposals.md)
- [Resident scalar and assembly integration](../design/resident-scalar.md)
- [Resident drawing contract](../design/resident-drawing.md)
- [Language examples, including proposed features](GOLEM-003/examples/examples.golem)
- [VDPlet Runtime](../../../agon-vdplet/README.md)
- [Hardware usage guide](../../../agon-vdplet/docs/STARTHERE.md)
- [Runtime API reference](../../../agon-vdplet/docs/API-reference.md)
- [Public C ABI](../../../agon-vdplet/include/vdplet.h)

Recorded from the Author's Golem/VDPlet integration discussion on 2026-10-12.
