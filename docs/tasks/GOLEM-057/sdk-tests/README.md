# Independent pixel and PCM SDK accessor checks

These host checks verify that pixel and PCM service discovery works separately,
while the existing combined helper continues to require both. They exercise the
maintained SDK header directly; they neither execute service callbacks nor
launch an emulator.

From the Golem repository, compile and run:

```bash
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -I /Agon/mystuff/agon-vdplet/include \
  docs/tasks/GOLEM-057/sdk-tests/accessors.c \
  -o /tmp/golem057-sdk-accessors
ASAN_OPTIONS=detect_leaks=0 /tmp/golem057-sdk-accessors
```

The cases cover capability combinations, each missing media callback, wrong
major/older minor versions, truncated extents, actual shorter prefix objects,
later table views and null unrelated callbacks. The shortened objects make
premature extension reads visible to AddressSanitizer. Any callback invocation
aborts the test.

The run passes **108 checks** on the 64-bit development host. AddressSanitizer
and UndefinedBehaviorSanitizer remain enabled. Leak detection is disabled because
the execution sandbox uses tracing, which LeakSanitizer cannot support; this
test allocates no heap memory.

This is a **host C** test: it deliberately avoids the SDK's C++ assertions that
require ESP32 pointer sizes. Its `struct_bytes` values use the host's actual
`sizeof` results, which differ on a 64-bit host. Passing checks prove accessor
admission behavior; they do not establish the 32-bit Xtensa table layout, ABI
binary compatibility or actual firmware callback behavior. Those require the
separate target compiler/module qualification.
