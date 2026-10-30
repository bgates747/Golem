# GOLEM-056 — C/C++ VDPlet capability review across agent projects

Give recent collaborating agents one evidence-backed description of what native
C/C++ VDPlets can now do, then collect their improvement ideas and missing
integration hooks through the mailbox. Golem coordinates the returns and later
assignments so the Author can review and dispatch work from this conversation.
This first phase is consultation, not permission to implement the proposals.

## Scope and frozen sequence

Freeze this plan in Git before reviewing message contents or sending outreach.
Use the fourteen-day window from **2026-10-16 17:01:35 EDT** through
**2026-10-30 17:01:35 EDT** for the initial mailbox inventory. Preserve message
IDs and exact agent/session identities; later consultation replies are tracked
separately rather than changing the original inventory window.

Inventory the recent mailbox's agent correspondents and relevant discussions,
including sent and received mail. Resolve each distinct agent session using the
app's existing-thread tools; record merged aliases, unavailable sessions and
exclusions with reasons rather than silently omitting them. Do not create new
agent sessions or implementation subtasks during this consultation.

Verify capability claims against the current agon-vdplet source, API reference,
task evidence and commits. Describe generic native services, VDU-byte submission,
buffers and lifetime rules, drawing/presentation, audio, capacity and compatibility
where supported. Separate available behavior from prototypes, pending research,
firmware requirements and emulator limitations. C/C++ usage is the focus; Golem
generation is optional context, not a prerequisite.

Keep the shared brief, source/mail inventory, outreach log and returns under
docs/tasks/GOLEM-056/. The brief must give agents enough detail and references to
identify useful changes without needing a long chat message. Notifications must
only direct each session to read the brief and follow its instructions.

## Work and acceptance

- PLN-001 [x] Commit and pin this plan before the mail audit and outreach.
- INV-001 [x] Inventory the frozen mail window and map distinct correspondents
  to their existing sessions; document coverage and any unresolved identities.
- CAP-001 [x] Write the shared capability brief with exact source/version
  references and clear implementation/qualification limits.
- REQ-001 [x] Specify a mailbox response format: proposed improvement, existing
  capabilities used, missing hooks, ownership, expected benefit, dependencies,
  validation and any conflicts. A reasoned no-change response counts as a return.
- OUT-001 [x] Notify each in-scope agent session in chat, linking the brief;
  record delivery and failures without treating delivery as acknowledgement.
- RET-001 [x] Collect and index all mailbox returns, preserving original IDs;
  track outstanding replies and clarify technical questions as needed.
- SYN-001 [x] Consolidate returned proposals, shared hook requests, dependencies
  and suggested owners for joint review with the Author after returns are in.
- CAT-001 [x] Compile a concise, deduplicated feature-request catalog using the
  stable CNS IDs, original proposal/message IDs, suggested owners, dependencies,
  existing task links and the distinction between new hooks, consumer work and
  already-covered/no-change dispositions. Present it for the Author's review;
  cataloging and publication do not dispatch implementation.
  Delivered as [feature-requests.md](GOLEM-056/feature-requests.md): all 22 source
  proposal IDs plus Wolf's handoff, consolidated into the existing 15 CNS groups.
- DIS-001 [ ] After the Author's review, dispatch approved assignments from
  here and track acknowledgement, progress, evidence and completion centrally.
  This step is explicitly gated; consultation does not authorize feature work.

The Author directly authorizes contacting these existing agent sessions and
requesting durable mailbox replies. Substantive coordination stays in the
mailbox; chat serves as notification. Do not alter peer project implementations,
flash firmware, launch graphical emulators or assign implementation work before
the agreed review. TODO.md remains the authoritative task register; the task's
tables record consultation and assignment state, not a second project backlog.

## Consultation artifacts

The plan is frozen/pushed as `2ea6059`. The [capability brief](GOLEM-056/capabilities-and-response-request.md), [coverage](GOLEM-056/mail-coverage.md), [frozen source/mail receipt](GOLEM-056/freeze.json) and [outreach/return register](GOLEM-056/coordination.md) hold the current consultation. All seven existing sessions have received mailbox invitations plus the requested brief-only chat notification; delivery is not a substantive return.


## Current consultation checkpoint

All seven requested sessions received invitations and chat notifications. Every
session returned a substantive mailbox report or reasoned no-change disposition;
all eight reports, including FSIM's Pingo/async follow-up, were read and
acknowledged. [Consolidated review](GOLEM-056/review.md)
preserves the original proposal IDs, overlaps, suggested owners, dependencies
and recommended order. RET-001 and SYN-001 are complete. DIS-001 remains gated
on the Author's review: no implementation assignment has been dispatched.

FSIM-004/005 add synchronous loadable Pingo feasibility, reusable offscreen
bitmap publication and separate generic asynchronous-job research. A synchronous
module can be assessed first; async is a later overlap option, not a prerequisite.
Renderer size/stack estimates do not qualify an actual module or justify raising
limits. Existing GOLEM-043 is the deferred concurrency research to reuse if
selected; this consultation does not start it.

The Author explicitly approved all seven local consultation replies after
reported automatic-review delivery rejections. [Authorization](GOLEM-056/consultation-authorization.md)
pins the direct user message; Rally and6502 returns are received. That approval
includes relevant project paths/measurements but no implementation or external
publication. The capability brief remains unchanged from its invited hash.

An additional Author-requested Wolf3DOrig handoff arrived after the original
returns. [Static review](GOLEM-056/wolf3d-review.md) and CNS-015 preserve its
requirements, evidence limits and overlap with shared pixel work. It is indexed
separately from the frozen roster. The Author approved the technical mailbox
reply and chat notification, both delivered, then added Wolf3DOrig to the
approved consultation list for continuing mailbox/chat coordination. No Wolf
implementation, deployment or hardware work is started.

On 2026-10-30 the Author explicitly requested committing/pushing pending work,
then compiling the request list. The existing consultation task owns that
compilation; CAT-001 makes the deliverable explicit without opening duplicate
feature-implementation tasks. This publication instruction supersedes the prior
hold on publishing consultation artifacts. DIS-001 remains gated on review.

Publication was specifically approved for both named repositories after automatic
review initially rejected the mailbox push. Mailbox checkpoint `e96aa22` and
Golem consultation checkpoint `75dd58c` are pushed. CAT-001 is complete; the
[compiled catalog](GOLEM-056/feature-requests.md) is ready for the Author's
selection. No implementation assignments have been sent.
