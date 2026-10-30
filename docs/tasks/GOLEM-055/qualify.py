"""Actual-Fab native compiler RAM regression, isolated from accepted reviews.

The existing SDL helper only injects keyboard events and captures Fab's own
framebuffer. It does not implement MOS, the compiler or buffered commands.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import shutil
import struct
import subprocess
import time
import zlib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = HERE / 'build'
PROFILE = ROOT / '.emulator/golem-055'
HELPERS = ROOT / 'docs/tasks/GOLEM-051'
if (BUILD / 'helper051/qualify.py').exists():
    HELPERS = BUILD / 'helper051'
spec = importlib.util.spec_from_file_location('ram055_session', HELPERS / 'qualify.py')
q = importlib.util.module_from_spec(spec)
spec.loader.exec_module(q)
q.ROOT, q.PROFILE, q.BUILD = ROOT, PROFILE, BUILD
spec = importlib.util.spec_from_file_location('ram055_ppm', HELPERS / 'ppm_png.py')
ppm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ppm)
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
word = lambda value: struct.pack('<H', value)
CONTROL = '''uint16 count = 7
float32 reading = 1.5
Sub adjust
    count = count + 1
    reading = reading * 2.0
EndSub
'''


def fixture(reference, destination):
    metadata = json.loads((reference / 'backends.json').read_text())
    interface = json.loads((reference / 'interface.json').read_text())
    fields = {field['name']: field for field in interface['globals']}
    assert list(fields) == ['reading', 'scale']
    initial = (reference / 'state.bin').read_bytes()
    offsets = [12, 13, 14, 15] + [field['offset'] + index for field in fields.values() for index in range(field['bytes'])]
    records = [(1.5, 2.0, True, 'reading=1.5 scaled=3'),
               (1.5, 2.0, False, 'reading=1.5 scaled=3'),
               (-2.25, 4.0, True, 'reading=-2.25 scaled=-9'),
               (3.125, 0.5, True, 'reading=3.125 scaled=1.5625'),
               (0.0, -8.0, True, 'reading=0 scaled=0')]
    output, expected_all = bytearray(), bytearray()
    cases = []
    for reading, scale, reset, text in records:
        state = bytearray(initial)
        for name, value in (('reading', reading), ('scale', scale)):
            struct.pack_into('<f', state, fields[name]['offset'], value)
        observations = bytes(state[offset] for offset in offsets)
        expected_all.extend(observations)
        encoded = text.encode('ascii')
        output.extend(word(0) + bytes((reset, len(encoded))) + state + observations + encoded)
        cases.append({'reading': reading, 'scale': scale, 'reset': reset, 'text': text})
    (destination / 'cases.dat').write_bytes(output)
    header = (f'#define STATE_BYTES {len(initial)}\n#define STATE_ID {metadata["shared_state"]}\n'
              f'#define CASE_COUNT {len(records)}\n#define OFFSET_COUNT {len(offsets)}\n')
    for name, values in [('offsets', offsets), ('entries', metadata['buffered_entries'])]:
        header += 'static const unsigned ' + name + '[]={' + ','.join(map(str, values)) + '};\n'
    (BUILD / 'fixture.h').write_text(header)
    tools = Path('/Agon/agondev/release')
    command([tools / 'bin/ez80-none-elf-clang', '-target', 'ez80-none-elf', '-mllvm', '-z80-gas-style',
             '-mllvm', '-z80-print-zero-offset', '-nostdinc', '-isystem', tools / 'include', '-I' + str(BUILD),
             '-Oz', '-DAGONDEV', '-Wa,-march=ez80+full', '-c', HERE / 'guest.c', '-o', BUILD / 'guest.o'])
    command([tools / 'bin/ez80-none-elf-ld', '-defsym=RAM_START=0x40000', '-defsym=RAM_SIZE=0x70000',
             '-defsym=_has_exit_handler=0', '-T', tools / 'config/linker.conf', '--oformat', 'binary',
             '-o', BUILD / 'review.bin', BUILD / 'guest.o', '-L' + str(tools / 'lib'), '-lagon'])
    shutil.copyfile(BUILD / 'review.bin', destination / 'review.bin')
    receipt = {'cases': cases, 'expected_observations': expected_all.hex(),
               'source_sha256': sha(destination / 'two-floats.golem'),
               'guest_sha256': sha(destination / 'review.bin'),
               'interface_sha256': sha(reference / 'interface.json')}
    (destination / 'expected.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


def command(arguments):
    subprocess.run(list(map(str, arguments)), check=True)


def stage_worker(worker):
    BUILD.mkdir(exist_ok=True)
    app = q.stage()
    destination = app / 'ram'
    destination.mkdir(exist_ok=True)
    shutil.copyfile(worker, app / 'compiler/portable.bin')
    shutil.copyfile(HERE / 'two-floats.golem', destination / 'two-floats.golem')
    return destination


def prepare(*, review=False):
    worker = ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin'
    destination = stage_worker(worker)
    mos = PROFILE / 'sdcard/mos'
    if mos.is_symlink():
        origin = mos.resolve()
        mos.unlink()
        shutil.copytree(origin, mos)
    shutil.copyfile(ROOT / 'docs/tasks/GOLEM-051/build/support/golemc.bin', mos / 'golemc.bin')
    shutil.copyfile(ROOT / 'docs/tasks/GOLEM-051/build/support/golemrun.bin', destination.parent / 'golemrun.bin')
    reference = BUILD / 'reference'
    command([ROOT / 'build/golemc', '--portable', HERE / 'two-floats.golem', reference])
    frozen = BUILD / 'baseline/reference'
    assert {path.name for path in reference.iterdir()} == {path.name for path in frozen.iterdir()}
    for path in frozen.iterdir():
        assert path.read_bytes() == (reference / path.name).read_bytes(), path.name
    receipt = fixture(reference, destination)
    receipt['worker_sha256'] = sha(worker)
    receipt['dispatcher_sha256'] = sha(mos / 'golemc.bin')
    receipt['runner_sha256'] = sha(destination.parent / 'golemrun.bin')
    (destination / 'expected.json').write_text(json.dumps(receipt, indent=2) + '\n')
    (destination / 'control.golem').write_text(CONTROL)
    (destination / 'two-floats.args').write_text('--portable two-floats.golem package-response\n')
    for target in ('buffered', 'native'):
        command([ROOT / 'build/golemc', '--portable', '--target', target,
                 destination / 'control.golem', BUILD / ('control-' + target)])
    for name in ('package', 'package-direct', 'package-response', 'control', 'native', 'rejected'):
        shutil.rmtree(destination / name, ignore_errors=True)
    for name in ('done.dat', 'results.dat', 'scene.dat', 'advance.dat', 'keyboard.dat',
                 'batch.log', 'batch-errors.log', 'batch-output.log', 'runner.dat'):
        (destination / name).unlink(missing_ok=True)
    auto = ('SET KEYBOARD 1\nVDU 22 0\ncd /golem/ram\n'
            'golemc --portable two-floats.golem package\n')
    if not review:
        auto += ('load ../compiler/portable.bin\nrun . --portable two-floats.golem package-direct\n'
                 'golemc @two-floats.args\n'
                 'golemc --portable --target buffered control.golem control\n'
                 'golemc --portable --target native control.golem native\n')
    auto += 'load review.bin\nrun .' + (' --review' if review else '') + '\n'
    if not review:
        # MOS autoexec waits for each application. Injecting a save command
        # while the runner installs buffers can lose its initial keystrokes.
        auto += 'VDU 12\nload ../golemrun.bin\nrun . --portable package\nsave runner.dat 40000 3\n'
    return destination, auto, receipt, reference


def verify_saved(destination):
    receipt = json.loads((destination / 'expected.json').read_text())
    observed = (destination / 'results.dat').read_bytes()
    assert observed == bytes.fromhex(receipt['expected_observations'])
    assert (destination / 'done.dat').read_bytes() == b'\0'
    assert sha(destination / 'two-floats.golem') == receipt['source_sha256']
    assert sha(destination / 'review.bin') == receipt['guest_sha256']
    return {'passed': True, 'cases': len(receipt['cases']), 'state_observations': len(observed),
            'results_sha256': sha(destination / 'results.dat'), 'source_input_preservation': True}


def matching_files(reference, actual):
    assert {path.name for path in reference.iterdir()} == {path.name for path in actual.iterdir()}
    for path in reference.iterdir():
        assert path.read_bytes() == (actual / path.name).read_bytes(), str(actual / path.name)
    return len(list(reference.iterdir()))


def execute_prepared(destination, auto, receipt, reference):
    session = q.Session(auto, 'fixed')
    report = {}
    pixels = 0
    try:
        for index, record in enumerate(receipt['cases']):
            captures = []
            for side in (0, 1):
                number = index * 2 + side
                deadline = time.monotonic() + (180 if not index else 30)
                while True:
                    path = destination / 'scene.dat'
                    scene = path.read_bytes() if path.exists() else b''
                    if scene == word(number):
                        break
                    if session.process.poll() is not None:
                        raise RuntimeError('Fab stopped')
                    if time.monotonic() > deadline:
                        session.capture('stalled')
                        raise TimeoutError(f'Scene {number}; inspect compiler output')
                    time.sleep(.02)
                captures.append(ppm.read(session.capture(f'case-{index}-{side}')))
                temporary = destination / 'advance.tmp'
                temporary.write_bytes(word(number))
                temporary.replace(destination / 'advance.dat')
            assert captures[0] == captures[1], (index, record)
            pixels += captures[0][0] * captures[0][1]
        session.wait(destination / 'done.dat', 20)
        report = verify_saved(destination)
        packages = {'plain_dispatcher': matching_files(reference, destination / 'package'),
                    'direct_worker': matching_files(reference, destination / 'package-direct'),
                    'dispatcher_response_file': matching_files(reference, destination / 'package-response'),
                    'buffered_control': matching_files(BUILD / 'control-buffered', destination / 'control'),
                    'native_control': matching_files(BUILD / 'control-native', destination / 'native')}
        # The same unchanged distribution runner independently installs/calls
        # the native-compiled package. It must display the initial product.
        session.wait(destination / 'runner.dat', 30)
        screenshot = session.capture('unchanged-runner')
        ppm.convert(screenshot)
        width, height, rgb = ppm.read(screenshot)
        w0, h0, expected_rgb = ppm.read(session.art / 'case-0-0.ppm')
        assert (width, height) == (w0, h0)
        span = len(receipt['cases'][0]['text']) * 8 * 3
        glyph = b''.join(expected_rgb[y * width * 3:y * width * 3 + span] for y in range(8))
        assert any(glyph)
        matches = [y0 for y0 in range(height - 7)
                   if b''.join(rgb[(y0 + y) * width * 3:(y0 + y) * width * 3 + span] for y in range(8)) == glyph]
        assert matches, 'Unchanged runner did not display the exact initial product'
        # A rejected source is run through the native worker's existing batch
        # logger so its exit code survives MOS autoexec's error-stop behavior.
        (destination / 'invalid.golem').write_text('uint16 x=1 float32 y=1.5 Sub fail x=y EndSub\n')
        (destination / 'invalid.args').write_text('--portable invalid.golem rejected\n')
        (destination / 'invalid.list').write_text('invalid.args\n')
        session.type('load ../compiler/portable.bin\nrun . --batch invalid.list\n')
        session.wait(destination / 'batch.log', 30)
        deadline = time.monotonic() + 10
        while not (destination / 'batch.log').read_text().strip():
            if time.monotonic() > deadline:
                raise TimeoutError('Missing rejected-source result')
            time.sleep(.05)
        status = (destination / 'batch.log').read_text().split()
        assert status[:2] == ['0', '1'], status
        assert not (destination / 'rejected').exists()
        errors = (destination / 'batch-errors.log').read_text()
        assert 'type' in errors.lower(), errors
        session.type('dir\nsave keyboard.dat 40000 3\n')
        session.wait(destination / 'keyboard.dat', 20)
        ppm.convert(session.capture('MOS-return'))
        report.update({'seconds': time.monotonic() - session.started,
                       'actual_MOS_compilation': True, 'normal_plain_dispatcher_path': True,
                       'optional_response_file_path': True,
                       'matching_package_files': packages, 'pixel_pairs': len(receipt['cases']),
                       'pixels_compared': pixels, 'unchanged_runner_exact_text_match': True,
                       'runner_text_y': matches, 'native_compile_error_exit': int(status[1]),
                       'native_compile_error': errors, 'MOS_keyboard_continuation': True,
                       'worker_sha256': sha(PROFILE / 'sdcard/golem/compiler/portable.bin'),
                       'profile': str(PROFILE), 'runtime': json.loads((BUILD / 'profile.json').read_text())['runtime'],
                       'human': 'pending', 'GUI_launched': False})
    finally:
        report['frontend_exit'] = session.close()
    (HERE / 'stock-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)


def execute():
    execute_prepared(*prepare())


def response_edges():
    worker = ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin'
    destination = stage_worker(worker)
    (destination / 'control.golem').write_text(CONTROL)
    (destination / 'control copy.golem').write_text(CONTROL)
    prefix = '--portable --target native control.golem edge-max'
    (destination / 'edge-max.args').write_text(prefix + ' ' * (8192 - len(prefix)))
    (destination / 'edge-large.args').write_text(' ' * 8193)
    (destination / 'edge-quoted.args').write_text('--portable --target buffered "control copy.golem" "edge quoted"')
    names = ['edge-max.args', 'edge-large.args', 'edge-quoted.args']
    (destination / 'edges.list').write_text('\n'.join(names) + '\n')
    for name in ('edge-max', 'edge quoted'):
        shutil.rmtree(destination / name, ignore_errors=True)
    for name in ('batch.log', 'batch-errors.log', 'batch-output.log', 'edges-return.dat'):
        (destination / name).unlink(missing_ok=True)
    auto = ('SET KEYBOARD 1\nVDU 22 0\ncd /golem/ram\nload ../compiler/portable.bin\nrun . --batch edges.list\n')
    session = q.Session(auto, 'response-edges')
    report = {}
    try:
        deadline = time.monotonic() + 180
        while True:
            path = destination / 'batch.log'
            lines = path.read_text().splitlines() if path.exists() else []
            if len(lines) == len(names):
                break
            if time.monotonic() > deadline:
                session.capture('stalled')
                raise TimeoutError('Native response-file boundary cases')
            time.sleep(.05)
        statuses = [int(line.split()[1]) for line in lines]
        assert statuses == [0, 1, 0], lines
        counts = {'8192_bytes': matching_files(BUILD / 'control-native', destination / 'edge-max'),
                  'quoted_paths': matching_files(BUILD / 'control-buffered', destination / 'edge quoted')}
        errors = (destination / 'batch-errors.log').read_text()
        assert 'response file exceeds 8192 bytes' in errors
        # The log is flushed before the final return through MOS's error path.
        time.sleep(.5)
        session.type('save edges-return.dat 40000 3\n')
        session.wait(destination / 'edges-return.dat', 20)
        report = {'passed': True, 'worker_sha256': sha(worker), 'actual_MOS_compilation': True,
                  'response_bytes': [8192, 8193], 'statuses': statuses,
                  'matching_package_files': counts, 'error': errors,
                  'normal_MOS_continuation': True, 'seconds': time.monotonic() - session.started}
    finally:
        report['frontend_exit'] = session.close()
    (HERE / 'native-response-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)


def baseline():
    worker = BUILD / 'baseline/golemc.bin'
    freeze = json.loads((HERE / 'baseline-freeze.json').read_text())
    assert sha(worker) == freeze['worker_sha256']
    destination = stage_worker(worker)
    shutil.rmtree(destination / 'package', ignore_errors=True)
    for name in ('baseline-return.dat', 'baseline-keyboard.dat'):
        (destination / name).unlink(missing_ok=True)
    auto = ('SET KEYBOARD 1\nVDU 22 0\ncd /golem/ram\n'
            'load ../compiler/portable.bin\n'
            'run . --portable two-floats.golem package\n')
    # The retained first baseline capture was read by the agent. Compare its
    # three diagnostic rows while waiting for this expected compiler error;
    # MOS deliberately stops autoexec after a nonzero application exit.
    png = (HERE / 'baseline-failure.png').read_bytes()
    width = struct.unpack_from('>I', png, 16)[0]
    position, data = 8, bytearray()
    while position < len(png):
        length = struct.unpack_from('>I', png, position)[0]
        if png[position + 4:position + 8] == b'IDAT':
            data.extend(png[position + 8:position + 8 + length])
        position += length + 12
    scan = zlib.decompress(data)
    assert all(scan[y * (width * 3 + 1)] == 0 for y in range(24))
    expected_rows = b''.join(scan[y * (width * 3 + 1) + 1:(y + 1) * (width * 3 + 1)] for y in range(24))
    session = q.Session(auto, 'baseline')
    report = {}
    try:
        deadline = time.monotonic() + 180
        while True:
            w, h, rgb = ppm.read(session.capture('diagnostic'))
            if w == width and rgb[:width * 3 * 24] == expected_rows:
                break
            if time.monotonic() > deadline:
                raise TimeoutError('Expected baseline allocation diagnostic')
            time.sleep(.1)
        compile_seconds = time.monotonic() - session.started
        session.type('save baseline-return.dat 40000 3\n')
        session.wait(destination / 'baseline-return.dat', 20)
        assert not (destination / 'package').exists(), 'Baseline unexpectedly produced a package'
        session.type('dir\nsave baseline-keyboard.dat 40000 3\n')
        session.wait(destination / 'baseline-keyboard.dat', 20)
        picture = ppm.convert(session.capture('allocation-failure'))
        shutil.copyfile(picture, HERE / 'baseline-failure.png')
        report = {'passed': True, 'purpose': 'Reproduce accepted native worker RAM failure',
                  'worker_sha256': sha(worker), 'source_sha256': sha(destination / 'two-floats.golem'),
                  'actual_MOS_compilation': True, 'normal_error_return_to_MOS': True,
                  'no_package_created': True, 'MOS_keyboard_continuation': True,
                  'seconds': time.monotonic() - session.started, 'seconds_to_failure_screen': compile_seconds,
                  'screen': 'baseline-failure.png', 'screen_sha256': sha(HERE / 'baseline-failure.png'),
                  'allocation_request_bytes': 32768, 'compiler_stage': 'building stock wrappers',
                  'diagnostic_text': 'out of compiler memory', 'diagnostic_rows_match_retained_baseline': True,
                  'profile': str(PROFILE),
                  'runtime': json.loads((BUILD / 'profile.json').read_text())['runtime'],
                  'GUI_launched': False, 'physical_target': False}
    finally:
        report['frontend_exit'] = session.close()
    (HERE / 'baseline-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)


def memory_probe(label, response=False):
    worker = BUILD / ('probe-' + label) / 'golemc.bin'
    destination = stage_worker(worker)
    shutil.rmtree(destination / 'package', ignore_errors=True)
    for name in ('heap.log', 'probe-return.dat'):
        (destination / name).unlink(missing_ok=True)
    (destination / 'probe.args').write_text('--portable two-floats.golem package\n')
    invocation = '@probe.args' if response else '--portable two-floats.golem package'
    auto = ('SET KEYBOARD 1\nVDU 22 0\ncd /golem/ram\n'
            'load ../compiler/portable.bin\nrun . ' + invocation + '\n')
    name = 'probe-' + label + ('-response' if response else '-direct')
    session = q.Session(auto, name)
    report = {}
    try:
        session.wait(destination / 'heap.log', 180)
        deadline = time.monotonic() + 10
        while '\nfinal ' not in (destination / 'heap.log').read_text():
            if time.monotonic() > deadline:
                raise TimeoutError('Incomplete heap report')
            time.sleep(.05)
        # A compiler error aborts MOS autoexec, so continuation is requested
        # only after the worker itself has saved its completed heap report.
        session.type('save probe-return.dat 40000 3\n')
        session.wait(destination / 'probe-return.dat', 20)
        heap = (destination / 'heap.log').read_text()
        (HERE / (name + '.log')).write_text(heap)
        picture = ppm.convert(session.capture('MOS-return'))
        matching = 0
        compile_succeeded = 'failures=0 ' in heap
        if compile_succeeded:
            for path in (BUILD / 'baseline/reference').iterdir():
                assert path.read_bytes() == (destination / 'package' / path.name).read_bytes(), path.name
                matching += 1
            assert 'failures=0 ' in heap
        else:
            assert 'failures=1 ' in heap
            assert not (destination / 'package').exists()
        report = {'passed': True, 'worker_sha256': sha(worker), 'source_sha256': sha(destination / 'two-floats.golem'),
                  'invocation': invocation, 'response_file_overhead': response,
                  'actual_MOS_compilation': True, 'matching_package_files': matching,
                  'native_compiler_succeeded': compile_succeeded,
                  'normal_return_to_MOS': True, 'seconds': time.monotonic() - session.started,
                  'heap_log': name + '.log', 'screen': str(picture.relative_to(HERE)),
                  'diagnostic_binary_only': True, 'GUI_launched': False}
    finally:
        report['frontend_exit'] = session.close()
    (HERE / (name + '-evidence.json')).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--probe', choices=('baseline', 'fixed'))
    parser.add_argument('--response', action='store_true')
    parser.add_argument('--staged', action='store_true')
    parser.add_argument('--response-edges', action='store_true')
    arguments = parser.parse_args()
    if arguments.probe:
        memory_probe(arguments.probe, arguments.response)
    elif arguments.baseline:
        baseline()
    elif arguments.response_edges:
        response_edges()
    elif arguments.staged:
        destination = PROFILE / 'sdcard/golem/ram'
        execute_prepared(destination, (PROFILE / 'sdcard/autoexec.txt').read_text(),
                         json.loads((destination / 'expected.json').read_text()), BUILD / 'reference')
    else:
        execute()
