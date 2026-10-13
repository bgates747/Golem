"""Transfer this small prepared stock fixture and retrieve Oryx qualification."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
RELATIVE = Path('docs/tasks/GOLEM-022/qualification')
SSH = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10', 'oryx-linuxlite']

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle-only', action='store_true')
    args = parser.parse_args()
    accepted = json.loads((PROJECT/'docs/tasks/GOLEM-007/oryx-review.json').read_text())['sha256']
    names = ('fab-agon-emulator.bin', 'mos_platform.bin', 'mos_platform.map', 'firmware/vdp_platform.so')
    (HERE/'stock-runtime.json').write_text(json.dumps({n: accepted['.emulator/'+n] for n in names}, indent=2)+'\n')
    files = sorted(p for p in HERE.iterdir() if p.is_file() and p.suffix in ('.py', '.golem', '.asm', '.inc', '.json', '.md'))
    files += [HERE/'build/wolf-review.agnb', HERE/'build/canonical-setup-emulator.py']
    for variant in ('default', 'relocated', 'review'):
        files += sorted(p for p in (HERE/'build'/variant).iterdir() if p.is_file())
    manifest = {str(p.relative_to(HERE)): sha(p) for p in files}
    (HERE/'build/bundle-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    bundle = HERE/'build/oryx-bundle.tar'
    with tarfile.open(bundle, 'w') as archive:
        for path in [*files, HERE/'build/bundle-manifest.json']:
            archive.add(path, arcname=str(path.relative_to(HERE)))
    print(f'Bundle: {len(manifest)} files, {bundle.stat().st_size} bytes, SHA256 {sha(bundle)}')
    if args.bundle_only:
        return
    subprocess.run([*SSH, 'test "$(hostname)" = oryx-linuxlite && test -x ~/Agon/mystuff/golem/.venv/bin/python'], check=True)
    subprocess.run([*SSH, 'mkdir -p ~/Agon/mystuff/golem/'+str(RELATIVE/'build')], check=True)
    destination = 'oryx-linuxlite:Agon/mystuff/golem/'+str(RELATIVE/'build/oryx-bundle.tar')
    subprocess.run(['scp', str(bundle), destination], check=True)
    extract = ('from pathlib import Path; import hashlib,tarfile; '
               'p=Path.home()/"Agon/mystuff/golem"/'+repr(str(RELATIVE))+'; '
               'b=p/"build/oryx-bundle.tar"; '
               'assert hashlib.sha256(b.read_bytes()).hexdigest()=='+repr(sha(bundle))+'; '
               'tarfile.open(b).extractall(p,filter="data")')
    subprocess.run([*SSH, '~/Agon/mystuff/golem/.venv/bin/python -c '+shlex.quote(extract)], check=True)
    subprocess.run([*SSH, '~/Agon/mystuff/golem/.venv/bin/python ~/Agon/mystuff/golem/'+str(RELATIVE/'run_oryx.py')], check=True)
    (HERE/'evidence').mkdir(exist_ok=True)
    subprocess.run(['scp', '-r', 'oryx-linuxlite:Agon/mystuff/golem/'+str(RELATIVE/'evidence')+'/.', str(HERE/'evidence')], check=True)
    for variant in ('default', 'relocated', 'review'):
        receipt = json.loads((HERE/'evidence'/f'{variant}.json').read_text())
        actual = HERE/'evidence'/f'{variant}.dat'
        expected = HERE/'build'/variant/'expected.dat'
        assert receipt['passed'] and receipt['observed_sha256'] == sha(actual) == sha(expected)
    print('Oryx receipts/data retrieved and matched locally; review profile is stopped.')

if __name__ == '__main__':
    main()
