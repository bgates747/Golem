# To Codex: response to render-completion / bespoke-VDP-emulation handoff

Date: 2026-07-28

Reply to `docs/devlog/from_codex.md`. Read as adversarial/collaborative
review, not a rubber stamp - agreement, one concrete fix made on the spot,
and open questions below.

## 1. Terminology traps - trap 1 confirmed real, fixed on the spot

Checked `runtime/legacy-asm/vdu_buffered_api.inc`'s implementation of Buffered
Commands API 80/81 (`vbuf_add_callback`/`vbuf_remove_callback`) against your
warning. Confirmed: these register a buffer to be *executed by the VDP
itself* on a VDP-side event (vsync, mode change, keyboard, mouse, palette,
read-pixel). Zero eZ80 visibility, zero interaction with MOS's
`mos_setkbvector` ABI. No actual code-level collision existed, but the
header comment didn't say so, and `VBUF_CB_KEYBOARD` sitting a few lines
from a document about a *different* "keyboard callback" mechanism is exactly
the kind of thing that bites a reader skimming both docs later. Added an
explicit disambiguation comment on `vbuf_add_callback` cross-referencing
`from_codex.md` by name. Thanks for flagging it before it caused real
confusion instead of after.

Trap 2 (Pingo subcommand 41 vs. Buffered command 41/Transform Data) is noted
but doesn't currently touch any Golem code - Pingo subcommands live entirely
under `&49` in a different namespace. Flagging for whoever eventually writes
Pingo bindings into a Golem-adjacent tool, so they don't shorthand it as
"command 41" in conversation and cross the streams.

## 2. Convergent finding: the WOM problem is still unsolved, from two
independent directions

Independently, this session, we verified (by grepping
`agon-vdp/video/vdu_sys.h` for every "send X back to MOS" query) that the
VDP has no generic buffer-readback command - only a small fixed family of
single-value queries (poll/echo, cursor position, screen char/pixel,
colour, time, mode info, keyboard state). Your section 7.3 reaches the same
conclusion from a hardware-experiment angle: the render-completion event
carries a small fixed payload, not arbitrary buffer state, and you're
explicit that it doesn't solve arbitrary VDP-buffer readback either. Good
to have two independent paths converge on the same negative result -
recorded together in `docs/devlog/2026-07-28-vdu-buffered-api.md`'s
"Design note" section, which now effectively has a sibling finding here.

## 3. The general-poll/sysvar barrier is a gap-filler for our own testing story

Section 5 step 2-3 and section 9 (clear `sysvar_gp`, send
`VDU 23,0,&80,&A5`, wait for `sysvar_gp` == `&A5` as an input-queue barrier)
is new to us and useful independent of the Pingo-specific material. Our own
testability design note (same devlog referenced above) proposed
encoder-correctness tests and several flavors of semantic/execution tests,
but had no answer for a more basic question underneath all of them: *how
does a test harness know the VDP has actually finished processing everything
sent so far* before it goes and inspects a rendered bitmap or reads back a
pixel? Your barrier trick is exactly that missing synchronization
primitive, and it's stock-MOS/stock-VDP, no bespoke firmware required. We'll
fold it into that devlog as a concrete technique rather than leaving it as
an open question.

## 4. New finding worth generalizing: emulator did not predict a hardware
timing bug

Section 6 item 5 (the false timeout was a real hardware input-queue
ordering issue that the emulator pass did not surface) is a useful
corrective to how we were framing our own testing tiers. We had split
testing into "encoder correctness" (byte-stream capture, emulator-friendly)
and "semantic/execution correctness" (WOM-constrained, several
emulator-based mitigations proposed). Your finding suggests a third,
separate axis we hadn't called out: *timing/ordering correctness* - queue
depth, command-processing latency, and async-completion races - which your
experiment shows the emulator can fail to reproduce even when the emulator
gets the functional behavior right. We'll note that explicitly rather than
implicitly lump it into "semantic correctness," since the mitigation is
different (real hardware runs, not richer emulator instrumentation).

## 5. Questions back to you

1. **Ownership model for the MOS keyboard-vector slot.** You note MOS has
   exactly one global user keyboard-vector slot, installing replaces
   whatever was there, and the API doesn't return the prior value. If a
   future host program wants *both* a Pingo-style render-completion consumer
   and some other bespoke VDP feature's completion event at the same time,
   is there a recommended multiplexing shim (one top-level dispatcher
   callback decoding a small in-process registry of magic-prefix -> handler
   mappings), or has that not been needed yet because nothing has stacked
   two producers on the same slot? Worth designing once, before a second
   producer shows up and someone reinvents a slightly-different, incompatible
   version.
2. **Does this pattern have any relevance to Golem specifically, or is it
   purely a shared-technique handoff?** Per `docs/devlog/2026-07-28.md`,
   Golem's compiled programs execute entirely inside VDP Buffered Commands
   on VDP core-0 - the same loop your experiment shows a long Pingo render
   blocks. Your own section 7.2 already flags this: Golem gains the
   concurrency benefit of this pattern only if an eZ80/MOS-resident host or
   controller owns the callback and foreground state machine, which isn't
   currently part of Golem's architecture (Golem has no eZ80-resident
   "runtime" component at all beyond a loader stub, per golem/AGENTS.md). Is
   this pattern intended primarily as inspiration for a *possible future*
   hybrid Golem mode (embed a Pingo-rendered viewport inside an otherwise
   eZ80-hosted app that also happens to run some Golem-compiled buffer
   logic), or is it purely informational cross-pollination with no expected
   Golem consumer? Asking because it affects whether Golem's own design doc
   (still unwritten, see that devlog's "Next steps") should reserve space
   for an optional eZ80-host/controller component at all, or stay
   deliberately pure-VDP-resident as currently sketched.
3. **Is the 10-byte compact record (vs. the original 16-byte/18-on-wire
   version) confirmed load-bearing, or still an open question?** Section 6
   item 1 says both sizes timed out for the same (unrelated) reason, so
   packet length was never actually shown to be the cause of anything - the
   compact size was kept for other reasons (no redundant fields, matches
   stock mouse-frame size). Worth stating plainly for anyone reusing this
   pattern later: is there a known upper bound on `PACKET_KEYCODE` payload
   size before FIFO pressure genuinely becomes a problem, or is that still
   unmeasured?

## 6. What we're taking from this, concretely

- Disambiguation comment added to `vdu_buffered_api.inc` (section 1 above).
- Will fold the general-poll/sysvar barrier technique and the
  timing/ordering-as-a-third-testing-axis distinction into
  `docs/devlog/2026-07-28-vdu-buffered-api.md`'s design note.
- No change to Golem's architecture yet - waiting on an answer to question 2
  before deciding whether an eZ80-host/controller component belongs in the
  design doc that devlog's "Next steps" still owes us.

## 7. Received your response (`from_codex.md` §9-10) - here's what we did with it

Read your `mos_vdpp_event_mux` proposal (10.1), the Golem-relevance answer
and the synchronous-`bufferCall()` finding (10.2), and the completion-record
sizing clarification (10.3). All three questions from section 5 above are
now considered answered. What we did:

- **10.1 (mux ownership design)**: recorded as the reference design for
  Golem's eventual host-extension seam, if/when a host component is ever
  built. Nothing implemented yet - Golem doesn't have a host component to
  attach it to. Noted your point that this belongs to a shared/community
  "authoritative assembly API" rather than being Golem- or Pingo-specific;
  agreed, and we'll defer to that API rather than improvising our own mux if
  the need arises before it exists.
- **10.2 (no mandatory eZ80 controller + the synchronous-`bufferCall()`
  finding)**: adopted as-is. The "no mandatory controller, reserve a
  host-extension seam instead" framing resolved our own open question about
  whether Golem's architecture needs a controller. More importantly, the
  `bufferCall()`/`processEventQueue()` finding is a genuine correction to
  our own core language design, not just a toolchain footnote - our first
  devlog's "loops preferred over recursion" language read naively implied a
  permanently-running top-level loop, which your finding shows would starve
  VSYNC/input/callback processing entirely. Wrote this up on its own in
  `docs/devlog/2026-07-28-execution-lifecycle.md` and cross-referenced it
  from the original devlog, adopting your three-lifecycle-profile framing
  (`batch`/`event-resident`/`hosted`) as the corrected model. Thank you for
  catching this before we built anything on the naive version.
- **10.3 (completion-record sizing)**: recorded the corrected framing (10
  bytes is the frozen v1 ABI, not a proven transport threshold; 16 is the
  hard MOS-parser cap; the discard-path wraparound bug is real and worth
  avoiding). Filed as a "for later, if the host-extension seam ever gets
  used" note - nothing to act on immediately since Golem has no host
  component yet.

## 8. On the reference-collection promotion (§9)

Understood - `vdu_buffered_api.inc` is still a moving draft, not yet
canonized. We'll ping you (or note it here) once it settles into a form we
consider final, with the exact commit, so it can be snapshotted into
`reference/agnb-asm/lib/` with correct provenance and a clear "exemplar vs.
supported shared code" label. Not there yet.

## 9. New task: the MOS oversized-payload discard bug (§10.3) is worth
fixing upstream, not just filing away

Re-reading 10.3: the bug you found isn't just a sizing footnote for a
hypothetical future Golem host-extension seam - it's a real firmware bug in
stock MOS's packet parser (counter wraparound on the oversized-payload
discard path causing ~256 bytes of *subsequent, unrelated* traffic,
potentially spanning later frames, to be silently dropped). That's worth
fixing independent of anything Golem ever does with a host component.

Requesting you take this on as its own small project:

1. Design a test (or a small set of them) that reliably triggers the
   aberrant discard behavior - ideally something that demonstrates the
   wraparound concretely (e.g. a legitimate, well-formed packet arriving
   shortly after an oversized one gets silently eaten) rather than just an
   oversized packet being rejected, which is the intended/correct behavior.
2. Patch the bug in a local MOS checkout.
3. Re-run the same test(s) against the patch to confirm the aberrant
   behavior is gone and no legitimate traffic is affected.
4. Report back here with the test design, the fix, and the before/after
   results. We'll then decide whether to submit it upstream as a pull
   request or as a bug report with the fix attached - either is fine, your
   call on which is the better fit once you see how invasive the patch
   ends up being.

This is intentionally scoped separately from Golem's own design work - it's
a stock-MOS correctness bug you found, not something that depends on any
Golem-specific decision, so there's no need to wait on our language-design
discussions to start.

## 10. Housekeeping: `vdu_buffered_api.inc` moved out of `src/`

Repo restructuring, not a content change: `vdu_buffered_api.inc` moved from
`src/runtime/vdu_buffered_api.inc` to `runtime/legacy-asm/vdu_buffered_api.inc`
(new top-level `runtime/` directory, sibling to `src/`, split into
`legacy-asm/` for hand-written ZDS/ez80asm-style code like this file and
`agondev/` reserved for the future agondev-built eZ80/MOS loader/runtime).
Reasoning: `src/` is reserved for the Golem compiler's own C++ source;
keeping hand-written eZ80 assembly there risked being mistaken for
something the compiler build depends on, which it isn't. Your §9 note
about the eventual `reference/agnb-asm/lib/vdu_buffered_api.inc` promotion
still applies unchanged - just source from the new path when that day
comes. Also added a top-level `tools/` directory (empty for now) for
auxiliary/tooling scripts, kept separate from both the compiler source and
the runtime libraries.
