# Project Handoff

Read `/home/smith/Agon/mystuff/agon-dev-env/codex/AGENTS.md` first — it owns
reusable environment, workflow, and repository conventions shared across all
Agon projects. This file contains only Golem-specific information.

## Purpose and current status

Golem is a compiled, low-to-medium-level, C/Pascal-like language targeting the
Agon Console8 VDP's **Buffered Commands API** directly (rather than the eZ80
via MOS). See the root [README.md](README.md) for the elevator pitch.

Status: **early design/exploration**, with a maintained compiler in `src/`.
Raw mode implements literal print, uint16 declarations/assignment/addition and
For/Next. GOLEM-008 promotes the qualified scalar backend and explicit resident
output mode: one bounded float32 floor-to-sint16 job, with optional persistent
uint16 globals. See `docs/design/resident-scalar.md` for the supported grammar,
package/host contract and initialized examples. Broader expressions and
If/Do/Switch remain proposed. `make -C src check` runs maintained contract tests.

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
