# GOLEM-025 — Checked integer backends with a visible moving-meter demo

Compile the same small uint16 Golem program into stock buffered commands and
a separately uploaded native VDPlet. Both operate on the shared frontend's
packed application state with identical logical entries/error behavior. Show a
moving square and growing meter whose coordinates are calculated by that Golem
source. Assembly owns backend selection, drawing invocation and VSYNC pacing;
the display and source must identify those roles. The Author authorized this
tranche on 2026-10-13 after accepting the shared frontend foundation.

`TODO.md` owns overall status. Keep code, fixtures, source, evidence and review
tooling under `docs/tasks/GOLEM-025/`; reuse the accepted GOLEM-024 task-local
typed model and maintained byte-emission/arithmetic helpers. Production promotion
and firmware/runtime changes are not required. No physical flashing is authorized.
Leave emulator changes uncommitted until human review and explicit commit approval.

## Scope

Initialized uint16 globals, Bounds, parameterless named Subs, assignment trees
with + / - / *, and multiple named entries. Reject float source and unsupported
constructs for both emitters. Preserve common admission, intermediate checks,
failed destinations, earlier committed effects and per-invocation status reset.
Separate shared state ownership from private buffers; coordinate IDs so both
implementations can coexist without reinstalling or resetting state.

The buffered emitter adapts qualified integer algorithms to the packed layout.
Native output is generated C built by the separate SDK. A module entry receives
the shared state buffer ID through a resident descriptor. Named six-byte calls
invoke either resident buffered entries or resident RUN packets. Fresh handles
are bound explicitly after LOAD. No runtime capability probe or automatic failure
replay is introduced.

For visual review, move a coloured square diagonally and grow/shrink a horizontal
meter. Source grow/shrink Subs calculate phase, positions, extents and a tick count.
The resident drawing routine copies those calculated words into VDU templates;
the eZ80 does not calculate or read coordinates for animation. Backend choice
changes the square/meter colour and on-screen label. Use finite frame batches
with stock VSYNC swaps, then return to MOS with the single on-screen dir instruction.

## Subtasks

- EMIT-001 [x] Generate checked buffered assignment code from the shared typed
  model, with packed offsets, Bounds admission, finite failures and private cleanup.
- EMIT-002 [x] Generate native C using the same model/status contract; compile
  and package through agon-vdplet without changing its runtime or SDK sources.
- ABI-001 [x] Publish identical common data/interface artifacts, named per-backend
  entries, native descriptor/handle binding and disjoint private ownership.
- CHK-001 [x] Compare source reference cases against emitted buffered operations
  and generated C host tests; include boundary values, failures, precedence,
  state retention and relocated IDs. Run ASan/UBSan and maintained checks.
- CHK-002 [x] Execute buffered packages through stock VDP and both packages through
  actual uploaded-Xtensa emulation. Check shared state after calls, alternating
  execution and cleanup without reset. Save independent expected/result evidence.
- VIS-001 [x] Build the finite visual fixture, label Golem/drawing/assembly roles,
  verify representative pixels and state, and deploy source plus a dedicated
  stopped review instance on Oryx. Prepare the visual review in this tranche.
- REV-001 [x] Provide the Bash launch command and expected animation. Retrieve
  saved results after the Author closes/reports success; preserve pending human
  acceptance and explicit commit approval until received.

Acceptance requires actual executable artifacts for both paths, shared state
evidence and a staged visual demonstration. Host models alone do not establish
target execution. Emulator timing is functional; no physical speedup is claimed.

## Evidence

See [report, source and review command](GOLEM-025/README.md),
[host receipt](GOLEM-025/host-checks.json) and [target receipts](GOLEM-025/evidence/).
Both packages share one 18-byte state buffer. Stock VDP qualifies arithmetic and
meter source; actual Xtensa JIT/interpreter qualify both implementations,
relocated IDs, failures and the alternating visual fixture. No production
compiler or standalone SDK/runtime source changed. The Author accepted the visual tranche ("excellent") and explicitly authorized
commit/push on 2026-10-13. Retrieved human results match all 481 observations;
the keyboard result was not separately stated.
