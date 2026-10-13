# Multiple finite drawing entries

Drawing packages now accept multiple finite named Subs with one shared packed
global input block. Assembly selects a named wrapper, and the first declared Sub
retains the legacy invoke/update-call behavior. The change needs no optimizer,
local frames, inter-Sub calls, native vdplet or new execution lifecycle. Host
qualification is recorded here; stock emulator and Oryx review belong to the
separate qualification fixture and the main task plan.

The initialized [multi.golem](multi.golem) example has a shared-input marker,
a literal-only guide and a later bitmap entry. Assembly must load reserved asset
50000 first. Compile with:

```bash
build/golemc --resident --emit-asm --reserve 50000:50000 \
    docs/tasks/GOLEM-022/compiler/multi.golem \
    docs/tasks/GOLEM-022/compiler/build/example
```

The manifest retains schema 1 and legacy entry, adding entries only for
multi-Sub packages. Each source-ordered object gives name/id/blocks/bytes.
Result.entries carries the corresponding name, ID and six-byte invoke; legacy
Result.entry/invoke are derived from its first element. The ordered source
procedures are in Spec.procedures; Spec.entry remains the first name.

Allocation installs one input block, then each entry and its private templates
in source order. First-entry/template owner strings retain their old spelling.
Later entries use drawing.entry.N and their templates use drawing.template.N.M.
All bitmap operands are checked against external reservations before any compiler
allocation, including those in later bodies. Affine support is enabled once if
any body uses a bitmap. Borrowed assets remain outside ownership and cleanup.

The assembly renderer collects entries in one include using one symbol-validation
set. Each named wrapper exports its buffer/call/call_len. Globals and package
payloads appear once. Scalar/table callers retain the single-entry adapter and
existing output text. The CLI still validates the complete package and requested
assembly include before making or modifying output files.

Run the focused qualification from the project root:

```bash
.venv/bin/python docs/tasks/GOLEM-022/compiler/run_checks.py
```

The script uses the parent-owned archived compiler at ../build/golemc-before.
It runs maintained contracts and ASan/UBSan, compares all six artifacts for
twenty old resident packages (ordinary and relocated IDs), compares two raw
payloads, assembles two actual three-entry ADL consumers and checks wrapper and
CALL opcodes, packets, field offsets and shared payload bytes. Output-preservation
cases exercise case folding, field/entry collisions, paint versus paint_call,
package labels, long symbols and malformed later bodies. Existing output files
remain unchanged, and absent output directories remain absent on failure.

[host-checks.json](host-checks.json) records results, commands, elapsed times and
source hashes. Generated parity packages, sanitizer binaries and assembler
consumers remain ignored under build/. The production drawing specification is
[resident-drawing.md](../../../design/resident-drawing.md); its deferred autonomous
sprite note is preserved unchanged.
