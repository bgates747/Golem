# Stage A findings and proposed increased budgets

The real Rally route/contact/motion kernels, unchanged renderer and resource-only
typed-copy/publication scaffold use **25,620 code bytes, 988 private data bytes
and a 28,184-byte would-be image** in the larger relocated build. The resolved
module render/preflight path uses **800 stack bytes** before callbacks. Recommend
runtime ceilings of **64 KiB code, 16 KiB data, 128 KiB image and 2 KiB declared
module stack**, with smaller initial qualification budgets below. These are
**proposals only**; SDK policy is unchanged, neither image was admitted or run,
and no physical capacity claim exists. Stage A is complete and stops here for
review before Stage B implementation.

## Exact scope and measurements

Verified Rally's route manifest `d9e5238859d6faa402dd069f145834cd3eabb47f46db5b91e1d276ad359e8296`
(20 files) and helper manifest `3fc5d32f16d84f8ecc53aabe2d418e317a5323aee1ea2543c85fd657b1d6f62c`
(14 files). Copied the accepted compiler source closure, checked against published
`b32c596`, and rebuilt locally. All **21 generated artifacts per layout** match
Rally's retained master receipt exactly. Peer source/helpers were read-only;
Rally's 319,072-call game replay was not rerun. No conformance entries were trimmed:
all 15 existing Subs remain in this useful, conservative sizing fixture.

| Build | Code bytes | Private data bytes | Would-be image bytes | Relative relocations |
| --- | ---: | ---: | ---: | ---: |
| Actual Golem kernels, default | 17,440 | 268 | 18,736 | 7 |
| Actual Golem kernels, relocated | 17,504 | 268 | 18,800 | 7 |
| Kernels + renderer + scaffold, default | 25,556 | 988 | 28,120 | 44 |
| Kernels + renderer + scaffold, relocated | 25,620 | 988 | 28,184 | 44 |

Code/data follow the unchanged SDK's per-section rounding. Image sizes include
the hypothetical unchanged 64-byte envelope plus stripped ELF, **not a produced
VDPL package**. Existing SDK rejects all four builds under 8,192 text. Diagnostic
ELFs are ignored build products; no enlarged-admission checker was substituted.
Only one public dispatcher and supported relative relocations remain, with no
PLT/imports. Native structure copies/initialization need module-local memcpy and
memset, both included in these measurements. No libc is implicitly linked.

[resources.json](evidence/resources.json) contains source/tool/SDK identities,
section/frame/relocation details and exact generated maps. [inventory.json](evidence/inventory.json)
pins the existing firmware binary/map/source receipts and selected disassembly.
Firmware baseline SHA256 is `65d81aaf49b88dc39414cf458f3c8b465efa5a84f4ae59921a97431a3d748946`;
it was inspected, not rebuilt or launched. Repeated resource builds/inspection
produce identical final receipts.

## What the scaffold measures, and what it omits

[scaffold.c.in](scaffold.c.in) links exact generated C and exact helper C. It
measures an explicit 108-byte native snapshot and 48-byte completion local,
464-byte private context, 76-byte binding, typed source-global and three-column
six-opponent copies, bind/prepare/stop calls, and retained completion copying.
Default/relocated IDs and offsets come from actual generated metadata.

It is **not deployable D2**. The existing source dispatcher returns and publishes
success too early; the scaffold reacquires root afterward. It has no generated
ready-clear wrapper, final error/success suffix, complete field-domain policy,
coordinated resource reservations/installation, qualified draw gate or scheduler.
Tick120 is used as a measurement hook, not an adopted 30-Hz presentation policy.
The actual source lacks per-opponent headings and renderer phase/steering/flags;
these are explicit zero placeholders, not invented game behavior. No rendering
output or full snapshot coherence is claimed. Final compiler binding work may
change these sizes and must be remeasured.

The actual D2 design remains: validate root, copy/release argument view before
helper, execute source and marshal fields while retaining root, use scoped array
views, call helper, then publish completion and successful root generation only
after required successful releases. Root+asset+one output should peak at three
views; completion publication should use two. Preserve application-owned arrays,
assets and completion across private reload/cleanup. Rally must supply or agree
missing presentation-field maps/domains; Golem must not invent recovery or game
rules. Completion ready alone never authorizes drawing.

## Stack: actual task, module and firmware reserve

The pinned VDP creates `processLoop` with **4,096 bytes**, confirmed in its source
and Espressif FreeRTOS header. The Arduino idle loop's 8,192-byte default is a
different task. The VDP source's old comment mentioning free "words" is not a
measurement. The module runs on the VDP command task; changing its envelope stack
field allocates no new stack and provides no runtime sandbox.

Resolved module chain in both layouts:

`entry 288 → prepare 352 → preflight 128 → acquire 32 = 800 bytes`

Summing **all** 29 module static frames gives 2,016, a deliberately loose
nonrecursive module-only bound, not a simultaneously observed chain or inclusive
firmware bound. The supplied code has no recursive native calls; service callbacks
are outside that sum. The exact three chain edges/literal targets are retained.

Existing firmware disassembly shows native dispatch 128, runtime call 64,
acquire callback 64 and release callback 32 bytes, plus process/interpreter frames.
A source-derived path through **one resident wrapper** has 736 selected outer
frame bytes before module entry. That is an inventory scenario, not a complete
firmware/library/interrupt callgraph bound. Native acquire's deeper C++/library
work and arbitrary nested buffered calls cannot be dismissed.

Propose a 2,048-byte module ceiling/declaration and reserve the other **2,048**
of the existing 4,096-byte command stack for firmware/services. This accommodates
the current loose 2,016 module-only sum; the observed 800-byte chain leaves scope
for real glue. It requires **no task-stack increase in the initial proposal**.
Stage B must verify the final nonrecursive module paths and firmware/service
reserve, qualify only bounded native-wrapper nesting, and then confirm at least
**1,024 bytes observed remaining command-task stack** in selected target runs.
These reserves/acceptance thresholds are proposed, not established hardware facts.
INFO17 is cumulative minimum free stack; use fresh qualification profiles when
attributing it. If final paths do not fit, return that concrete result rather
than silently increasing the task or running an unsafe probe.

## Memory and load footprint

Executable allocation currently requests `MALLOC_CAP_EXEC|MALLOC_CAP_32BIT`;
private data requests `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`. The existing firmware
ELF has 1,027 vector + 93,795 text bytes in dedicated IRAM, and 19,880 initialized
+ 25,152 BSS bytes in its static DRAM sections. These are static-link footprints,
not available heap. Flash code/rodata are separate and not uploaded execution RAM.

The pinned SoC headers and **actual libheap memory-layout object** confirm
executable D/IRAM aliases as well as dedicated IRAM. Executable and internal data
requests can therefore compete for the **same physical memory**. Do not infer
the whole executable heap from the dedicated IRAM link remainder, add aliases
twice, or assume the largest suitable block equals total free memory. The binary
capability/region tables and hashes are retained in the inventory. Current free
and largest contiguous allocations remain unmeasured in Stage A.

The source uses a 292-byte root and 658 bytes of retained arrays. The existing
Fuji fixture additionally declares 149,518 asset bytes, 22,784 output bytes and
48 completion bytes: **173,300 bytes** of logical retained buffer payload overall.
These are fixture allocations, not a complete game's assets or measured heap;
bitmap images and control/allocation objects add more. Private module residency
is another 25,620 executable + 988 data bytes, in suitable internal memory.

LOAD retains the uploaded package while allocating separate executable/data
sections. It reads relocation metadata from that retained source; it does not
need another full ELF copy. For package size P, module text T and private data D,
logical peak at loading includes retained assets + P + T + D, plus objects/stack/
firmware. If stock chunk consolidation is needed, its earlier peak retains old
chunks totaling P while allocating the final P-sized block. Distinguish these
phases rather than assuming either no temporary copy or both peaks always coincide.

Stock upload writes have u16 chunk lengths; `bufferConsolidate` command14 already
produces a single uint32-sized block. LOAD carries a u32 package length, and the
runtime still requires exactly one source block. **Propose preserving that ABI**:
host writes chunks then consolidates before LOAD for large images. No new loader
scatter/gather or module slot is required. Stage B must verify chunk totals,
allocation failures, retained identity and near/above-65,535 boundaries, since
the 128-KiB ceiling cannot be sent in one ordinary stock write. Protocol payload/
RUN arguments remain at current 32 bytes; increasing code does not enlarge them.

## Numerical proposal: ceilings versus practical budgets

| Resource | Current ceiling | Proposed runtime ceiling | Initial qualification budget | Current scaffold declaration/requirement |
| --- | ---: | ---: | ---: | ---: |
| Text/literals | 8 KiB | **64 KiB** | **32 KiB** | 28 KiB declaration covers 25,620 actual |
| Private rodata/data/BSS | 4 KiB | **16 KiB** | **4 KiB** | 2 KiB declaration covers 988 actual |
| Whole image | 16 KiB | **128 KiB** | **64 KiB** | 28,184 actual total bytes |
| Module stack | 512 B | **2 KiB** | **2 KiB** | 2 KiB covers 2,016 loose module-frame sum |

The 32-KiB code budget gives the current scaffold about 7 KiB for complete glue/
field checks without squeezing source. The 64-KiB ceiling allows future larger
single applications where the runtime allocator can supply them; it **does not
assert that this firmware/mode can load 64 KiB**. The 16-KiB data ceiling provides
room for larger private scratch while normal worlds/assets remain in retained
buffers; the first workload needs under 1 KiB. The 128-KiB image ceiling allows
code/data/relocation/ELF overhead and deliberately exercises chunked upload, while
the present workload remains under 32 KiB. All higher points remain conditional
on allocation and service headroom, followed by actual qualification.

Image total is an existing exact package-length field, not a new module-declared
image allowance. Text/data declaration fields bound actual sections; the runtime
allocates actual section bytes, not the whole declared maximum. Stack declarations
must be derived from final paths; metadata alone is not proof of safety.
No claim of a universal maximum or Wolf feasibility follows from these numbers.

## Exact Stage B change and service-gap proposal

| File/surface | Proposed change and owner |
| --- | --- |
| agon-vdplet `tools/build_module.py` | SDK owner: coherent ceilings, explicit bounded text/data/stack declaration options and useful actual-versus-limit diagnostics; defaults/legacy compatibility documented |
| agon-vdplet `src/vdplet_image.h` | SDK owner: revised numeric admission with unchanged format/ABI, actual section checks and overflow-safe image spans; preserve rejection/cleanup |
| agon-vdplet tests/check_image.cpp and focused tests | SDK owner: below/at/above declarations/ceilings, malformed inputs, allocator refusal and coherent old/new-runtime controls |
| agon-vdplet `src/vdplet_runtime.h` | No numerical patch currently needed there; inventory/telemetry review and changed-header firmware rebuild still required. Keep one module, current buffer/view services, existing 32-byte arguments |
| agon-vdplet `tools/build_firmware.py`, usage/API docs | Record new source/artifact identity and policy; patch no upstream checkout or task-stack value by implication |
| Golem GOLEM-037 compiler/glue/packaging | Golem owner: real agreed native field maps/hooks, ready-clear offsets, status/publication ordering, static stack and resource receipts; host chunk/consolidate packaging where required |
| Rally contracts/fixtures | Rally owner: missing presentation fields/domain mapping; source/helper pinned and unchanged until an explicit peer-owned update |

Rally renderer writes command lists into retained buffers and uses existing
buffer services; it needs no new graphics service for this tranche. The fixture
lists must be installed/drawn/paced by the application and qualified separately.
Wolf-class direct bitmap/framebuffer access, queued drawing/drain, broader formats
and any audio services remain CAP-006 research. Do not expose private C++ classes
or add callbacks/background execution merely to meet the larger-capacity ambition.
The basic route/renderer contract does not require a new ABI version; larger
declarations will be cleanly rejected by an older runtime.

## Reproduce, limits and review boundary

From Golem root:

```bash
.venv/bin/python docs/tasks/GOLEM-002/capacity/stage-a/measure.py
.venv/bin/python docs/tasks/GOLEM-002/capacity/stage-a/inventory.py
```

These run compilers and inspect existing artifacts only. No emulator/profile/
firmware launch is part of either command. All build output stays ignored in
stage-a/build; pinned reports, generated source/layout and selected diagnostics
are siloed in evidence. Repeated hashes are recorded separately. Current SDK
worktree remains clean at `33a554c82377b10d428f0d2ec258d1b48f27ed22`.

Stop for Author review of the proposed numbers, stack reserve, coherent SDK/glue
scope and qualification sequence. Stage B would implement the selected changes
and remeasure real D2; it is **not released by these results**. No stress matrix,
Xtensa execution, target pixels, physical memory/high-water/timing measurement,
hardware action, Wolf port, upstream modification, commit or push occurred.
