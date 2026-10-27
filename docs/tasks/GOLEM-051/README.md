# GOLEM-051 — Golem development directly on Agon

This task brings the current Golem compiler to MOS as AgonDev-built eZ80
executables. Source is edited and compiled on the Agon; stock buffered programs
then run on its VDP. The standalone package includes initialized examples and
a language reference. Native C emission is included; a C-to-Xtensa toolchain
on the Agon is a separate option.

The [plan](../GOLEM-051.md) and [contract](contract.md) define scope and ownership.
Use [HOWTO.md](HOWTO.md) for the end-user workflow and [REBUILD.md](REBUILD.md)
for rebuilding the supplied binaries. New implementation remains in `port/`;
`exploratory-subset/` is superseded and excluded from the package.

## Reproduce qualification

Run these from the repository root using the project-local environment:

```bash
.venv/bin/python docs/tasks/GOLEM-051/capture_cases.py
.venv/bin/python docs/tasks/GOLEM-051/port/build_all.py
.venv/bin/python docs/tasks/GOLEM-051/port/build.py
.venv/bin/python docs/tasks/GOLEM-051/check_parity.py
.venv/bin/python docs/tasks/GOLEM-051/port/build.py --sanitize
.venv/bin/python docs/tasks/GOLEM-051/check_parity.py --sanitize
.venv/bin/python docs/tasks/GOLEM-051/qualify_corpus.py
.venv/bin/python docs/tasks/GOLEM-051/check_cli.py
.venv/bin/python docs/tasks/GOLEM-051/check_examples.py
.venv/bin/python docs/tasks/GOLEM-051/check_rebuild.py
.venv/bin/python docs/tasks/GOLEM-051/stage_review.py --check
.venv/bin/python docs/tasks/GOLEM-051/report.py
.venv/bin/python docs/tasks/GOLEM-051/package.py
```

Builds share generated include files: run them sequentially. Headless emulator
scripts share this task's dedicated Pi profile: run them sequentially too.
They start only SDL dummy sessions through the profile-local Fab wrapper and
stop each session. They never contact Oryx or physical hardware.

The corpus captures the maintained compiler's real test inputs in an ignored
source copy. It does not alter production sources or tests. Accepted outputs
must match byte for byte; rejected inputs must remain rejected. Native-only
variants exercise the currently supported math backend, and CLI checks cover
flags, 32-bit fixed bindings and response files with quoted names. Examples
exercise the actual installed dispatcher/runner and observe VDP results before
cleanup. Production execution has no numerical readback probes.

Recorded JSON files separate host, sanitizer, eZ80/MOS, stock execution and
human/hardware evidence. The Author released publication on 2026-10-27; the standalone walkthrough
remains in progress. GOLEM-053 numeric bitwise visual/keyboard review passed.
