"""Optional sanitized callback-failure qualification of the maintained emitter."""
from pathlib import Path
import argparse
import json
import struct
from check_integration import HERE, run
from command_model import reference


def check(args):
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    command = [args.compiler.resolve(), '--portable']
    if args.relocated: command += ['--reserve', '1:254']
    run(command + [HERE / 'numeric.golem', out])
    iface, ast, root, meta = [json.loads((out / f).read_text()) for f in
                            ('interface.json', 'program.json', 'root.json', 'backends.json')]
    fields = {f['name']: f for f in iface['globals']}
    entries = {e['name']: e['logical_id'] for e in iface['entries']}
    initial = (out / 'state.bin').read_bytes()
    arrays = {a['buffer']: (out / a['initial_data']).read_bytes() for a in iface['arrays']}
    header = (f"#define STATE_ID {meta['shared_state']}\n#define STATE_BYTES {len(initial)}\n"
              f"#define ARGS_ID {meta['native_arguments']}\n#define ARRAY_COUNT {len(arrays)}\n")
    for name, values in [('ids', list(arrays)), ('bytes', [len(x) for x in arrays.values()]),
                         ('readonly', [int(a['access'] == 1) for a in iface['arrays']])]:
        header += 'static const unsigned array_' + name + '[]={' + ','.join(map(str, values)) + '};\n'
    (out / 'fixture.h').write_text(header)
    runner = out / 'release-host'
    run([args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O1', '-g', '-ffp-contract=off',
         '-fsanitize=address,undefined', '-I' + str(args.sdk.resolve() / 'include'), '-I' + str(out),
         '-DGENERATED_SOURCE="' + str(out / 'native.c') + '"', HERE / 'release_host.c', '-o', runner])
    cases = []
    for name, updates, malformed, extra in [
        ('addUnsigned', {'ua': 2, 'ub': 3}, None, 0),
        ('priorEffect', {'ua': 0xffffffff, 'ub': 1}, None, 0),
        ('addUnsigned', {}, (2, 2), 0),  # invalid root schema
        ('addUnsigned', {}, None, 8),   # root acquire fails; argument alone cleans up
        ('addUnsigned', {}, None, 16),  # short root shape; no root status writes
        ('readArray', {}, None, 32),    # checked preflight release
        ('readArray', {}, None, 64),    # checked element-read release
        ('writeArray', {'sa': 7, 'sb': 3}, None, 64),  # store precedes release failure
        ('addUnsigned', {}, None, 128), # argument acquire fails
        ('addUnsigned', {}, None, 256), # rejection before any acquisition
    ]:
        for faults in (0, 1, 2, 3):
            for refuse in (0, 4):
                mode = extra | faults | refuse
                state = bytearray(initial); entry = entries[name]
                struct.pack_into('<H', state, 28, entry)
                for key, value in updates.items():
                    f = fields[key]
                    struct.pack_into('<I' if f['bytes'] == 4 else '<H', state, f['offset'],
                                     value & ((1 << (f['bytes'] * 8)) - 1))
                if malformed: struct.pack_into('<H', state, *malformed)
                data = {k: bytearray(v) for k, v in arrays.items()}
                if extra in (8, 16, 128, 256):
                    want = bytes(state); primary = {8: 2, 16: 3, 128: 2, 256: 1}[extra]
                elif extra in (32, 64):
                    want = bytearray(state); struct.pack_into('<H', want, 12, 8)
                    if name == 'writeArray':
                        struct.pack_into('<I', next(iter(data.values())), 4, 21)
                    want = bytes(want); primary = 7
                else:
                    want = reference(iface, ast, state, entry, root, arrays=data)
                    primary = 1 if struct.unpack_from('<HH', want, 12) != (0, 0) else 0
                has_args = extra not in (128, 256)
                has_root = has_args and extra != 8
                rc = 8 if has_root and faults & 1 else 13 if has_args and faults & 2 else primary
                cases.append((name, mode, entry, bytes(state), want, data, rc, has_root, has_args))
    payload = b''.join(struct.pack('<HH', entry, mode) + state + b''.join(arrays.values())
                       for _, mode, entry, state, *_ in cases)
    output = run([runner], input=payload)
    size = 32 + len(initial) + sum(map(len, arrays.values()))
    assert len(output) == len(cases) * size
    for n, (name, mode, entry, state, want, data, rc, has_root, has_args) in enumerate(cases):
        row = output[n * size:(n + 1) * size]
        got, roots, arguments, temporary, order, remaining, _, _ = struct.unpack_from('<8I', row)
        assert got == rc, (name, mode, got, rc)
        assert roots == int(has_root) and arguments == int(has_args), (name, mode, roots, arguments)
        if has_root: assert order % 100 == 12, (name, mode, order)
        elif has_args: assert order == 2
        else: assert order == 0
        expected_remaining = int(bool(mode & 4) and has_root and bool(mode & 1))
        expected_remaining += int(bool(mode & 4) and has_args and bool(mode & 2))
        expected_remaining += int(bool(mode & 4) and bool(mode & (32 | 64)))
        assert remaining == expected_remaining, (name, mode, remaining, expected_remaining)
        assert row[32:32 + len(initial)] == want, (name, mode, 'state')
        assert row[32 + len(initial):] == b''.join(data.values()), (name, mode, 'array')
    result = dict(cases=len(cases), relocated=args.relocated, sanitized=True,
                  post_release_access_poisoned=True, cleanup_order='root then argument; both attempted once',
                  error_precedence='first cleanup error, else primary',
                  refused_release_cleanup='modeled runtime normal-return cleanup',
                  scope='injected host callbacks; not actual firmware failure injection')
    (out / 'release-result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--relocated', action='store_true')
    check(parser.parse_args())
