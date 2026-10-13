"""Prepare and qualify only this silo's stock Oryx profile, leaving it stopped."""
from __future__ import annotations
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[3]
PROFILE = HERE / '.emulator'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(variant):
    package = HERE / 'build' / variant
    meta = json.loads((package / 'expected.json').read_text())
    app = PROFILE / 'sdcard/golem022'
    app.mkdir(exist_ok=True)
    shutil.copyfile(package / 'probe.bin', app / 'probe.bin')
    shutil.copyfile(HERE / 'build/wolf-review.agnb', app / 'wolf-review.agnb')
    result = app / 'results.dat'
    result.unlink(missing_ok=True)
    (PROFILE / 'sdcard/autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem022\r\nload probe.bin\r\nrun .\r\n')
    expected = (package / 'expected.dat').read_bytes()
    identities = {name: sha(PROFILE / name) for name in
                  ('fab-agon-emulator', 'fab-agon-emulator.bin', 'mos_platform.bin',
                   'mos_platform.map', 'firmware/vdp_platform.so')}
    identities.update({'sdcard/golem022/probe.bin': sha(app / 'probe.bin'),
                       'sdcard/golem022/wolf-review.agnb': sha(app / 'wolf-review.agnb')})
    environment = {k: v for k, v in os.environ.items() if k not in ('LD_PRELOAD', 'BASH_ENV')}
    environment.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    start, process, mapped = time.monotonic(), None, set()
    try:
        with (HERE / 'evidence' / (variant+'.log')).open('w') as log:
            process = subprocess.Popen(['./fab-agon-emulator', '--renderer', 'sw'], cwd=PROFILE,
                                       env=environment, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            while not result.exists() or result.stat().st_size < len(expected):
                if process.poll() is not None:
                    raise RuntimeError(f'{variant}: frontend exited before results')
                try:
                    maps = Path(f'/proc/{process.pid}/maps').read_text().splitlines()
                    mapped.update(str(Path(line.split(maxsplit=5)[5]).resolve()) for line in maps
                                  if '/vdp_' in line and '.so' in line)
                except FileNotFoundError:
                    pass
                if time.monotonic()-start > 60:
                    raise TimeoutError(f'{variant}: no complete results; inspect evidence/{variant}.log')
                time.sleep(.05)
            # Give the finite guest its last message/return time. Keyboard and
            # actual MOS prompt remain explicit human checks, not inferred here.
            time.sleep(.4)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    assert mapped == {str((PROFILE / 'firmware/vdp_platform.so').resolve())}, mapped
    actual = result.read_bytes()
    (HERE / 'evidence' / (variant+'.dat')).write_bytes(actual)
    if actual != expected:
        wrong = [c for c in meta['checks'] if actual[c['start']:c['start']+c['bytes']] !=
                 expected[c['start']:c['start']+c['bytes']]]
        raise AssertionError(f'{variant}: {len(wrong)} mismatches, first={wrong[:8]}')
    assert all(sha(PROFILE / name) == value for name, value in identities.items())
    receipt = dict(variant=variant, passed=True, checks=len(meta['checks']), observed_bytes=len(actual),
                   expected_sha256=meta['expected_sha256'], observed_sha256=sha(result),
                   execution_seconds=round(time.monotonic()-start, 3),
                   host=os.uname().nodename, architecture=os.uname().machine,
                   profile=str(PROFILE), loaded_vdp=sorted(mapped), runtime_identities=identities,
                   entry_calls=meta['entry_calls'], installs=meta['installs'],
                   graphical_emulator_launched=False, emulator_stopped=True,
                   MOS_prompt_and_keyboard='pending human review')
    (HERE / 'evidence' / (variant+'.json')).write_text(json.dumps(receipt, indent=2)+'\n')
    print(f'{variant}: {len(meta["checks"])} checks, {len(actual)} saved bytes, {receipt["execution_seconds"]}s')

def main():
    assert os.uname().nodename == 'oryx-linuxlite'
    for name, value in json.loads((HERE / 'build/bundle-manifest.json').read_text()).items():
        assert sha(HERE/name) == value, name
    spec = importlib.util.spec_from_file_location('canonical_setup', HERE / 'build/canonical-setup-emulator.py')
    canonical = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(canonical)
    runtime = PROJECT / '.emulator/runtime/fab-1.2.5'
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            if (proc/'exe').resolve() == (runtime/'fab-agon-emulator').resolve():
                raise RuntimeError('Close the stock emulator before staging')
        except (FileNotFoundError, PermissionError):
            pass
    canonical.link_shared_runtime(PROFILE, runtime)
    assert not (PROFILE/'fab-agon-emulator').is_symlink()
    assert not (PROFILE/'.bespoke-vdp-profile').exists()
    for name, value in json.loads((HERE / 'stock-runtime.json').read_text()).items():
        assert sha(PROFILE/name) == value, name
    (HERE / 'evidence').mkdir(exist_ok=True)
    for variant in ('default', 'relocated', 'review'):
        run(variant)
    # The last run selected the actual Golem-only scene. Remove its machine
    # result so the Author's next launch produces fresh review observations.
    (PROFILE/'sdcard/golem022/results.dat').unlink()
    (HERE/'evidence/host.json').write_text(json.dumps(dict(hostname=os.uname().nodename,
        uname=list(os.uname()), emulator_stopped=True, human_fixture='review',
        result_removed_for_fresh_human_review=True), indent=2)+'\n')
    print('Dedicated stock profile ready and stopped; human visual/keyboard review is pending.')

if __name__ == '__main__':
    main()
