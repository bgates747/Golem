# Project Handoff

Read `/home/smith/Agon/mystuff/agon-dev-env/codex/AGENTS.md` first — it owns
reusable environment, workflow, and repository conventions shared across all
Agon projects. This file contains only Golem-specific information.

## Purpose and current status

Golem is a compiled, low-to-medium-level, C/Pascal-like language targeting the
Agon Console8 VDP's **Buffered Commands API** directly (rather than the eZ80
via MOS). See the root [README.md](README.md) for the elevator pitch.

Status: **early design/exploration**. No compiler code exists yet (`src/`,
`tests/`, and `examples/` are still placeholder `.gitkeep` trees). Design
work has moved beyond the kickoff devlog, though: `docs/design/` now holds
a living design-proposals doc, and several follow-on devlogs (toolchain
decision, execution-lifecycle correction, VDU wrapper library writeup) plus
a cross-agent collaboration thread with an external "Codex" agent have been
added. See "Repository layout" and "Session-start reading order" below for
where everything now lives; treat `docs/design/` as the most current design
reference where it overlaps with older devlog material.

## Repository layout

```
docs/
  devlog/     Dated development log entries (chronological, append-only)
  design/     Living design docs (amended in place, not dated)
reference/    Vendored/curated reference material (not a build dependency)
runtime/
  legacy-asm/ Hand-written ZDS/ez80asm-style eZ80 runtime library code
  agondev/    (placeholder) future eZ80/MOS-side loader/runtime via agondev
src/          Compiler/toolchain source (C++, not yet started)
tools/        (placeholder) auxiliary/tooling scripts, not the compiler
examples/     Example Golem programs (placeholder)
tests/        Test suite (placeholder)
```

## Canonical branch and important external dependencies

No branches beyond the working history yet; nothing has been tagged/released.

External references (read-only, outside this repo):

- `/home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md` — the
  **primary compilation-target reference**: commands 0–65+ for buffer
  writing, call/jump/conditional control flow, byte arithmetic/logic
  (command 5), matrix ops (command 34), compression/decompression (command
  65), split/spread (commands 16–22), etc. Read this before designing any new
  IR lowering.
- `/home/smith/Agon/agon-docs/docs/vdp/VDP-Variables.md` — VDP Variables,
  used by some buffered conditional commands as an alternative operand to
  buffer bytes.
- `/home/smith/Agon/agon-vdp/video/vdu_buffered.h` (plus sibling headers in
  the same directory: `buffers.h`, `buffer_stream.h`,
  `multi_buffer_stream.h`, `compression.h`) — the **actual VDP firmware
  implementation** of the Buffered Commands API. This is the definitive
  source of truth when the prose docs are ambiguous or when verifying an
  unusual/undocumented behavior (e.g. the devlog's open question about
  whether command 34's 1x1-matrix `invert` yields a scalar reciprocal —
  check `dspm_mult`/`mat.h` usage in this header before trusting the docs
  alone).
- `/home/smith/Agon/fab-agon-emulator` — emulator for testing generated
  buffer-command sequences without hardware. No Golem-specific emulator
  profile exists yet; one will need to be created under
  `agon-dev-env/emulators` (or a project-local profile, per
  `agon-dev-env/codex/emulator.md`) once there's something runnable.
- `/home/smith/Agon/mystuff/agon-utils` — canonical utility/writer/validator
  patterns for Agon binary formats; useful idiom reference for the eventual
  Golem binary-blob loader/packer, even though Golem's output format
  (compressed buffer-block blob, per the devlog's proposed loader) is not
  itself an AGNB format. `agon-utils/examples/agnb/` is the most modern
  collection of "regular" (eZ80/MOS-targeting) Agon application assembly
  available locally; a curated, provisional snapshot of it has been pulled
  into [reference/agnb-asm/](reference/agnb-asm/) as a seed for Golem's own
  eZ80/MOS/VDU idiom reference — see
  [docs/devlog/2026-07-28-agnb-asm-reference-collection.md](docs/devlog/2026-07-28-agnb-asm-reference-collection.md)
  for the full provenance/collision analysis.
- `/home/smith/Agon/mystuff/agon-dev-env/codex/assembly.md` — eZ80 assembly
  conventions; only tangentially relevant since Golem targets the VDP buffer
  machine, not eZ80 code, but worth checking if the toolchain ever needs an
  eZ80-side stub/loader stage.

Golem is not yet listed in `agon-dev-env/codex/repositories.md`; it has been
added there as part of this handoff so other agents can discover it.

## Session-start reading order

1. Canonical `agon-dev-env/codex/AGENTS.md`.
2. This file.
3. Root [README.md](README.md).
4. [docs/devlog/](docs/devlog/), in chronological order — currently
   `2026-07-28.md` (kickoff), `2026-07-28-vdu-buffered-api.md` (wrapper
   library + WOM-testability design note),
   `2026-07-28-agnb-asm-reference-collection.md` (provenance for
   `reference/agnb-asm/`), `2026-07-28-compiler-toolchain.md` (host
   language/toolchain decision), `2026-07-28-execution-lifecycle.md` (a
   correction to the kickoff devlog's execution model — read this one even
   if skimming others). `from_codex.md`/`to_codex.md` are a paired,
   ongoing collaboration thread with an external "Codex" agent working on
   a related project (Pingo) — read them together, in order, as a
   conversation, not as standalone devlogs.
5. [docs/design/](docs/design/) — living documents, amended in place rather
   than dated. Currently: `language-type-proposals.md` (paradigm/type-
   system/memory-model/abstraction-level options, with a stated tentative
   lean). Prefer this over older devlog summaries where they overlap.
6. `git log`/`git status` — inspect for any work in progress before assuming
   the state described above is current.

## Build, test, emulator, and hardware deployment commands

No compiler build system exists yet, and no emulator profile has been
created. The compiler's host language and toolchain layout are decided
(see [docs/devlog/2026-07-28-compiler-toolchain.md](docs/devlog/2026-07-28-compiler-toolchain.md)):
the compiler itself is written in **C++**, built with an ordinary host
compiler (not agondev), targeting Linux first (Mac later if there's
community interest; Windows not planned). It is not written in Python -
the project-local `.venv` below is for auxiliary tooling only. agondev is
used separately, to build the eZ80/MOS-side loader/runtime that loads and
runs Golem-compiled output, not to build the compiler itself.

A project-local `.venv` (Python 3.14.6, per `agon-dev-env/codex/environment.md`)
has been created at `.venv/` for auxiliary/tooling scripts (not the compiler
itself), with the canonical `agon-utils` checkout installed editable and
verified per that document's Python section:

```bash
.venv/bin/python -m pip install --no-build-isolation --no-deps \
  -e /home/smith/Agon/mystuff/agon-utils
.venv/bin/python -m pip check
.venv/bin/python -c "import agonutils; print(agonutils.__file__)"
```

Re-verify the agon-utils checkout itself (separately from this project's
venv) with:

```bash
cd /home/smith/Agon/mystuff/agon-utils && .venv/bin/python tests/test_agonutils.py
```

## Architecture and application-specific conventions

Summarized from the devlog and cross-checked against
[docs/design/language-type-proposals.md](docs/design/language-type-proposals.md)
for anything still open (prefer that doc over this summary where they
overlap):

- One compiled instruction per VDP buffer block; block index doubles as a
  jump/call target (a clean program-counter/address unit).
- Typed three-address-code IR with explicit widening passes for multi-byte
  integer ops (add-with-carry chains) and explicit "patch, then execute"
  pseudo-ops for any runtime-computed address (arrays, pointers, stack-frame
  locals) — the VDP has no indexed/pointer addressing mode.
- Memory model: globals get fixed `bufferId`/offset at compile time; locals
  live in a shared stack buffer with frame-relative access compiled to
  self-modified offsets patched in the function prologue; heap
  objects/arrays get a dedicated buffer or region, indexed via
  patch-then-execute.
- Loops preferred over recursion (zero stack cost, maps to conditional
  jump); tail calls are free (the VDP auto-converts a trailing conditional
  call into a jump) and should be used for recursion-shaped code wherever
  possible. Non-tail recursion needs bounding or an explicit software stack
  — the VDP's internal call stack can overflow and crash it.
  **Correction**: this does NOT mean the whole program can be one
  permanently-running top-level loop — `bufferCall()` runs synchronously
  and does not service the VDP's outer event queue, so a non-returning
  `main` would starve VSYNC/input/callback processing entirely. See
  [docs/devlog/2026-07-28-execution-lifecycle.md](docs/devlog/2026-07-28-execution-lifecycle.md)
  for the corrected `batch`/`event-resident`/`hosted` execution-lifecycle
  model.
- Runtime library (multiply, divide/mod, shifts, memcpy, string ops) resident
  in VDP buffers and called like any other function, to avoid eZ80↔VDP
  round-trip chattiness.
- Loader/linker: ship the compiled program as one compressed blob,
  decompress into a scratch buffer (command 65), then fan it out to static
  buffer IDs via split/spread commands (16–22) in a couple of VDU calls,
  rather than streaming many small `write block` commands.
- No general floating point initially; possible 1x1-matrix-`invert`
  reciprocal trick (command 34) is unverified — verify on real
  hardware/emulator before relying on it.
- 8/16-bit integers are the fast path; 32-bit is "taxed" (more instructions);
  comparisons are hardware-capped at 16 bits, so wider comparisons must be
  decomposed byte-by-byte.

## Current task and unresolved checklist

From the devlog's "Next steps" checklist:

1. [ ] Write a formal design doc for the Golem IR and buffer/block
       addressing model in `docs/design/`. (A language-type-level proposals
       doc exists; the IR/addressing model itself is still open.)
2. [ ] Hand-prototype the "one instruction per block" encoding for a trivial
       loop; verify tail-call jump conversion behaves as documented (check
       against `vdu_buffered.h`, not just the prose docs).
3. [ ] Verify the 1x1-matrix float trick (command 34 `invert`) on real
       hardware/emulator.
4. [x] Decide on the host implementation language and toolchain layout for
       the compiler itself. Resolved: C++, ordinary host compiler, Linux
       first — see
       [docs/devlog/2026-07-28-compiler-toolchain.md](docs/devlog/2026-07-28-compiler-toolchain.md).
5. [ ] Codex is separately working, at our request, on reproducing and
       fixing a stock-MOS packet-parser discard bug found during the
       collaboration (see `docs/devlog/to_codex.md` §9) - not a Golem
       compiler task, but track for a PR/bug-report outcome.
