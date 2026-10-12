"""Stock Fab/MOS qualification; no firmware hooks or host execution model."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import os
import signal
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK / 'build'
PROFILE = PROJECT / '.emulator'
RUNTIME = PROFILE / 'runtime/fab-1.2.5'
SETUP = PROJECT.parent / 'agon-dev-env/scripts/setup_emulator.py'
QUERY, ASSET, FOREIGN = 60000, 60001, 50000


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return struct.pack('<H', value)


def packet(key, op):
    return bytes([23, 0, 160]) + word(key) + bytes([op])


def write(key, data):
    return packet(key, 2) + packet(key, 0) + word(len(data)) + data


def update(key, data):
    return packet(key, 5) + b'\xc2' + word(0) + word(len(data)) + data


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


class Tape:
    def __init__(self):
        self.data = bytearray()
        self.expected = bytearray()
        self.checks = []
        self.sent = 0

    def send(self, data):
        assert 0 < len(data) <= 65535
        self.data.extend(b'\x01' + word(len(data)) + data)
        self.sent += len(data)

    def read(self, key, offset, expected, label):
        assert expected and 0 <= offset < 65536 and offset + len(expected) <= 65536
        self.data.extend(b'\x02' + word(key) + word(offset) + word(len(expected)))
        self.checks.append({'label': label, 'id': key, 'offset': offset,
                            'start': len(self.expected), 'bytes': len(expected)})
        self.expected.extend(expected)


def generate(smoke=False):
    BUILD.mkdir(exist_ok=True)
    cli = PROJECT / 'docs/tasks/GOLEM-006/build/golemc-integrated'
    subprocess.run(['make', '-C', str(PROJECT / 'docs/tasks/GOLEM-006')], check=True)
    values = [-32768, -32767.998046875, -32767.5, -32767, -256.25,
              -2.000000238418579, -2, -1.9999998807907104, -1.5,
              -1.0000001192092896, -1, -0.9999999403953552, -0.75,
              -0.00006103515625, -2**-126, -2**-149, -0.0, 0.0,
              2**-149, 2**-126, 0.00006103515625, 0.75,
              0.9999999403953552, 1, 1.0000001192092896, 1.5,
              1.9999998807907104, 2, 2.000000238418579, 256.25,
              32766.5, 32766.998046875, 32767]
    if smoke:
        values = [-2**-149, -0.75, 32767]
    tape = Tape()
    tape.send(write(QUERY, bytes([23, 0, 128, 0])))
    tape.send(write(ASSET, bytes(range(256))))
    tape.read(ASSET, 0, bytes(range(256)), 'all 256 byte values')
    packages = []
    for mixed in (False, True):
        for relocated in (False, True):
            name = ('mixed' if mixed else 'floor') + ('-relocated' if relocated else '-default')
            output = BUILD / name
            source = PROJECT / 'docs/tasks/GOLEM-006/examples' / ('counted-floor.golem' if mixed else 'floor.golem')
            command = [str(cli), '--resident', '--reserve', '50000:50000', '--reserve', '60000:60002']
            canaries = {0: b'root', FOREIGN: b'\x00\xff\x55\xaa'}
            if relocated:
                command += ['--reserve', '1:254', '--reserve', '260:264']
                canaries.update({key: word(key) + b'\xa5' for key in (1, 254, 260, 264)})
            command += [str(source), str(output)]
            subprocess.run(command, check=True)
            manifest = json.loads((output / 'interface.json').read_text())
            owned = {item['id']: item for item in manifest['owned']}
            assert not set(owned).intersection({*canaries, QUERY, ASSET})
            for key, data in canaries.items():
                tape.send(write(key, data))
            bootstrap = (output / 'bootstrap.bin').read_bytes()
            cleanup = (output / 'cleanup.bin').read_bytes()
            invocation = (output / 'invoke.bin').read_bytes()
            # Obtain exact installed program bytes from the serialized writes,
            # not from the host model. This slice's bootstrap has one block/ID.
            blocks = {}
            pos = 7  # Stock affine-support enable prefix.
            while pos < len(bootstrap):
                assert bootstrap[pos:pos+3] == bytes([23, 0, 160])
                key = struct.unpack_from('<H', bootstrap, pos + 3)[0]
                op = bootstrap[pos + 5]
                if op == 2:
                    pos += 6
                elif op == 0:
                    size = struct.unpack_from('<H', bootstrap, pos + 6)[0]
                    blocks[key] = bootstrap[pos + 8:pos + 8 + size]
                    pos += 8 + size
                elif op == 5:
                    flags = bootstrap[pos + 6]
                    assert flags == 0xc2
                    size = struct.unpack_from('<H', bootstrap, pos + 9)[0]
                    pos += 11 + size
                else:
                    raise AssertionError((pos, op))
            programs = {key: blocks[key] for key, item in owned.items() if item['kind'] == 'program'}
            assert all(blocks[key][0] != 0x55 for key in owned)
            globals_ = {item['name']: item['id'] for item in manifest['globals']}

            def state(calls, previous, label):
                if mixed:
                    tape.read(globals_['calls'], 0, word(calls), name + ' calls ' + label)
                    tape.read(globals_['previous'], 0, word(previous), name + ' previous ' + label)

            tape.send(bootstrap)
            state(0, 0, 'after install')
            tape.read(manifest['output']['id'], 0, b'\x00\x00', name + ' zero-initialized output')
            tape.read(manifest['input']['id'], 0, struct.pack('<f', -0.00006103515625), name + ' declared initial input')
            for key, data in programs.items():
                tape.read(key, 0, data, name + ' installed program')
            for index, value in enumerate(values):
                raw = struct.pack('<f', value)
                tape.send(update(manifest['input']['id'], raw) + invocation)
                tape.read(manifest['input']['id'], 0, raw, name + ' input ' + str(index))
                expected = struct.pack('<h', math.floor(f32(value)))
                tape.read(manifest['output']['id'], 0, expected, name + ' floor ' + repr(value))
                state(index + 1, index, 'after input ' + str(index))
            # Same input, no update: calls must not reinitialize ordinary state.
            tape.send(invocation)
            tape.read(manifest['output']['id'], 0, expected, name + ' repeated invoke')
            state(len(values) + 1, len(values), 'after repeated invoke')
            if mixed:
                tape.send(update(globals_['calls'], word(65534)))
                for calls, previous in ((65535, 65534), (0, 65535), (1, 0)):
                    tape.send(invocation)
                    state(calls, previous, 'wrap ' + str(calls))
            for key, data in programs.items():
                tape.read(key, 0, data, name + ' unchanged program')
            # Reinstall over live buffers as well as after cleanup: the clear
            # commands must prevent append-only writes from retaining old state.
            tape.send(bootstrap)
            state(0, 0, 'live reinstall reset')
            tape.send(invocation)
            tape.read(manifest['output']['id'], 0, b'\xff\xff', name + ' live reinstall result')
            state(1, 0, 'live reinstall invoke')
            for key, data in programs.items():
                tape.read(key, 0, data, name + ' live reinstalled program')
            for key in owned:
                # Probe source readability before cleanup with bytes observed
                # from each live buffer. Mutable first bytes have no fixed
                # expectation, so overwrite DATA byte zero with an owned tag
                # only after all numeric/state checks, then probe before clear.
                if owned[key]['kind'] == 'data':
                    tape.send(update(key, b'\xa6'))
                    first = b'\xa6'
                else:
                    first = blocks[key][:1]
                tape.read(key, 0, first, name + ' present before cleanup')
            tape.send(cleanup)
            for key in owned:
                tape.read(key, 0, b'\x55', name + ' absent after cleanup')
            for key, data in canaries.items():
                tape.read(key, 0, data, name + ' preserved external')
            tape.read(ASSET, 0, bytes(range(256)), name + ' preserved diagnostic asset')
            # Reinstall from a known disturbed/cleared state, then invoke the
            # declaration's initial input without a host update.
            tape.send(bootstrap)
            state(0, 0, 'reinstall reset')
            tape.send(invocation)
            tape.read(manifest['output']['id'], 0, b'\xff\xff', name + ' declared initial tiny negative')
            state(1, 0, 'first reinstalled invoke')
            tape.send(cleanup)
            for key in owned:
                tape.read(key, 0, b'\x55', name + ' absent after second cleanup')
            for key, data in canaries.items():
                tape.read(key, 0, data, name + ' external after second cleanup')
            packages.append({'name': name, 'command': command, 'numeric_cases': len(values),
                             'manifest': manifest, 'source_sha256': sha(source),
                             'artifacts_sha256': {p.name: sha(p) for p in sorted(output.iterdir())}})
    tape.data.append(0)
    assert len(tape.expected) <= 24000
    (BUILD / 'tape.bin').write_bytes(tape.data)
    (BUILD / 'expected.dat').write_bytes(tape.expected)
    meta = {'packages': packages, 'checks': tape.checks, 'tape_bytes': len(tape.data),
            'observation_bytes': len(tape.expected), 'tape_sent_vdu_bytes': tape.sent,
            'query_vdu_bytes_per_observed_byte': 2 * 51,
            'numeric_inputs': values, 'smoke': smoke}
    (BUILD / 'fixture.json').write_text(json.dumps(meta, indent=2) + '\n')
    subprocess.run([str(BUILD / 'ez80asm'), 'guest.asm', 'build/probe.bin', '-l', '-c'], cwd=TASK, check=True)
    return meta


def prepare():
    spec = importlib.util.spec_from_file_location('canonical_setup', SETUP)
    setup = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(setup)
    setup.link_shared_runtime(PROFILE, RUNTIME)
    assert not (PROFILE / 'fab-agon-emulator').is_symlink()
    app = PROFILE / 'sdcard/golem'
    app.mkdir(exist_ok=True)
    binary = app / 'probe.bin'
    if binary.is_symlink():
        binary.unlink()
    assert not binary.exists()
    binary.symlink_to(BUILD / 'probe.bin')
    (PROFILE / 'sdcard/autoexec.txt').write_bytes(b'SET KEYBOARD 1\r\ncd /golem\r\nload probe.bin\r\nrun .\r\n')
    return app


def identity():
    paths = [PROFILE / 'fab-agon-emulator', PROFILE / 'fab-agon-emulator.bin',
             PROFILE / 'mos_platform.bin', PROFILE / 'mos_platform.map',
             PROFILE / 'firmware/vdp_platform.so', BUILD / 'probe.bin', BUILD / 'tape.bin',
             TASK / 'guest.asm', TASK / 'qualify.py', SETUP,
             PROJECT / 'reference/agnb-asm/lib/mos_api.inc', BUILD / 'ez80asm',
             PROJECT / 'docs/tasks/GOLEM-006/build/golemc-integrated',
             *[PROJECT / 'src' / name for name in ('golemc.cpp', 'token_reader.hpp', 'buffer_allocator.hpp')],
             *[PROJECT / 'docs/tasks/GOLEM-006' / name for name in
               ('golemc_integrated.cpp', 'integration.hpp', 'scalar_backend.hpp')]]
    return {str(p.relative_to(PROJECT)) if p.is_relative_to(PROJECT) else str(p): sha(p) for p in paths}


def execute(meta, app):
    result = app / 'results.dat'
    result.unlink(missing_ok=True)
    before = identity()
    env = {k: v for k, v in os.environ.items() if k not in ('LD_PRELOAD', 'BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    command = ['./fab-agon-emulator', '--renderer', 'sw']
    started = time.monotonic()
    mapped_vdp = set()
    actual_executable = None
    with (BUILD / 'emulator.log').open('w') as log:
        process = subprocess.Popen(command, cwd=PROFILE, env=env, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            while not result.exists():
                if process.poll() is not None:
                    raise RuntimeError('Emulator exited before guest results')
                maps = Path(f'/proc/{process.pid}/maps').read_text()
                mapped_vdp.update(Path(line.split(maxsplit=5)[5]).resolve()
                                  for line in maps.splitlines()
                                  if '/vdp_' in line and '.so' in line)
                if mapped_vdp:
                    assert mapped_vdp == {(PROFILE / 'firmware/vdp_platform.so').resolve()}, mapped_vdp
                    actual_executable = Path(f'/proc/{process.pid}/exe').resolve()
                    assert actual_executable == (PROFILE / 'fab-agon-emulator.bin').resolve()
                if time.monotonic() - started > 180:
                    raise TimeoutError('Guest completion; inspect build/emulator.log')
                time.sleep(0.05)
            # MOS save may create the host file before writing it completely.
            while result.stat().st_size < meta['observation_bytes']:
                if process.poll() is not None or time.monotonic() - started > 180:
                    raise TimeoutError('Guest result write did not finish')
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
    elapsed = round(time.monotonic() - started, 3)
    assert mapped_vdp and actual_executable, 'No observed native runtime identity'
    actual = result.read_bytes()
    (BUILD / 'observed.dat').write_bytes(actual)
    (TASK / 'observed.dat').write_bytes(actual)
    expected = (BUILD / 'expected.dat').read_bytes()
    (TASK / 'expected.dat').write_bytes(expected)
    failures = []
    for check in meta['checks']:
        start, size = check['start'], check['bytes']
        got, want = actual[start:start+size], expected[start:start+size]
        if got != want:
            failures.append({**check, 'observed': got.hex(), 'expected': want.hex()})
    after = identity()
    assert before == after, 'Runtime/source changed during qualification'
    report = {'pass': not failures and len(actual) == len(expected), 'command': command,
              'loaded_vdp': str(next(iter(mapped_vdp))), 'executed_fab': str(actual_executable),
              'runtime_identity': before, 'fab_tag': '1.2.5', 'mos_release': 'v3.0.2 Arthur',
              'architecture': os.uname().machine, 'execution_seconds': elapsed,
              'checks': len(meta['checks']), 'observed_bytes': len(actual),
              'numeric_jobs': sum(p['numeric_cases'] for p in meta['packages']),
              'resident_calls': sum(p['numeric_cases'] + 3 + (3 if p['manifest']['globals'] else 0)
                                    for p in meta['packages']),
              'failures': failures, 'fixture': meta, 'guest_results_sha256': sha(result),
              'expected_sha256': sha(TASK / 'expected.dat'),
              'scope': 'Stock emulator VDP via MOS/eZ80 UART; GP byte observations, no firmware changes',
              'human_validation': 'pending', 'hardware_qualified': False}
    (TASK / 'emulator-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    if not report['pass']:
        raise AssertionError(f'{len(failures)} failed checks; see emulator-checks.json')
    print(f'Stock emulator checks passed: {report["checks"]} checks, {len(actual)} observed bytes, {elapsed}s')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    started = time.monotonic()
    meta = generate(args.smoke)
    app = prepare()
    meta['setup_seconds'] = round(time.monotonic() - started, 3)
    print(f'Prepared {meta["observation_bytes"]} observation bytes in {meta["setup_seconds"]}s', flush=True)
    if not args.prepare_only:
        execute(meta, app)


if __name__ == '__main__':
    main()
