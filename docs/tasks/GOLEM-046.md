# GOLEM-046 — Streamed AGNB program packages

Use generic AGNB plain-buffer loading so generated Golem programs need not be
embedded wholesale in eZ80 application RAM. AGNB owns format/reader/writer and
bounded transfer; Golem owns compiled layout, named APIs and bootstrap behavior.

Author approved this separation and assigned generic AGNB ownership on2026-10-23.
Generic dependency: agon-utils/examples/agnb/docs/tasks/AGNB-001.md.

- SEP-001 [x] Record cross-project ownership, version and dependency boundary; preserve ongoing GOLEM-045 and unrelated agon-utils work.
- PKG-001 [x] Emit explicit compiler-assigned buffer records from structured package data, using the generic AGNB writer. Keep symbol/header/manifest IDs derived from the same layout, with no application-source numeric bookkeeping.
- BOOT-001 [x] Separate uploadable ordinary buffers from post-load setup commands such as typed matrix creation, native module admission/fresh-handle binding and activation. Never infer a general installer by stripping arbitrary VDU bytes. Retain existing embedded output as an option.
- HOST-001 [x] Supply generated assembly/C integration and a small reusable host-side loading adapter around the generic loader. Host owns file I/O, successful completion and entry activation; stock VDP never asks MOS to load files.
- CHK-001 [x] Compare package contents/layout/setup behavior to accepted embedded output; prove bounded host memory and loading of aggregate packages larger than application RAM, using actual stock and applicable native target checks.
- REV-001 [x] Both Oryx human reviews accepted and retrieved. The Author explicitly authorized commit/push on2026-10-23; publish the owned producer/bootstrap/C++ integration and retained evidence.

Ordinary command/data records contain opaque bytes, not implicit executable
entries. Buffer/block boundaries are compiler contracts; file read windows do
not define them. Matrices may require API creation rather than raw64-byte
uploads. Native module image loading still obtains a fresh runtime handle;
container IDs cannot encode that dynamic handle. Loading does not change
runtime wire costs of the resident algorithm. Template reuse is separate work.


Generic first tranche is implemented in agon-utils: provisional0.3 plain
BHDR/RAWD/DATA records, shared RIFF framing, explicit-ID writer/reader/manifest
CLI, additive agnb_load_buffers/callback runtime entry points. Host tools and
formal task published as3edf8bc on codex/agnb-plain-buffers; canonical local
record is e6f1bcc. Divergent unrelated agon-utils history was preserved by
publishing a cherry-pick on a clean upstream worktree. Actual stock Pi loading
of an aggregate package above0x70000 passes; runtime/profile publication is
held for review. Golem producer/bootstrap integration was still pending at this dependency checkpoint; the following tranche implements it.


### Generic dependency accepted — 2026-10-23

The Author reports AGNB's Oryx stock-Fab review worked. Agent retrieval matches
all66 expected result bytes and pinned runtime identities. Generic AGNB-001 is
complete and published on agon-utils codex/agnb-plain-buffers: host tools3edf8bc,
qualified loader35dd8ef (canonical local records e6f1bcc/24596fc). The existing
image/audio behavior is preserved. Generic dependency entry predicate is now
satisfied; PKG-001/BOOT-001/HOST-001 remain Golem-owned implementation work.
No current Golem package silently acquires AGNB output from this dependency.


### Golem producer/bootstrap tranche — 2026-10-23

The compiler exports structured buffer records with `--emit-load-plan`; the
auxiliary packager calls the generic AGNB writer. Existing embedded artifacts
remain identical. New `agnb.inc` supplies thin assembly load/setup/cleanup and
fresh-native-handle binding; `agnb.h` supplies named C/C++ fields/calls and
transport hooks. See [maintained contract](../design/agnb-packages.md).

HOST-001 is partly implemented: the assembly adapter is working and exercised;
the generated C++ header compiles, but an actual reusable AgonDev C++ file reader
has not been supplied. Do not claim that the header contains that reader.
This remains the next host-integration tranche after review.

CHK-001 covers eight examples/profiles at default and relocated IDs, 152
embedded-artifact/component comparisons, seven malformed-package rejections,
real assembler/C++ header consumers, and maintained allocator/lifecycle checks.
Large target source and the separately labelled visual source live in this silo.
Actual stock/native evidence and human acceptance are recorded below as available;
The target checkpoint below completes CHK-001; REV-001 remains open for human acceptance/publication.
No new changes in this tranche have been committed or pushed.


### Target checkpoint and review — 2026-10-23

Both stock hosts and both actual-Xtensa engines pass. Every run loads/runs/cleans
and reloads/runs/cleans, with exact expected status/value/generation observations.
Native engines also confirm fresh admission/binding, actual runtime results,
unload, the real MOS prompt, zero execution faults and settled white/black drawing
samples. The fixture waits for the emulator child to finish shutdown before
claiming the profile is stopped; shell-wrapper exit alone is insufficient.

| Host | Backend / engine | Application bytes | Result bytes | Run seconds |
| --- | --- | ---: | ---: | ---: |
| oryx-linuxlite | buffered / native-stock-vdp | 13,223 | 83 | 10.845 |
| oryx-linuxlite | native / interp | 13,353 | 95 | 16.152 |
| oryx-linuxlite | native / jit | 13,353 | 95 | 16.631 |
| raspberrypi | buffered / native-stock-vdp | 13,223 | 83 | 10.802 |

Stock container553,664bytes; native545,168bytes including the4,296-byte module;
transfer window8,192bytes. These durations include boot/loading/test execution
and cleanup, with native presentation/shutdown settling. They are functional
emulator evidence, not a physical loading-throughput benchmark.

Oryx review is staged/stopped at `~/Agon/mystuff/golem/.emulator/golem-046`, using
its generated `./flexe-agon-emulator` wrapper. Human review/publication remains
open. The Author launches, sees the separately labelled white pyramid, follows
the single onscreen MOS instruction, closes/reports; agent retrieval uses
`GOLEM-046/verify_review.py`. Reruns clear old result/failure files automatically.
No separate terminal or human-run verifier is required. See the silo README.


### Human review accepted — 2026-10-23

The Author reports "good test." Agent retrieval independently matches all95
expected bytes, verifies the pinned runtime/fixture inputs and absence of failure
output, and confirms the emulator is stopped. `human-review.json` retains the
receipt; the retrieved data is under ignored `build/human-results.dat`.
REV-001's human-review portion passes. Publication approval has not been given
in this message, and HOST-001's reusable AgonDev C++ file reader remains open.
No implementation changes or new test runs were needed for this acceptance.


### Released C++ host tranche — 2026-10-23

The Author says "proceed" after the accepted assembly review. Complete HOST-001
using the generic reader/adapters owned by agon-utils AGNB-002 and a small
Golem-specific AgonDev adapter. Keep the accepted assembly fixture/profile intact;
new application source, scripts and evidence stay under GOLEM-046/cpp/ and a
separate project-root emulator profile. Exercise the same large source/data and
named APIs on stock VDP and native engines; stage a stopped Oryx visual review
within this tranche. No commit/push authority is inferred from this release.


### C++ adapter implementation — 2026-10-23

HOST-001 is implemented by `runtime/agondev/agnb_host.cpp` and agon-utils'
AGNB-002 `agnb_buffers.hpp`, `agnb_stdio.hpp`, `agnb_agondev.hpp`. The generic
reader owns framing/bounded transfer/file closure; the Golem shim supplies the
four named transport hooks. Source entries, setup, native admission/binding,
completion and cleanup stay explicit in the application.

The new ordinary AgonDev application under `GOLEM-046/cpp` uses the same
large.golem/review.golem and generated headers, with its second drawing package
in a separate translation unit. It loads no whole package into eZ80 RAM. The
accepted assembly fixture and saved human evidence are preserved. A separate
`.emulator/golem-046-cpp` profile qualifies missing, bad-metadata, truncated and
late-error containers, retention of a complete earlier record and successful
loading after errors, as well as the existing numeric/lifecycle/drawing checks.
C++ human review/publication remains separate from the accepted assembly review.


### C++ qualification checkpoint — 2026-10-23

Stock and native C++ consumers pass with the generic reader and unchanged
Golem source, module, SDK and firmware. The native binary and every staged
container are byte-identical to the qualified JIT/interpreter inputs after the
final rebuild. The isolated C++ profile is staged/stopped on Oryx.

| Host | Backend / engine | Image bytes | Result bytes | Run seconds |
| --- | --- | ---: | ---: | ---: |
| oryx-linuxlite | buffered / native-stock-vdp | 9,696 | 83 | 11.813 |
| oryx-linuxlite | native / interp | 10,603 | 95 | 16.120 |
| oryx-linuxlite | native / jit | 10,603 | 95 | 16.909 |
| raspberrypi | buffered / native-stock-vdp | 9,696 | 83 | 11.860 |

Stock executable9,696bytes / static RAM18,090bytes; native executable10,603bytes
/ static RAM19,001bytes. Static RAM includes BSS and the8,192-byte workspace;
stack and any library heap are additional. The actual SDK FILE state is small
and binary fread delegates to MOS block reads; no whole-file allocation is made.

Missing, metadata, truncation, late-error preservation and subsequent-valid-load
paths pass in the application before its successful result is saved. Generic
sanitizer checks cover callbacks, repeated IDs, exact transfer bytes and injected
I/O failures. The Author reviews the C++ scene through its separate generated
`./flexe-agon-emulator` launcher, then the agent retrieves result/log evidence.
Human acceptance/publication remains pending for this new C++ tranche.


### C++ human review accepted — 2026-10-23

The Author reports "works." Retrieved C++ results match all95 expected bytes;
the application log ends in PASS. Runtime/fixture pins verify, no failure file
exists and the dedicated profile is stopped. `cpp/human-review.json` records
the receipt; retrieved result/log files stay under ignored `cpp/build/`.
Both assembly and C++ human reviews now pass. REV-001 remains open only for
publication authority and scoped publication; this report does not authorize
commit/push. No implementation changes or new emulator runs were needed.


### Publication release — 2026-10-23

The Author explicitly requested commit/push of all pending work in logical
groups after both human reviews passed. Generic AGNB C++ dependency is published
as6467b74 on agon-utils codex/agnb-plain-buffers (canonical d337783).
GOLEM-046 is complete and removed from the active register. This publication
includes the structured producer, packager, thin assembly/C interfaces, reusable
C++ adapter and qualified assembly/C++ consumers. Physical qualification is
separate; no graphical launch or hardware change is performed.
