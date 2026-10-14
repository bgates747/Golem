# Substantial VDPlet capacity — investigation and qualification plan

Increasing the conservative code and stack limits is already approved. This
plan determines justified text/data/image/stack budgets for substantial single
VDPlets, rather than sizing only for Rally. It is **proposed, not executed**:
no replacement numbers, runtime changes or new admission/capacity claims exist.
The first deliverable is a coherent policy/build proposal, followed by the real
Rally composition and controlled stress qualification. Physical measurements
require a separately authorized bench tranche. Return this plan for Author review.

The authoritative register is [TODO.md](../../../../TODO.md); stable investigation
items are GOLEM-002 CAP-001–009 in [the parent task](../../GOLEM-002.md).
This document specifies their execution order and evidence, without creating a
second task register. All thresholds and repetition counts below are proposed
test parameters, not implemented runtime policy or claimed safe hardware limits.

## Ownership and frozen starting point

| Surface | Owner and destination |
| --- | --- |
| Protocol, generators, cases, receipts and reports | Golem; this capacity silo, with generated build/profile output ignored |
| Generic admission, runtime services, SDK builder/header and loader checks | VDPlet runtime owner; `/Agon/mystuff/agon-vdplet`; no upstream checkout edits |
| Golem source-to-C/binding glue, layouts and completion publication | Golem; GOLEM-037 silo, no unagreed production promotion |
| Rally helper, assets, game source and independent replay/oracles | Rally; AgonArcade, consumed by pinned identity, never rewritten here |
| Wolf renderer/game and integration | Wolf owner; read-only inspiration unless an extracted fixture is agreed |
| Firmware/profile build and physical device | Explicitly identified owner per execution tranche; no bench or flash inferred |

Unchanged SDK baseline is commit `33a554c82377b10d428f0d2ec258d1b48f27ed22`;
[exact source/tool hashes](../../GOLEM-037/budget-baseline.json) and the
[historical fit receipt](../../GOLEM-037/evidence/fit.json) are retained. Current
text/data/image/declared-stack ceilings are 8,192/4,096/16,384/512 bytes. Minimal
helper and tiny composition stack paths are 544/576 before callbacks; tiny
composition text is 8,224 before real glue. These exact-build observations are
neither the real Rally workload nor physical ceilings. Do not rewrite old receipts.

## Budget model and policy coherence

Separate three concepts for each resource:

| Concept | Meaning |
| --- | --- |
| Runtime ceiling | Largest request this runtime policy permits; not a promise of current memory availability |
| Module declaration | This module's required allowance; actual sections and credible stack evidence must fit it |
| Recommended budget | A practical tested target with useful room left for normal VDP work; may be lower than the ceiling |

Record actual text/literals, individually aligned rodata/data/BSS/data.rel.ro,
relocation/ELF/upload storage, retained buffers/assets and framebuffer allocations.
Account for both steady residency and simultaneous old/new/temporary load state
where the selected lifecycle permits it. Total free RAM, largest contiguous
executable block and largest suitable internal data block are different limits.
Do not treat flash or PSRAM as interchangeable with executable RAM.

The current allocator requests executable `MALLOC_CAP_EXEC|MALLOC_CAP_32BIT` and
data `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`. Verify pinned target behavior, aligned
literal/data access and firmware allocation order before selecting sizes. INFO
9–16 already expose relevant free/largest-block figures; INFO17 exposes command-
task minimum free stack. Verify units and reset scope against the pinned build.
That minimum is accumulated, not automatically a per-call measurement.

The stack declaration **does not allocate a new stack or enforce a sandbox**.
Changing its ceiling alone cannot make a call safe. Determine the actual command
task stack, firmware/service live frames, native maximum call path and required
remaining reserve. Include drawing callbacks and relevant interrupt/task effects
without double-counting frames. State exactly what static analysis/high-water
measurements cover and leave unknown paths unqualified.

Trace all policy changes together: SDK section/image checks, envelope declarations,
loader actual-section admission, runtime transport/one-block handling, diagnostics,
capability/discovery behavior and documentation. Check existing u16/u32 lengths,
overflow, block concatenation and temporary copies before allowing larger images.
Choose whether multi-block loading is needed and specify it before implementation;
do not assume raising 16 KiB magically enables it. Retain ABI/format compatibility
where possible; older runtimes must reject unsupported declarations clearly.

## Ordered work and review gates

**Stage A — Inventory and build proposal (CAP-001/002/005/006).** Pin all source,
tool and firmware inputs. Inventory allocations, task stack and existing telemetry.
Build a resource-only composition using the real Rally-owned generated source,
unchanged renderer and an explicit marshaling/publication design. Any scaffolding
must state omitted semantics and cannot count as a runnable D2 implementation.
Use native object/ELF analysis to find requirements even when current packaging
rejects them. No capacity run or target launch is needed to obtain that inventory.
Return proposed numerical ceilings, declarations, recommended budgets, stack reserve,
service gaps and exact changed files before the execution tranche is selected.

**Stage B — Coherent SDK changes and real glue (CAP-002/006; GOLEM-037).** Implement
the selected policy in agon-vdplet and bounded binding glue in GOLEM-037. One SDK
dispatcher; typed field copies; argument release before helper; application-owned
completion buffer; distinct language/helper/service/runtime failures; no success
generation after failed helper/publication/release. Measure the completed combined
source/glue again. Private cleanup/unload must preserve application state/assets.
Keep stack and drawing-service changes explicit; no general scheduler or private
firmware C++ imports are included by implication.

**Stage C — Host and image admission controls (CAP-003/004).** Run focused normal
and ASan/UBSan loader/marshaling checks. Verify below/at/above each ceiling and each
module declaration, overflow-safe spans, relocation/alignment validation, unsupported
capabilities, malformed/truncated/multi-block images, allocation failure and partial
INIT cleanup. Cross-check builder versus loader on the same case matrix. Old valid
packages retain behavior; new oversized declarations fail on old runtime controls.

**Stage D — Actual Xtensa execution (CAP-003–007).** Run profiles sequentially,
first simple controls, then real composition, then the graduated matrix below.
Default and relocated layouts run under both JIT and interpreter with identical
module/fixture identities. Measure functional results, resource recovery and
commands/pixels using owner-supplied independent fixtures. Do not chase disposed
Rally pixel exceptions or repeat its game-specific host replay as Golem work.
Separate numerical qualification from host timing; no hardware capacity claim.

**Stage E — Human review and package checkpoint (CAP-009; GOLEM-037).** In the same
implementation tranche, stage a finite, recognizable source-labelled Rally scene
in a dedicated Oryx profile and leave it stopped. Expose Golem/assembly/helper roles.
Give the Author only the generated launch command; one onscreen MOS keyboard check.
After the Author closes/reports, retrieve results independently. Record human
acceptance separately from numerical evidence and explicit commit/push approval.
Deliver exact SDK/runtime/compiler/source/artifact pins to Rally before adoption.

**Stage F — Physical capacity qualification (CAP-007/009).** After explicit bench
release, identify device/firmware owner, preserve/recover its approved baseline and
repeat only preflight-safe cases on actual Agon. Measure executable/internal free
and contiguous memory, command-task stack, video/audio/input responsiveness and
load/steady/teardown behavior. Fresh boots separate cumulative stack minima;
preserved sessions exercise allocation history. Never flash or claim a device from
this plan alone. Physical results can tighten recommendations without rewriting
earlier emulator evidence or pretending all configurations share one maximum.

## Graduated case matrix

Synthetic code must execute distinct paths and use returned/resident values;
inspect final ELF so dead-code removal, merging or inlining cannot erase pressure.
Independent knobs keep image size, BSS, relocation count and stack distinguishable.

| Case family | Proposed cases and observations |
| --- | --- |
| Text/literals | 4, 8, 12, 16, 24, 32, 48, 64 KiB rounded actual text; verify code results and relocations, then aligned below/at/above selected ceilings |
| Initialized data and BSS | Independently 1, 4, 8, 16, 32, 64 KiB; verify sentinels, initialization/zeroing and recovery; distinguish file bytes from resident bytes |
| Image/relocations | Independently grow supported relocation tables/ELF overhead and valid spans; test selected image/block boundaries and invalid records without conflating these with text residency |
| Stack | Compiler-measured paths around 256, 512, 768, 1,024, 1,536, 2,048 bytes; execute only after total task/service reserve preflight, never by intentional overflow |
| Mixed residency | Pair independently qualified low/middle/high code and data points with retained buffers/assets, then representative framebuffer modes and service calls |
| Allocation history | Fresh profile, alternating retained buffer sizes, holes freed in different orders, assets loaded before versus after module; observe largest executable/internal blocks and peak load demand |
| Useful composition | Actual Rally source/glue/helper and default/relocated resources; source effects, commands/pixels, completion gate, lease peak and failure paths |
| Wolf-inspired service load | Owner-approved fixture or self-contained textured columns/sorted sprites/HUD scratch; drain before reuse, vary scene density and asset churn, preserve input/audio service |

These ladders are exploration coordinates, **not proposed admission ceilings**.
Do not execute every upper point automatically. Skip points that fail static,
contiguous-memory, transport or stack-reserve preflight; record why. Stop native
growth at the first unsafe condition. Near a safe allocation refusal, use smaller
increments only if useful; do not keep retrying until a run happens to pass.

Use 10 accepted load/RUN/FINI/unload cycles for each newly admitted exploratory
point, interleaving rejected-load cases and a known-small control. For candidate
recommended budgets, propose 100 cycles and a 10-minute representative mixed-service
run. Record all attempts and actual duration; extend only for a specific unresolved
failure or drift. Counts/durations may be revised at plan review, not reported as
executed evidence now.

## Expected failure and stop criteria

An oversized declaration/image, invalid relocation or insufficient suitable memory
must return a defined error without a handle, retained loader allocation, false
completion or broken normal VDP operation. Preserve original return/error identities;
do not conflate SDK, language, helper or drawing errors. Granted leases must close
or be reclaimed by the runtime's established entry cleanup; a refused release is
not proof of successful cleanup. Keep application-owned state/assets intact where
the selected operation promises it; partial source/output effects are not rollback.

Immediately stop a case family on reset/fault/watchdog, corruption, unexplained
resource loss, failed control/poll/input, unsafe stack reserve, stale completion,
unbalanced views or inability to drain/return to MOS. Capture the first failure
and baseline identities. A host timeout is not proof native execution has stopped;
do not unload/reuse its code or leased data until quiescence is established.
Treat clean expected admission refusal as a result, not a reason to raise limits
again automatically. Only continue independent safe cases with the failure scope
explicit. Transport/observer failures invalidate performance/capacity conclusions
until isolated; do not attribute them to large modules without evidence.

For service stress, verify finite-call completion, queued-drawing drain and actual
presentation separately. Keep stable controls for ordinary VDP commands, input and
audio. No per-copy checksum/readback is added to production rendering; independent
test oracles/sentinels belong to qualification only.

## Receipts and final decisions

Every case receipt records source/module/tool/SDK/firmware/profile hashes, actual
ELF sections and relocation count, proposed versus implemented versus declared
limits, configuration/engine/layout, retained allocation history, measured stack
scope/reserve, memory snapshots, install/load/steady/teardown wire bytes and elapsed
time, expected and observed result, attempts, cleanup and control outcome. Preserve
raw diagnostics, first failures and skipped points beside summaries. Failed runs
cannot leave an old master receipt masquerading as a new pass.

Publish a compact matrix of highest qualified points **per configuration**, first
clean refusals, unsafe/unmeasured regions, stack/service gaps and useful remaining
VDP resources. Recommend numerical ceilings/defaults from those observations with
stated reserve; do not advertise the largest successful single run as a universal
safe maximum. Report upstream compatibility and optional generic services separately
from application-specific code. Wolf migration, autonomous scheduling and upstream
acceptance remain separate decisions.

The immediate review deliverable is this protocol/sequence. Numerical budget
selection and implementation/execution follow the selected tranche; the direction
to increase code and stack is not reopened. No new tests/builds, runtime edits,
hardware actions or commits were performed to produce this document.
