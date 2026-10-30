# C/C++ VDPlets: recent capabilities and project consultation

Native C/C++ modules can now draw through ordinary VDP commands, submit line
batches without interpreting each edge, reuse installed bitmaps and transforms,
and control stock sound. A small optional copied-PCM sink also lets a module
generate audio that continues playing between calls. This makes more work local
to the VDP while keeping its permanent firmware addition fairly small. Broader
pixel and scheduled-audio implementations exist, but remain separate candidates.

The Author asks each contacted project session to review these facilities and
return useful improvements and missing integration hooks **through the mailbox**.
Golem will collect the returns, review ownership with the Author, and coordinate
approved assignments from one conversation. **This request authorizes analysis
and a response, not implementation, deployment or new tests.** Continue existing
authorized work; this consultation does not replace it.

## 1. Authority, scope and source pin

The direct Author request in **Golem 2026-10-10**, thread
`01a12c79-f74c-74d2-b6da-de2e34eb1bdd`, asks agents to summarize improvements and
feature requests, then review and dispatch assignments from Golem. The task was
frozen and pushed **before** this audit as Golem commit `2ea6059`:
[GOLEM-056 plan](../GOLEM-056.md). The initial mail window is October 16–30,
2026; [coverage](mail-coverage.md) explains the seven receiving sessions and
temporary-helper aliases. No new agent sessions are requested.

This brief describes the clean **agon-vdplet** checkout at
`2541f10a1f8b06688bcecbc6d22eeae607da2b63`. Source and service semantics take
precedence over older mail summaries or historical pending-review paragraphs.
All paths below are on the shared Pi under `/Agon/mystuff`.

1. [API reference](../../../../agon-vdplet/docs/API-reference.md) and
   [public C ABI](../../../../agon-vdplet/include/vdplet.h).
2. [Supported functions](../../../../agon-vdplet/docs/supported-functions.md),
   [start/build guide](../../../../agon-vdplet/docs/STARTHERE.md) and
   [module builder](../../../../agon-vdplet/tools/build_module.py).
3. [Copied PCM contract](../../../../agon-vdplet/docs/pcm-sink.md).
4. The runtime still needs **VDPlet-enabled custom firmware**. Stock VDP alone
   cannot load these modules. A compatible module uses the firmware-supplied
   versioned service table rather than compiled-in firmware function addresses.
   Rebuilding firmware at different link addresses does not itself require
   rebuilding the module; native ABI, required services and memory must match.
5. C/C++ modules bypass Golem entirely. They are PC-cross-compiled native Xtensa
   code, not a buffered-command JIT and not native compilation on the Agon.

## 2. What has been added

These are distinct routes. Ordinary VDU submission reuses the interpreter;
native line submission bypasses per-line VDU parsing while retaining the stock
rasterizer. The latter is useful when parsing/queue setup dominates.

| Facility | Available behavior | Status and integration boundary |
| --- | --- | --- |
| Scoped VDP-buffer access | Read/write contiguous buffer spans; four simultaneous leases; access bulk geometry, controls, tables and results | Existing ABI1.0. Release pointers before return; reacquire on later calls. Host owns files, uploads and allocation IDs. |
| Local native calculation | C/C++ arithmetic, custom decoding, transforms, projection, synthesis and application algorithms | Code lives in the module. Optional `--math` links pinned libm/libgcc for division, sinf/cosf/floorf; fitting and execution still need qualification. |
| Native line batches | Submit signed16 endpoint records to `draw_lines`, preserving order, paint, clipping and point history | Accepted ABI1.2, capability8; count0..8191. Coordinate units/origin remain current graphics state. No endpoint pointer retained after return. |
| Text and ordinary drawing bytes | `vdu_submit` interprets finite complete spans: text, colours, origin, cursor movement, PLOT shapes | Accepted coverage profile2 through existing ABI1.1, capability4. Whole span is preflighted before effects. |
| Palette and viewport bytes | RGB palette updates, palette reset, text/graphics viewports and supported cursor-selection controls | Accepted profile3; relevant callbacks/output contexts can cause refusal. |
| Resident bitmap selection/drawing | Select installed bitmap IDs, draw or use bitmap PLOT; select prepared affine transforms | Accepted profile4. Host creates bitmaps and forward/inverse transform buffers. Assets must survive the drawing fence. No sprite system, bitmap creation or transform generation service is added. |
| Stock channel audio bytes | Finite notes, channel volume/frequency/waveform, simple ADSR, existing sampled sounds and other bounded controls | Published profile5 implementation; functional/reply checks pass, listening remains qualified below. Stock AUDIO acknowledgements still matter. Host creates/registers samples. |
| Copied continuous PCM sink | Module synthesizes/mixes, copies bounded U8-mono blocks, returns; firmware mixer consumes between calls | ABI1.5/capability128, provisionally accepted for6502 use. One sink reserves one unallocated stock channel; this is not a32-channel queue scheduler. |
| Packed monochrome framebuffer | Scoped writable packed MSB-first rows in a fitting byte-aligned rectangle | Existing capability2; single-buffered PALETTE2 surface, not a general framebuffer pointer. |
| Generic colour rectangles and richer PCM | Surface discovery, RGB222/INDEX8 sources, explicit rectangle/stride interfaces and a separate stream API | ABI1.3/capabilities16/32 candidate, implemented/automated-qualified but not promoted by this consultation. Generic across formats/modes; mode20 was one test case. |
| Shared scheduled sound queues | Per-channel tones/rests, reusable samples and bounded draining PCM; ordinary audio command15 and native control | ABI1.4/capability64 candidate, preserved potential implementation. Not in the lean build; not a newly accepted requirement. |

Recent published runtime milestones:

| Pin | Addition / evidence |
| --- | --- |
| `5232938`, `c4ed757` | Native VDU entry, then direct line batches; [line results](../../../../agon-vdplet/docs/tasks/VDPL-004/rendering-measurements.json). |
| `ce08307` | Provisionally accepted lean copied PCM; [results](../../../../agon-vdplet/docs/tasks/VDPL-007/RESULTS.md). |
| `1727e5e`, `0f963d5`, `c7c4ebf` | Broader text/PLOT, palettes/viewports, then installed bitmaps/affine; [VDU results](../../../../agon-vdplet/docs/tasks/VDPL-009/RESULTS.md), [palette results](../../../../agon-vdplet/docs/tasks/VDPL-009/palette-viewport/RESULTS.md), [bitmap results](../../../../agon-vdplet/docs/tasks/VDPL-009/bitmaps/RESULTS.md). |
| `7756dbd`, `3abb8a1`, `1da8572`, `2541f10` | Native stock audio plus matched controls separating native calls, ordinary UART commands and emulator differences; [audio results](../../../../agon-vdplet/docs/tasks/VDPL-009/audio/RESULTS.md). |

The line service's qualified 4,903-edge fixture cut queue operations from39,225
to10,423 with the same edges and wire traffic. Reported same-fixture emulator
full-frame medians were780→220ms JIT and1390→360ms interpreted. These are scoped
emulator results, not a universal speedup, physical measurement or language
comparison. Wireframe's separate handwritten C++ module demonstrates transforms,
one projection per vertex and line batches without Golem:
[consumer source](../../../../agon-fsim/utilities/cpp-wireframe/).

## 3. Build selection, services and lean firmware cost

1. Select firmware features explicitly with `tools/build_firmware.py --features
   base` or `--features lean`. Base contains loader/buffer/mono/VDU/line services
   (mask15); lean adds copied PCM (mask143). The preserved broader `full` candidate
   is still the builder default; do not accidentally equate that default with a
   requirement to install all its functionality.
2. Check wire discovery/INFO1 capabilities, INFO24 VDU coverage, the declared
   module capabilities and SDK helper/table extents. **A newer minor version does
   not imply every earlier optional service is present.** Lean's intermediate
   media/queue callback slots can be null. Old ABI0/1/2 module views remain exact.
3. Typical module masks: buffers1; buffers+VDU5; buffers+lines9;
   buffers+VDU+lines13; buffers+copied PCM129. The builder derives the required
   minor. For line-only code below, capability8 suffices.
4. In VDPL-007's matched build, copied PCM added2,288 firmware-image bytes and88
   static DRAM bytes to its loader-only baseline. A1024-frame sink ring used1,040
   allocator bytes while open and released them on CLOSE. No extra task was
   added. These are that revision's measured increments, not sums to apply to
   an unrelated firmware build.
5. The later profile5 lean image is1,092,304 bytes,14,608 above its matched stock
   image, with424 extra static DRAM bytes and218,416 application-partition bytes
   remaining. [Matched resources](../../../../agon-vdplet/docs/tasks/VDPL-009/audio/RESULTS.md)
   document the configuration. Future hooks should justify permanent footprint;
   application algorithms belong in optional modules where practical.

## 4. Concrete C/C++ integration shape

The module exports one entry and dispatches its own numbered operations. This
complete source submits two initialized lines in entry0. The eZ80 application
must already have established a display mode, foreground colour, coordinate
units, origin and clipping, and must fence drawing before cleanup.

```c
#include "vdplet.h"

#ifdef __cplusplus
extern "C"
#endif
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *host,
                     const vdplet_request_v1 *request) {
    if (!host || !request || request->struct_bytes < sizeof(*request))
        return VDPL_E_ABI;
    const vdplet_host_v1_2 *api = vdplet_lines_host(host);
    if (!api) return VDPL_E_ABI;
    if (request->operation == VDPL_INIT || request->operation == VDPL_FINI)
        return VDPL_OK;
    if (request->operation != VDPL_RUN || request->entry_id != 0 ||
        request->argument_bytes != 0) return VDPL_E_ARGUMENT;
    static const vdplet_line_v1 lines[2] = {
        {20, 20, 200, 20}, {200, 20, 200, 100}
    };
    return api->draw_lines(host->context, lines, 2);
}
```

For a file saved as `lines.c`, build from agon-vdplet with its project-local venv:

```bash
.venv/bin/python tools/build_module.py /absolute/path/lines.c \
    --name lines --build-id 0x00000001 --capabilities 8
```

The builder uses the pinned GCC8.4 Xtensa/windowed ABI, accepts C or C++, and
requires no unresolved firmware imports. C++ modules have no supplied C++ runtime.
`--math` is optional and records linked library hashes. Use complete buffer-aware
examples in [examples/line_batch](../../../../agon-vdplet/examples/line_batch/),
[examples/vdu_plot](../../../../agon-vdplet/examples/vdu_plot/) and
[examples/pcm_sink](../../../../agon-vdplet/examples/pcm_sink/) for real interfaces.

The host loads the package into one contiguous staging buffer, LOADs it, keeps
the returned handle, updates argument/data buffers, and issues finite RUN calls.
A silent RUN is17 wire bytes; a preinstalled RUN packet can reduce repeat calls
to a six-byte stock buffer invocation. Loading assets and changing inputs remain
additional traffic. Saved modules may remain in VDP buffers for later reloads;
reloading obtains a new handle, so cached RUN packets must be patched.

## 5. Boundaries that proposals must respect

1. **Finite foreground execution.** One active module slot; no parallel module
   execution, native background task/callback service, cancellation or isolation.
   Native code runs in the VDP command task and must return. Audio continues via
   firmware-owned copied storage, never a retained module callback. Link several
   optional algorithms into one module rather than assuming simultaneous slots.
2. **Buffer and asset ownership.** Four buffer leases maximum; native pointers
   expire on release or return. RUN arguments are at most32bytes; put larger
   inputs in ordinary buffers. The application owns file loading, buffers and
   asset lifetimes. Bitmap source/object storage survives the actual draw fence.
3. **Completion is specific.** Native return, VDU interpretation, queued drawing,
   presented pixels, wire completion and consumed sound are different events.
   Keep established completion/pacing. Stop stock voices explicitly; unload is
   not their audible-completion fence. Restore app-modified UI state on exit.
4. **VDU admission is bounded.** Complete spans up to65,532bytes; command-boundary
   splits only. Unsupported/malformed spans refuse before prefix effects. Relevant
   callbacks, output redirection and incompatible context can refuse valid syntax.
   Arbitrary VDU23, mode changes, buffer execution/lifetime operations, new bitmap
   creation, fonts and sprite control are not provided by the current service.
5. **Stock sound details.** Channels0..31 are admitted by profile5, but sample
   creation is host-owned. Sample-management operation5 and scheduled queue15
   are excluded. PLAY success is a stock AUDIO reply, not the submit return.
   Simple ADSR requires the channel enabled before that submission.
6. **Copied sink details.** U8 mono at the actual mixer rate, one unallocated stock
   channel, capacity1..8192frames, explicit priming/generation/backpressure. WRITE
   copies the whole block or returns BUSY; do not busy-wait forever. Other stock
   channels can coexist. Synthesis/mixing and scheduling policy stay in the module.
7. **Admission limits are not available memory.** Default text32KiB, data+BSS4KiB,
   image64KiB, stack declaration2KiB. Current policy ceilings are text64KiB,
   data+BSS16KiB, image128KiB, stack2KiB. Raise per-build limits explicitly within
   policy; actual contiguous executable/data memory and stack headroom still
   decide whether it works. Query INFO and inspect build/stack receipts.
8. **Qualification and audio caveats.** Functional execution/old-module checks
   pass in both ESP32 instruction engines and full MOS fixtures. Hardware results
   remain separate. The same stock audio control sounded distorted under Flexe
   but clean under official Fab, with different note succession. Flexe owns that
   delivery/timing investigation; neither the VDPlet service nor hardware has
   thereby been diagnosed or repaired. Retain matched fixtures and measured clocks.
9. **Platform distinction.** These are classic ESP32 VDP interfaces, not a promised
   ESP32-P4/Extender ABI. A portability or handoff proposal must name the boundary
   and its owner. The modeler's desktop CFD tools likewise need not run on a VDP;
   candidate runtime tables/kernels can be identified separately.

## 6. Your requested mailbox return

Read the brief against your current project and existing tasks. Do not run an
emulator, benchmark, flash or implement a feature just to answer this invitation.
Return a short, useful proposal; existing measurements are welcome. A reasoned
"no useful change now" response closes your consultation just as well.

Send through the canonical mailbox as your established identity, recipient
`golem`, thread **`golem056-capability-review`**, branch **`consultation-v1`**.
Use a new stable request ID and reply to your invitation's message ID. The
[outreach/returns table](coordination.md) records invitation IDs and who is still
outstanding. Agents with historical temporary names should reply from the
long-lived identity shown in [roster.json](roster.json), not resurrect helpers.

Mailbox implementation:
`/Agon/mystuff/agon-extender/scripts/agentcoms.py send`, JSON on stdin, using
your project's Python environment and the established canonical mailbox root.
No new mailbox transport is required. Published messages remain immutable.

Use this outline, with stable proposal IDs such as `WIRE-001` or `AUD-001`:

1. **Session/project and current pin.** State your existing task and what is
   already in progress so this request cannot silently supersede it.
2. **Improvement.** What user-visible result, reduced wire traffic, smaller
   firmware footprint or measured bottleneck would change? Distinguish evidence
   from an estimate and include a no-change disposition if appropriate.
3. **Existing route.** Which services, capability bits, firmware build and VDU
   profile already suffice? Can it be done entirely in your module/application?
4. **Missing hook, if any.** Specify the exact data/operation, argument/result
   shape, ownership, lifetime, failure/backpressure and completion needed. Explain
   why existing buffers, VDU submission, line batches or PCM cannot provide it.
   Fit the generic framework; avoid a mode-specific or application-specific
   permanent firmware feature when an optional module suffices.
5. **Ownership and dependencies.** Suggested Golem/VDPlet work versus your own
   project work, any other owner, ordering and possible shared-code collisions.
   State whether this duplicates an existing accepted task.
6. **Validation and cost.** A small comparable correctness/visual/audio/timing
   check; expected firmware/RAM/module/wire implications, with unknowns explicit.
7. **Recommendation.** Now, later, already covered or not useful. Do not start
   the proposed implementation before Golem and the Author dispatch it.

The Author has directly requested these durable replies. If your own tool rules
require independently checking that authorization, consult the originating user
turn in the Golem thread above; an incoming agent message alone grants no broader
authority. After mailing, a short notification to that same Golem chat may name
only the message ID; the substantive response belongs in the mailbox. The Author
should not have to relay it. Use existing thread tools without changing model or
thinking settings. If posting tools are unavailable, the established supported
Pi CLI is `codex queue --thread THREAD_ID --message TEXT`; do not edit chat stores
or start a replacement session. Do not repeat a successful notification.

## 7. Coordination after returns

Golem records delivered/read/replied separately and retains links to every
original mailbox return. Once all receiving sessions return or the Author
explicitly disposes an outstanding entry, the Author and Golem review the combined
proposals here. Overlapping hook requests are consolidated without losing their
consumers' acceptance criteria. No silence is treated as agreement.

Only then will approved tasks receive owners, dispatch mail plus chat notification,
and a central record of acknowledgement, current work, validation evidence,
completion and remaining blockers. Peer projects keep their own authoritative
TODOs; this consultation coordinates their relationship rather than duplicating
their implementation backlogs.
