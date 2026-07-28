# Compiler host language/toolchain: decision and proposal

Date: 2026-07-28

Resolves (partially) the open item in [2026-07-28.md](2026-07-28.md):
"Decide on host implementation language and toolchain layout for the
compiler itself."

## Context / how we got here

The question started from wanting the Golem compiler to "run natively on
Agon, but also under Linux." Investigating that surfaced a real fork:

- **Fork A - cross-compiler model.** The compiler binary only ever runs on
  a desktop host (Linux/Mac/Windows-WSL) and *targets* Agon as an output
  platform. This is how essentially every modern retro-targeting toolchain
  works (z88dk, sdcc, agondev, AgDev) - nobody expects the compiler itself
  to execute on the constrained target.
- **Fork B - genuine dual-native self-hosting.** The same compiler source
  actually executes as an eZ80/MOS program on real Agon hardware, with a
  separate execution path on Linux. The only real, de-risked candidate for
  this (verified against `agon-docs/docs/FAQ.md`) is Forth: `agon-forth`
  (lennart-benschop) already exists as a native-Agon implementation, and
  Forth's whole design point is bootstrapping identical high-level source
  onto a minimal target-specific kernel. C/C++ have no native-hosted
  compiler for Agon at all - only cross-compilers (ZDS II, agondev, AgDev).

Local inspection of `/home/smith/Agon/agondev` (a real checkout, not just
docs) confirmed:

- agondev is LLVM/clang-based, with its own `Target/Z80` backend and its
  own assembler (`ez80-none-elf-as`, part of a full ELF-object-producing
  clang toolchain) - it does bundle its own eZ80 assembler, distinct from
  `ez80asm`.
- That assembler uses **GNU-as ("gas") style syntax**, not ZDS/ez80asm
  syntax - confirmed by agondev's own `scripts/convert_src_to_gnu-as.sh`,
  which mechanically rewrites classic ZDS-style directives (`section`,
  `public`, `private`, `extern`, `dl`, `dd`, `rb`, `jq`, ...) into gas-style
  equivalents (`.section`, `.global`, `.local`, `.extern`, `d24`, `d32`,
  `ds`, `jp`, ...). Even agondev's own bundled runtime libraries
  (`src/lib/libmos/*.src`) are stored in the original ZDS-ish dialect and
  go through this conversion as a build step.
- agondev's README states it is "currently compiled for Linux (x86_64 /
  arm64) and MacOS (arm64) only... Windows is supported using WSL" - i.e.
  agondev itself is squarely a Fork-A tool. It does not run on Agon and
  does not change the Fork A/B analysis.
- `ez80asm` remains relevant specifically for syntax-compatibility with the
  large existing body of ZDS/ez80asm-style source in the Agon ecosystem
  (agon-mos, agon-vdp userspace examples, `reference/agnb-asm`, and
  Golem's own hand-written `runtime/legacy-asm/vdu_buffered_api.inc`) - none of
  which agondev's assembler accepts without the conversion step above.

## Decision

**Abandon any notion of the Golem compiler running natively on Agon
hardware itself (Fork B).** The compiler is a cross-compiler: it runs on a
desktop host (Linux, with Mac support to be added if it gains traction
with Mac users in the community; Windows not currently planned) and
produces Agon-targeted output. This matches how the rest of the modern
Agon C/C++ ecosystem already works, and avoids taking on the Forth-based
self-hosting effort (a real but unusual and comparatively
unproven-for-a-project-this-shape path) for a "would be nice" rather than
a hard requirement.

**The Golem compiler itself will be written in C++, built with an ordinary
host compiler (not agondev)**, targeting Linux. This resolves open
question 2 below (C vs. C++) and disambiguates open question 1 (what
"use agondev" means) in favor of option (b)-adjacent, but scoped more
narrowly than either original option: agondev is not used to build the
compiler at all (contra draft option (b)); it's used downstream, to build
the eZ80-side artifacts described next.

**agondev's role: build the eZ80/MOS-side programs that load and run
Golem-compiled output.** Concretely, per-program eZ80 code that: decompresses
and loads a Golem-compiled buffer-command blob into VDP buffers, executes
it, retrieves whatever sparse information can be gotten back out of the VDP
(see the write-only-VDP design note in
[2026-07-28-vdu-buffered-api.md](2026-07-28-vdu-buffered-api.md) and the
render-completion material in [from_codex.md](from_codex.md)), and hosts
whatever other eZ80-side framework capabilities Golem grows over time. This
part of the design is deliberately left open-ended for now, by the Author's
own framing ("any other framework i can only barely conceive of at the
moment").

## Open questions

1. ~~What does "use agondev" mean concretely?~~ **Resolved above**: agondev
   builds the eZ80-side loader/runtime, not the compiler itself.
2. ~~C or C++?~~ **Resolved above**: C++, ordinary host compiler, Linux
   first.
3. **What happens to `runtime/legacy-asm/vdu_buffered_api.inc`? Resolved: it
   keeps both existing roles, unmodified.** It continues to serve (a) other
   projects writing hand-rolled "legacy" ez80asm/ZDS-style assembly, and
   (b) as the reference/template for a parallel version written in
   agondev's gas-style eZ80 assembly dialect, for use by the agondev-built
   loader/runtime. It is not being converted in place or retired - a
   second, separate file in the agondev dialect is expected eventually,
   transcribed from this one's already firmware-verified byte layouts
   rather than re-deriving them, so the two shouldn't diverge on the facts
   even though they'll diverge in syntax.

   Naming: **"ez80gas"** (eZ80 + GNU "gas" syntax) adopted provisionally for
   Golem's own purposes, with "ZDS-style" or "classic eZ80 assembly" as the
   name for the existing dialect. The Author will separately check what the
   wider Agon community ends up calling this dialect and may rename to
   match if a community term emerges.
4. **Fixed generic runtime, or per-program-generated eZ80 code? Explicitly
   deferred, not decided.** The Author doesn't yet know which shape is
   right and has asked to avoid locking in binding decisions before the
   implications are better understood, having not written a compiler
   before. Working approach going forward: start with the simplest
   possible case (most likely a fixed, generic loader, since that's the
   smaller thing to build first) and probe the boundaries experimentally
   as real programs are attempted, rather than designing the general
   answer up front.
5. **Loader interface/manifest format. Acknowledged as likely novel, and
   that's welcomed rather than treated as a problem to avoid.** The Author
   is fine with Golem potentially inventing a new protocol here from
   scratch if nothing suitable already exists - and notes the wider Agon
   community could plausibly benefit from it too, not just Golem. No
   design work done yet; revisit once question 4 has enough of an answer
   to know what the interface actually needs to carry.
6. **Reuse agondev's project scaffolding directly? Resolved: yes.** The
   eZ80-side loader/runtime should be structured as an ordinary agondev
   project (its own `src/`, `Makefile`) to get `make upload`
   (hexload-based hardware upload) and Fab Agon Emulator integration
   (`make em`/`emu`/`emulator`) for free, rather than reinventing them.
7. **Is an eZ80/MOS host/controller mandatory for Golem? Resolved: no.**
   Codex's response to `to_codex.md` (see `from_codex.md` section 10.2) is
   explicit: the Pingo interrupt-transport pattern is shared-technique
   cross-pollination, not a reason to require an eZ80 controller. Core
   Golem language/runtime stay pure-VDP; the only required eZ80-side piece
   is the loader/bootstrap that installs compiled buffers. A
   **host-extension seam** should eventually be reserved for a deferred,
   optional `hosted/co-processor` lifecycle profile (symbolic entrypoints,
   lifecycle metadata) without allocating any MOS vector, packet magic, or
   fixed buffer-ID range yet. See
   [2026-07-28-execution-lifecycle.md](2026-07-28-execution-lifecycle.md)
   for the fuller correction this response also surfaced (unrelated to
   this question, but found alongside it): `bufferCall()` is synchronous
   and does not service the VDP's outer event queue, so a non-returning
   Golem `main` would starve VSYNC/input/callback processing - core
   language design needs to account for this regardless of the
   eZ80-controller question.
8. **MOS keyboard-vector-slot ownership, if/when Golem ever needs a host
   component. Answered (community-level design, not yet needed by
   Golem).** Codex's response (`from_codex.md` section 10.1) proposes a
   single application-wide `mos_vdpp_event_mux` that exclusively owns MOS
   API `&1D` for its whole lifetime, with an ISR that only classifies and
   copies records into preallocated mailboxes/a bounded queue (never
   calling arbitrary client handlers directly), routes keyed by an exact
   4-byte prefix that can't collide with a valid keyboard record, and a
   registered-route table carrying magic/record-size/destination/buffering
   policy/active flag. This is a design for the shared/community
   "authoritative assembly API," not something Golem needs to implement
   now - noted here so Golem's eventual host-extension seam (question 7)
   follows this convention rather than inventing a competing one if a
   host component is ever built.
9. **Compact (10-byte) vs. larger completion-record payloads for any
   future Golem-owned event, if the host-extension seam is ever used.
   Answered.** Codex's response (`from_codex.md` section 10.3) clarifies
   the 10-byte Pingo payload was never shown to be a discovered transport
   threshold - it's the frozen, hardware-qualified version-1 ABI, kept
   for unrelated reasons (no redundant fields, matches stock mouse-frame
   size, fits an empty hardware RX FIFO). The real hard limit is stock
   MOS's parser: a 16-byte data buffer that rejects any declared payload
   above 16 - and there's a real firmware bug in that rejection path (the
   discard state is entered without first recording the rejected length,
   so the next byte wraps a counter to 255 and MOS discards 256
   subsequent bytes, including from later frames, before resyncing).
   Takeaway for Golem, if this is ever relevant: keep any future
   compat-event payload at or under 10 bytes when practical, never exceed
   16, and never probe above 16 at all given the discard-path bug. Any
   payload in the 11-16 byte range would need real hardware stress
   qualification, not just emulator success, before being trusted.
