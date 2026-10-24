# AgonDev host adapters

`agnb_host.cpp` implements the transport functions declared by the compiler's
`--emit-load-plan` C/C++ header. It streams through the generic AGNB reader;
compiled buffers remain on disk rather than embedded in application RAM.

1. Add agon-utils' `examples/agnb/api/` and this directory to the include path,
   then compile/link `agnb_host.cpp` **once** with your AgonDev application.
2. Include the generated `agnb.h`. Call its named load function, inspect its
   result, then perform explicit setup and named source calls. File I/O and
   framing come from generic AGNB; the shim supplies the MOS transport and
   little-endian16-bit updates.
3. The shim owns a single static8KiB workspace. Calls are synchronous and must
   not overlap/reenter. Separate generated package headers currently belong in
   separate translation units; see GOLEM-046/cpp/src/visual.cpp for a working
   second drawing package.
4. A zero load return reports local parsing/file close/submission success; it
   is not a stock VDP memory acknowledgement. The application owns completion,
   pacing, input admission, failure/partial-install cleanup, native discovery,
   LOAD/fresh-handle binding, result queries and UNLOAD. The shim performs none
   of those implicitly.
5. Native `golem_agnb_native_bind(handle)` uses `golem_host_write16` to patch all
   wrappers. Obtain the current nonzero handle through the existing public SDK
   protocol first. Do not free storage whose execution/unload is unconfirmed.

A minimal initialized integration, for a package containing `Sub step`:

```cpp
#include "generated/agnb.h"

int install_and_step() {
    const int result=golem_agnb_buffered_load();
    if(result!=0)return result;
    golem_agnb_buffered_setup();
    golem_buffered_step();
    return 0; // submission only; caller owns completion before updates/cleanup
}
```

The generic API is documented in agon-utils `examples/agnb/api/cpp-reader.md`.
Exact package/setup/ownership behavior is in Golem's maintained
[AGNB package contract](../../docs/design/agnb-packages.md). The task-local
AgonDev Makefile under GOLEM-046/cpp demonstrates linking an out-of-tree adapter
object without copying its implementation or embedding generated installations.
