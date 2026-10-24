# Streamed Golem program packages

Golem can describe its compiled buffers as an AGNB container instead of requiring
an assembly application to embed the complete installation. MOS reads the file
through a bounded transfer window; the VDP retains the program and data. Loading,
setup and execution remain separate host actions. Existing embedded output stays
available and unchanged.

## Build

Use the maintained generic plain-buffer API from agon-utils, currently published
on `codex/agnb-plain-buffers` at `35dd8ef`. Its generic format, Python writer and
MOS loader live in `examples/agnb/api/`; Golem owns the layout and setup semantics.

```bash
make -C src check
build/golemc --portable --emit-load-plan \
  examples/portable/counter.golem build/counter
.venv/bin/python tools/package_agnb.py build/counter
```

`--emit-load-plan` also works with `--resident` and `--stock-matrix`. Flags precede
source/output arguments. Use a fresh directory when changing source/target shape;
old output files are not automatically removed. Normal compilation without this
flag does not gain an AGNB dependency. The compiler remains C++; the auxiliary
Python packager delegates RIFF framing to the generic writer. Override a different
checkout location with `--agnb-api PATH/TO/examples/agnb/api`.

1. Existing manifests, field layouts, entry points and embedded installers are
   preserved. Additional `load-plan.json` and `load-buffers/` describe compiler-owned
   records directly. They are not reconstructed by parsing an installation stream.
2. `agnb.inc` contains symbols, small named wrappers and load/setup/cleanup helpers.
   Include it **instead of** the corresponding embedded `golem.inc`, or portable
   `layout.inc` plus `calls.inc`. It does not embed the entire installation.
3. `agnb.h` contains packed field buffer/offset/byte constants, named calls, and
   host transport hooks. `GOLEM_FIELD_total_BUFFER/OFFSET/BYTES` names a field;
   `GOLEM_ENTRY_buffered_step` names an entry. Values use the existing little-endian
   interface contract. No host pointer to a VDP field is implied.
4. The packager writes `program-stock.agnb`, or the portable
   `program-buffered.agnb` and `program-native.agnb`, plus an `agnb-package.json`
   receipt. `--backend stock|buffered|native` selects one available component.
   Build-machine filenames are not runtime directory lookups in the container:
   its records carry their assigned buffer IDs.

For native output, build the emitted C using the standalone VDPlet SDK, then add
that **already built** image to its compiler-assigned staging buffer:

```bash
.venv/bin/python ../agon-vdplet/tools/build_module.py \
  build/counter/native.c --name counter --output build/counter
.venv/bin/python tools/package_agnb.py build/counter \
  --module build/counter/counter.vdpl
```

Use the SDK's `--math` option when required by the existing native profile. The
packager does not compile or admit native code. Without `--module`, native
wrappers/state can still be packaged, but the host must supply the module image
separately before LOAD. Manifest `module_buffer` and the generated module symbol
reserve that buffer; no arbitrary staging ID is needed in application source.

## Host sequence

1. Include the canonical `agnb_api.inc` and its application's existing dependencies
   exactly once. `agnb_dependencies.inc` is a reference for missing definitions,
   not a second copy of definitions already present in the application.
2. For stock portable code, call `golem_agnb_buffered_load`; inspect A/Z using the
   generic loader contract before calling `golem_agnb_buffered_setup`. Resident
   profiles use the `stock` name. Generated load helpers use fixed package filenames
   relative to the current MOS directory; alternatively supply DE to the canonical
   loader yourself. Generic errors and partial installation semantics apply.
3. Setup enables required stock features, initializes ordinary scalar helpers or
   creates **typed** matrices as appropriate. A matrix is not an opaque 64-byte
   upload with equivalent metadata. No source entry runs just because the file
   was loaded or setup was called. A source `Sub setup` remains a separate named
   entry chosen by the application.
4. Call the existing named wrapper, such as `golem_buffered_step` or
   `golem_drawPackage`. Existing entry wrappers require A=0 for MOS binary output;
   the new AGNB helpers set it themselves. Submission is not a completion reply.
   The application still owns input updates, serialization, completion, pacing,
   drawing fences and errors.
5. For native execution, discover the compatible runtime and check slot ownership,
   load `module_buffer` with the SDK protocol and obtain its fresh nonzero handle.
   Pass that handle in HL to `golem_agnb_native_bind`, which patches every native
   wrapper at the compiler's handle offset. Only then call native entries. Check
   the actual runtime result under the existing root-v1 contract. AGNB never
   discovers a runtime, loads a module slot or manufactures a persistent handle.
6. After proven completion, explicitly UNLOAD a native module before freeing its
   package. Call `golem_agnb_BACKEND_cleanup` for final teardown. Its resident
   cleanup clears common state and that backend's private buffers, setup and
   optional staging image; the host helper subsequently clears the cleanup buffer
   itself. Borrowed assets, fixed role bindings and external reservations survive.
   Other installed backends require their own private cleanup too. Do not free a
   module/storage whose execution or unload outcome is still uncertain.

Each backend container includes the same initialized common state and arrays.
Thus either can be selected independently at startup. To offer runtime selection,
load both **before the first invocation**, then choose their named wrappers with
the shared state/layout. Loading a full second container later resets common
state; final AGNB cleanup is whole-package teardown, not a state-preserving
backend eviction API. Existing private cleanup/install contracts remain available
for hosts needing finer control. Dependency graphs, demand paging, automatic
scheduling and state-preserving incremental backend eviction are not added here.

## C/C++ integration boundary

The generated header delegates to these application functions:

```cpp
extern "C" {
    int golem_host_load_agnb(const char *filename); // return generic success/error
    void golem_host_call(uint16_t buffer);
    void golem_host_clear(uint16_t buffer);
    void golem_host_write16(uint16_t buffer, uint16_t offset, uint16_t value);
}
```

`golem_agnb_buffered_load/setup/cleanup`, `golem_buffered_step`, and
`golem_agnb_native_bind(handle)` then use these hooks. The application performs
file loading, SDK discovery/admission and completion. Use one generated `agnb.h` per translation unit; helper names currently have no
package namespace option. Link `runtime/agondev/agnb_host.cpp` once with the
application, and add agon-utils' `examples/agnb/api/` to the include path. The
adapter implements all four hooks using the generic C++ reader and MOS transport;
its single static8KiB workspace is non-reentrant. See the
[AgonDev adapter guide](../../runtime/agondev/README.md) and the working
GOLEM-046/cpp consumer. No compiler, SDK or firmware change is required by this
C++ adapter. Native admission/completion remains application-owned.

## Resource and qualification limits

1. Generic AGNB0.3 uses nonempty opaque DATA, explicit IDs and consolidation into
   one logical block per record. File transfer windows do not become executable
   block boundaries. Current compiler profile limits remain in force; the AGNB
   format's larger length field does not remove language/runtime admission bounds.
2. The transfer window is 8,192 bytes, plus loader state/stack. Compiled installation
   bytes stay on disk rather than consuming the application's RAM. The VDP still
   needs enough memory for all resident records and any consolidation allocation.
3. This changes installation storage and packaging. Resident invocation remains
   six wire bytes; it does not optimize the compiled algorithm or eliminate data
   upload traffic. There is no production post-copy readback/checksum/rollback.
4. GOLEM-046 compares eight profile/examples at default/relocated IDs, preserves
   152 old artifact/component comparisons, compiles real assembly/C++ consumers,
   and checks package rejection before output mutation. Maintained host tests
   include allocator/lifecycle/collision/exhaustion cases under sanitizers.
5. Actual stock and native qualification loads nine 60,000-byte arrays through the
   generic MOS reader. It observes zero before activation, totals45/90 after calls,
   statuses/generation, cleanup and reload. Native qualification additionally uses
   the SDK-built Xtensa image, fresh handles, generated binding, actual result
   queries and confirmed unload. Target evidence distinguishes stock Fab from
   actual-Xtensa Flexe; physical hardware and performance are not claimed.

The numerical qualification reads results for testing. Application loading itself
performs no data readback. The separate visible `review.golem` draws a labelled
nine-tier pyramid; assembly supplies screen setup and transport, and
`large.golem` supplies the large-data calculation. No diagnostic geometry is
silently mixed into that Golem drawing.
