"""Focused portable XOR conformance: independent commands and sanitized native C."""
from pathlib import Path
import argparse
import hashlib
import json
import random
import struct
import time
from check_integration import HERE, run
from command_model import Machine, reference


def check(args):
    started = time.monotonic()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    command = [args.compiler.resolve(), '--portable']
    if args.relocated: command += ['--reserve', '1:254']
    run(command + [HERE / 'xor.golem', out])
    interface, ast, root, backend = [json.loads((out / name).read_text()) for name in
                                    ('interface.json', 'program.json', 'root.json', 'backends.json')]
    fields = {f['name']: f for f in interface['globals']}
    entries = {e['name']: e['logical_id'] for e in interface['entries']}
    initial = (out / 'state.bin').read_bytes()
    images = {a['buffer']: (out / a['initial_data']).read_bytes() for a in interface['arrays']}
    cases = []

    def add(name, updates=None, known=None):
        state = bytearray(initial); entry = entries[name]
        struct.pack_into('<H', state, 28, entry)
        for key, value in (updates or {}).items():
            f = fields[key]
            struct.pack_into('<I' if f['bytes'] == 4 else '<H', state, f['offset'],
                             value & ((1 << (f['bytes'] * 8)) - 1))
        cases.append((entry, bytes(state), known or {}))

    rng = random.Random(42)
    for entry, left, right, bits in [('xorU16', 'a16', 'b16', 16), ('xorS16', 'as16', 'bs16', 16),
                                    ('xorU32', 'a32', 'b32', 32), ('xorS32', 'as32', 'bs32', 32)]:
        mask = (1 << bits) - 1
        basis = [0, 1, mask, 1 << (bits - 1), (1 << (bits - 1)) - 1,
                 0xaaaaaaaa & mask, 0x55555555 & mask, 0xff00ff00 & mask]
        for a in basis:
            for b in basis: add(entry, {left: a, right: b})
        for _ in range(32): add(entry, {left: rng.randrange(mask + 1), right: rng.randrange(mask + 1)})
    add('chain', known={'r16': 7})
    add('precedence', {'a32': 32, 'b32': 2, 'c32': 3, 'count': 1}, {'r32': 19})
    add('parentheses', known={'r32': 7})
    add('comparison', {'c32': 6}, {'flag': 0})
    add('comparison', {'c32': 5}, {'flag': 1})
    add('constants', known={'r16': 21845, 'rs16': 32767, 'r32': 1431655765, 'rs32': 2147483647})
    for a, b in [(0, 0), (5, 3), (7, 7), (7, 8), (32768, 0), (65535, 65535)]:
        add('boundedUnsigned', {'a16': a, 'b16': b})
        add('priorEffect', {'a16': a, 'b16': b})
    add('priorEffect', {'effect': 65535})
    for a, b in [(-1, -1), (-1, 0), (0, 1), (1, 3), (-2147483648, -1), (-2147483648, -2147483648)]:
        add('boundedSigned', {'as32': a, 'bs32': b})
    for name in ('operandOrder', 'rightFailure', 'skipped'):
        for a, b, c in [(5, 3, 1), (0xffffffff, 2, 0), (1, 0, 0), (1, 1, 0)]:
            add(name, {'a32': a, 'b32': b, 'c32': c})
    for name in ('arrayRead', 'arrayWrite'):
        for index in (0, 1, 2, 65535):
            for signed_index in (-2, -1, 0, 1):
                add(name, {'index': index, 'signedIndex': signed_index})
    for a, b in [(0, 0), (65535, 43690), (65536, 1), (0xffffffff, 0xffffffff)]:
        add('conversion', {'a32': a, 'b32': b})
    for count in (0, 1, 15, 16, 31, 32, 65535):
        add('countExpression', {'a32': 0xffffffff, 'a16': count, 'b16': 0})
    for divisor in (0, 1, 3, 0xffffffff): add('paired', {'c32': divisor})

    machine = Machine()
    machine.execute((out / 'common-install.bin').read_bytes())
    machine.execute((out / 'buffered-install.bin').read_bytes())
    expected = bytearray(); statuses = set()
    for entry, state, known in cases:
        arrays = {key: bytearray(value) for key, value in images.items()}
        wanted = reference(interface, ast, state, entry, root, arrays=arrays)
        for key, value in known.items():
            f = fields[key]
            assert struct.unpack_from('<I' if f['bytes'] == 4 else '<H', wanted, f['offset'])[0] == value
        status = int(struct.unpack_from('<HH', wanted, 12) != (0, 0))
        statuses.add(struct.unpack_from('<H', wanted, 14)[0])
        expected += struct.pack('<i', status) + wanted + b''.join(arrays.values())
        machine.memory[backend['shared_state']][:] = state
        for key, value in images.items(): machine.memory[key][:] = value
        machine.steps = 0
        machine.execute(bytes((23, 0, 160)) + struct.pack('<H', backend['buffered_entries'][entry]) + b'\x01')
        assert bytes(machine.memory[backend['shared_state']]) == wanted, ('state', entry, state.hex())
        assert all(machine.memory[key] == value for key, value in arrays.items()), ('arrays', entry)
    assert statuses == {0, 1, 2, 3, 4, 5, 32}, statuses
    header = (f"#define STATE_ID {backend['shared_state']}\n#define STATE_BYTES {len(initial)}\n"
              f"#define ARGS_ID {backend['native_arguments']}\n#define ARRAY_COUNT {len(images)}\n")
    for name, values in [('ids', list(images)), ('bytes', [len(x) for x in images.values()]),
                         ('readonly', [int(a['access'] == 1) for a in interface['arrays']])]:
        header += 'static const unsigned array_' + name + '[]={' + ','.join(map(str, values)) + '};\n'
    (out / 'fixture.h').write_text(header); runner = out / 'xor-host'
    run([args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O1', '-g', '-ffp-contract=off',
         '-fsanitize=address,undefined', '-I' + str(args.sdk.resolve() / 'include'), '-I' + str(out),
         '-DGENERATED_SOURCE="' + str(out / 'native.c') + '"', HERE / 'native_host.c', '-o', runner])
    payload = b''.join(struct.pack('<HH', entry, 0) + state + b''.join(images.values()) for entry, state, _ in cases)
    assert run([runner], input=payload) == expected, 'generated C differs from reference'
    assembly = False
    if args.assembler:
        (out / 'consumer.asm').write_text(' .assume adl=1\n .org $40000\n include "layout.inc"\n include "calls.inc"\n'
                                         ' call golem_buffered_xorU16\n call golem_native_xorS32\n ret\n')
        run([args.assembler.resolve(), 'consumer.asm', 'consumer.bin', '-c'], cwd=out); assembly = True
    receipt = dict(cases=len(cases), relocated=args.relocated, sanitized_generated_c=True,
                   buffered_model=True, actual_assembler=assembly, language_statuses=sorted(statuses),
                   expected_sha256=hashlib.sha256(expected).hexdigest(),
                   elapsed_seconds=round(time.monotonic() - started, 3),
                   abi_header_sha256=hashlib.sha256((args.sdk / 'include/vdplet.h').read_bytes()).hexdigest(),
                   scope='host XOR/reference/commands/generated-C; not target execution')
    (out / 'xor-result.json').write_text(json.dumps(receipt, indent=2) + '\n'); print(json.dumps(receipt))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--assembler', type=Path)
    parser.add_argument('--relocated', action='store_true')
    check(parser.parse_args())
