# Project Handoff

Read `/home/smith/Agon/mystuff/agon-dev-env/codex/AGENTS.md` first — it owns
reusable environment, workflow, and repository conventions shared across all
Agon projects. This file contains only Golem-specific information.

## Purpose and current status

Golem is a compiled, low-to-medium-level, C/Pascal-like language targeting the
Agon Console8 VDP's **Buffered Commands API** directly (rather than the eZ80
via MOS). See the root [README.md](README.md) for the elevator pitch.

Current maintained portable profile (GOLEM-040): `build/golemc --portable`
emits stock commands and native C through shared typed syntax/state/root/layout.
Supports accepted 16/32-bit integers, float32, bool, checked arithmetic/conversions,
If/ElseIf/Else, finite Range loops, paired division and integer arrays/right shift.
Read `docs/design/portable-compiler.md` for exact limits and assembly ownership.
The promoted code and required tests have no task implementation dependency.
`make -C src check` uses the host C++ toolchain only; SDK/generated-C/assembler
checks are optional separate integration commands. GOLEM-040 implementation
was accepted and explicitly approved for commit/push on 2026-10-14, with
unchanged accepted output bytes. No emulator run or new package deployment was
needed for this promotion. Older summaries below describe legacy profiles.

Status: **bounded implemented profiles**, with a maintained compiler in `src/`.
Raw mode implements literal print, uint16 declarations/assignment/addition and
For/Next. GOLEM-008 promotes the qualified scalar backend and explicit resident
output mode: one bounded float32 floor-to-sint16 job, with optional persistent
uint16 globals. See `docs/design/resident-scalar.md` for the supported grammar,
package/host contract and initialized examples. Broader expressions and
If is available in portable mode; Do/Switch remain proposed.
`make -C src check` runs maintained contract tests.

GOLEM-012 adds checked resident uint16 addition, per-invocation error status and
a finite overflow exit preserving the failed destination. Host/sanitizer checks
and stock Fab/MOS qualification pass. The Author confirmed Oryx human review
worked; the Author subsequently explicitly approved commit and push.
See `docs/tasks/GOLEM-012/README.md` for evidence and exact review commands.

GOLEM-013 adds resident uint16 subtraction and mixed left-to-right + / - chains.
Underflow preserves the failed destination and exits with status 2. Automated
host/sanitizer/stock-emulator qualification and human Oryx review pass. The Author
approved commit/push as 964d546. See `docs/tasks/GOLEM-013/README.md`.


GOLEM-007's stock Fab 1.2.5/MOS 3.0.2 qualification and Oryx keyboard review
passed. The Author explicitly approved committing it and promoting the scalar
slice. GOLEM-008 verified identical reviewed command bytes and recorded host
checks under its silo. Historical task artifacts remain intact. See `TODO.md`
for active work. Design work
has moved beyond the kickoff devlog too: `docs/design/` now holds a living
design-proposals doc, and several follow-on devlogs (toolchain decision,
execution-lifecycle correction, VDU wrapper library writeup, the golemc
milestone) plus a cross-agent collaboration thread with an external
"Codex" agent have been added. See "Repository layout" and "Session-start
reading order" below for where everything now lives; treat `docs/design/`
as the most current design reference where it overlaps with older devlog
material.

GOLEM-014 adds checked resident uint16 multiplication with standard precedence
and status 3 on overflow. Shared bounded VDP helpers keep installation traffic
small. Host/sanitizer/stock-emulator and Oryx human review pass. The agent retrieved
matching saved results. The Author subsequently explicitly approved commit/push.
See `docs/tasks/GOLEM-014/README.md`.

GOLEM-015 adds paired uint16 `(q, r) = divmod(n, d)`. Arguments evaluate once,
left to right; division by zero preserves both outputs and exits with status 4.
Host/sanitizer and stock Pi/Oryx headless qualification pass (2,126 checks,
156 jobs). The Author confirmed Oryx human review succeeded; the agent retrieved
and independently verified all saved results. The Author explicitly approved commit and push. See `docs/tasks/GOLEM-015/README.md`.

## Assembly integration direction — 2026-10-12

The intended use is PC-compiled VDP buffers invoked by an eZ80 assembly
application. Assembly owns loading, installation, updates, calls, pacing and
cleanup. The compiler publishes assigned buffer IDs, offsets, layouts and entry
points; it does not place data on hardware. Stock VDP cannot ask MOS to load a
file. AGNB loading belongs to assembly, with filenames and embedded IDs resolved
by the build. Current Table/Array review samples describe storage without
ImportBinary; configured data must be installed before access.

The Author favors direct generated assembly output, as used in their toolchains:
symbolic equates plus labeled VDP command/data bytes and lengths, generated from
the same layout as the manifest. GOLEM-016 implements this for current resident
packages via --resident --emit-asm, producing golem.inc. A source Sub tick exports
call golem_tick; its wrapper transmits the six-byte VDP call without a completion
acknowledgement. Ten actual assembler consumers preserve all five prior artifacts.
The Author accepted this tranche; no new emulator fixture was deployed by it.
See docs/tasks/GOLEM-016/README.md.

GOLEM-017 now implements the separate resident-table profile: one flat
uint16/sint16 Record, one immutable zero-based Table and checked whole-record
selection, without a dummy float/floor. Build mapping --table-data NAME=PATH
supplies packed data; --emit-asm exports entry, field and table layout symbols.
Index failure preserves the destination and exits with status 5; status resets
per call. Host/model, stock Pi/Oryx headless and Oryx human review pass. The
agent retrieved and verified all 873 checks across 98 jobs (3,453 bytes). The
Author explicitly approved commit and push. See docs/design/resident-table.md and docs/tasks/GOLEM-017/README.md.
An interactive BASIC-like Golem application or standalone MOSlet is an optional
future frontend, not a prerequisite or current priority. See the maintained
resident contract and GOLEM-010 review documents.

## Human review workflow

GOLEM-018 implements a standalone resident drawing profile: initialized
uint8/sint16 packed globals, one finite Sub, draw_point/draw_line/fill_quad and
matching literals. Named assembly exports include offsets and uint8 type tag 5.
Host/sanitizer, prior-profile parity, real assembler and stock Pi/Oryx headless
pixel checks pass (5,704 direct/resident pairs; 12,589 saved bytes). The Oryx demo
was staged and stopped. The Author confirmed dir worked; retrieved human results
match all 12,589 bytes. Intentional cyan clipping geometry was explained with a
reference image; the Author confirmed it matches what they saw. Human visual
and keyboard review pass. The Author explicitly approved commit and push on 2026-10-12.
See docs/design/resident-drawing.md and docs/tasks/GOLEM-018/README.md.

The Author launches the staged Oryx test, follows its onscreen `dir` instruction,
closes the emulator and reports whether it worked. The agent then retrieves and
verifies the saved results over SSH. Do not ask the Author to open another
terminal or run the verifier; the verifier only reports numerical results and
must not repeat the guest's keyboard instruction. The emulator need not remain
running. A rerun should remove the previous results file first.

The visual test is part of the Author's review. Complete implementation,
qualification and Oryx visual staging in the same tranche before stopping for
review. Do not ask the Author to approve internal qualification/staging steps
separately or stop while the visual test is still unprepared.
Human demos must identify the Golem source/entry and distinguish assembly's
role on screen. Keep direct VDU comparisons and intermediate diagnostic scenes
in automated qualification; the human demo should show the actual Golem program
being reviewed without hidden extra bitmap drawing from the harness.

## Repository layout

GOLEM-019 adds plain draw_bitmap(bufferId,x,y) and uint16 fields to the resident
drawing profile. Initial/literal bitmap IDs must be reserved; host updates must
select reserved, loaded assets. Bitmap installation enables stock affine support
so transform-disable commands are consumed. Borrowed assets are excluded from
bootstrap/cleanup. The named four-image Wolf example installs in 262 bytes and
repeats in six wire bytes (23 after a full six-byte input update). Host/sanitizer,
previous-geometry parity and actual assembler checks pass. CHK-001 stock bitmap
qualification passes: 18,526 checks, 18,869 saved bytes, 7.299 seconds in the
final development-host run. Assembly loads the real AGNB through the unchanged canonical
API snapshot. Model/direct/resident pixels verify placement, alpha and clipping;
partial ID/position writes, active-transform reset and preservation after cleanup
pass. Oryx staging/headless verification pass and the emulator is stopped.
The final 512x384 screen centres native-size artwork above a separate text area
so dir output cannot scroll the images. The original combined visual/diagnostic
run was confusing and is superseded: prepare_demo.py/demo_guest.asm now call
review.golem once, label its four bitmap calls and run no direct bitmap draws or
intermediate scenes. This human demo passes 864 model pixel checks locally and
on Oryx. verify_review.py uses demo-expected.dat/demo-checks.json; the full
automated 18,526-check evidence remains separate. The Author accepted the
corrected static pictures; retrieved results match all 864 bytes. Explicit MOS
keyboard check is superseded by successful GOLEM-020 review; commit and push were explicitly approved on 2026-10-12.
See docs/tasks/GOLEM-019/README.md and the maintained drawing contract.

```
docs/
  devlog/     Dated development log entries (chronological, append-only)
  design/     Living design docs (amended in place, not dated)
reference/    Vendored/curated reference material (not a build dependency)
runtime/
  legacy-asm/ Hand-written ZDS/ez80asm-style eZ80 runtime library code
  agondev/    (placeholder) future eZ80/MOS-side loader/runtime via agondev
src/          Maintained toy C++ compiler and host Makefile
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
  buffer-command sequences without hardware. GOLEM-007's ignored project
  `.emulator/` profile uses a native build of official tag 1.2.5 in its own
  runtime directory; the reference checkout's inherited x86-64 binaries cannot
  run on the Pi. See the task README and canonical emulator guide.
- `/home/smith/Agon/mystuff/agon-dev-env/codex/bespoke-vdp-emulator.md` —
  canonical cross-project workflow (prepared by the external "Codex" agent)
  for running a *modified/bespoke* VDP firmware build natively inside Fab
  Agon Emulator (native `.so` module + `--vdp` loader contract, adapter/
  profile/snapshot conventions, fail-closed launch, `debug_log()` vs.
  `force_debug_log()` diagnostics). Not needed for Golem's current
  stock-firmware/stock-Buffered-Commands-API workflow, but is the reference
  to read first if Golem ever needs to add a new VDP command, verify
  undocumented firmware behavior via a live instrumented build (rather than
  just reading `vdu_buffered.h` statically), or otherwise hack on VDP
  source rather than only emit commands the stock firmware already
  supports.
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
  for the full provenance/collision analysis. `agon-utils/examples/flower/`
  is a real, working MOSlet with command-line argument parsing (argv/argc
  tokenizer plus verb-dispatch helpers); a vendored, not-yet-implemented
  copy lives at [reference/flower-moslet-cli/](reference/flower-moslet-cli/)
  as a prototype for the future MOSlet-based loader CLI described in
  `docs/design/language-type-proposals.md`'s "Axis 17".
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
   if skimming others),
   `2026-07-28-golemc-milestone-and-vba-syntax-question.md` (first working
   toy compiler, `src/golemc.cpp`, verified end to end in the emulator; plus
   a BASIC/VBA syntax-vs-semantics detour), `2026-07-29-emulator-debug-tooling.md`
   (VDP debug-output hooks, including a correction about `BUFFERED_DEBUG_INFO`
   being a no-op in the emulator as shipped), and
   `2026-07-29-loop-tail-call-prototype.md` (hand-prototyped loop +
   verified tail-call-to-jump conversion, plus a conditional-check gotcha
   found and worked around). `from_codex.md`/`to_codex.md` are a paired,
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

Human emulator reviews default to a dedicated local instance on Oryx Linux Lite,
unless the Author explicitly selects another host. The Pi has no accessible
desktop, mouse, keyboard or monitor. Do not launch graphical emulators for the
Author: stage and verify the Oryx instance, leave it stopped, and provide Bash
launch commands plus review instructions. Pi headless evidence does not satisfy
human review. See GOLEM-007 for the current review fixture.

A host compiler Makefile exists: `make -C src check` builds `build/golemc`
and runs maintained tests. Syntax is terminator-free; newlines are whitespace.
Raw mode is `build/golemc source payload`; resident mode is
`build/golemc --resident [--reserve FIRST:LAST ...] source directory [example-input]`.
Read `docs/design/resident-scalar.md` before using resident output. GOLEM-003/005/006
retain historical prototype and regression fixtures; production has no task
implementation dependency. GOLEM-008 owns promotion parity and sanitizer checks.
GOLEM-007 owns the current stock qualification script and evidence. Its ignored
project `.emulator/` profile uses a native ARM64 build of official Fab 1.2.5,
tagged MOS 3.0.2 and a bounded MOS/UART guest. From that directory launch only
`./fab-agon-emulator`; read the task README before rerunning or changing it.
The compiler's
host language and toolchain layout are decided
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

Language convention revised 2026-10-11: no statement terminators; newlines
remain whitespace, and the parser determines statement boundaries from grammar.
See design Axes 5 and 15. GOLEM-005 implements this for maintained ordinary
statements and its task-local scalar successor. Historical GOLEM-003 fixtures
and frozen Rally source retain semicolons; do not migrate them in place.

Summarized from the devlog and cross-checked against
[docs/design/language-type-proposals.md](docs/design/language-type-proposals.md)
for anything still open (prefer that doc over this summary where they
overlap):

- Author preference, 2026-10-11: multiple commands per block, with one or more
  blocks per buffer; small procedures may use one block. This replaces the
  early one-instruction-per-block proposal as the preferred direction. Exact
  grouping and branch-address rules remain open; see GOLEM-001's review notes.
- Earlier proposal, not implemented: typed three-address-code IR with widening passes for multi-byte
  integer ops (add-with-carry chains) and explicit "patch, then execute"
  pseudo-ops for any runtime-computed address (arrays, pointers, stack-frame
  locals) — the VDP has no indexed/pointer addressing mode.
- Current ordinary globals use fixed per-variable buffers with checked buffer
  allocation. Raw mode preserves accumulator ID 1 and allocation from 2.
  GOLEM-006 injects the same allocator into ordinary and scalar emission, with
  external reservations before compiler claims; IDs 0 and 65535 are reserved.
  Installed ownership/cleanup and an explicit resident mode are task-local.
  Shared local-stack
  frames, heap objects/arrays and frame-relative patched offsets are earlier
  proposals, not implemented requirements for the small scalar integration.
- Loops are preferred over recursion. Main's For body uses one initial call
  and a trailing conditional self-call converted to a jump, avoiding per-iteration
  stack growth. Tail-call conversion avoids an additional call frame; it does
  not eliminate command-dispatch work. Use it for recursion-shaped code wherever
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

## Active work and historical follow-ups

`TODO.md` is the authoritative register. GOLEM-003's scalar prototype is
promoted through GOLEM-008 after GOLEM-007 qualification. GOLEM-004's review was
accepted for the GOLEM-005 parser tranche, now accepted at its commit/push
checkpoint. GOLEM-006 was accepted at commit/push checkpoint `76458a9` and delivers
shared checked allocation plus an explicit experimental resident output profile.
GOLEM-007 supplies stock-emulator qualification, separate from the host model.
Human emulator validation passed on Oryx and the Author explicitly approved
committing that tranche and proceeding with scalar/profile production promotion.

The July devlogs' next-step checklists are historical context, not another
active register. They recorded formal IR/addressing work, reciprocal experiments
and the external MOS packet-parser investigation. The host toolchain decision
and hand-built loop/tail-call prototype were resolved in those logs. Check the
current task register and later evidence before reviving an old follow-up.

## Complexity budget

The Author emphasizes this is a hobby project. Keep useful runtime bounds and
admission checks simple and proportionate. CPY-001 was accepted on that basis:
validate changing external inputs, then reuse established validity for trusted
resident copies while it remains true. Checked copying is source-value admission,
not post-copy byte verification. Do not add readback, checksums, redundant checks
or rollback machinery for this feature. No elaborate optimizer is required.

## Task documents and artifacts

Code samples must include the explicit typed declarations and initial state
required to understand or use each sample independently. Do not rely on another
sample's variables or imply declaration through assignment or a loop header.
An iteration header may supply the initial value of an already-declared counter.

`TODO.md` is the authoritative task register; keep entries concise and link
each detailed task plan at `docs/tasks/<TASK-ID>.md`. For tasks producing
multiple files, keep reports, exploratory code, scripts, fixtures and other
task artifacts together under `docs/tasks/<TASK-ID>/`. Promote useful artifacts
into maintained documentation or the production package only when that
promotion is agreed. Task plans contain scope and acceptance criteria; task
overall status belongs in the register. Track subtasks in the detailed task
document using stable IDs with short subsection-specific prefixes and
zero-padded numbers (for example, `SRC-001`, `INV-001`, `DEL-001`). Place the
checkbox after the ID: `- SRC-001 [ ] ...`. Use `[x]` for completed or otherwise
disposed subtasks and record the disposition when it is not completion. Never
renumber existing IDs or use Markdown ordered-list numbers for subtask identity.

## Network access authority

For access between development machines, read the [canonical network instructions](https://github.com/bgates747/agon-dev-env/blob/feature/standardized-deployment/codex/environment.md#ssh-between-development-machines) in `~/Agon/mystuff/agon-dev-env/codex/environment.md` and its machine-local `.local/ssh-001/CONNECTIONS.md` guide. They own current hostnames, discovery, SSH identities, sudo, and LAN shares. Do not duplicate access recipes or revive old aliases, addresses, or share mappings in this project. Project-specific device/protocol and bench-safety instructions remain local.

## Wolf animation — GOLEM-020

The Author requested animation and suggested vblank as the timer. The finite
600-frame demo uses existing Golem calls only: animation.golem clears and draws
each frame. animation.asm loads assets once, reflects three sprites at the
playfield edges, updates twelve position bytes, calls golem_drawFrame and sends
the stock VSYNC swap. This is the sole pacing clock; no additional host-side
vblank wait or completion packet. The 320x240 64-colour double-buffered mode136
returns to single-buffered mode8 before MOS. Labels identify both source roles.
The resident drawing profile still has no assignments/branches; do not imply
that movement arithmetic runs in Golem. All artifacts live in docs/tasks/GOLEM-020/.
GOLEM-020 local and Oryx qualification pass 841 checks/946 bytes. The dedicated
Oryx visual fixture passed human animation and MOS keyboard review. Retrieved
human results match all 946 bytes and the staged runtime/fixture identities.
The emulator is stopped. The Author explicitly approved commit and push on 2026-10-12.

## Experimental Fab/Flexe and vdplets — GOLEM-021

The Author authorized an eight-hour implementation goal on 2026-10-12. The owned
Fab 1.2.5 frontend now runs actual ESP32 firmware through a Flexe VDP adapter on
Pi and Oryx. Runtime-uploaded C/C++ numeric and packed-renderer Xtensa modules
pass both engines; Oryx's final main fixture passes 56 replies/560 bytes and
61,440 image/margin pixels, with an actual return to MOS. This needs the generic
vdplet-enabled firmware once; stock VDP alone has no such loader. Subsequent
modules need no firmware rebuild. No physical target was flashed.

The agon-6502 owner accepted the buffer/frame ABI and supplied its own F2/F3
module. Its unchanged package passes 36 fresh Oryx runs, 32,708 replies and
2,211,840 image/margin pixels, with eight complete retained-buffer observations.
The evidence was returned through the canonical agent mailbox; source and
hardware ownership remain with agon-6502. See docs/tasks/GOLEM-021/README.md and
codec/README.md beneath that task silo. GOLEM-002 owns loader/service source.

The main human profile is staged and stopped on Oryx at
`~/Agon/mystuff/golem/docs/tasks/GOLEM-021/.emulator`; launch only its generated
`./fab-agon-emulator` wrapper. The separate codec profile is under codec/.emulator.
Do not overwrite the accepted root .emulator profile. Keyboard uses the VDP
serial console, not physical PS/2 ULP emulation. Timing is functional, with
possible frame coalescing; native modules are trusted and must return.
Human review remains pending. The Author explicitly authorized committing and
pushing all pending work on 2026-10-12; this does not claim human acceptance.

## Multiple resident drawing Subs — GOLEM-022

The drawing compiler accepts multiple finite named Subs sharing initialized
packed globals. Assembly chooses each generated `golem_<SubName>` wrapper;
first Sub remains the default invoke/update-call target. Single-Sub schema and
all 120 compared legacy package files are unchanged. Multi-Sub manifests add
ordered entries. There are still no drawing assignments, locals, inter-Sub calls,
branches, loops or implicit scheduler. Keywords/builtins retain their existing
case insensitivity; variable/procedure identifiers retain case sensitivity.

Host contracts (132 rejection cases), ASan/UBSan, real assembler consumers and
stock Oryx default/relocated checks pass. The recognizable Wolf/cyan-line human
fixture is separately staged and stopped under
`~/Agon/mystuff/golem/docs/tasks/GOLEM-022/qualification/.emulator`.
See docs/tasks/GOLEM-022.md and its qualification/README.md. Human acceptance remains pending. The Author explicitly authorized commit/push
of all pending work on 2026-10-12; that checkpoint does not claim human review.

## Native Golem research — GOLEM-023

The maintained hardware runtime/SDK has moved to `/Agon/mystuff/agon-vdplet`.
Historical task paths are compatibility links and builder adapters; keep future
generic runtime changes in that repository. GOLEM-023 researches approachable
Golem source compiled to native VDPlets, with generated C as the candidate route.
The first research tranche is complete. The Author requires common source,
semantics and storage for buffered/native targets, including application/end-user
runtime selection; see `docs/design/backend-portability.md`. GOLEM-024 supplied
a task-local shared scalar frontend and layout prototype. GOLEM-025 adds bounded
executable integer emitters; physical deployment remains later work. See the detailed
task plans and authoritative `TODO.md`.

GOLEM-024's task-local shared scalar frontend/layout prototype was accepted.
It supports initialized uint16/float32 globals, Bounds, named parameterless Subs
and typed + / - / * assignments. Both selections produce identical common
interface/typed-program/state/assembly-layout artifacts for the same reservations.
Host ASan/UBSan (44 checks/42 rejections), ten CLI rejection cases, two real
assembler consumers and maintained compiler checks pass. This is not an emitter
or a production promotion: no native C, VDPlet or buffered executable is generated.
See `docs/tasks/GOLEM-024/README.md` and the acceptance devlog dated 2026-10-13.

## Shared integer backends and visual review — GOLEM-025

Task-local checked uint16 emitters compile the GOLEM-024 typed model into stock
buffered commands and SDK-packaged native C. Initialized globals, Bounds, finite
named parameterless Subs and + / - / * assignments are supported. Float output
and broader syntax remain rejected. Both packages use identical packed state,
error behavior and entry order with disjoint private ownership. Fresh native
handles are explicitly bound to resident wrappers. Native unload and private
cleanups preserve shared state. Production compiler and generic SDK/runtime code
are unchanged.

Host ASan/UBSan and model/generated-C checks pass 218 cases. Stock Oryx VDP
qualifies arithmetic and meter source; actual ESP32/Xtensa JIT and interpreter
qualify both backends, relocated IDs and failures. The 600-frame review alternates
buffered cyan and native orange along one continuous moving-square/meter path.
Golem computes coordinates; resident VDU commands draw; assembly selects/paces
with stock VSYNC. All 56,320 final playfield pixels, periodic full-state checks,
cleanup retention and actual MOS return pass. The Author accepted the visual
tranche and authorized commit/push on 2026-10-13; retrieved human observations
match all 4,729 bytes. Keyboard success was not separately stated.

The dedicated Oryx review is staged/stopped at
`~/Agon/mystuff/golem/docs/tasks/GOLEM-025/.emulator`. Launch its generated
`./fab-agon-emulator` wrapper only. The Author watches, types dir when instructed
onscreen, closes and reports; the agent retrieves/verifies saved results. Source
is deployed beside the report. The Author explicitly approved committing and pushing this tranche.
See `docs/tasks/GOLEM-025/README.md`.

## Portable comparisons and structured branches — GOLEM-026

The task-local GOLEM-026 fork adds uint16 == != < > <= >= conditions and
If/ElseIf/Else/EndIf to both emitters. Conditions evaluate once left to right;
unselected tests/bodies are skipped. Eight nested If levels and 64 ElseIf arms
are the current limits. Bool storage, signed/float comparisons, Boolean
composition, loops and source drawing remain outside this slice.

Buffered comparisons use zero-extended bytes, including full 65535 values.
Straight-line statements share blocks; tail jumps connect branches and joins so
arithmetic/Bounds failures exit the entire invocation. Native C follows the same
typed tree and shared-state/error contract. Historical prototypes, production
compiler and generic SDK/runtime are unchanged.

Host/sanitizer checks pass 2,236 cases, eight grammar/reference acceptances and
26 rejections; native ELF and maintained checks pass. Stock VDP and actual Xtensa
JIT/interpreter qualify conditional failures, skipped work, joins and relocated
IDs. The 600-frame visual now uses one tick Sub: Golem chooses direction, while
assembly selects backend/draws/paces. The meter layout is 20 bytes; install it
explicitly rather than reusing GOLEM-025's 18-byte state. Both new backends share
that layout. Full-state observations and all 56,320 final playfield pixels pass.

Human review is staged/stopped on Oryx at
`~/Agon/mystuff/golem/docs/tasks/GOLEM-026/.emulator`. Source is beside that
profile. Launch only the generated `./fab-agon-emulator` wrapper; follow the
existing single-terminal review protocol. The Author explicitly approved
commit/push on 2026-10-13 and accepted the visual review;
retrieved human results match all 533 observations/5,249 bytes. Keyboard success
was not separately stated. See `docs/tasks/GOLEM-026/README.md`.

## Portable bounded Range loops — GOLEM-027

The task-local GOLEM-027 fork adds For counter In Range(start:end:step) / Next
with optional matching counter name. Counters are initialized uint16 globals;
endpoints and signed step are integer literals. Default step is 1; reject zero
or wrong-direction steps. PC-computed trip/last values are inclusive and admit
all visited counter values against Bounds. The body cannot assign an active
counter; nested loops use distinct counters. Limits are eight nested loops and
262144 worst-case statement/condition/iteration visits per entry. The interface
publishes worst_case_visits; typed loop nodes publish trips and last.

Buffered loop bodies are not unrolled. Jump-connected header/body/latch blocks
stop before the final increment, preserving 65535 and descending zero without
wrapping. Compile-time validity permits scratch-counter advancement without
redundant runtime admission. Body/condition failure retains the current counter
and earlier effects, then exits the whole invocation. Native C shares those
semantics. Historical tasks, production compiler and generic SDK/runtime are unchanged.

Host ASan/UBSan/model/generated-C checks pass 2488 cases, 22 grammar/reference
acceptances and 51 rejections, plus ELF and maintained checks. Stock VDP and
actual Xtensa JIT/interpreter qualify full ascending/descending uint16 traversals,
nested loops/branches, failures, relocated IDs and cleanup retention. The initial
hybrid full traversal exceeded the inherited 240-tick guest reply timeout; the
task's qualification guest now uses a 24-bit 12000-tick bounded wait. Both engines
then pass. This changes no runtime/SDK behavior; diagnostic evidence is retained.

The 600-frame review performs 2400 simulation steps using four loop iterations
per tick call. The 22-byte state adds batch before error; install it explicitly.
State checks and all 56320 final playfield pixels pass. Human review is staged
and stopped on Oryx at `~/Agon/mystuff/golem/docs/tasks/GOLEM-027/.emulator`.
Source is beside the profile; launch its generated wrapper only. Follow the
single-terminal review protocol. This tranche remains uncommitted pending human
review and explicit commit approval. The Author subsequently reported that the
review worked; retrieved results identify GOLEM-027 and match all 585 observations/
5769 bytes, including 2400 steps. Human acceptance is recorded; only explicit
commit/push approval was subsequently given on 2026-10-13. See `docs/tasks/GOLEM-027/README.md`.

## Typed Boolean prototype — GOLEM-028

The task-local GOLEM-028 fork adds bool globals with true/false initializers,
Boolean copying, all six uint16 comparison assignments and Boolean If/ElseIf.
Bool uses a canonical two-byte 0/1 representation and assembly type tag 6;
invalid external encodings fail entry admission with status 32. Numeric/Boolean
coercion, Boolean arithmetic and compound Boolean operators are unsupported.
Checked comparison failures preserve their Boolean destination and earlier
effects. Both emitters share the same typed expression tree and state layout.

Host ASan/UBSan/model/generated-C checks pass 4804 cases, 24 grammar/reference
acceptances and 64 rejections, plus native ELF and maintained checks. Stock VDP
and actual Xtensa JIT/interpreter checks pass, including relocated IDs, invalid
flags, selected/skipped failures and cleanup retention. Production compiler and
generic runtime/SDK are unchanged.

The visibly distinct traffic-light review uses lights.golem. Golem computes and
copies Boolean flags, then chooses lamp colours through If flag; resident VDU
draws, and assembly selects/paces. The 20-byte layout must be installed explicitly.
All 56320 final pixels, 533 observations/5249 bytes and actual MOS return pass.
The Oryx profile is staged/stopped at
`~/Agon/mystuff/golem/docs/tasks/GOLEM-028/.emulator`. Source is beside it. Follow
the single-terminal human protocol; launch its generated wrapper only. The Author explicitly approved commit/push on 2026-10-13 and accepted
the visual review; retrieved human results match all 533 observations/5249 bytes.
Keyboard success was not separately stated. See `docs/tasks/GOLEM-028/README.md`.

## Compound Boolean prototype — GOLEM-029

The task-local GOLEM-029 fork supports typed Not/And/Or with parentheses and
precedence arithmetic, comparison, Not, And, Or. And/Or short circuit left to
right. Skipped arithmetic cannot fault; selected failures preserve destinations
and earlier effects, then halt. Buffered helper calls explicitly propagate
arithmetic status so a nested failure cannot resume outer writes. Canonical bool
layout, type tag 6, entry admission, ownership and named assembly calls remain
unchanged. Production compiler and generic SDK/runtime are unchanged.

Host sanitizer/model/generated-C checks pass 5,284 cases, 59 grammar/reference
cases and 74 rejections plus maintained checks. Target qualification and stopped
Oryx visual staging are documented in docs/tasks/GOLEM-029/README.md. The Author accepted human review; saved results pass all 533 observations/5,249
bytes. The Author explicitly approved commit/push on 2026-10-13. The traffic-light appearance
is intentionally unchanged; lights.golem now expresses its decisions with the
new compound operators. Use its task-specific profile and single-terminal review.

## Paired integer division prototype — GOLEM-030

The task-local GOLEM-030 fork implements `(q,r)=divmod(n,d)` on both backends.
Arguments evaluate once left to right, before either distinct uint16 destination
changes; input/output aliasing is supported. Zero divisor reports status 4;
arithmetic or either destination Bounds failure preserves both outputs and prior
effects, then stops. Active loop counters cannot be destinations. The buffered
backend reuses the maintained qualified division kernel; native uses integer
quotient/remainder. Production compiler and generic SDK/runtime remain unchanged.

Host ASan/UBSan, command model/generated-C, ELF and maintained checks pass
9,104 cases, 66 grammar/reference cases and 82 rejections. Target qualification
and visual staging are recorded in docs/tasks/GOLEM-030/README.md. lights.golem
uses quotient for phase and remainder for green blinking; state is now 24 bytes
and must be explicitly installed. Use its separate Oryx profile and the normal
single-terminal review protocol. The Author accepted visual review; retrieved
results pass all 637 observations/6,289 bytes and runtime identities pass.
The Author explicitly approved commit/push on 2026-10-13.

## Signed integer prototype — GOLEM-031

The task-local GOLEM-031 fork adds sint16 globals/literals with inclusive Bounds,
checked + / - / * arithmetic, all six same-type comparisons, and explicit
sint16(...)/uint16(...) conversions. Two-byte two's-complement storage uses type
tag 2. Mixed operations require a conversion. Negative-to-unsigned or unsigned
above 32767-to-signed conversions fail with status 32. Arithmetic overflow uses
operator statuses 1/2/3; failed destinations and prior effects follow the existing
contract. For counters and paired divmod remain uint16; use 0 - value for checked
variable negation. Production compiler and generic runtime/SDK remain unchanged.

Signed buffered comparisons flip the sign bit for unsigned byte ordering;
addition/subtraction inspect sign changes, multiplication uses checked unsigned
magnitudes and signed result limits. Bounds failures now set private status as
well as public status, allowing conversion errors to propagate through logical
helpers before outer writes resume. Native arithmetic uses checked int32
intermediates and packs only validated results.

Host sanitizer/model/generated-C, ELF and maintained checks pass 14,912 cases,
75 grammar/reference cases and 93 rejections. Target qualification and Oryx
visual staging are recorded in docs/tasks/GOLEM-031/README.md. motion.golem owns
signed velocity, displacement, position, screen conversion and colour. Its state
is 18 bytes; the square crosses a gray origin marker, red when negative, green
otherwise. Use its dedicated Oryx profile and single-terminal review protocol.
The Author accepted signed-motion review; saved results pass 481 observations/4,729
bytes and runtime identities pass. The Author explicitly approved commit/push on 2026-10-13.

## Float32 prototype — GOLEM-032

The task-local GOLEM-032 fork adds float32 + / - / *, all six comparisons and
explicit conversions to/from uint16/sint16 on both backends. Four-byte packed
storage uses tag 3, including unaligned fields. Native/host builds disable
contraction. Inputs admit finite normal values and signed zero; non-finite or
subnormal globals fail with status 33. Subnormal literals are rejected. Arithmetic
subnormals flush to positive zero; expression zero is canonical positive zero.
Non-finite results report 33. Float-to-integer conversion truncates toward zero
and checks range before any potentially unsafe cast; failure reports 32.
Inclusive Bounds account for thresholds between representable float values.

Stock arithmetic uses scalar matrices and enables the existing affine feature
flag. Conversion widens signed/unsigned words into freshly reset signed-32-bit
scratch, or checks a float's domain and transforms only its nonnegative magnitude
before restoring the integer sign. Entry admission is shared across named Subs;
errors propagate before source effects. Production compiler and generic SDK/
runtime remain unchanged. Division/sqrt/pow/floor are outside this tranche.

Host ASan/UBSan, independent command model/generated C, ELF and maintained checks
pass 24,494 cases, 84 grammar/reference cases and 101 rejections. Stock and both
actual Xtensa engines pass representative numeric edges including rounding,
underflow, decimal Bounds and conversion limits; see docs/tasks/GOLEM-032/README.md
for receipts and human review status. drift.golem uses fractional position and
0.75-pixel displacement, owns reversals, casts and colour. Its shared state is
24 bytes. The dedicated guest's script allocation is 128 KiB in free memory;
individual packets remain bounded to 65535 bytes. The Author accepted review;
saved results pass 637 observations/6,289 bytes and runtime identities pass.
The Author explicitly approved commit/push on 2026-10-13; no physical target was flashed.

## Accepted Rally planning contract — GOLEM-033

Rally accepted Golem's eight-clause R25-G0 v0.2 counterproposal without amendments
in mailbox message 2026-10-13_15-12-37_rally_rally25-contract-v02-accepted-five.
See docs/tasks/GOLEM-033/accepted-contract.md and its immutable receipts. GOLEM-033
negotiation is complete; the Author requested task documents and a stop for review.
That planning document did not release implementation. The Author subsequently
released GOLEM-034 at high effort; later firmware/capacity work remains gated.

GOLEM-034 plans D0 baseline/artifacts/root ABI; GOLEM-035 plans D1a bounded arrays;
GOLEM-036 plans D1b faithful wide math; GOLEM-037 plans D2 helper/renderer linking;
GOLEM-038 plans D3 firmware ownership, scheduler, snapshots and event design.
TODO.md owns their overall status. The Author requested committing accepted
GOLEM-032; it is pushed at 25901e52eb80a4d66847c11fbc70d1bac68ee633 and
supersedes 031 as the delivered baseline, with Rally verified acceptance. The
original counterproposal/acceptance preserve historical planning provenance.
Sparse-input time alignment and recovery remain Author decisions before D3.

## Pinned Rally scalar/interface delivery — GOLEM-034

D0 exports the exact GOLEM-032 six-file compiler closure from 25901e5 and adds a
separate task-local root/interface layer; production src and the generic SDK are
unchanged. R25-G0/root-v1 was accepted by Rally without amendments. Its packed
112-byte header separates interface/language/host-owned runtime results and
last-success generation/entry/session. Fixed build-time descriptors bind retained
input/event/table/asset-directory buffers. Native validates actual root/ranges
with at most three live leases; stock requires the host-installed storage/lifetime
invariant. Every named assembly call still sends six wire bytes. Native handle
patch offset is generated (currently 24); do not reuse the old 11.

The five-name scalar probe is not Rally gameplay or an autonomous scheduler.
Generation counts successful invocations modulo 2^32, not simulation steps.
Source failures retain earlier effects without publishing new success markers.
All source globals are admitted at entry, including Init/Reset: invalid external
state needs host repair/reinstallation before those entries can run.

Host ASan/UBSan/model/generated-C checks pass 2,270 cases/seven CLI rejections;
rebuilds preserve 12 executable/layout files per package. An additional regression
retains signed/float/bool types, an unaligned float and unbound descriptors. Stock
and actual Xtensa JIT/interpreter pass default/relocated root admission, physical
native failures and cleanup checks. The 600-frame source-labelled review exercises
all five Subs, including a pause/reset/restart, passes 1,115 observations/11,069 bytes
and 56,320 playfield pixels, and returns to MOS. Immutable delivery identity is in
docs/tasks/GOLEM-034/delivery.json; source, instructions and receipts are linked
from its README.md. Rally has the delivery for its independently owned adapter.

Oryx review is staged/stopped at ~/Agon/mystuff/golem/docs/tasks/GOLEM-034/.emulator.
The Author launches ./fab-agon-emulator there, types dir when instructed onscreen,
closes and reports. Agent retrieves/verifies saved results. Human acceptance and
explicit commit/push approval remain pending; no physical target was flashed.

GOLEM-034 human review subsequently passed: the Author reported it works;
retrieved results match all 1,115 observations/11,069 bytes and runtime/fixture
identities pass. See evidence/human-review.json beneath the task. Explicit
commit/push approval remains pending.

The Author subsequently explicitly approved committing/pushing all pending work
and proceeding to GOLEM-035. GOLEM-034 is accepted and removed from TODO.

## Retained typed arrays — GOLEM-035

The task-local D0 successor adds one-dimensional Array uint16/sint16 storage,
inclusive ascending numeric bounds, typed name(index) reads/writes, ReadOnly
permissions and explicit Fill/Values or exact build-supplied packed initialization.
Each array has a generated application-owned buffer; optional manifest descriptors
publish width/bounds/access/ownership/initial-data identity. Root-v1 is unchanged.
Native preflight validates actual shape; indexed views are released before the
next access, for root+arguments+one-array peak 3. Stock relies on the established
installation/lifetime invariant. Both private cleanups and native unload retain
root/arrays. No per-call array scan or runtime readback verification is added.

Stores check/evaluate index before RHS, then write once. Logical index failure is
language 5, preserving the failed element/destination and adjacent data; arithmetic
codes and prior effects remain. Scalar Bounds/conversion rules still apply. The
initial single-block policy is 32767 two-byte elements, bounds -32768..65535;
float/wide/multidimensional arrays and paired array divmod outputs remain outside
this slice. Six-slot pixel motion does not qualify wide Rally world units.

Host ASan/UBSan/model/generated-C checks pass 2,838 array cases, 866 Rally/six-slot
cases, eight destination/conversion checks and 23 grammar/build rejections. Scalar
executable/layout bytes match D0. Stock validates all 257 Rally sine values, all
six route records and 60 motion steps. Actual Xtensa JIT/interpreter default and
relocated cases pass 2,046 observations/20,460 bytes each. Both engines' 600-frame
review passes 1,302 observations/12,912 bytes, 56,320 final playfield pixels and MOS
return. The complete source export rebuild and three package/storage reproducibility
checks pass. Generic SDK/runtime and production src remain unchanged.

Oryx review is staged/stopped at ~/Agon/mystuff/golem/docs/tasks/GOLEM-035/.emulator.
Read animation.golem: Golem owns six positions/velocities/phases/colours and reads
Rally's speed/sine arrays; assembly draws/paces VSYNC. Backend changes retain state.
A frame needs 15 wire bytes (calculate 6, draw 6, swap 3), with no per-marker position
updates. Human review and explicit commit/push approval remain pending. See
docs/tasks/GOLEM-035/README.md and its immutable delivery.json.

Rally independently verified D0's complete bundle/adapter mapping and accepted
D1a storage without mismatch. Signed minimum/-1 division overflow status 6 is
agreed for GOLEM-036, not implemented here. GOLEM-037 records the received renderer
proposal and D2 recommendation to release the argument lease before helper entry,
so root+asset+output can fit planned three leases; D0/D1a still hold argument/root
through source execution. Concrete helper structs/grammar/error mapping remain open.

GOLEM-035 human review subsequently passed: the Author reported the tests look
good. Retrieved results match all 1,302 observations/12,912 bytes; staged runtime
and fixture identities pass. See docs/tasks/GOLEM-035/evidence/human-review.json.
The Author subsequently explicitly approved commit/push.

The Author subsequently explicitly authorized committing/pushing all pending
work and proceeding. GOLEM-035 is accepted and removed from TODO; the next
released tranche is the GOLEM-036 wide integer foundation.

## Wide integer foundation — GOLEM-036 first tranche

Task-local source now supports uint32/sint32 globals and four-byte retained arrays,
checked+/- and six comparisons, integer-only checked conversions and Bounds. Tags 7/8
preserve old layouts; no implicit operand widening. Wide multiplication/divmod/
shifts/float conversions reject explicitly and remain later reviewed work.
Four-byte elements admit 16,383 entries; indices remain uint16/sint16. Prior035
executable/layout/native-C bytes, including narrow float support, match exactly.

Host ASan/UBSan/model/generated-C 3,872 cases, direct Rally 8 arithmetic / 29 conversion
vectors, two widening-order and four maximum-array boundary cases pass. Stock
probes/motion and actual Xtensa JIT/interpreter default/relocated pass.600-frame
review passes590 observations/5846 bytes,56320 final playfield pixels and MOS return
on both engines. SDK policy/source and production src are unchanged. Immutable
packages/source/receipts and resources are in docs/tasks/GOLEM-036/delivery.json.

Oryx review is staged/stopped at ~/Agon/mystuff/golem/docs/tasks/GOLEM-036/.emulator.
Read animation.golem: Golem moves three stations around3.2million, signed Q8
laterals±38400 and colours; assembly draws/paces VSYNC. Red/green indicates lateral
sign, not horizontal direction. Caption alternates backend every 12 frames. The
Author launches the wrapper, follows onscreen dir, closes/reports; agent retrieves.
Human review and explicit commit/push approval remain pending. GOLEM-036 remains
open for subsequent multiplication/division/shifts/exact helpers/full Rally replay.

GOLEM-035 was accepted/committed/pushed as 93bf1e4 before this work. R25-RENDER v0.3
helper contract is accepted for D2 planning (exact header/proposal retained037;
private pointer-free context 464, separate completion 48). Proposed native binding
flag/hooks are not implemented. No GOLEM-037 implementation release or composed
resource/stack qualification is inferred.

GOLEM-036 foundation human review subsequently passed: the Author reported 'good test.' Retrieved results match all 590 observations/5,846 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/evidence/human-review.json. The foundation tranche is accepted; the Author subsequently explicitly approved commit/push. Multiplication/division and remaining GOLEM-036 work are still open.

## Checked wide multiplication — GOLEM-036 second tranche

Artifacts/sources are siloed in docs/tasks/GOLEM-036/multiplication/; accepted
foundation files/packages are preserved. Checked uint32/sint32 * has standard
precedence/evaluation order and language overflow 3. Buffered emission uses one
bounded 32-round shared full-product magnitude kernel with private carries and
per-expression snapshots. Native uses checked 64-bit intermediates of 32-bit values.
No source 64-bit type, wide divmod/shift/float cast or SDK policy change is added.

Host 1,682 model/sanitized-C cases, all six published Rally product vectors/default
and relocated, foundation byte parity and maintained compiler checks pass. Stock
probes/motion, both actual Xtensa engines/default-relocated and 300-frame pixels/
state/MOS return pass. Ordinary frame traffic remains 15 bytes. Human review is
staged under docs/tasks/GOLEM-036/multiplication/.emulator; launch only its wrapper.
Golem computes quadratic x and signed y; assembly draws/paces VSYNC. The Author
watches, follows onscreen dir, closes/reports; agent retrieves. Human acceptance
and explicit commit/push approval remain pending. Division remains next separate
tranche, without an implementation release in this checkpoint.

GOLEM-036 multiplication human review subsequently passed: the Author accepted it; retrieved results match all 460 observations / 4,546 bytes, with runtime/fixture identities verified. See docs/tasks/GOLEM-036/multiplication/evidence/human-review.json. The Author subsequently explicitly approved commit/push; division is a separate next tranche. Rally acknowledged the candidate identity and is planning its own bounded host qualification using generated metadata; this is not independent target acceptance or a shared ABI change.

## Paired wide division — GOLEM-036 third tranche

Artifacts/sources/profiles are under docs/tasks/GOLEM-036/division/, preserving
accepted foundation/multiplication. `(q,r)=divmod(n,d)` accepts distinct matching
uint32/sint32 globals and existing uint16 pairing; no signed16/array destinations.
Arguments evaluate once left to right, including input/output aliases. Signed
quotient truncates toward zero; remainder is n-q*d, numerator sign or zero. Zero
is language 4; minimum sint32/-1 overflow 6; either preserves both outputs. Both
results pass Bounds before either store; failure 32 preserves both and prior
effects remain. New metadata publishes signed-division overflow 6 when applicable.

One bounded 32-round stock long-division kernel uses private carry/remainder
storage and signed magnitudes/restoration. Native 32-bit division is guarded
before zero/minimum-overflow operations. Host 2,528 model/sanitized-C cases pass,
including 100 supplied Rally division vectors and 600 product-to-delta divisions
per layout. This does not qualify full route/contact/projection replay. Old
multiplication executable/layout/C artifacts retain exact bytes; SDK limits/source
and production src are unchanged. Stock and actual Xtensa/default-relocated pass.
One incomplete parallel default JIT run is preserved in evidence/hybrid-incomplete.json;
subsequent serialized JIT/interpreter both pass, with no established cause/fix.

The 300-frame division animation passes 488 observations/4826 bytes, 56,320 final
playfield pixels and MOS return on both engines. Source computes quadratic x/64
and signed y/8, with retained fractional remainders; assembly draws/paces VSYNC.
Ordinary frame wire remains 15 bytes. Oryx review is staged/stopped under the new
division/.emulator; the Author launches its wrapper, follows onscreen dir, closes/
reports, and the agent retrieves. Human review and explicit commit/push approval
remain pending. Wide shifts/exact helpers/full Rally replay remain later work.
Multiplication was accepted/committed/pushed as 1ddfd64 before this tranche.

GOLEM-036 division human review subsequently passed: the Author reported 'looks good.' Retrieved results match all 488 observations / 4,826 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/division/evidence/human-review.json. WID-002 is accepted; the Author explicitly approved commit/push. Wide shifts/exact helpers and full Rally replay remain open.

## Typed32-bit right shifts — GOLEM-036 fourth tranche

The Author directs Golem to supply language features/qualified packages; Rally
owns game-specific replay and checks. CHK-002 is disposed as delegated to Rally;
no game implementation/replay is authorized here. New artifacts/sources/profiles
are siloed in docs/tasks/GOLEM-036/right-shift/, preserving prior packages.

`value >> count` accepts uint32 logical or sint32 arithmetic/sign-extending
shifts with uint16 counts 0..31. Signed results round down; −65 >> 6 is −2. Invalid
constant counts reject; dynamic counts report 32 before store. Multiplication/
addition bind above shifts, comparisons below; shifts associate left to right
and require adjacent >>. Operands evaluate once left to right. Narrow/float/left
shifts and other bitwise operators remain deferred. Production src is unchanged.

One stock 256-byte half lookup and shared one-bit round execute at most 31 times,
with private carry/count and per-expression snapshots. Native uses unsigned
shifts and explicit sign filling, guarding zero/count width. Host 2,032 sanitized
C/model cases cover all counts/extrema/ordering/arrays and 20 supplied primitive
vectors per layout. Prior division executable/layout/C bytes are identical;
maintained checks, parser sanitizer and actual assembler pass. Stock and both
actual Xtensa/default-relocated qualify, run sequentially. The 300-frame review
passes 382 observations/3766 bytes, 56,320 final pixels and MOS return. SDK policy/
source remain unchanged; frame wire 15 bytes. Native animation text/data 3,108/268,
module static-frame sum 192 excluding service/firmware callers, peak 3 views.

Oryx review is staged/stopped under docs/tasks/GOLEM-036/right-shift/.emulator.
Golem moves three curves with unsigned lane shifts 2/3/4 and signed vertical >>2;
assembly draws/paces VSYNC. The Author launches its wrapper, follows onscreen
dir, closes/reports; agent retrieves. Human review/explicit commit-push approval
remain pending. Division and the shift plan/ownership fence were published as
df7a647 before implementation. Wider helper decisions/D2 remain separately scoped.

Rally reports its own independent foundation host qualification passes 2,924
generated-C cases and real exported metadata/adapter checks. Its package identity/
boundary ledger were read/acknowledged; Golem did not repeat peer game checks.

GOLEM-036 right-shift human review subsequently passed: the Author reported 'looks good'; retrieved results match all 382 observations / 3,766 bytes and staged runtime/fixture identities. See docs/tasks/GOLEM-036/right-shift/evidence/human-review.json. The Author explicitly approved commit/push after an empty mailbox check. Rally retains ownership of game-specific replay and validation.

## Renderer composition resource gate — GOLEM-037

The Author released the first D2 tranche after right-shift was published as
b32c596. FIT-001–003 are complete, with a measured resource failure returned for
review. Exact accepted Rally v0.3 helper plus minimal dispatcher uses 7,600 text/
864 data bytes and an actual 544-byte module call path (32+352+128+32), excluding
firmware/service callbacks. Tiny real generated Golem plus helper uses 8,224 text/
1,132 data and a 576-byte path; SDK rejects its text against 8,192. Declared stack
remains 512. Final builds have one public dispatcher and no PLT references;
8,272 from the initial multi-export diagnostic is superseded.

Task-local source/pins/reproducer/evidence are in docs/tasks/GOLEM-037/. No real
binding/marshaling/publication is implemented and no combined program has run or
been visually staged. SDK/firmware limits, Rally sources, production compiler and
profiles are unchanged. Do not deploy measurement fixtures: Golem's success
suffix runs before helper there. Next decision is Author review of SDK budgets
or a Rally-owned helper reduction; no capacity probe/change/split is authorized.
See docs/tasks/GOLEM-037/README.md. New work is uncommitted for review.

Rally route-kernels-twenty-four was subsequently read/acknowledged; all 20 manifest
files verified. Peer reports 319,072 generated-C host calls passing; Golem did not
rerun game checks or claim target qualification. Rally has read resource-gate-sixteen;
both are current and await the same Author resource decision. Latest exchange/read
status leads the canonical cross-agent/rally_golem/README.md; historical pending
receipt text below that summary is superseded. Fsim's separate hardware-speed/
Flexe-naming note was also read/acknowledged without starting implementation.

The Author subsequently reaffirmed approval to increase VDPlet limits. Do not
ask again whether to increase them or reduce Rally's helper. No numerical
replacement budgets are agreed or implemented yet. GOLEM-002 CAP-001–009 now
records further investigation of substantial single-module applications, Wolf-
inspired workloads, graduated capacity/lifecycle stress, stable generic graphics
services and upstream adoption. Current instruction is documentation only; no
new builds, stress campaign, SDK change, peer port or hardware work is started.
Keep future multifile investigation artifacts in GOLEM-002/capacity/. Runtime
changes belong in agon-vdplet, game code/replay with Rally/Wolf. Historic fit
receipts remain immutable and the pending resource-choice text above is superseded.

Rally twenty-six acknowledged the budget clarification; twenty-seven requested a
concrete capacity plan before execution. The proposed plan now lives at
docs/tasks/GOLEM-002/capacity/README.md, with staged ownership/resource/policy/glue,
host, sequential actual-Xtensa engines/layouts, stopped Oryx review and separately
authorized physical qualification. CAP-003 documentation is complete; no new
numeric budgets, builds, stress runs or runtime edits occurred. Sent exact plan
path/hash to Rally in capacity-plan-review-eighteen and stopped for Author review.
All new documentation remains uncommitted. Fsim also reported a missed general-
poll reply in its matched benchmark (message fsim-f027-general-poll-timeout-two);
read/acknowledged, diagnosis not started and no capacity/runtime cause established.

Rally twenty-eight reviewed the plan; twenty-nine relayed Author release of Stage A
only. Stage A is complete at docs/tasks/GOLEM-002/capacity/stage-a/RESULTS.md.
Actual unchanged Rally kernels/helper plus explicit resource-only scaffold measure
25,556/25,620 text,988 data,28,120/28,184 would-be image default/relocated; all21
peer generated artifacts per layout match. Resolved module path800, loose all-module
static frame sum2016; pinned processLoop stack4096 and selected one-wrapper outer
scenario736. Actual libheap object confirms shared executable D/IRAM aliases;
runtime free/contiguous/high-water and physical capacity remain unmeasured.

Proposal only: ceilings64KiB text/16KiB data/128KiB image/2KiB module stack;
initial budgets32KiB/4KiB/64KiB/2KiB. Keep task4096 with proposed2048 firmware/service
reserve, conditional on real D2 analysis/qualification. Existing u16 chunks plus
stock consolidate14 can supply one u32-sized LOAD block; no scatter loader needed.
Twenty repeated resource/inventory receipts are identical. No new runtime policy,
target/stress run, hardware action, helper edit or admitted package occurred.
Sent exact findings/hashes in capacity-stage-a-findings-nineteen and stopped for
Author review BEFORE Stage B. No commit/push authority was conveyed. All new work
remains uncommitted; reviewed capacity plan hash896e9bd4... is unchanged.

## Real composed renderer candidate — 2026-10-14

GOLEM-037 binding implementation and increased generic SDK admission are built.
Rally independently qualifies actual native simulation+renderer in one module
for 900 frames/3,600 steps in two modes, with matching final state and MOS return.
Host oracle/sanitizer/failure controls pass. Human profile is staged/stopped on
Oryx in AgonArcade/.emulator/rally25-native-demo; launch its generated
./flexe-agon-emulator. Rally owns retrieval. Human acceptance and explicit
commit/push approval remain pending. See binding/README.md under GOLEM-037.

GOLEM-037 human review and explicit commit/push approval were given on 2026-10-14. The bounded composition is complete; see the acceptance devlog. Older pending-review statements are historical.
