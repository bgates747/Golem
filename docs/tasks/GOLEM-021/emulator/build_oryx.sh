#!/usr/bin/env bash
set -euo pipefail
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project=$(CDPATH= cd -- "$here/../../../.." && pwd)
[[ $(hostname) == oryx-linuxlite && $(uname -m) == x86_64 ]] || {
  echo 'Run this native build on the canonical oryx-linuxlite x86_64 host.' >&2
  exit 2
}
[[ -x "$project/.venv/bin/python" ]] || { echo 'Project .venv/bin/python is required.' >&2; exit 2; }
if ! command -v cargo >/dev/null && [[ -x "$HOME/.cargo/bin/cargo" ]]; then
  export PATH="$HOME/.cargo/bin:$PATH"
fi
# Canonical emulator.md keeps the host's SDL3 under this local prefix.
export LIBRARY_PATH="$HOME/.local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="$HOME/.local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
for build_tool in cc cmake pkg-config cargo; do
  command -v "$build_tool" >/dev/null || { echo "Missing build tool: $build_tool" >&2; exit 2; }
done
pkg-config --exists openssl zlib sdl3 || {
  echo 'Native build requires OpenSSL, zlib and SDL3 development packages.' >&2
  exit 2
}
bash "$here/prepare_core.sh"
bash "$here/build.sh"
bash "$here/build_fab.sh"
"$project/.venv/bin/python" - "$here" <<'PY'
from datetime import datetime,timezone
import hashlib,json,platform,socket,sys
from pathlib import Path
h=Path(sys.argv[1])
outputs={name:{'path':str(h/path),'sha256':hashlib.sha256((h/path).read_bytes()).hexdigest()}
         for name,path in [('adapter','build/flexe-vdp.so'),
                           ('frontend','build/fab-source/target/release/fab-agon-emulator')]}
r=dict(schema=1,created_utc=datetime.now(timezone.utc).isoformat(),
       host=socket.gethostname(),architecture=platform.machine(),outputs=outputs,
       core=json.loads((h/'build/flexe-source/.golem-flexe-source.json').read_text()),
       frontend=json.loads((h/'build/fab-source/.golem-fab-source.json').read_text()),
       graphical_instance_launched=False)
(h/'build/native-build.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r,indent=2))
PY
