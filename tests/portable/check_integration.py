"""Optional model/generated-C/assembler checks, using maintained sources only."""
from pathlib import Path
import argparse
import hashlib
import json
import random
import struct
import subprocess
import time
from command_model import Machine, reference, float_bits

HERE = Path(__file__).resolve().parent


def run(args, **kwargs):
    result = subprocess.run(list(map(str, args)), capture_output=True, **kwargs)
    if result.returncode:
        raise RuntimeError(result.stdout.decode() + result.stderr.decode())
    return result.stdout


def check(args):
    started = time.monotonic()
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    compiler = args.compiler.resolve()
    command = [compiler, '--portable']
    if args.relocated:
        command += ['--reserve', '1:254']
    run(command + [HERE / 'numeric.golem', directory])
    interface, ast, root, backend = [json.loads((directory / name).read_text()) for name in
                                    ('interface.json', 'program.json', 'root.json', 'backends.json')]
    fields = {f['name']: f for f in interface['globals']}
    entries = {e['name']: e['logical_id'] for e in interface['entries']}
    initial = (directory / 'state.bin').read_bytes()
    images = {a['buffer']: (directory / a['initial_data']).read_bytes() for a in interface['arrays']}
    vectors = []

    def add(name, updates=None, root_update=None):
        state = bytearray(initial)
        entry = entries[name]
        struct.pack_into('<H', state, 28, entry)
        for key, value in (updates or {}).items():
            f = fields[key]
            struct.pack_into('<I' if f['bytes'] == 4 else '<H', state, f['offset'],
                             value & ((1 << (f['bytes'] * 8)) - 1))
        if root_update:
            at, fmt, value = root_update
            struct.pack_into('<' + fmt, state, at, value)
        vectors.append((entry, bytes(state)))

    for a, b in [(0, 1), (0xffffffff, 1), (0xffffffff, 0), (65536, 65536),
                 (65535, 65535), (1, 0), (0x80000000, 2)]:
        for name in ('addUnsigned', 'multiplyUnsigned', 'skippedOverflow', 'priorEffect'):
            add(name, {'ua': a, 'ub': b})
    for a, b in [(-2147483648, -1), (-2147483648, 1), (2147483647, -1),
                 (-65, 6), (65, -6), (-65, -6), (0, 0), (7, 3), (5, 3), (1, 3)]:
        for name in ('subtractSigned', 'divideSigned', 'boundedPair'):
            add(name, {'sa': a, 'sb': b})
    for count in list(range(32)) + [32, 65535]:
        for a in (-2147483648, -65, -1, 0, 2147483647):
            add('shiftSigned', {'sa': a, 'count': count})
    for a in (-2147483648, -32769, -32768, -1, 0, 32767, 32768, 2147483647):
        add('narrowInteger', {'sa': a})
    for index in (-2, -1, 0, 1, 2):
        add('readArray', {'index': index})
        for a, b in [(-65, 2), (2147483647, 2), (0, 1)]:
            add('writeArray', {'index': index, 'sa': a, 'sb': b})
    for number in (-32769.0, -32768.0, -0.9, -0.0, 0.0, 0.9, 32767.9, 32768.0):
        add('floatConvert', {'fa': float_bits(number)})
    for a in [0, 0x80000000, 0x00800000, 0x3f800001, 0x7f7fffff,
              1, 0x007fffff, 0x7f800000, 0x7fc00000]:
        for b in [float_bits(2.0), float_bits(-2.0), 0x7f7fffff]:
            add('floatMath', {'fa': a, 'fb': b})
    for f in ('fa', 'fb', 'fc', 'fr'):
        add('skippedOverflow', {f: 1})  # Admission also checks unused float globals.
    add('descendingLoop')
    add('descendingLoop', {'flag': 1})
    add('descendingLoop', {'effect': 65535})
    add('descendingLoop', {'flag': 2})
    for update in [(0, 'H', 0), (2, 'H', 2), (8, 'I', 0), (48, 'H', 0)]:
        add('addUnsigned', root_update=update)
    rng = random.Random(40)
    for _ in range(40):
        add('addUnsigned', {'ua': rng.randrange(1 << 32), 'ub': rng.randrange(1 << 32)})
        add('divideSigned', {'sa': rng.randrange(-(1 << 31), 1 << 31),
                             'sb': rng.randrange(-(1 << 31), 1 << 31)})

    machine = Machine()
    machine.execute((directory / 'common-install.bin').read_bytes())
    machine.execute((directory / 'buffered-install.bin').read_bytes())
    expected = bytearray()
    statuses = set()
    for entry, state in vectors:
        arrays = {key: bytearray(value) for key, value in images.items()}
        wanted = reference(interface, ast, state, entry, root, arrays=arrays)
        status = 1 if struct.unpack_from('<HH', wanted, 12) != (0, 0) else 0
        statuses.add(struct.unpack_from('<H', wanted, 14)[0])
        expected += struct.pack('<i', status) + wanted + b''.join(arrays[k] for k in images)
        machine.memory[backend['shared_state']][:] = state
        for key, value in images.items():
            machine.memory[key][:] = value
        machine.steps = 0
        machine.execute(bytes((23, 0, 160)) + struct.pack('<H', backend['buffered_entries'][entry]) + b'\x01')
        assert bytes(machine.memory[backend['shared_state']]) == wanted, ('buffered state', entry, state.hex())
        assert all(machine.memory[k] == arrays[k] for k in images), ('buffered array', entry)

    header = (f"#define STATE_ID {backend['shared_state']}\n#define STATE_BYTES {len(initial)}\n"
              f"#define ARGS_ID {backend['native_arguments']}\n#define ARRAY_COUNT {len(images)}\n")
    for key, values in [('ids', list(images)), ('bytes', [len(x) for x in images.values()]),
                        ('readonly', [int(a['access'] == 1) for a in interface['arrays']])]:
        header += 'static const unsigned array_' + key + '[]={' + ','.join(map(str, values)) + '};\n'
    (directory / 'fixture.h').write_text(header)
    runner = directory / 'native-host'
    run([args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O1', '-g',
         '-ffp-contract=off', '-fsanitize=address,undefined', '-I' + str(args.sdk.resolve() / 'include'),
         '-I' + str(directory), '-DGENERATED_SOURCE="' + str(directory / 'native.c') + '"',
         HERE / 'native_host.c', '-o', runner])
    stdin = b''.join(struct.pack('<HH', entry, 0) + state + b''.join(images.values())
                     for entry, state in vectors)
    assert run([runner], input=stdin) == expected, 'sanitized generated native C differs from independent reference'
    assembly = False
    if args.assembler:
        consumer = directory / 'consumer.asm'
        consumer.write_text(' .assume adl=1\n .org $40000\n include "layout.inc"\n include "calls.inc"\n'
                            ' call golem_buffered_addUnsigned\n call golem_native_divideSigned\n ret\n')
        run([args.assembler.resolve(), consumer.name, 'consumer.bin', '-c'], cwd=directory)
        assembly = True
    receipt = dict(cases=len(vectors), relocated=args.relocated, observed_language_statuses=sorted(statuses),
                   sanitized_generated_c=True, buffered_model=True, actual_assembler=assembly,
                   expected_sha256=hashlib.sha256(expected).hexdigest(),
                   elapsed_seconds=round(time.monotonic() - started, 3),
                   abi_header_sha256=hashlib.sha256((args.sdk / 'include/vdplet.h').read_bytes()).hexdigest(),
                   scope='host model/generated-C integration; no target execution')
    (directory / 'integration-result.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--assembler', type=Path)
    parser.add_argument('--relocated', action='store_true')
    check(parser.parse_args())
