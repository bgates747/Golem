# Checked resident subtraction

GOLEM-013 implements checked uint16 subtraction and mixed + / - expressions in
the existing finite resident scalar job. Underflow preserves the failed
destination, stops later statements and resumes normal VDP processing. Host,
sanitizer and stock-emulator qualification pass. Oryx is staged and stopped for
human review; new-input/call traffic remains 21 bytes. This tranche is uncommitted.

The Author explicitly approved committing/pushing accumulated work and proceeding
with this tranche. The preceding design notes, semantics review and GOLEM-012
implementation were committed and pushed as ddb5e87. GOLEM-012 was removed from
TODO after acceptance; GOLEM-013 is the current implementation/review item.

Binary + and - share precedence and evaluate left to right. Every intermediate
operation is checked, so later terms cannot conceal earlier failure. Constant
underflow is a compile error; variable initializer arithmetic must move into the
job. Raw-mode subtraction and general arithmetic remain deferred.

Subtraction uses the VDP's existing XOR and add-with-carry commands to compute
a + complement(b) + 1. It retains both carries to distinguish borrow, then jumps
to a finite error helper on underflow. Resident status code 2 identifies underflow;
code 1 still identifies addition overflow. Status clears at entry. The same
destination-commit, previous-write preservation and recovery rules apply.

Maintained tests pass with warnings as errors and 32 rejection cases. ASan/UBSan
pass. The independent command model executes 88 jobs across default and relocated
layouts. Stock Fab 1.2.5/MOS v3.0.2 passes 1,248 checks over 88 jobs, producing
4,392 expected bytes. Oryx's pinned x86-64 stock runtime produces identical bytes.
Cases include full-width values, byte borrow, early/late errors, mixed overflow,
self-reference, repeated failure, recovery, relocation, cleanup and reinstall.

Pure floor retains all five package artifacts. The previously reviewed
counted-floor retains all four command artifacts against ddb5e87; its manifest
now names underflow code 2. Four raw outputs remain byte-identical to ddb5e87.
The fixture installs 1804 bytes across 42 buffers; new-input calls remain 21 bytes,
plain calls 6 bytes. Physical hardware and other firmware remain unqualified.

Host checks took 23.673 seconds including sanitizer compilation. Pi preparation
took 0.037 seconds and execution/collection 6.435 seconds. Oryx execution,
collection and verifier took 8.380 seconds. These are local monotonic timings.

The guest prints the instruction to type `dir` at the MOS prompt and report
that it works. Automated Oryx results were removed, and the emulator is stopped.
The prior SD startup is backed up locally under the task's ignored build tree.
No GUI or attention alert was launched. [Review commands and evidence](../tasks/GOLEM-013/README.md)
are ready. Human review and explicit commit approval remain required by canonical
Agon AGENTS.md before committing this tranche.
