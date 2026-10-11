# Golem continuation: review disposition and first implementation tranche

GOLEM-001 delivered the Rally capability comparison and readable-syntax
recommendation. The Author authorized proceeding with a modest implementation
tranche after recording a preference for buffers containing command sequences
in one or more blocks. The first promoted work is GOLEM-003: a task-local scalar
floor/conversion frontend over extracted Rally emission, with host verification
and explicit wire accounting. Production promotion and firmware execution are
not part of this first tranche.

## GOLEM-001 disposition

Completed investigation, accepted as the basis for a narrower incremental path.
Evidence is preserved in `docs/tasks/GOLEM-001/` and review considerations in
`docs/tasks/GOLEM-001.md`. The Author's priority is reduced recurring wire
traffic, with performance optimization deferred. Reuse selected Rally machinery
behind readable source, preserving a modest compiler budget. Longer command
blocks do not require smart optimization, and block boundaries are distinct
from calls and event-loop yields.

This disposition accepts that direction and the first prototype, not every
proposed type, grammar spelling, architecture or phase in the draft report.
Remove GOLEM-001 from the active TODO; retain its stable ID and evidence here.
GOLEM-002 remains queued for native-module research. GOLEM-003 is the immediate
continuation, ahead of that separate research task.

## Checkpoint and next steps

The Author requested committing and pushing all current dirt before proceeding.
That checkpoint includes the existing deployment wrapper/configuration, project
task conventions, GOLEM-001 investigation and review notes, GOLEM-002 research
plan, and the GOLEM-003 plan. The deployment files are included as requested;
they have not been deployed or validated on an emulator/hardware in this work.

GOLEM-003 will trace one useful calculation from readable source through a typed
emitter to resident stock-VDP commands. It will preserve Rally as an oracle,
verify matched command bytes and failure cases on the host, and count initial
installation separately from repeated input updates and calls. All new
experimental files stay in its task silo.

## GOLEM-003 first tranche result

The initial checkpoint was committed and pushed as `a8e3ff8` on
`feature/standardized-deployment`. The task-local C++ prototype now accepts
ordinary float/signed-word declarations and a finite named job containing
`result = sint16(floor(x));`. A typed backend, separate from both parsers,
extracts Rally's scalar load, true-floor correction and signed-word conversion.
No Rally source translation or language-wide IR/optimizer was introduced.

The generated installation is 1,008 bytes, the root command block is 259 bytes,
and a repeated input update/call is 21 bytes. Twenty complete bootstrap/cleanup
comparisons match the frozen Rally oracle, including tiny negative and subnormal
input, signed zero, extrema and narrowed domains. Strict host sanitizer builds,
the real production compiler's compatibility baseline, the preserved Rally
binary32-integer boundary model and an independent command decoder pass. The
decoder checks ownership, spans, conditional helper targets and finite call
structure; negative CLI cases preserve existing output files.

Evidence, provenance, commands and durations are in
`docs/tasks/GOLEM-003/host-checks.json` and its `reference/provenance.json`.
The prototype contract and limitations are in its README. There was no VDP
firmware execution or emulator/hardware deployment, and the maintained compiler
and Rally worktree are unchanged. README, living design and handoff now reflect
the Author's longer-command-block preference rather than repeating the early
one-instruction-per-block assumption.

GOLEM-003 remains in TODO pending review of this concrete result. The next
increment is an agreed integration of this small syntax/backend boundary with
main's frontend and storage policy, plus firmware qualification before production
promotion. Broader expression syntax, module linking and native Xtensa research
are not prerequisites. The output stays resident; the 21-byte recurring count
does not include result readback or another application's use of that output.
