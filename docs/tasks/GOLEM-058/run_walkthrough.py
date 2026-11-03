#!/usr/bin/env python3
"""Qualify the stopped consumer walkthrough once, then leave it for review."""
import argparse
import json
import math
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path
from stage_stock import sha


def run(profile, output, lease):
    here = Path(__file__).resolve().parent
    identity = json.loads((profile / 'stock-profile.json').read_text())
    for name, expected in identity['pins'].items():
        assert sha(profile / name) == expected, name
    assert not output.exists()
    output.mkdir(parents=True)
    app = profile / 'sdcard/golem'
    previous = output / 'previous-artifacts'
    previous.mkdir()
    for p in (app / 'review-done.dat', profile / 'last-run'):
        if p.exists():
            p.rename(previous / p.name)
    idle = json.loads(subprocess.check_output([sys.executable, str(here / 'inspect_oryx.py')], text=True, timeout=10))
    stamp = idle['epoch']
    assert idle['machine'] == 'oryx-linuxlite' and not idle['emulators'] and math.isfinite(stamp) and 0 <= time.time()-stamp <= 5
    subprocess.run(['ssh','-o','BatchMode=yes','raspberrypi','/Agon/mystuff/agon-dev-env/.venv/bin/python','/Agon/mystuff/agon-dev-env/cross-agent/vdplet-rollout/oryx_lease.py','assert-owner','--owner','golem','--lease',lease], check=True, capture_output=True, timeout=10)
    assert 0 <= time.time()-stamp <= 5
    (output / 'before-idle.json').write_text(json.dumps(idle, indent=2) + '\n')
    start = time.monotonic()
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    env.pop('LD_PRELOAD', None)
    env.pop('BASH_ENV', None)
    process = None
    success = False
    try:
        with (output / 'frontend.log').open('x') as log:
            process = subprocess.Popen(['./flexe-agon-emulator'], cwd=profile, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            while True:
                if process.poll() is not None:
                    raise RuntimeError('Walkthrough frontend exited early')
                if time.monotonic()-start > 240:
                    raise TimeoutError('Finite walkthrough preparation deadline')
                if (app / 'review-done.dat').exists() and (app / 'review-done.dat').stat().st_size == 3:
                    wire = profile / 'last-run/mos-to-vdp.bin'
                    if wire.exists() and b'/golem *' in wire.read_bytes().partition(b'Type dir at the MOS prompt')[2]:
                        success = True
                        break
                time.sleep(.1)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
                success = False
        deadline = time.monotonic()+15
        while True:
            closing = json.loads(subprocess.check_output([sys.executable, str(here / 'inspect_oryx.py')], text=True, timeout=10))
            if not closing['emulators']:
                break
            if time.monotonic() > deadline:
                raise TimeoutError('Owned walkthrough frontend did not stop')
            time.sleep(.05)
        if (profile / 'last-run').exists():
            shutil.copytree(profile / 'last-run', output / 'runtime')
        if (app / 'review-done.dat').exists():
            shutil.copyfile(app / 'review-done.dat', output / 'review-done.dat')
    assert success
    for name in ('hello.bin','counter','float-print','bitwise','cube'):
        expected = here / 'reference' / name
        actual = app / 'output' / name
        paths = [expected] if expected.is_file() else [p for p in expected.rglob('*') if p.is_file()]
        for p in paths:
            q = actual if expected.is_file() else actual / p.relative_to(expected)
            assert q.read_bytes() == p.read_bytes(), q
    stats = json.loads((output / 'runtime/stats.json').read_text())
    assert not stats['failed'] and all(stats[k] == 0 for k in ('unmapped','unhandled','unregistered'))
    magic, size, scale, rgb = (output / 'runtime/final.ppm').read_bytes().split(b'\n',3)
    assert (magic,size,scale,len(rgb)) == (b'P6',b'1024 768',b'255',1024*768*3)
    anchors = [(256,288),(576,288),(576,608),(256,608),(448,448),(768,448),(768,128),(448,128)]
    for x,y in anchors:
        assert rgb[(y*1024+x)*3:(y*1024+x)*3+3] == b'\xff\xff\xff', (x,y)
    record = dict(passed=True, seconds=time.monotonic()-start, stock_firmware=True, native_example_compilations=5, exact_example_output=True, white_cube_anchors=len(anchors), actual_MOS_prompt=True, human_review='pending', archive_sha256=identity['archive_sha256'])
    (output / 'result.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record),flush=True)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('profile','output'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--lease',required=True)
    a=p.parse_args()
    run(a.profile.resolve(),a.output.resolve(),a.lease)
