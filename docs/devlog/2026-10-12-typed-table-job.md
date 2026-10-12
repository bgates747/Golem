# Typed table job

The Author accepted GOLEM-016's working generated assembly interface and
authorized GOLEM-017: a small typed table job invoked by assembly, without a
dummy float/floor conversion. GOLEM-016 is removed from the unfinished register;
its host and actual assembler evidence remains under its task silo. No emulator
execution was claimed for that earlier tranche. All existing work is preserved.

Implemented a separate resident-table profile, preserving the scalar profile's
five original artifacts. One flat mixed uint16/sint16 record and immutable
zero-based table compile to consecutive row IDs plus a checked whole-row copy.
Build mapping --table-data resolves packed bytes; --emit-asm exports call
golem_fetch, field locations and table layout.
The original index is admitted before calculation/patch/copy. Failure preserves
the whole destination, exits with status 5 and skips later statements; a later
valid invocation resets status and recovers. No byte verification enters the
production hot path. The four-row package is 528 installation bytes/19 buffers;
recurring input-and-call is 19 bytes, call alone six.

Independent byte-model checks pass 65,872 jobs, including every uint16 index for
four rows and the maximum host shape (1,024 rows, 32 fields). Five real ez80asm
consumers match wrapper instructions, payloads and field exports. Eleven CLI
failures preserve six previous files. Maintained compiler/ASan/UBSan checks pass
67 rejection cases; ten scalar packages preserve all prior artifacts against
e52a427. Host table checks took 3.129s; scalar parity/sanitizers took 34.215s.

Stock Pi Fab 1.2.5/MOS v3.0.2 passes 873 checks across 98 jobs, 3,453 bytes, in
5.362s. Default jobs call the actual generated eZ80 wrapper. Relocation across
ID 256, 258 rows, near-ID-limit allocation, singleton, invalid full-width indices,
recovery, immutable rows/programs, intended template mutation, cleanup/reinstall
and external preservation pass. Dedicated Oryx headless verification matches all
saved bytes in 7.218s. The maximum schema and physical hardware are unqualified
by stock execution. Headless results were removed; Oryx is stopped and ready.

Human Oryx MOS/keyboard validation and explicit commit approval remain pending.
The canonical AGENTS.md requires that gate before committing emulator-related
changes. All existing dirty work remains uncommitted. The task silo retains
scripts, fixtures, results and provenance; the maintained contract and EX-029
record the supported slice. Broader bounded records and arrays remain proposals.

The Author subsequently reported “looks like success.” The agent retrieved the
fresh saved Oryx observations and independently verified all 873 checks across
98 jobs, all 3,453 bytes. Runtime/fixture hashes match the staged review.
Human validation is recorded as passed in GOLEM-017/human-review.json and the
maintained documents. Explicit commit approval remains pending; no commit or
push was performed.

The Author explicitly approved committing the reviewed work: “i approve. what’s
your next move”. GOLEM-017 is accepted and removed from TODO.md. Package code,
assembly integration, the preceding accepted GOLEM-010 review and their evidence
are included in this checkpoint. The next queued feature is GOLEM-011 typed
drawing review; its first subtask DRW-001 examines Rally operations, operand
types, graphics state and resource requirements. That review has not begun.

Before committing, removed a trailing blank line from the task-only command
model and reran its qualification: all 65,872 jobs pass in 3.151s.
Compiler/emulator fixture bytes are unchanged; evidence hashes are refreshed.
