# R19-04 arithmetic qualification (in progress)

`kernel.golem` runs products, multiply-add, reciprocal via affine diagonal
inversion, division, rotation, signed integer stores and computed PLOT on the
stock VDP. Five different inputs enter buffer 1000; the eZ80 loader performs no
tested arithmetic. An eight-trip resident loop wraps a uint16 through 65535 and
checks its adjacent field remains intact. It returns after each finite call.

The host-side driver currently lives in the shared Rally evidence bucket:

```sh
# Golem worktree
make -C src
.venv/bin/python tests/run_hosted.py
./build/golemc --hosted examples/native_arithmetic/kernel.golem build/kernel.bin
# AgonArcade-rally19 worktree, using its own venv
.venv/bin/python docs/tasks/RALLY-19/probe_golem.py UNIQUE-RUN-NAME
```

The driver creates a fresh build below Golem/build/native-arithmetic/ and a fresh
canonical profile below Rally's RALLY-19/.emulator/. It uses stock Fab 1.2.4 /
MOS 3.0.2 with normal CPU/UART settings, dummy SDL video/audio, captured native
pixels and injected Escape. No graphical launch, custom module or alert.

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

Current limits: R19-04 remains unchecked. Native tests of zero/invalid-depth
admission, zero/one/120-trip loops, broader type/lifetime/index cases and kernel
cost measurements are still needed. Public TransformData currently qualifies
bounded binary32 tuples only; checked fixed output uses StoreI16Floor. Bulk field
copy and raw matrix mutation deliberately reject until their range/cache behavior
is qualified. Matrix state must be explicitly created with an intrinsic before
use; allocation alone does not install stock matrix metadata. Assets, dynamic
indexing, production state admission and full resource lifecycle are R19-05.
Hosted legacy uint16-expression/For-Next integration is not yet implemented;
ordinary Golem mode and its regression contract are preserved.

Source formatting used clang-format 20.1.8 in Golem's local venv. It is optional
developer tooling, not a compiler/runtime dependency. The C++ host tool needs
ordinary g++, and the native loader uses the installed agondev C++17 toolchain
without exceptions or RTTI. The Rally driver requires its existing Pillow/SDL
capture dependencies.
