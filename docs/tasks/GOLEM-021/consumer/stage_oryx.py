"""Bundle the exact staged fixture, deploy to canonical Oryx, qualify headlessly.

Run with the project .venv/bin/python. --bundle-only performs no SSH or launch.
The native Oryx frontend/adapter must first be built by emulator/stage_oryx.sh.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
PROJECT = HERE.parents[3]
HOST = 'oryx-linuxlite'
REMOTE_TASK = 'Agon/mystuff/golem/docs/tasks/GOLEM-021'
SSH = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10', HOST]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run(command, *, script=None, timeout=60):
    result = subprocess.run(command, input=script, text=True, capture_output=True,
                            timeout=timeout)
    if result.returncode:
        raise RuntimeError(f'{command[0]} failed ({result.returncode}):\n'
                           + result.stdout + result.stderr)
    return result.stdout


def bundle(profile: Path):
    app = profile / 'sdcard/vdplet'
    expected = json.loads((app / 'expected.json').read_text())
    files = {}

    def add(name: str, source: Path):
        files[name] = source.read_bytes()

    for name in ('probe.bin', 'script.bin', 'expected.json'):
        add('consumer/build/' + name, app / name)
    for name in ('probe', 'script'):
        if digest(files[f'consumer/build/{name}.bin']) != expected[f'{name}_sha256']:
            raise ValueError(f'Staged {name}.bin differs from staged expected.json')
    for name, identity in expected['modules'].items():
        if Path(name).name != name or name in ('.', '..'):
            raise ValueError(f'Module package must have a plain filename: {name!r}')
        data = (app / name).read_bytes()
        if len(data) != identity['bytes'] or digest(data) != identity['sha256']:
            raise ValueError(f'Staged module differs from staged expected.json: {name}')
        files['consumer/build/staged-modules/' + name] = data
    add('consumer/build/staged-firmware.bin', profile / 'esp32-firmware.bin')
    elf = profile / 'esp32-firmware.elf'
    if elf.is_file():
        add('consumer/build/staged-firmware.elf', elf)
    add('consumer/build/staged-source-runtime.sha256', profile / '.runtime.sha256')
    if expected.get('packed_asset'):
        for name in ('loderunner.packed', 'assets.json'):
            add('consumer/build/apple-assets/' + name, app / name)
        if digest(files['consumer/build/apple-assets/loderunner.packed']) != expected['packed_asset']['packed_sha256']:
            raise ValueError('Staged packed pixels differ from staged expected.json')
    for path in sorted(HERE.glob('*.py')):
        add('consumer/' + path.name, path)
    for name in ('probe.asm', 'README.md', 'PLAN.md'):
        add('consumer/' + name, HERE / name)
    add('review.sh', TASK / 'review.sh')
    add('consumer/build/canonical-setup-emulator.py',
        PROJECT.parent / 'agon-dev-env/scripts/setup_emulator.py')
    native = PROJECT / 'docs/tasks/GOLEM-002'
    for name, source in (
            ('numeric.c', native / 'proof/numeric.c'),
            ('packed_mono.cpp', native / 'packed-mono/packed_mono.cpp'),
            ('vdplet.h', native / 'runtime/vdplet.h')):
        add('consumer/build/native-source/' + name, source)

    manifest = dict(schema=1, fixture=expected.get('fixture', 'numeric'),
                    source_profile=str(profile),
                    files={name: dict(bytes=len(data), sha256=digest(data))
                           for name, data in sorted(files.items())})
    files['consumer/build/staged-bundle-manifest.json'] = (
        json.dumps(manifest, indent=2) + '\n').encode()
    build = HERE / 'build'
    build.mkdir(exist_ok=True)
    archive = build / 'consumer-bundle.tar.gz'
    with tarfile.open(archive, 'w:gz') as output:
        for name, data in sorted(files.items()):
            info = tarfile.TarInfo(name)
            info.size = len(data)
            info.mode = 0o755 if name == 'review.sh' else 0o644
            output.addfile(info, io.BytesIO(data))
    receipt = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                   archive=str(archive), archive_sha256=digest(archive.read_bytes()),
                   archive_bytes=archive.stat().st_size, **manifest)
    (build / 'consumer-bundle.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'Bundled {len(manifest["files"])} files: {manifest["fixture"]}, '
          f'{len(expected["checks"])} checks, {archive.stat().st_size} compressed bytes.',
          flush=True)
    return archive, receipt


REMOTE_RUN = r'''
set -euo pipefail
remote_project="$HOME/Agon/mystuff/golem"
remote_task="$remote_project/docs/tasks/GOLEM-021"
"$remote_project/.venv/bin/python" - "$remote_task" "$1" "$2" <<'PY'
from datetime import datetime, timezone
import hashlib, io, json, os, platform, socket, subprocess, sys, tarfile
from pathlib import Path
t=Path(sys.argv[1]); project=t.parents[2]; consumer=t/'consumer'
assert socket.gethostname() == 'oryx-linuxlite', 'Unexpected remote hostname'
assert platform.machine() == 'x86_64', 'Expected native Oryx x86_64 runtime'
archive=consumer/'build/consumer-bundle.tar.gz.new'
sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
if sha(archive) != sys.argv[2]: raise SystemExit('Transferred consumer bundle hash mismatch')
with tarfile.open(archive) as source:
    source.extractall(t, filter='data')
manifest=json.loads((consumer/'build/staged-bundle-manifest.json').read_text())
for name, identity in manifest['files'].items():
    path=t/name
    if path.stat().st_size != identity['bytes'] or sha(path) != identity['sha256']:
        raise SystemExit(f'Extracted consumer file differs: {name}')
archive.rename(archive.with_name('consumer-bundle.tar.gz'))
frontend=t/'emulator/build/fab-source/target/release/fab-agon-emulator'
adapter=t/'emulator/build/flexe-vdp.so'
if not os.access(frontend, os.X_OK) or not adapter.is_file():
    raise SystemExit('Build the native Oryx frontend/adapter with emulator/stage_oryx.sh first')
def stopped():
    for child in Path('/proc').glob('[0-9]*'):
        try:
            if (child/'exe').resolve() == frontend.resolve():
                raise RuntimeError('Close the dedicated Oryx frontend before staging')
        except (FileNotFoundError, PermissionError, ProcessLookupError): pass
stopped()
python=str(project/'.venv/bin/python'); profile=t/'.emulator'
command=[python,str(consumer/'profile.py'),'--profile',str(profile),
         '--frontend',str(frontend),'--adapter',str(adapter),
         '--firmware',str(consumer/'build/staged-firmware.bin'),
         '--modules',str(consumer/'build/staged-modules'),
         '--setup-script',str(consumer/'build/canonical-setup-emulator.py')]
elf=consumer/'build/staged-firmware.elf'
if 'consumer/build/staged-firmware.elf' in manifest['files']:
    command += ['--elf',str(elf)]
subprocess.run(command,check=True)
evidence=t/'evidence'; evidence.mkdir(exist_ok=True)
host=dict(schema=1,created_utc=datetime.now(timezone.utc).isoformat(),
          hostname=socket.gethostname(),architecture=platform.machine(),
          uname=list(os.uname()),python=sys.executable,
          source_bundle_sha256=sys.argv[2],source_fixture=manifest['fixture'],
          frontend_sha256=sha(frontend),adapter_sha256=sha(adapter),
          graphical_instance_launched=False)
(evidence/'oryx-host.json').write_text(json.dumps(host,indent=2)+'\n')
names=['oryx-host.json']
fixture=manifest['fixture']
for engine in ('jit','interp'):
    subprocess.run([python,str(consumer/'qualify.py'),'--profile',str(profile),
                    '--engine',engine,'--timeout',sys.argv[3]],check=True)
    for suffix in ('.json','.dat','-expected.json'):
        source=evidence/f'{fixture}-{engine}{suffix}'
        name=f'oryx-{fixture}-{engine}{suffix}'
        (evidence/name).write_bytes(source.read_bytes()); names.append(name)
    for artifact in ('final.ppm','esp32-uart0.log','frontend.log','stats.json'):
        name=f'oryx-{fixture}-{engine}-{artifact}'
        (evidence/name).write_bytes((profile/'last-run'/artifact).read_bytes())
        names.append(name)
stopped()
receipts={name:sha(evidence/name) for name in names}
evidence_archive=consumer/'build/oryx-evidence.tar.gz'
with tarfile.open(evidence_archive,'w:gz') as output:
    for name in names: output.add(evidence/name,arcname=name,recursive=False)
    data=(json.dumps({'schema':1,'files':receipts},indent=2)+'\n').encode()
    info=tarfile.TarInfo('oryx-evidence-manifest.json'); info.size=len(data); info.mode=0o644
    output.addfile(info,io.BytesIO(data))
print(json.dumps({'evidence_archive_sha256':sha(evidence_archive),
                  'fixture':fixture,'profile':str(profile),'stopped':True}))
PY
'''


def stage(archive: Path, receipt: dict, timeout: float):
    remote_build = REMOTE_TASK + '/consumer/build'
    run([*SSH, 'mkdir', '-p', remote_build])
    run(['scp', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10', str(archive),
         f'{HOST}:{remote_build}/consumer-bundle.tar.gz.new'], timeout=180)
    output = run([*SSH, 'bash', '-s', '--', receipt['archive_sha256'], str(timeout)],
                 script=REMOTE_RUN, timeout=2 * timeout + 180)
    print(output, end='', flush=True)
    remote = json.loads(output.splitlines()[-1])
    local_archive = HERE / 'build/oryx-evidence.tar.gz'
    run(['scp', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10',
         f'{HOST}:{remote_build}/oryx-evidence.tar.gz', str(local_archive)], timeout=180)
    if digest(local_archive.read_bytes()) != remote['evidence_archive_sha256']:
        raise ValueError('Retrieved Oryx evidence archive hash mismatch')
    evidence = TASK / 'evidence'
    evidence.mkdir(exist_ok=True)
    with tarfile.open(local_archive) as source:
        for member in source.getmembers():
            if not member.isfile() or not member.name.startswith('oryx-') or Path(member.name).name != member.name:
                raise ValueError(f'Unexpected evidence archive member: {member.name!r}')
        source.extractall(evidence, filter='data')
    manifest = json.loads((evidence / 'oryx-evidence-manifest.json').read_text())
    for name, expected in manifest['files'].items():
        if not name.startswith('oryx-') or Path(name).name != name:
            raise ValueError(f'Unexpected evidence filename: {name!r}')
        if digest((evidence / name).read_bytes()) != expected:
            raise ValueError(f'Retrieved Oryx evidence differs: {name}')
    print(f'Oryx {remote["fixture"]} passes JIT and interpreter; GUI remains stopped.\n'
          f'Retrieved {len(manifest["files"])} evidence files under {evidence}/oryx-*.',
          flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', type=Path, default=TASK / '.emulator',
                        help='Exact already-staged local profile to bundle')
    parser.add_argument('--bundle-only', action='store_true',
                        help='Validate and bundle locally without SSH or launching any emulator')
    parser.add_argument('--timeout', type=float, default=180,
                        help='Independent remote qualification wall limit per engine')
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    if not args.bundle_only:
        identity = run([*SSH, 'bash', '-s'], script=r'''
set -euo pipefail
test "$(hostname)" = oryx-linuxlite
test -x "$HOME/Agon/mystuff/golem/.venv/bin/python"
hostname
uname -a
''')
        print('Verified canonical Oryx and project venv:\n' + identity, flush=True)
    archive, receipt = bundle(args.profile.resolve())
    if not args.bundle_only:
        stage(archive, receipt, args.timeout)


if __name__ == '__main__':
    main()
