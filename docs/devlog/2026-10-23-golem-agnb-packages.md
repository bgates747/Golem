# Streamed Golem packages

Golem now publishes an explicit buffer load plan and uses the generic AGNB writer
to package it. An assembly host can load a compiled program larger than its own
application RAM, then perform setup and call named entries. Embedded output remains
unchanged. The working adapter is assembly; C/C++ names/hooks are generated, but a
reusable AgonDev C++ file reader remains a following tranche.

1. GOLEM-046 implements `--emit-load-plan` for current resident, portable and stock
   matrix profiles. Initial bytes come directly from the compiler's structured
   maps; the producer never reverse-engineers arbitrary VDU installation bytes.
2. Generic format/framing/streaming stays in agon-utils. The accepted dependency
   is AGNB0.3 on `codex/agnb-plain-buffers`, commits3edf8bc and35dd8ef. No generic
   loader, format or SDK change was needed for this Golem producer tranche.
3. Each component's thin include/header has named loading/setup/call/cleanup
   interfaces; native binding uses a freshly admitted SDK handle. Typed matrix
   creation stays setup, not a raw64-byte record. No load automatically executes
   source code or admits a native module.
4. Eight examples at default/relocated IDs preserve152 original artifact/component
   comparisons. Seven invalid packages reject before container mutation. A real
   assembler consumer, C++ header consumer, maintained compiler suite and sanitized
   load-plan allocation/lifecycle rejection checks pass.
5. The target fixture loads nine60,000-byte arrays through the8,192-byte generic
   window; stock container553,664bytes, native545,168bytes including its4,296-byte
   SDK-built image. It checks no execution on load, successive45/90 totals,
   status/generation, cleanup/reload and native fresh binding/runtime results/unload.
   Source roles are labelled; `review.golem` alone draws the nine-tier pyramid.
6. Pi/Oryx stock Fab and Oryx actual-Xtensa Flexe engine evidence is retained in
   the task silo. Native presentation requires a short settling interval after
   Canvas completion; this is a capture concern, not an added program delay.
   Test durations are functional emulator evidence, not physical performance.
7. Oryx human review is staged/stopped in `.emulator/golem-046`. Human acceptance
   and publication are pending. GOLEM-045's earlier stock matrix work and unrelated
   repository dirt remain intact and uncommitted.

Full integration/limits: [AGNB packages](../design/agnb-packages.md).
Detailed plan and remaining C++/review gates: [GOLEM-046](../tasks/GOLEM-046.md).


### Human acceptance

The Author reports "good test." on2026-10-23. Retrieved native-review output
matches all95 expected bytes, runtime/fixture pins verify, no failure output
exists and the emulator is stopped. GOLEM-046's producer/assembly review passes;
its reusable AgonDev C++ file reader and explicit publication approval remain
open. No further tranche, commit or push was initiated by this review report.


### Reusable C++ continuation

The Author released the next tranche. AGNB-002 supplies a freestanding0.3 reader,
stdio ownership adapter and AgonDev MOS transport. Golem's maintained
`runtime/agondev/agnb_host.cpp` implements its four generated-header hooks with a
single static8KiB workspace. Generic format/framing stays in agon-utils; names,
setup and native lifecycle remain consumer-owned. Compiler/SDK/firmware code and
the accepted assembly fixture are unchanged.

Twenty sanitizer host checks pass, plus the existing compiled-extension/plain/
image tests. The real C++ consumer qualifies on Pi/Oryx stock Fab and both Oryx
native engines, including missing/bad/truncated files, late-record preservation
and a successful load afterward. Executables are9,696/10,603bytes stock/native;
static RAM18,090/19,001bytes includes the workspace, with stack/library heap
additional. Containers still exceed application RAM. Final native inputs match
both qualified engine runs byte-for-byte. See GOLEM-046/cpp/summary.json.

The distinct root `.emulator/golem-046-cpp` profile is staged/stopped for Oryx
review; the screen identifies C++ loading versus Golem math/drawing. Human
review/publication is pending. No commit, push, graphical launch or flash was
performed for this release.


### C++ human acceptance

The Author reports "works." Retrieved C++ output matches all95 expected bytes
and its application log ends in PASS. Runtime/fixture pins match, failure output
is absent and the emulator is stopped. `GOLEM-046/cpp/human-review.json` records
the acceptance, also retained under generic AGNB-002. Implementation and both
human reviews pass; explicit publication approval remains open. No code changes,
new emulator runs, commits or pushes were performed for this acceptance.


### Publication release — 2026-10-23

The Author explicitly requested commit/push of all pending work in logical
groups after both human reviews passed. Generic AGNB C++ dependency is published
as6467b74 on agon-utils codex/agnb-plain-buffers (canonical d337783).
GOLEM-046 is complete and removed from the active register. This publication
includes the structured producer, packager, thin assembly/C interfaces, reusable
C++ adapter and qualified assembly/C++ consumers. Physical qualification is
separate; no graphical launch or hardware change is performed.
