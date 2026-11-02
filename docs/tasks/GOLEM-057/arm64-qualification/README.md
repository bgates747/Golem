# ARM64 mechanism gate preparation

The [frozen plan](plan.md) permits preparation only. No ARM backend pin or
execution grant is inferred from a successful build. The four eventual cases
are fresh, sequential Pi direct-library processes: original toy JIT, original
toy interpreter, frozen row suite JIT, frozen row suite interpreter. Stop at the
first unexpected failure, preserve all evidence and shut down normally. This
does not consume or simulate an Oryx lease.

## Inputs and source boundary

`toy_gate.py` reuses the frozen SDK `docs/tasks/VDPL-012/qualification/target.py`
without modifying or copying it. Its logical fixture matches read-only
`agon-6502/experiments/vdp005/color/coherence/proof.py` lines 74–94: four initial
buffers, 32-byte `<4H6I` arguments, entry 0 / budget 0 then entry 1 / budget 8.
The complete 64-byte work state must match the saved generation21 oracle, with
8 steps and 29 cycles. Loader INIT, module FINI/unload, empty slot and same-mode
executable heap recovery are required; target resources and zero execution
faults are recorded. Mode 11 stays unchanged throughout the CPU control.

The original package is 35,876 bytes; **35,072 bytes is executable text**.
Its original source, package, policy and gen21 driver remain unchanged. Only
the thin SDK-transport harness and host architecture differ. The row suite,
worker, compiled modules, SDK headers, independent pixel expectations and
rows-r1 firmware all retain their original frozen hashes.

`source-freeze.json` records preparation inputs. Backend library/core/host-check
receipts and architecture pins remain pending until the owner build is returned
and root supplies the actual pin. Add a separate execution receipt before any
launch; do not relabel an x86 hash as the ARM artifact.

## Preparation checks

```bash
cd /Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-057/arm64-qualification/toy_gate.py --prepare-only
.venv/bin/python -m py_compile \
  docs/tasks/GOLEM-057/arm64-qualification/toy_gate.py \
  docs/tasks/GOLEM-057/arm64-qualification/stage_arm64.py
```

These commands do not load a backend library or instantiate an emulator.

## Later staging and execution

After root supplies and inspects the actual owner ARM receipt/pin, stage only
the new canonical `.emulator/vdpl012-arm64-qualification` SDK profile using
`stage_arm64.py --backend /absolute/arm/library.so --backend-sha256 ACTUAL_SHA
--output /absolute/new/profile-receipt.json`. It copies the existing native
frontend and matched rows firmware using the canonical generator; it does not
build or launch anything and refuses to replace an existing profile. Previous
SDK/Oryx profiles are untouched. Verify its `.runtime.sha256` from the profile
directory. This staging command is not released until the actual pin arrives.

After the separate explicit root Pi permit, run the two toy cases with the
staged backend and distinct new output directories:

```bash
cd /Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-057/arm64-qualification/toy_gate.py \
  --backend /Agon/mystuff/agon-vdplet/.emulator/vdpl012-arm64-qualification/flexe-vdp.so \
  --backend-sha256 ACTUAL_SHA --engine jit \
  --output /Agon/mystuff/agon-vdplet/build/vdpl012arm64qualification/toy-jit-r01
```

Use `--engine interp` and a new `toy-interp-r01` directory only after the first
case passes. Then run the existing frozen SDK `qualification/qualify.py` with
`--rows present --capabilities 287`, the staged rows firmware/backend, first JIT
then interpreter in distinct new directories. No recompilation, source patch,
new expectation or additional no-option sweep is required.

Estimated total execution is roughly one to five minutes on the Pi, with native
ARM JIT speed unqualified until observed. Each process has a 600-second failure
bound, not a lease expiry. Startup, transport and captures are included in elapsed
time; the gate establishes mechanism/lifecycle correctness, not consumer speed
or physical timing. Root owns the later run permit and consumer coordination.
