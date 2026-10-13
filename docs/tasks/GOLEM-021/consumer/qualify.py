"""Run the isolated proof headlessly and save fresh runtime evidence."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time
from verify import verify, verify_pixels

HERE = Path(__file__).resolve().parent
TASK = HERE.parent

def qualify(profile: Path, engine: str, timeout: float):
    result = profile / 'sdcard/vdplet/results.dat'
    result.unlink(missing_ok=True)
    (result.parent / 'failure.dat').unlink(missing_ok=True)
    expected_path = result.parent / 'expected.json'
    expected = json.loads(expected_path.read_text())
    artifacts = profile / 'last-run'
    artifacts.mkdir(exist_ok=True)
    for name in ('frontend.log', 'esp32-uart0.log', 'stats.json', 'final.ppm'):
        (artifacts / name).unlink(missing_ok=True)
    env = {k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD', 'BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
               GOLEM_HYBRID_PROFILE=str(profile), AGON_FLEXE_ENGINE=engine)
    started = time.monotonic()
    process = None
    mapped = set()
    try:
        with (artifacts / 'frontend.log').open('w') as log:
            process = subprocess.Popen(['bash', str(TASK / 'review.sh')],
                         env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            while not result.exists():
                if process.poll() is not None:
                    raise RuntimeError(f'Emulator exited {process.returncode}; inspect {artifacts}/frontend.log')
                if time.monotonic() - started > timeout:
                    raise TimeoutError(f'No saved proof in {timeout}s; inspect {artifacts}')
                for child in Path('/proc').glob('[0-9]*'):
                    try:
                        if os.getpgid(int(child.name)) != process.pid:
                            continue
                        lines = (child / 'maps').read_text().splitlines()
                    except (PermissionError, FileNotFoundError, ProcessLookupError):
                        continue
                    mapped.update(line.split(maxsplit=5)[5] for line in lines
                                  if str(profile / 'flexe-vdp.so') in line)
                time.sleep(0.1)
            # File creation precedes completion of its write on some hosts.
            until = time.monotonic() + 3
            while result.stat().st_size < expected['reply_bytes'] and time.monotonic() < until:
                time.sleep(0.05)
            receipt = verify(result, expected_path)
            # Saving results precedes printing instructions and returning to
            # MOS. Keep execution alive until actual firmware consumes both.
            marker = b'VDPLET: uploaded native code passed.'
            while True:
                debug = (artifacts / 'esp32-uart0.log').read_bytes()
                after = debug.partition(marker)[2]
                if b'/vdplet *' in after:
                    break
                if process.poll() is not None:
                    raise RuntimeError('Frontend exited before return to the MOS prompt')
                if time.monotonic() - started > timeout:
                    raise TimeoutError('Native results passed, but return to MOS was not observed')
                time.sleep(0.1)
    finally:
        if process and process.poll() is None:
            # Ask the frontend to use its normal VDP cleanup path. Keep its
            # launcher alive long enough to flush captures and remove temp cwd.
            for child in Path('/proc').glob('[0-9]*'):
                try:
                    pid = int(child.name)
                    if os.getpgid(pid) == process.pid and (child / 'exe').resolve() == (profile / 'fab-agon-emulator.bin').resolve():
                        os.kill(pid, signal.SIGTERM)
                except (PermissionError, FileNotFoundError, ProcessLookupError):
                    pass
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    if not mapped:
        raise RuntimeError('No Flexe backend mapping was observed')
    if process.returncode != 0:
        raise RuntimeError(f'Frontend shutdown failed: exit {process.returncode}')
    stats = json.loads((artifacts / 'stats.json').read_text())
    if any(stats.get(name) for name in ('failed', 'unhandled', 'unregistered', 'unmapped')):
        raise RuntimeError(f'Backend reported unresolved execution: {stats}')
    if expected.get('packed_asset'):
        receipt.update(verify_pixels(artifacts / 'final.ppm', result.parent / 'loderunner.packed'))
    receipt.update(engine=engine, elapsed_seconds=round(time.monotonic()-started, 3),
                   mapped_vdp_libraries=sorted(mapped), graphical_instance_launched=False,
                   returned_to_mos_prompt=True, backend_stats=stats,
                   runtime_identities=(profile / '.runtime.sha256').read_text().splitlines(),
                   expected_sha256=hashlib.sha256(expected_path.read_bytes()).hexdigest())
    evidence = TASK / 'evidence'
    evidence.mkdir(exist_ok=True)
    prefix = f'{expected.get("fixture", "numeric")}-{engine}'
    (evidence / f'{prefix}.json').write_text(json.dumps(receipt, indent=2)+'\n')
    (evidence / f'{prefix}.dat').write_bytes(result.read_bytes())
    (evidence / f'{prefix}-expected.json').write_bytes(expected_path.read_bytes())
    return receipt

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--profile', type=Path, default=TASK / '.emulator')
    p.add_argument('--engine', choices=['jit', 'interp'], default='jit')
    p.add_argument('--timeout', type=float, default=180)
    a = p.parse_args()
    qualify(a.profile.resolve(), a.engine, a.timeout)
