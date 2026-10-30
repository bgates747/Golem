"""Stage/qualify only GOLEM-055's actual-Fab Oryx profile; leave it stopped."""
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
PROFILE = REMOTE + '/.emulator/golem-055'


def ssh(code):
    return subprocess.run(['ssh', '-o', 'BatchMode=yes', HOST, 'python3 -'],
                          input=code.encode(), check=True)


def stage(*, review=False):
    destination, auto, receipt, reference = qualify.prepare(review=review)
    (BUILD / 'review.txt').write_bytes(auto.replace('\n', '\r\n').encode())
    bundle = BUILD / 'oryx.tar'
    with tarfile.open(bundle, 'w') as archive:
        for name in ('qualify.py', 'review_headless.py'):
            archive.add(HERE / name, arcname='docs/tasks/GOLEM-055/' + name)
        for name in ('qualify.py', 'sdl_io.c', 'ppm_png.py'):
            archive.add(ROOT / 'docs/tasks/GOLEM-051' / name,
                        arcname='docs/tasks/GOLEM-055/build/helper051/' + name)
        for source, target in [(reference, 'reference'),
                               (BUILD / 'control-buffered', 'control-buffered'),
                               (BUILD / 'control-native', 'control-native')]:
            for path in source.iterdir():
                archive.add(path, arcname='docs/tasks/GOLEM-055/build/' + target + '/' + path.name)
        for name in ('two-floats.golem', 'two-floats.args', 'control.golem', 'cases.dat', 'review.bin', 'expected.json'):
            archive.add(destination / name, arcname='.emulator/golem-055/sdcard/golem/ram/' + name)
        archive.add(ROOT / 'docs/tasks/GOLEM-051/build/support/golemc.bin',
                    arcname='.emulator/golem-055/sdcard/mos/golemc.bin')
        archive.add(ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin',
                    arcname='.emulator/golem-055/sdcard/golem/compiler/portable.bin')
        archive.add(ROOT / 'docs/tasks/GOLEM-051/build/support/golemrun.bin',
                    arcname='.emulator/golem-055/sdcard/golem/golemrun.bin')
        archive.add(BUILD / 'review.txt', arcname='.emulator/golem-055/sdcard/autoexec.txt')
    ssh('''from pathlib import Path
import runpy,shutil,subprocess
base=Path.home()/'Agon/mystuff';p=base/'golem/.emulator/golem-055'
for proc in Path('/proc').glob('[0-9]*'):
 try:
  if str(p/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\\0'):raise RuntimeError('GOLEM-055 is running; preserve it')
 except OSError:pass
if p.exists():
 if not (p/'.runtime.sha256').exists():raise RuntimeError('Refuse unowned existing GOLEM-055 profile')
 subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=p,check=True)
setup=runpy.run_path(str(base/'agon-dev-env/scripts/setup_emulator.py'))
mos=p/'sdcard/mos';hold=p/'.mos-local';preserve=mos.is_dir() and not mos.is_symlink()
if preserve:mos.rename(hold)
try:sd=setup['link_shared_runtime'](p,base/'golem/.emulator/runtime/fab-1.2.5')
finally:
 if preserve:
  if mos.is_symlink():mos.unlink()
  hold.rename(mos)
if mos.is_symlink():
 origin=mos.resolve();mos.unlink();shutil.copytree(origin,mos)
for name in ('golem/ram','golem/compiler','golem/cache'):(sd/name).mkdir(parents=True,exist_ok=True)
''')
    subprocess.run(['scp', str(bundle), HOST + ':' + PROFILE + '/review-files.tar'], check=True)
    ssh('''from pathlib import Path
import tarfile,hashlib,json,shutil
root=Path.home()/'Agon/mystuff/golem';p=root/'.emulator/golem-055';d=p/'sdcard/golem/ram'
with tarfile.open(p/'review-files.tar') as archive:archive.extractall(root,filter='data')
for name in ('package','package-direct','package-response','control','native','rejected'):shutil.rmtree(d/name,ignore_errors=True)
for name in ('done.dat','results.dat','scene.dat','advance.dat','keyboard.dat','runner.dat','batch.log','batch-errors.log','batch-output.log'):(d/name).unlink(missing_ok=True)
w=p/'fab-agon-emulator';content=w.read_text();anchor='cd "${profile_dir}"\\n'
reset='rm -f -- "$profile_dir/sdcard/golem/ram/done.dat" "$profile_dir/sdcard/golem/ram/results.dat" "$profile_dir/sdcard/golem/ram/scene.dat" "$profile_dir/sdcard/golem/ram/advance.dat" "$profile_dir/sdcard/golem/ram/keyboard.dat" "$profile_dir/sdcard/golem/ram/runner.dat"\\n'
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
              'mode': 'review' if review else 'automated', 'human': 'pending',
              'native_eZ80_compiler_sha256': receipt['worker_sha256'],
              'source_sha256': receipt['source_sha256'],
              'expected_texts': [case['text'] for case in receipt['cases']]}
    (HERE / 'deployment.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def run_headless():
    command = ('cd ' + shlex.quote(REMOTE) + ' && .venv/bin/python docs/tasks/GOLEM-055/qualify.py --staged')
    subprocess.run(['ssh', '-o', 'BatchMode=yes', HOST, command], check=True)
    subprocess.run(['scp', HOST + ':' + REMOTE + '/docs/tasks/GOLEM-055/stock-evidence.json',
                    str(HERE / 'stock-oryx-evidence.json')], check=True)


def capture_review():
    stage(review=True)
    command = ('cd ' + shlex.quote(REMOTE) + ' && .venv/bin/python docs/tasks/GOLEM-055/review_headless.py')
    subprocess.run(['ssh', '-o', 'BatchMode=yes', HOST, command], check=True)
    for name in ('review-headless-evidence.json', 'review-reference.png'):
        subprocess.run(['scp', HOST + ':' + REMOTE + '/docs/tasks/GOLEM-055/' + name,
                        str(HERE / name)], check=True)
    # Prepare fresh observations for the Author's own run and restore the
    # generated launcher plus its reset commands after the headless session.
    stage(review=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--review', action='store_true')
    parser.add_argument('--qualify', action='store_true')
    parser.add_argument('--capture-review', action='store_true')
    args = parser.parse_args()
    assert sum((args.review, args.qualify, args.capture_review)) <= 1
    if args.capture_review:
        capture_review()
    else:
        stage(review=args.review)
        if args.qualify:
            run_headless()
