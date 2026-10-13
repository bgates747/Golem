#!/usr/bin/env bash
# Stage/build only emulator-owned files; root owns the consumer and review profile.
set -euo pipefail
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project=$(CDPATH= cd -- "$here/../../../.." && pwd)
ssh -o BatchMode=yes -o ConnectTimeout=10 oryx-linuxlite \
  'test "$(hostname)" = oryx-linuxlite && test -x "$HOME/Agon/mystuff/golem/.venv/bin/python"'
"$project/.venv/bin/python" "$here/build_bundle.py"
archive_sha=$(sha256sum "$here/build/source-bundle.tar.gz")
archive_sha=${archive_sha%% *}
ssh -o BatchMode=yes oryx-linuxlite \
  'mkdir -p "$HOME/Agon/mystuff/golem/docs/tasks/GOLEM-021/emulator/build"'
scp -o BatchMode=yes "$here/build/source-bundle.tar.gz" \
  oryx-linuxlite:Agon/mystuff/golem/docs/tasks/GOLEM-021/emulator/build/source-bundle.tar.gz.new
ssh -o BatchMode=yes oryx-linuxlite bash -s -- "$archive_sha" <<'REMOTE'
set -euo pipefail
remote_project="$HOME/Agon/mystuff/golem"
remote_task="$remote_project/docs/tasks/GOLEM-021"
"$remote_project/.venv/bin/python" - "$remote_task" "$1" <<'PY'
import hashlib,json,sys,tarfile
from pathlib import Path
t=Path(sys.argv[1]);a=t/'emulator/build/source-bundle.tar.gz.new'
if hashlib.sha256(a.read_bytes()).hexdigest()!=sys.argv[2]:
    raise SystemExit('Transferred source bundle hash mismatch')
with tarfile.open(a) as archive:
    archive.extractall(t,filter='data')
m=json.loads((t/'emulator/build/source-bundle.manifest.json').read_text())
for path,digest in m['files'].items():
    if hashlib.sha256((t/'emulator'/path).read_bytes()).hexdigest()!=digest:
        raise SystemExit(f'Extracted source differs: {path}')
a.rename(a.with_name('source-bundle.tar.gz'))
PY
bash "$remote_task/emulator/build_oryx.sh"
REMOTE
