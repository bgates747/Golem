#!/usr/bin/env bash
set -euo pipefail
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project=$(CDPATH= cd -- "$here/../../../.." && pwd)
"$project/.venv/bin/python" "$here/prepare_fab.py" "$@"
FORCE=1 cargo build --release --locked --jobs 2 \
  --manifest-path "$here/build/fab-source/Cargo.toml" -p fab-agon-emulator
