#!/usr/bin/env python3
"""One finite stock-MOS compilation/execution, with normal FIFO and shutdown."""
import argparse
import hashlib
import json
import math
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def run(profile, cohort, output, lease):
    assert not output.exists()
    plan = json.loads((cohort / 'cohort.json').read_text())
    selection = json.loads((cohort / 'selection.json').read_text())
    assert selection['selected'] and selection['cohort_sha256'] == sha(cohort / 'cohort.json')
    assert selection['maximum_processes'] == 1 and selection['engine'] == 'jit' and not selection['retry']
    for name, h in plan['files'].items():
        assert sha(cohort / name) == h, name
    identity = json.loads((profile / 'stock-profile.json').read_text())
    assert identity['cohort_sha256'] == sha(cohort / 'cohort.json')
    for name, h in identity['pins'].items():
        assert sha(profile / name) == h, name
    output.mkdir(parents=True)
    previous = output / 'previous-artifacts'
    previous.mkdir()
    app = profile / 'sdcard/golem'
    for name in ('probe-done.dat', 'probe-status.dat', 'probe-results.dat', 'continuation.dat', 'batch.log', 'batch-errors.log', 'batch-output.log'):
        if (app / name).exists():
            (app / name).rename(previous / name)
    if (profile / 'last-run').exists():
        (profile / 'last-run').rename(previous / 'last-run')
    idle = json.loads(subprocess.check_output([sys.executable, str(cohort / 'inspect_oryx.py')], text=True, timeout=10))
    stamp = idle['epoch']
    assert idle['machine'] == 'oryx-linuxlite' and not idle['emulators'] and math.isfinite(stamp) and 0 <= time.time()-stamp <= 5
    subprocess.run(['ssh','-o','BatchMode=yes','raspberrypi','/Agon/mystuff/agon-dev-env/.venv/bin/python','/Agon/mystuff/agon-dev-env/cross-agent/vdplet-rollout/oryx_lease.py','assert-owner','--owner','golem','--lease',lease], check=True, capture_output=True, timeout=10)
    assert 0 <= time.time()-stamp <= 5
    (output / 'before-idle.json').write_text(json.dumps(idle, indent=2) + '\n')
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    env.pop('BASH_ENV', None)
    env.pop('LD_PRELOAD', None)
    start = time.monotonic()
    process = None
    success = False
    try:
        with (output / 'frontend.log').open('x') as log:
            process = subprocess.Popen(['./flexe-agon-emulator','--renderer','sw'], cwd=profile, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            while True:
                if process.poll() is not None:
                    raise RuntimeError('Frontend exited before MOS continuation')
                if time.monotonic()-start > 600:
                    raise TimeoutError('Finite native compilation/execution deadline')
                if (app / 'batch.log').exists():
                    rows = [line.split() for line in (app / 'batch.log').read_text().splitlines()]
                    failed = [row for row in rows if len(row) == 3 and int(row[1])]
                    if failed:
                        raise RuntimeError('Native compilation failed: ' + repr(failed))
                if (app / 'continuation.dat').exists() and (app / 'continuation.dat').stat().st_size == 3:
                    wire = profile / 'last-run/mos-to-vdp.bin'
                    if wire.exists() and b'/golem *' in wire.read_bytes().partition(b'Directory: /golem')[2]:
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
            closing = json.loads(subprocess.check_output([sys.executable, str(cohort / 'inspect_oryx.py')], text=True, timeout=10))
            if not closing['emulators']:
                break
            if time.monotonic() > deadline:
                raise TimeoutError('Owned frontend did not finish shutdown')
            time.sleep(.05)
        if (profile / 'last-run').exists():
            shutil.copytree(profile / 'last-run', output / 'runtime')
        for name in ('probe-done.dat','probe-status.dat','probe-results.dat','continuation.dat','batch.log','batch-errors.log','batch-output.log'):
            if (app / name).exists():
                shutil.copyfile(app / name, output / name)
        shutil.copytree(app / 'output', output / 'native-output')
    assert success
    assert (output / 'probe-done.dat').read_bytes() == b'\0'
    assert (output / 'probe-status.dat').read_bytes() == b'\0'
    expected = bytes.fromhex(json.loads((cohort / 'checks.json').read_text())['expected'])
    assert (output / 'probe-results.dat').read_bytes() == expected
    log = [line.split() for line in (output / 'batch.log').read_text().splitlines()]
    assert len(log) == len(plan['jobs']) and all(int(line[1]) == 0 for line in log), log
    compared = []
    for job in plan['jobs']:
        ref = cohort / 'reference' / job['output']
        native = output / 'native-output' / job['output']
        if ref.is_file():
            assert ref.read_bytes() == native.read_bytes(), job['name']
            compared.append(job['output'])
        else:
            expected_files = {str(p.relative_to(ref)) for p in ref.rglob('*') if p.is_file()}
            actual_files = {str(p.relative_to(native)) for p in native.rglob('*') if p.is_file()}
            assert expected_files == actual_files
            for name in expected_files:
                assert (ref / name).read_bytes() == (native / name).read_bytes(), (job['name'], name)
                compared.append(job['output'] + '/' + name)
    stats = json.loads((output / 'runtime/stats.json').read_text())
    assert not stats['failed'] and all(stats[k] == 0 for k in ('unmapped','unhandled','unregistered'))
    result = dict(passed=True, seconds=time.monotonic()-start, native_MOS_compilations=len(log), identical_output_files=len(compared), stock_execution_observations=len(expected), normal_MOS_continuation=True, human_review=False, firmware_sha256=identity['source_component_pins']['esp32-firmware.bin'], archive_sha256=plan['archive_sha256'], compared=sorted(compared))
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'compared'}), flush=True)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('profile','cohort','output'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--lease', required=True)
    a = p.parse_args()
    run(a.profile.resolve(), a.cohort.resolve(), a.output.resolve(), a.lease)
