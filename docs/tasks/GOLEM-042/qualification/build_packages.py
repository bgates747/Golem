"""Build changed native packages and finite MOS fixtures without firmware edits."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SDK = Path(os.environ.get('GOLEM_VDPLET_SDK', ROOT.parent / 'agon-vdplet')).resolve()


def run(args):
    p = subprocess.run(list(map(str, args)), capture_output=True, text=True)
    if p.returncode: raise RuntimeError(p.stdout + p.stderr)
    return p.stdout


def build():
    receipts = []
    for name, source, relocated in [('probe', 'probe', False), ('probe-relocated', 'probe', True),
                                     ('animation', 'animation', False)]:
        out = HERE / 'build' / name; out.mkdir(parents=True, exist_ok=True)
        args = [ROOT / 'build/golemc', '--portable']
        if relocated: args += ['--reserve', '1:254']
        run(args + [HERE / (source + '.golem'), out])
        run([sys.executable, SDK / 'tools/build_module.py', out / 'native.c', '--name', source,
             '--build-id', '0x474c3432', '--output', out])
        meta = json.loads((out / (source + '.json')).read_text())
        run([SDK / 'build/check_image', out / (source + '.vdpl'), meta['module']['relative_relocations']])
        receipts.append(dict(package=name, module=meta['module'],
                             native_c_sha256=hashlib.sha256((out / 'native.c').read_bytes()).hexdigest()))
    (HERE / 'packages.json').write_text(json.dumps(receipts, indent=2) + '\n')
    print(json.dumps([{'package': r['package'], 'image_bytes': r['module']['package_bytes']} for r in receipts]))


if __name__ == '__main__': build()
