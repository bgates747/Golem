# GOLEM-051 — Compile and run Golem directly on Agon

Implement a MOS-native compiler for every currently implemented Golem feature and deliver
a standalone how-to package. A user with an editor and the package on SD must
be able to compile source, save the result and run it without a PC, an LLM or
custom firmware. Headless qualification uses the Pi; Oryx and the physical
bench are off. Human review and publication remain later gates.

## Plan and acceptance

- OPT-001 [x] Record all on-machine development options and their tradeoffs in
  [the design note](../design/on-agon-development.md).
- SPEC-001 [x] Specify retained grammar, bounded resource use, file formats,
  installation/cleanup contract and error behavior before implementation.
- CMP-001 [x] Disposed: the initial compact C subset was superseded by the
  Author's full-language requirement. Preserve its exploration separately;
  PORT-001 implements source reuse instead.
- MOS-001 [x] Cross-build actual eZ80 compiler/runner binaries with AgonDev.
  Compilation reads source from SD and writes executable buffered packages.
  The runner owns finite installation, invocation, completion and cleanup.
- CHK-001 [x] Check accepted/rejected syntax, errors, boundary arithmetic,
  loop endpoints/reentry and resource bounds on the host with sanitizers;
  compare meaningful overlapping programs against existing Golem behavior.
- TGT-001 [x] Run the actual compiler under MOS on a Pi headless emulator;
  compare its saved output to host output and execute generated examples on
  stock VDP. Verify failure paths and normal return to MOS.
- DOC-001 [x] Make a coherent language reference with explicit feature support;
  include it in the package alongside initialized examples and a small how-to.
- PKG-001 [x] Build a reproducible ZIP with native binaries, source/build recipe,
  examples, quick start, language reference and source. Internal qualification
  records stay in the repository, outside the consumer ZIP. Keep all new
  implementation/artifacts in this task silo until promotion is agreed.
- REV-001 [x] Leave a stopped, reproducible review fixture and record headless
  results. Human validation/publication are pending; do not flash hardware,
  contact the powered-off hosts or commit coupled work before approval.

- REV-002 [ ] Author visual/keyboard review and explicit commit/publication
  approval. Physical-hardware qualification is not claimed by the headless run.

Evidence and measured limits: [results](GOLEM-051/RESULTS.md).

## Scope

This is option 1 of [on-Agon development](../design/on-agon-development.md).
The Author expanded scope on 2026-10-26: do not stop at a subset; support
everything the current language/compiler implements, using C/C++ targeting
AgonDev. This supersedes the initial bounded-subset scope in this plan and
contract.md. Proposed/unimplemented syntax is not implementation scope.
All raw/resident/table/drawing/portable/stock-matrix profiles, including native
C source emission, must be covered. Native C emission does not itself supply
an on-Agon C-to-Xtensa toolchain (options 2/3 remain separate).
The initial C subset exploration is retained under exploratory-subset/ and is
not the deliverable. The preferred implementation reuses the actual current
C++ parsers/backends with a MOS platform/library adaptation and compares output
against the existing host compiler. Resource limits must be measured and honest.
Existing desktop source/tests and pending GOLEM-049/050 work
must remain intact. A separate future task may promote the compiler or extend
it to precompiled VDPlet calls/direct Xtensa output.

- PAR-001 [x] Pin current source/fixtures and inventory every implemented profile,
  language construct and output option. Record full feature parity as the gate.
- PORT-001 [x] Adapt current C++ compiler to AgonDev/MOS, including necessary
  containers, files, error recovery and faithful wide/float literal processing.
- PAR-002 [x] Exercise all maintained positive/negative compiler fixtures on the
  adapted host build and actual eZ80/MOS build; explain any resource-limited
  case without claiming full completion while language gaps remain.

## Publication checkpoint — 2026-10-27

The Author explicitly authorized commit/push of all pending Golem work. The
standalone distribution walkthrough is in progress on the accepted Oryx stock
instance. Publication approval is satisfied; the remaining walkthrough in
REV-002 is not represented as complete. GOLEM-053 separately passed human
review, including actual MOS compilation and stock execution.
