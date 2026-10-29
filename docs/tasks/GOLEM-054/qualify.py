"""Pixel-exact actual-Fab checks and native MOS compilation for float printing.

Only GOLEM-054's owned profile/build directory is modified. The SDL helper is
reused unchanged from GOLEM-051; it only injects keyboard events and captures
Fab's existing framebuffer, and never implements guest/VDP commands.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import shutil
import struct
import subprocess
import sys
import time

from oracle import MAX_FINITE, MIN_NORMAL, target_cases, text
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = HERE / 'build'
PROFILE = ROOT / '.emulator/golem-054'
HELPERS = ROOT / 'docs/tasks/GOLEM-051'
if (BUILD / 'helper051/qualify.py').exists():
    HELPERS = BUILD / 'helper051'
spec = importlib.util.spec_from_file_location('float054_session', HELPERS / 'qualify.py')
q = importlib.util.module_from_spec(spec)
spec.loader.exec_module(q)
q.ROOT, q.PROFILE, q.BUILD = ROOT, PROFILE, BUILD
spec = importlib.util.spec_from_file_location('float054_ppm', HELPERS / 'ppm_png.py')
ppm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ppm)
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
word = lambda value: struct.pack('<H', value)

SOURCE = '''// One float input; all formatting runs in resident stock VDP commands.
float32 x = 0.0
Sub show
    print "[" print str(x) print "]"
EndSub
'''


def command(arguments):
    subprocess.run(list(map(str, arguments)), check=True)


def stop_guard():
    for process in Path('/proc').glob('[0-9]*'):
        try:
            if str(PROFILE / 'sdcard').encode() in (process / 'cmdline').read_bytes().split(b'\0'):
                raise RuntimeError('GOLEM-054 profile is running; close it before staging')
        except OSError:
            pass


def record_values(review=False):
    if review:
        values = (0, 0x3fc00000, 0xbfc00000, 0x3f804000, 0x3f80c000,
                0x3dcccccd, 0x38d1b717, 0x38d1b718, 0x4b800000,
                0x4cbebc20, 0x4e6e6b28, MIN_NORMAL, MAX_FINITE, 0xff7fffff)
        return [{'bits': bits, 'status': 0, 'text': '[' + text(bits) + ']'} for bits in values]
    result = [{'bits': item['bits'], 'status': 0, 'text': '[' + item['text'] + ']'} for item in target_cases()]
    for bits in (1, 0x007fffff, 0x80000001, 0x807fffff,
                 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00001):
        result.append({'bits': bits, 'status': 33, 'text': ''})
        result.append({'bits': 0, 'status': 0, 'text': '[0]'})
    return result


def fixture(reference, destination, *, review=False):
    metadata = json.loads((reference / 'backends.json').read_text())
    interface = json.loads((reference / 'interface.json').read_text())
    field = interface['globals'][0]
    assert field['name'] == 'x' and field['bytes'] == 4
    initial = (reference / 'state.bin').read_bytes()
    offsets = [12, 13, 14, 15] + list(range(field['offset'], field['offset'] + 4))
    records = record_values(review)
    output = bytearray()
    expected_all = bytearray()
    for record in records:
        state = bytearray(initial)
        struct.pack_into('<I', state, field['offset'], record['bits'])
        expected = bytearray(state)
        struct.pack_into('<H', expected, 14, record['status'])
        observations = bytes(expected[offset] for offset in offsets)
        expected_all.extend(observations)
        encoded = record['text'].encode('ascii')
        output.extend(word(0) + bytes((1, len(encoded))) + state + observations + encoded)
    destination.mkdir(parents=True, exist_ok=True)
    (destination / 'numbers.golem').write_text(SOURCE)
    (destination / 'cases.dat').write_bytes(output)
    header = (f'#define STATE_BYTES {len(initial)}\n#define STATE_ID {metadata["shared_state"]}\n'
              f'#define CASE_COUNT {len(records)}\n#define NUMERIC_CASES {len(target_cases())}\n'
              f'#define OFFSET_COUNT {len(offsets)}\n')
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
    receipt = {'cases': records, 'normal_cases': 14 if review else len(target_cases()),
               'observations': len(expected_all), 'expected_observations': expected_all.hex(),
               'source_sha256': sha(destination / 'numbers.golem'), 'guest_sha256': sha(destination / 'review.bin'),
               'interface_sha256': sha(reference / 'interface.json')}
    (destination / 'expected.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


def prepare(*, review=False, relocated=False, compile_on_agon=True):
    BUILD.mkdir(exist_ok=True)
    stop_guard()
    app = q.stage()
    # Keep the compiler dispatcher private to this owned SD card rather than
    # modifying the shared standard /mos directory.
    mos = PROFILE / 'sdcard/mos'
    if mos.is_symlink():
        origin = mos.resolve()
        mos.unlink()
        shutil.copytree(origin, mos)
    support = ROOT / 'docs/tasks/GOLEM-051/build/support'
    shutil.copyfile(support / 'golemc.bin', mos / 'golemc.bin')
    shutil.copyfile(ROOT / 'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin', app / 'compiler/portable.bin')
    destination = app / 'floats'
    reference = BUILD / ('target-relocated' if relocated else 'target-reference')
    source = BUILD / 'target.golem'
    source.write_text(SOURCE)
    args = [ROOT / 'build/golemc', '--portable']
    if relocated:
        args += ['--reserve', '1:511']
    command(args + [source, reference])
    receipt = fixture(reference, destination, review=review)
    for name in ('done.dat', 'results.dat', 'scene.dat', 'advance.dat', 'keyboard.dat'):
        (destination / name).unlink(missing_ok=True)
    package = destination / 'package'
    shutil.rmtree(package, ignore_errors=True)
    auto = 'SET KEYBOARD 1\nVDU 22 0\ncd /golem/floats\n'
    if compile_on_agon:
        auto += 'golemc --portable ' + ('--reserve 1:511 ' if relocated else '') + 'numbers.golem package\n'
    else:
        shutil.copytree(reference, package)
    auto += 'load review.bin\nrun .' + (' --review' if review else '') + '\n'
    return destination, auto, receipt, reference


def expected_receipt(destination):
    return json.loads((destination / 'expected.json').read_text())


def verify_saved(destination):
    receipt = expected_receipt(destination)
    observed = (destination / 'results.dat').read_bytes()
    expected = bytes.fromhex(receipt['expected_observations'])
    assert observed == expected, 'Saved state/admission/source-preservation observations disagree'
    assert (destination / 'done.dat').read_bytes() == b'\0'
    return {'passed': True, 'cases': len(receipt['cases']), 'state_observations': len(observed),
            'results_sha256': sha(destination / 'results.dat'), 'source_preservation': True,
            'human_listening_or_visual_claim': False}


def execute(*, relocated=False, compile_on_agon=True):
    destination, auto, receipt, reference = prepare(relocated=relocated, compile_on_agon=compile_on_agon)
    return execute_prepared(destination, auto, receipt, reference, relocated=relocated, compile_on_agon=compile_on_agon)


def execute_prepared(destination, auto, receipt, reference, *, relocated=False, compile_on_agon=True):
    label = 'relocated' if relocated else 'default'
    session = q.Session(auto, 'float-054-' + label)
    result = {}
    pixels = 0
    try:
        for index, record in enumerate(receipt['cases']):
            captures = []
            for side in (0, 1):
                number = index * 2 + side
                deadline = time.monotonic() + (600 if not index else 180)
                while True:
                    path = destination / 'scene.dat'
                    try:
                        scene = path.read_bytes()
                    except FileNotFoundError:
                        scene = b''
                    if len(scene) == 2 and struct.unpack('<H', scene)[0] == number:
                        break
                    if time.monotonic() > deadline:
                        session.capture('stalled')
                        raise TimeoutError(f'Scene {number}; inspect compiler output and guest screen')
                    if session.process.poll() is not None:
                        raise RuntimeError('Emulator stopped')
                    time.sleep(.02)
                captures.append(ppm.read(session.capture(f'case-{index}-{side}')))
                temporary = destination / 'advance.tmp'
                temporary.write_bytes(word(number))
                temporary.replace(destination / 'advance.dat')
            assert captures[0] == captures[1], (index, record)
            pixels += captures[0][0] * captures[0][1]
            if (index + 1) % 10 == 0:
                print(f'Fab float pixels: {index + 1}/{len(receipt["cases"])}', flush=True)
        session.wait(destination / 'done.dat', 30)
        result = verify_saved(destination)
        matching = 0
        for path in reference.iterdir():
            assert path.read_bytes() == (destination / 'package' / path.name).read_bytes(), path.name
            matching += 1
        session.type('dir\nsave keyboard.dat 40000 3\n')
        session.wait(destination / 'keyboard.dat', 20)
        ppm.convert(session.capture('MOS-return'))
        result.update({'seconds': time.monotonic() - session.started, 'actual_MOS_compilation': compile_on_agon,
                       'matching_package_files': matching, 'pixel_pairs': len(receipt['cases']),
                       'pixels_compared': pixels, 'MOS_keyboard_continuation': True,
                       'profile': str(PROFILE), 'runtime': json.loads((BUILD / 'profile.json').read_text())['runtime'],
                       'human': 'pending'})
    finally:
        result['frontend_exit'] = session.close()
    (HERE / ('stock-relocated-evidence.json' if relocated else 'stock-evidence.json')).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--relocated', action='store_true')
    parser.add_argument('--host-package', action='store_true')
    parser.add_argument('--staged', action='store_true', help='run already prepared owned profile, without rebuilding guest')
    arguments = parser.parse_args()
    if arguments.staged:
        destination = PROFILE / 'sdcard/golem/floats'
        auto = (PROFILE / 'sdcard/autoexec.txt').read_text()
        execute_prepared(destination, auto, expected_receipt(destination), BUILD / 'target-reference')
    else:
        execute(relocated=arguments.relocated, compile_on_agon=not arguments.host_package)
