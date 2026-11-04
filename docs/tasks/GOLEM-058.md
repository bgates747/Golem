# GOLEM-058 — Capacity, VDU coverage and standalone distribution

Make VDPlets easier to size and use, and bring the standalone Golem package
up to date. This task has three deliverables: measured capacity/lifecycle
guidance, useful additional ordinary VDU operations from native modules, and
a reproducible on-Agon compiler distribution with complete user instructions.
Consumer support continues alongside this independent work.

## Authority and ownership

The Author selected all three items on 2026-11-03, stopped the older goal, and
required this plan to be committed and pushed before implementation. This is
the new execution goal; unfinished GOLEM-057 consumer work remains supported
under its existing contracts, not silently declared complete or reimplemented.

Golem owns this umbrella plan, compiler and distribution. Generic runtime,
SDK, VDU changes and capacity tooling belong in `../agon-vdplet`, using its
authoritative TODO and detailed task silos. Extend the existing VDPL-009 work
for VDU submission rather than creating a competing API. Reconcile the older
GOLEM-002 capacity proposal with current code and physical evidence before
reusing its numbers. Each repository retains exactly one authoritative TODO.

New multi-file artifacts stay under this task's subdirectory or the appropriate
SDK task silo. Build outputs use ignored project SSD storage, not large /tmp
archives. Promote reusable tooling into maintained locations when its role is
established. Preserve existing uncommitted work and frozen consumer inputs.

## 1. Practical capacity and lifecycle qualification

Answer what can actually be loaded, called and replaced on the classic ESP32
VDP under representative display/audio allocations. Separate package-policy
limits, largest executable allocation, total free memory and observed stack
floor; none is a substitute for another. Establish useful operating guidance,
not a promise that every module below one advertised byte count always fits.

- CAP-001 [x] Inventory current admission limits, executable/data allocation,
  compiler policy, image/ELF sections, resource queries and prior physical
  evidence. Reconcile outdated capacity-plan statements without rewriting old
  results. Publish the selected baseline and exact questions for the tests.
- CAP-002 [x] Prepare a small generated family of valid executable modules
  with meaningful code/literal/data/BSS use, representative relocation and
  finite self-checking calls. Exercise increasing sizes and both sides of
  relevant admission/allocation boundaries without executing malformed code.
  Keep baseline code and module identities explicit.
- CAP-003 [x] Qualify load/call/unload/reload and replacement over bounded
  repeated cycles. Include expected refusal preserving established state,
  stale handles, partial initialization cleanup and subsequent successful use.
  Sample resources at stable lifecycle points; add no per-call checksum or
  readback machinery to production hot paths.
- CAP-004 [ ] Run a bounded matrix of representative supported mono/colour
  and double-buffered display configurations, with inactive and active audio
  allocations where supported. Use actual firmware mode capabilities. Confirm
  useful limits on the physical bench; keep emulator and physical observations
  separate. Retain failures and recovery/restoration evidence.
- CAP-005 [ ] Publish a concise capacity guide, linked firmware flash/static
  RAM costs for any runtime change, observed dynamic resources and reproducible
  commands. Explain fragmentation/contiguity and stack limits only as far as
  measured. Do not increase production limits merely to pass a fixture.

## 2. Broader ordinary VDU submission

Extend the existing synchronous `vdu_submit` entry, preserving its complete-span
preflight, normal interpreter behavior, old ABI views and explicit coverage
discovery. The first useful extension is font/glyph and related text support.
Then qualify safe resource/execution-lifetime operations; classify operations
that cannot safely run inside an executing native module instead of admitting
them by accident. This does not authorize a background interpreter thread.

- VDU-001 [x] Reconcile existing coverage with the pinned interpreter source.
  Inventory remaining font/text, resource mutation, execution, waits, transport
  and callback families; record exact packet lengths, ownership and hazards.
- VDU-002 [x] Freeze an implementation-sized font/text contract with concrete
  wire examples, discovery, inline-payload/retained-storage rules, callback and
  truncation behavior. Implement useful supported font/glyph operations through
  the existing entry, without per-operation public dispatch-table functions.
- VDU-003 [x] Implement and qualify the useful bounded resource/lifetime subset
  justified by the inventory. Preserve the current module, call arguments,
  borrowed views and transport; explicitly refuse unsafe re-entry, unload,
  indefinite waits and storage invalidation. Record deliberate exclusions and
  their technical reasons rather than claiming all VDU operations work.
- VDU-004 [x] Verify full-span rejection before effects, exact operand framing,
  direct-UART/native parity, normal reply ordering, legacy modules and finite
  return/cleanup in both instruction engines and a real MOS client. Run focused
  sanitizer checks and report matched firmware/static-memory costs.
- VDU-005 [x] Stage a readable, stopped Oryx demonstration showing custom text
  and the added useful operations. Identify the native source and host role;
  restore mode/cursor on exit. Obtain human review and explicit publication
  approval for coupled emulator/runtime changes before committing them.

## 3. Refresh the on-Agon Golem distribution

The deliverable is a ZIP someone can use independently with stock VDP and MOS,
with no LLM or PC required to compile its supported buffered programs. Keep
VDPlet C emission and the separate PC Xtensa toolchain clearly distinguished.
Refresh the existing GOLEM-051 builder and package rather than starting another
compiler port. Every implemented language/profile feature must be accounted
for; proposed syntax must not be presented as implemented.

- PKG-001 [x] Compare maintained compiler syntax/options and subsequent accepted
  changes against native source adaptation, runner, examples, reference and
  package contents. Record concrete gaps; do not assume old parity evidence
  covers a newer compiler or ZIP.
- PKG-002 [x] Fix distribution/native-compiler gaps found by that comparison,
  preserving all implemented profiles and output semantics. Include useful
  initialized examples with visible output, including integer/float printing
  and bitwise behavior where implemented. No string-concatenation extension.
- PKG-003 [x] Make the how-to independently understandable: installation,
  compiling, `--resident`, target/profile choices, invocation, retained state,
  expected output, errors and return to MOS. Include the language reference,
  source/build recipe and examples in the ZIP. Exclude internal evidence and
  consumer-irrelevant qualification bookkeeping.
- PKG-004 [x] Rebuild from the shipped source in isolation, verify archive
  contents and maintained/native compiler parity, and run shipped examples on
  stock firmware under MOS. Keep tests proportional to actual changes and
  carry forward existing meaningful parity fixtures.
- PKG-005 [x] Stage the updated standalone walkthrough stopped on Oryx, with
  simple launch commands and visible results. Complete human review and any
  required publication gate, then record the final ZIP identity and supported
  limits in maintained user documentation.

## Sequence and completion

- PLN-001 [x] Commit and push this plan and its task-register entry before
  implementation. Confirm the remote commit identity.
- INT-001 [x] Complete the initial CAP-001/VDU-001/PKG-001 audit, then freeze
  concrete finite subtranches before code/device work. Start capacity tooling
  first; use unavailable machine time for source/distribution work.
- INT-002 [ ] Keep accepted interfaces and current results in maintained docs,
  dispose completed tasks into the dated development log, and leave each
  authoritative TODO with only genuinely unfinished work. Close the goal only
  when all three deliverables and their required gates are proved.

This plan does not manufacture tests for already-settled behavior, require
consumer projects to finish their games, or turn their reports into extra
coordinator approval gates. Resolve actual failures with their owners; continue
independent implementation when a consumer or human review is waiting.

## Scheduling, communications and gates

Use the central Oryx FIFO and Extender's physical bench registry as an ordinary
participant. The Author permits an exception when a run would clear blockers
for more than one agent: record those concrete blockers and coordinate with
the scheduler/affected owners; never infer expiry or kill another owner's
process. No general priority or permanent reservation follows.

Continue reading authorized agent mail promptly and send substantive responses
through durable mail plus immediate chat steering. Consumers own their code,
tests and cleanup. No new recipient or external publication authority is implied.

Build and inspect before reserving target time. Use current confirmed fast
transfers for physical work. Prefer the installed capable firmware; any needed
flashing uses a concrete bounded plan, existing applicable authorization or
explicit approval, and exact incoming restoration. Do not launch interactive
emulators for the Author. Human-review/default/publication gates remain in
force; permission to publish this plan does not accept later implementation.
