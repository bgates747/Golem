# Golem language design proposals: "what type of language is this?"

Status: **living document** - actively updated as decisions are made or
reconsidered. Not a frozen spec. See individual sections for current
status markers.

Started 2026-07-28, seeded from the discussion in
[docs/devlog/2026-07-28.md](../devlog/2026-07-28.md),
[docs/devlog/2026-07-28-execution-lifecycle.md](../devlog/2026-07-28-execution-lifecycle.md),
and [docs/devlog/2026-07-28-compiler-toolchain.md](../devlog/2026-07-28-compiler-toolchain.md).
Read those first for the platform constraints this document assumes
throughout: no MMU/indexed addressing (patch-then-execute instead), one
buffer block = one compiled instruction, 8/16-bit arithmetic as the fast
path with 32-bit "taxed" and comparisons hardware-capped at 16 bits, the
VDP is write-only from the eZ80's perspective (hard to introspect at
runtime), and the corrected `batch`/`event-resident`/`hosted` execution
lifecycle model.

This question is deliberately broken into independent axes - "what type of
language" bundles several genuinely separate decisions that don't have to
be resolved together or in this order.

## Axis 1: Paradigm

**Status: leaning imperative/procedural, not decided.**

- **Imperative/procedural, C/Pascal-like** (the working sketch since the
  first devlog). Maps directly onto the buffer/jump execution model:
  functions = buffers, control flow = conditional jumps, straight-line
  statement sequences. Lowest compiler-engineering lift, most familiar to
  write and reason about.
- **Structured assembler.** An even thinner layer - loops/`if`/functions
  as sugar over jumps, otherwise close to 1:1 with buffer commands, no
  real expression-level abstraction. Maximizes predictability of output
  size/timing (relevant given the 32-bit tax and 16-bit comparison cap -
  you can see exactly what a statement costs). Smallest possible compiler.
- **Stack-based / concatenative (Forth-like).** Worth naming explicitly
  given the retro fit, the "buffers as inert clay, animated by a command
  sequence" metaphor (a natural match for Forth's colon-definitions), and
  its historical track record on constrained hardware. Not part of the
  original sketch.
- **Event/reactive-first.** Given the `event-resident` lifecycle profile
  (VSYNC/input handled via bounded, VDP-dispatched callback buffers, not a
  permanently-running loop), a language that treats *handlers* as a
  primary top-level construct (`on vsync { ... }`, `on key { ... }`)
  rather than bolting event registration on as a library call could fit
  the platform's actual execution model unusually well.

## Axis 2: Type system strength

**Status: leaning Pascal-style static + range-checked, not decided.**

- **Minimal/weak, C-like.** Bytes/words/dwords, arrays, structs-as-byte-
  layout, no inference. Matches the metal closely, simplest compiler.
- **Pascal-style static + range-checked.** Stronger nominal typing,
  checked subranges/enums. Pascal's original selling point was catching
  bugs at compile time on hardware with a slow, awkward debug loop -
  that argument applies with unusual force here, since the VDP is
  write-only and genuinely hard to introspect at runtime (see the
  testability design note in
  [2026-07-28-vdu-buffered-api.md](../devlog/2026-07-28-vdu-buffered-api.md)).
  A stricter static type system is a deliberate mitigation for that: catch
  at compile time what can't easily be caught later.
- **Untyped/word-oriented (BCPL/Forth-style).** "Everything is a machine
  word." Fastest to compile, zero safety net. Given the same WOM argument,
  probably the riskiest option here, not just a stylistic one.

## Axis 3: Memory model

**Status: leaning static/arena-only, not decided.**

- **Fully static/arena only, no dynamic allocation.** Matches the original
  sketch ("globals get fixed bufferId/offset at compile time"). Simplest,
  fully deterministic, easiest to reason about with no runtime
  introspection available.
- **Region/bump allocation** (a "heap buffer," allocate-only, no free).
  Slightly more flexible for variable-lifetime data, still fully
  deterministic, no fragmentation/dangling-reference risk.
- **Manual alloc/free.** Most powerful, but fragmentation and dangling
  references are exactly the class of bug this platform is worst at
  helping find after the fact. Probably worth avoiding, or at least not
  defaulting to.

## Axis 4: Abstraction level / compiler ambition

**Status: leaning thin/structured-assembler for v1, not decided.**

- **Thin, literal, "structured assembler" for v1.** Smallest possible
  thing that could work; matches the Author's "start simple, probe
  boundaries" stance (see
  [2026-07-28-compiler-toolchain.md](../devlog/2026-07-28-compiler-toolchain.md))
  and is realistic for a first compiler written by someone still learning
  how compilers work.
- **A real small systems-language compiler** (proper IR, optimization
  passes). The more ambitious long-term target implied by the original
  devlog's three-address-code IR sketch, but a much bigger lift.

## Current lean (not a decision)

Procedural/imperative + Pascal-style static typing with range checks +
static/arena-only memory + thin structured-assembler abstraction for v1,
with event-handler declarations treated as a first-class top-level form
once the `event-resident` profile gets built. The typing axis is the one
place worth actively pushing back against "just do C-style" - the
debuggability argument is concrete and specific to this platform, not
generic language-design taste. The Author has indicated agreement with
this lean as of 2026-07-28, but it is not being treated as locked in.

## Standing note: upstream contribution is on the table

If Golem's design surfaces something genuinely valuable at the VDP/MOS
level - a new Buffered Commands API feature, a generalized version of the
`mos_vdpp_event_mux` pattern from `docs/devlog/from_codex.md`, a fix for
the MOS packet-discard bug noted there, or anything else - an upstream
pull request to the relevant project (agon-vdp, agon-mos) is a real,
intentional possibility, not just a hypothetical. Design work here
shouldn't assume everything must be worked around purely inside Golem;
"this would require a firmware change" is a legitimate proposal outcome,
not necessarily a dead end. Worth flagging candidate items here as they
come up, so they don't get lost.

**Candidates so far:** none formally proposed yet. (The MOS
oversized-payload discard bug documented in
[from_codex.md](../devlog/from_codex.md) §10.3 is arguably already a
bug-report-worthy finding, independent of any Golem-specific feature -
worth considering as a first candidate, separate from Golem's own design.)
