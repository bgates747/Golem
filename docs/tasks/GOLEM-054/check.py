"""Check emitted stock float-print bytes with an independent decoder/oracle.

Run from the repository root with .venv/bin/python.  This imports the previously
independent command decoder unchanged; expected decimal text comes only from
the exact-rational oracle beside this file.
"""
from pathlib import Path
import argparse
import importlib.util
import json
import struct
import subprocess
import sys
import time

from oracle import MAX_FINITE, MIN_NORMAL, cases, fraction, nearest_bits, self_check, target_cases, text

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = HERE / 'build'
spec = importlib.util.spec_from_file_location('integer_print_machine', ROOT / 'docs/tasks/GOLEM-052/machine.py')
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)
Machine = decoder.Machine


def program():
    return '''float32 x = 0.0
float32 y Bounds(-10,10) = 0.0
uint16 i = 0
uint16 marker = 7
Sub show
    print "[" print str(x) print "]"
EndSub
Sub pair
    print str(x) print "/" print str(y)
EndSub
Sub expression
    print "expr=" print str(x * 2.0 + 1.0)
EndSub
Sub repeated
    For i In Range(0:2)
        print str(x)
    Next i
EndSub
Sub overflow
    print "before:" print str(x * 2.0) print ":after"
EndSub
Sub lateFailure
    print str(y) print ":" print str(x * 2.0) print ":after"
EndSub
Sub normalized
    print str(x * 0.5)
EndSub
Sub mixed
    print str(marker) print ":" print str(x)
EndSub
'''


def compile_source(source, directory, extra=()):
    process = subprocess.run([str(ROOT / 'build/golemc'), '--portable', *extra,
                              str(source), str(directory)], capture_output=True, text=True)
    if process.returncode:
        raise AssertionError(process.stdout + process.stderr)
    return (json.loads((directory / 'backends.json').read_text()),
            json.loads((directory / 'interface.json').read_text()))


def load(directory):
    machine = Machine()
    machine.execute((directory / 'common-install.bin').read_bytes())
    machine.execute((directory / 'buffered-install.bin').read_bytes())
    return machine


def invoke_bytes(buffer_id):
    return bytes((23, 0, 160)) + struct.pack('<H', buffer_id) + b'\x01'


def check_host_reference(values):
    """Compare the maintained host model to the same independent expectations."""
    source = BUILD / 'host-reference.cpp'
    source.write_text('#include "portable/portable.hpp"\n#include <iostream>\n'
                      'int main(){uint32_t bits;while(std::cin>>bits)'
                      'std::cout<<portable::decimal({portable::Type::F32,bits})<<"\\n";}\n')
    binary = BUILD / 'host-reference'
    subprocess.run(['g++', '-std=c++17', '-ffp-contract=off', '-O2', '-I' + str(ROOT / 'src'),
                    str(source), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], input=''.join(str(case['bits']) + '\n' for case in values),
                            capture_output=True, text=True, check=True)
    observed = result.stdout.splitlines()
    assert observed == [case['text'] for case in values]
    return {'passed': True, 'formatted_values': len(values)}


def resource_costs():
    """Measure shared formatter and additional-site costs, without speed claims."""
    packages = []
    for label, body in [('literal', 'print "."'), ('one', 'print str(x)'),
                        ('two', 'print str(x) print str(x)')]:
        source = BUILD / ('cost-' + label + '.golem')
        source.write_text('float32 x = 1.5\nSub show\n' + body + '\nEndSub\n')
        directory = BUILD / ('cost-' + label)
        metadata, _ = compile_source(source, directory, ('--target', 'buffered'))
        machine = load(directory)
        owned = metadata['buffered_owned']
        code = set(map(int, metadata['programs']))
        packages.append({'fixture': label,
                         'common_install_bytes': (directory / 'common-install.bin').stat().st_size,
                         'buffered_install_bytes': (directory / 'buffered-install.bin').stat().st_size,
                         'shared_state_bytes': metadata['state_bytes'], 'private_buffers': len(owned),
                         'private_data_bytes': sum(len(machine.memory[key]) for key in owned if key not in code),
                         'private_allocation_bytes': sum(len(machine.memory[key]) for key in owned)})
    assert packages[2]['private_data_bytes'] - packages[1]['private_data_bytes'] < 40
    result = {'passed': True, 'packages': packages,
              'one_extra_float_site_install_bytes': packages[2]['buffered_install_bytes'] - packages[1]['buffered_install_bytes'],
              'one_extra_float_site_data_bytes': packages[2]['private_data_bytes'] - packages[1]['private_data_bytes']}
    (HERE / 'resource-evidence.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def check(directory, values, progress=False):
    started = time.monotonic()
    metadata = json.loads((directory / 'backends.json').read_text())
    interface = json.loads((directory / 'interface.json').read_text())
    assert metadata['available_backends'] == ['buffered'] and not metadata['native_wrappers']
    assert not (directory / 'native.c').exists()
    machine = load(directory)
    state = machine.memory[metadata['shared_state']]
    fields = {field['name']: field for field in interface['globals']}
    entries = {entry['name']: metadata['buffered_entries'][entry['logical_id']]
               for entry in interface['entries']}
    # Print helpers CALL a fixed text allocation from a patched offset.  The
    # helper itself is exactly one op11 command in the existing integer backend.
    text_buffers = sorted({int.from_bytes(bytes(code)[3:5], 'little')
                           for code in metadata['programs'].values()
                           if len(code) == 9 and code[:3] == [23, 0, 160] and code[5] == 11})
    assert text_buffers, 'no per-site text buffers found'
    assert all(not any(machine.memory[key]) for key in text_buffers)
    text_sizes = [len(machine.memory[key]) for key in text_buffers]
    assert text_sizes.count(16) == 10 and text_sizes.count(6) == 1, text_sizes
    private_bytes = sum(len(machine.memory[key]) for key in metadata['buffered_owned'])
    private_data_bytes = sum(len(machine.memory[key]) for key in metadata['buffered_owned']
                             if str(key) not in metadata['programs'])
    maximum_steps = 0
    maximum_case = None
    total_steps = 0
    calls = 0
    text_bytes = 0

    def setvalue(name, bits):
        field = fields[name]
        state[field['offset']:field['offset'] + field['bytes']] = bits.to_bytes(field['bytes'], 'little')

    def invoke(entry, expected, status=0, changed=()):
        nonlocal maximum_steps, maximum_case, total_steps, calls, text_bytes
        old = bytes(state)
        begin = len(machine.text)
        machine.steps = 0
        machine.execute(invoke_bytes(entries[entry]))
        observed = bytes(machine.text[begin:])
        assert observed == expected.encode(), (entry, old.hex(), observed, expected)
        assert struct.unpack_from('<HH', state, 12) == (0, status)
        assert all(not any(machine.memory[key]) for key in text_buffers), ('uncleared per-site text', entry)
        allowed = set()
        for name in changed:
            field = fields[name]
            allowed.update(range(field['offset'], field['offset'] + field['bytes']))
        assert all(state[offset] == old[offset] for offset in range(112, len(state)) if offset not in allowed), entry
        calls += 1
        total_steps += machine.steps
        text_bytes += len(observed)
        if machine.steps > maximum_steps:
            maximum_steps = machine.steps
            maximum_case = {'entry': entry, 'x_bits': f'{struct.unpack_from("<I", old, fields["x"]["offset"])[0]:08x}',
                            'text': observed.decode()}

    for index, case in enumerate(values):
        setvalue('x', case['bits'])
        invoke('show', '[' + case['text'] + ']')
        if progress and (index + 1) % 64 == 0:
            print(f'{directory.name}: {index + 1}/{len(values)} float cases', flush=True)

    # Several sites use one shared formatter, without stale digits or sign.
    for a, b in ((MAX_FINITE, 0), (MIN_NORMAL, 0xbf800000),
                 (0x80000000, 0x3f804000), (0xff7fffff, 0x3f800000), (0, 0)):
        setvalue('x', a)
        setvalue('y', b)
        invoke('pair', text(a) + '/' + text(b))
        invoke('mixed', '7:' + text(a))
    setvalue('y', 0)

    # Each arithmetic operation is rounded independently to binary32.  The
    # reference binary32 rounding is an exact-rational neighbour search.
    for bits in (0, 0x3f800000, 0xbf800000, 0x3f804000, 0x3f80c000,
                 MIN_NORMAL, 0x4b800000, 0xcb800000, 0x38d1b717):
        setvalue('x', bits)
        multiplied = nearest_bits(fraction(bits) * 2)
        result = nearest_bits(fraction(multiplied) + 1)
        invoke('expression', 'expr=' + text(result))
    setvalue('x', 0x3f80c000)
    invoke('repeated', text(0x3f80c000) * 3, changed=('i',))
    assert struct.unpack_from('<H', state, fields['i']['offset'])[0] == 2

    # Admission failures produce no output; all source bytes survive.  A later
    # successful call must reset status and work with the same installed code.
    for bits in (1, 0x007fffff, 0x80000001, 0x807fffff,
                 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00001):
        setvalue('x', bits)
        invoke('show', '', 33)
        setvalue('x', 0)
        invoke('show', '[0]')
    setvalue('y', nearest_bits(11))
    invoke('show', '', 32)
    setvalue('y', 0)
    invoke('show', '[0]')
    for bits in (MAX_FINITE, 0xff7fffff):
        setvalue('x', bits)
        invoke('overflow', 'before:', 33)
        setvalue('y', 0x3fc00000)
        invoke('lateFailure', '1.5:', 33)
    setvalue('y', 0)
    for bits in (MIN_NORMAL, 0x80800000):
        setvalue('x', bits)
        invoke('normalized', '0')
    setvalue('x', 0)
    invoke('show', '[0]')

    machine.execute((directory / 'buffered-cleanup.bin').read_bytes())
    assert not any(key in machine.memory for key in metadata['buffered_owned'])
    assert metadata['shared_state'] in machine.memory
    return {'calls': calls, 'float_cases': len(values), 'text_bytes': text_bytes,
            'seconds': time.monotonic() - started, 'maximum_commands_per_call': maximum_steps,
            'maximum_command_case': maximum_case, 'total_commands': total_steps,
            'buffered_install_bytes': (directory / 'buffered-install.bin').stat().st_size,
            'common_install_bytes': (directory / 'common-install.bin').stat().st_size,
            'shared_state_bytes': len(state), 'private_buffers': len(metadata['buffered_owned']),
            'private_allocation_bytes': private_bytes, 'private_data_bytes': private_data_bytes,
            'per_site_text_buffers': len(text_buffers),
            'per_site_text_sizes': text_sizes,
            'source_preservation': True, 'cleared_text_temporaries': True,
            'error_recovery': True, 'cleanup_preserves_shared_state': True}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--quick', action='store_true', help='56 target cases, rather than full exponent/random set')
    parser.add_argument('--limit', type=int, help='cap numeric cases for implementation debugging')
    parser.add_argument('--default-only', action='store_true', help='omit relocated package for debugging')
    parser.add_argument('--progress', action='store_true')
    parser.add_argument('--resources-only', action='store_true')
    args = parser.parse_args()
    BUILD.mkdir(exist_ok=True)
    if args.resources_only:
        print(json.dumps(resource_costs(), indent=2))
        return
    oracle_report = self_check()
    values = target_cases() if args.quick else cases()
    if args.limit is not None:
        values = values[:args.limit]
    host_report = check_host_reference(values)
    source = HERE / 'numbers.golem'
    source.write_text(program())
    reports = []
    for label, extra in [('default', ())] + ([] if args.default_only else [('relocated', ('--reserve', '1:511'))]):
        directory = BUILD / label
        compile_source(source, directory, extra)
        report = check(directory, values, args.progress)
        report['package'] = label
        reports.append(report)
    rejections = 0
    for target in ('native', 'both'):
        output = BUILD / ('rejected-' + target)
        assert not output.exists(), output
        result = subprocess.run([str(ROOT / 'build/golemc'), '--portable', '--target', target,
                                 str(source), str(output)], capture_output=True)
        assert result.returncode == 1 and not output.exists()
        rejections += 1
    result = {'passed': True, 'oracle': oracle_report, 'host_reference': host_report,
              'coverage': 'debug-limited' if args.limit is not None else 'target-subset' if args.quick else 'full',
              'packages': reports, 'target_rejections': rejections}
    name = 'model-evidence.json' if not args.quick and args.limit is None and not args.default_only else 'debug-model-evidence.json'
    (HERE / name).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
