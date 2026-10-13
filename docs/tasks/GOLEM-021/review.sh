#!/usr/bin/env bash
# One foreground command: close the emulator window when the review is complete.
set -euo pipefail
task_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
profile_dir=${GOLEM_HYBRID_PROFILE:-"$task_dir/.emulator"}
profile_dir=$(realpath "$profile_dir")
cd "$profile_dir"
exec ./fab-agon-emulator "$@"
