#!/usr/bin/env bash
set -euo pipefail
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_root=${FLEXE_SOURCE:-"$here/build/flexe-source"}
core_build=${FLEXE_BUILD:-"$here/build/core"}
mkdir -p "$here/build"
cc -std=c17 -O2 -fPIC -shared -Wall -Wextra -Werror \
  -I"$source_root/src" "$here/flexe_vdp.c" "$core_build/libxtensa-emu-lib.a" \
  -o "$here/build/flexe-vdp.so.new" -lssl -lcrypto -lz -lm -lpthread
mv "$here/build/flexe-vdp.so.new" "$here/build/flexe-vdp.so"
