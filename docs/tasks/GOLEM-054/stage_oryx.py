"""Stage and headlessly qualify actual Fab on Oryx, then leave review stopped.

This copies only GOLEM-054 artifacts and its dedicated profile. The existing
GOLEM-051/052/053 review instances, shared runtime and upstream code are read-only.
"""
from pathlib import Path
import argparse
import json
import shlex
import subprocess
import tarfile

import qualify
HERE, ROOT, BUILD = qualify.HERE, qualify.ROOT, qualify.BUILD
HOST = 'oryx-linuxlite'
REMOTE = 'Agon/mystuff/golem'
PROFILE = REMOTE + '/.emulator/golem-054'


def ssh(code):
    return subprocess.run(['ssh', '-o', 'BatchMode=yes', HOST, 'python3 -'],
                          input=code.encode(), check=True)


def stage(*, review=False):
    destination, auto, receipt, reference = qualify.prepare(review=review)
    (BUILD / 'review.txt').write_bytes(auto.replace('\n', '\r\n').encode())
    bundle = BUILD / 'oryx.tar'
    with tarfile.open(bundle, 'w') as archive:
        for name in ('qualify.py', 'oracle.py'):
            archive.add(HERE / name, arcname='docs/tasks/GOLEM-054/' + name)
        for name in ('qualify.py', 'sdl_io.c', 'ppm_png.py'):
            archive.add(ROOT / 'docs/tasks/GOLEM-051' / name, arcname='docs/tasks/GOLEM-054/build/helper051/' + name)
        for path in reference.iterdir():
            archive.add(path, arcname='docs/tasks/GOLEM-054/build/target-reference/' + path.name)
        for name in ('numbers.golem', 'cases.dat', 'review.bin', 'expected.json'):
            archive.add(destination / name, arcname='.emulator/golem-054/sdcard/golem/floats/' + name)
        archive.add(ROOT / 'docs/tasks/GOLEM-051/build/support/golemc.bin',
                    arcname='.emulator/golem-054/sdcard/mos/golemc.bin')
        archive.add(ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin',
                    arcname='.emulator/golem-054/sdcard/golem/compiler/portable.bin')
        archive.add(BUILD / 'review.txt', arcname='.emulator/golem-054/sdcard/autoexec.txt')
    ssh('''from pathlib import Path
import runpy,shutil
base=Path.home()/'Agon/mystuff';p=base/'golem/.emulator/golem-054'
for proc in Path('/proc').glob('[0-9]*'):
 try:
  if str(p/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\\0'):raise RuntimeError('GOLEM-054 profile is running; preserve it')
 except OSError:pass
setup=runpy.run_path(str(base/'agon-dev-env/scripts/setup_emulator.py'))
if p.exists():
 import subprocess
 subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=p,check=True)
mos=p/'sdcard/mos';hold=p/'.mos-local';preserve=mos.is_dir() and not mos.is_symlink()
if preserve:mos.rename(hold)
try:sd=setup['link_shared_runtime'](p,base/'golem/.emulator/runtime/fab-1.2.5')
finally:
 if preserve:
  if mos.is_symlink():mos.unlink()
  hold.rename(mos)
if mos.is_symlink():
 origin=mos.resolve();mos.unlink();shutil.copytree(origin,mos)
for directory in ('golem/floats','golem/compiler','golem/cache','golem/output'):(sd/directory).mkdir(parents=True,exist_ok=True)
''')
    subprocess.run(['scp', str(bundle), HOST + ':' + PROFILE + '/review-files.tar'], check=True)
    ssh('''from pathlib import Path
import tarfile,hashlib,json,shutil
root=Path.home()/'Agon/mystuff/golem';p=root/'.emulator/golem-054';destination=p/'sdcard/golem/floats'
with tarfile.open(p/'review-files.tar') as archive:archive.extractall(root,filter='data')
shutil.rmtree(destination/'package',ignore_errors=True)
for name in ('done.dat','results.dat','scene.dat','advance.dat','keyboard.dat'):(destination/name).unlink(missing_ok=True)
# Each invocation begins with fresh observation files, including reviewer reruns.
w=p/'fab-agon-emulator';content=w.read_text();anchor='cd "${profile_dir}"\\n'
reset='rm -f -- "$profile_dir/sdcard/golem/floats/done.dat" "$profile_dir/sdcard/golem/floats/results.dat" "$profile_dir/sdcard/golem/floats/scene.dat" "$profile_dir/sdcard/golem/floats/advance.dat" "$profile_dir/sdcard/golem/floats/keyboard.dat"\\n'
assert content.count(anchor)==1
w.write_text(content.replace(anchor,anchor+reset,1))
names=['fab-agon-emulator','fab-agon-emulator.bin','mos_platform.bin','mos_platform.map','firmware/vdp_platform.so']
pins={name:hashlib.sha256((p/name).read_bytes()).hexdigest() for name in names}
(p/'.runtime.sha256').write_text(''.join(f'{value}  {name}\\n' for name,value in pins.items()))
(p/'identity.json').write_text(json.dumps(pins,indent=2)+'\\n')
print('Staged actual Fab and left stopped:',p)
''')
    report = {'host': HOST, 'profile': PROFILE, 'product': 'Fab Agon Emulator 1.2.5',
              'MOS': '3.0.2', 'stock_native_VDP': True, 'GUI_launched': False,
              'human': 'pending', 'mode': 'review' if review else 'automated',
              'native_eZ80_compiler_sha256': qualify.sha(ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin'),
              'expected_texts': [case['text'] for case in receipt['cases']]}
    (HERE / 'deployment.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def run_headless():
    command = ('cd ' + shlex.quote(REMOTE) + ' && .venv/bin/python '
               'docs/tasks/GOLEM-054/qualify.py --staged')
    subprocess.run(['ssh', '-o', 'BatchMode=yes', HOST, command], check=True)
    subprocess.run(['scp', HOST + ':' + REMOTE + '/docs/tasks/GOLEM-054/stock-evidence.json',
                    str(HERE / 'stock-oryx-evidence.json')], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--review', action='store_true', help='stage the 14-row stopped human review')
    parser.add_argument('--qualify', action='store_true', help='run automated exact-pixel qualification before returning')
    arguments = parser.parse_args()
    assert not (arguments.review and arguments.qualify)
    stage(review=arguments.review)
    if arguments.qualify:
        run_headless()
