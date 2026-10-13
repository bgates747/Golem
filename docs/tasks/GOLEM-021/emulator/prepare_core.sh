#!/usr/bin/env bash
set -euo pipefail
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project=$(CDPATH= cd -- "$here/../../../.." && pwd)
"$project/.venv/bin/python" "$here/prepare_core.py" "$@"
cmake -S "$here/build/flexe-source" -B "$here/build/core" \
  -DCMAKE_BUILD_TYPE=Release -DFLEXE_LTO=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_C_FLAGS=-Wno-error=stringop-truncation
cmake --build "$here/build/core" --target xtensa-emu-lib flexe-agon-vdp-test -j2
