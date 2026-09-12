# R19-04 arithmetic qualification

`kernel.golem` runs products, multiply-add, reciprocal via affine diagonal
inversion, division, rotation, signed integer stores and computed PLOT on the
stock VDP. Five different inputs enter buffer 1000; the eZ80 loader performs no
tested arithmetic. An eight-trip resident loop wraps a uint16 through 65535 and
checks its adjacent field remains intact. It returns after each finite call.
Additional probes cover subtraction, scalar multiplication, nonzero matrix-cell
replacement/extraction, zero/one/eight/120-trip loops and a finite denominator
guard that rejects zero, negative and oversized values before inversion.

The host-side driver currently lives in the shared Rally evidence bucket:

```sh
# Golem worktree
make -C src
.venv/bin/python tests/run_hosted.py
./build/golemc --hosted examples/native_arithmetic/kernel.golem build/kernel.bin
# AgonArcade-rally19 worktree, using its own venv
.venv/bin/python docs/tasks/RALLY-19/probe_golem.py UNIQUE-RUN-NAME
.venv/bin/python docs/tasks/RALLY-19/probe_golem.py UNIQUE-COST-NAME --cost-only
```

The driver creates a fresh build below Golem/build/native-arithmetic/ and a fresh
canonical profile below Rally's RALLY-19/.emulator/. It uses stock Fab 1.2.4 /
MOS 3.0.2 with normal CPU/UART settings, dummy SDL video/audio, captured native
pixels and injected Escape. No graphical launch, custom module or alert.
Cost-only mode omits the capture interposer, runs two sets of five 4096-call
batches and exits the guest automatically. The host then terminates the isolated
emulator process after its guest exit marker and complete reports are present.

Diagnostic readback copies each result byte to a stock GP command's token field
and calls that small buffer. MOS supplies the echo byte to the loader. This is
data readback and a parser milestone, not a render-completion callback. The
loader uses the complement of the expected byte as its initial sentinel; an
unexpected result equal to that sentinel times out and fails rather than falsely
passing. A fully general arbitrary-byte readback needs stronger handshaking in
the later runtime diagnostic work. GP traffic is diagnostic overhead and must
not appear in production performance measurements.

The first run passed arithmetic/PLOT but failed a rotation reference using
Cartesian Y. Pinned stock code explicitly uses `[cos sin; -sin cos]` for screen
coordinates. The corrected reference passes; both results are retained. A second
attempt exposed agondev's non-idempotent `make -B` directory target. Unique build
directories resolve it without changing upstream tooling.

The final `complete-intrinsics` visual run and `complete-intrinsics-cost` timing
run pass. Raw readback after 40,960 calls remains correct, including the adjacent
field. The final bootstrap is 2461 bytes, 66 owned IDs and 1477 declared resident
payload bytes (not allocator/metadata/peak memory). Its full arithmetic batch
averages 61.0 microseconds/call on this Linux host, including eZ80 command
submission, UART and a final GP parser echo. Do not infer pure VDP execution time
by subtracting the similarly sized empty-job measurement: timer quantization
dominates the small difference. This is not ESP32 hardware performance or a
production rendering-completion measurement. Full scene performance remains a
later goal milestone.

Public TransformData currently qualifies
bounded binary32 tuples only; checked fixed output uses StoreI16Floor. Unstructured bulk field
copy and raw matrix mutation deliberately reject. R19-05 adds fully covered typed
CopyRecord and validated Asset/Table lookup; see `rally19-resident-v1.md`. Matrix state must be explicitly created with an intrinsic before
use; allocation alone does not install stock matrix metadata. Resident data/admission/lifecycle qualification is described in
`docs/design/rally19-resident-v1.md`; full-scene integration remains later work.
Hosted legacy uint16-expression/For-Next integration is not yet implemented;
ordinary Golem mode and its regression contract are preserved.

Source formatting used clang-format 20.1.8 in Golem's local venv. It is optional
developer tooling, not a compiler/runtime dependency. The C++ host tool needs
ordinary g++, and the native loader uses the installed agondev C++17 toolchain
without exceptions or RTTI. The Rally driver requires its existing Pillow/SDL
capture dependencies.
