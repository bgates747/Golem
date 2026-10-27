# Rebuilding the on-Agon compiler

Ordinary users run the supplied eZ80 binaries. This optional recipe rebuilds
them from the included C++ compiler snapshot and AgonDev adapter. It does not
need a desktop Golem executable or a Python package installation.

From the extracted package's `source` directory, with Python 3 and AgonDev
installed on a development computer:

```bash
python3 -m venv .venv
export AGONDEV_ROOT=/absolute/path/to/agondev/release
.venv/bin/python docs/tasks/GOLEM-051/port/build_all.py
```

Outputs are beneath `docs/tasks/GOLEM-051/build/`:

| Build output | Install on the Agon SD card |
| --- | --- |
| `support/golemc.bin` | `/mos/golemc.bin` |
| `support/golemrun.bin` | `/bin/golemrun.bin` |
| `port/ez80-raw/golemc.bin` | `/golem/compiler/raw.bin` |
| `port/ez80-resident/golemc.bin` | `/golem/compiler/resident.bin` |
| `port/ez80-portable/golemc.bin` | `/golem/compiler/portable.bin` |
| `port/ez80-stock-matrix/golemc.bin` | `/golem/compiler/stock-matrix.bin` |

Retain `/golem/cache` and `/golem/output` as directories. The compiler's
metadata scratch file uses the first of these.

For an adapted host build, a normal g++/gcc toolchain is sufficient:

```bash
.venv/bin/python docs/tasks/GOLEM-051/port/build.py
.venv/bin/python docs/tasks/GOLEM-051/port/build.py --sanitize
```

The full repository's GOLEM-051 task contains capture/parity/emulator scripts
and evidence for development. Those scripts are not required for daily native
compilation or rebuilding from this package. The generated source adapter is
mechanical: profile selection, platform library/error recovery, faithful numeric
processing, metadata spooling and explicit descriptor widths. It does not add
another language grammar.
