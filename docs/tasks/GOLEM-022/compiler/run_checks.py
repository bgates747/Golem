#!/usr/bin/env python3
"""Focused maintained contracts, legacy byte parity and real ADL consumers."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
ROOT = TASK.parents[2]
BUILD = HERE / 'build'
BUILD.mkdir(exist_ok=True)
BEFORE = TASK / 'build/golemc-before'
COMPILER = ROOT / 'build/golemc'
ASSEMBLER = ROOT / 'docs/tasks/GOLEM-007/build/ez80asm'
ARTIFACTS = ['bootstrap.bin', 'invoke.bin', 'update-call.bin', 'cleanup.bin',
             'interface.json', 'golem.inc']
started = time.monotonic()
commands = []


def run(args, cwd=ROOT, ok=True):
    tick = time.monotonic()
    p = subprocess.run(list(map(str, args)), cwd=cwd, capture_output=True, text=True)
    commands.append({'args': list(map(str, args)),
                     'seconds': round(time.monotonic() - tick, 3)})
    if ok and p.returncode:
        raise RuntimeError(p.stdout + p.stderr)
    return p


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


assert BEFORE.is_file(), 'Root must archive the current compiler before edits'
flags = '-std=c++17 -Wall -Wextra -Werror -O2'
host = run(['make', '-C', 'src', 'check', 'CXXFLAGS=' + flags]).stdout.strip()
print(host, flush=True)
sanitized = BUILD / 'test-sanitized'
run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-O1', '-g',
     '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
     ROOT / 'tests/test_compiler.cpp', '-o', sanitized])
sanitizer = run([sanitized]).stdout.strip()
print(sanitizer, flush=True)

legacy = []
sources = ['examples/resident/floor.golem', 'examples/resident/counted-floor.golem',
           'docs/tasks/GOLEM-013/checked-subtract.golem',
           'docs/tasks/GOLEM-014/checked-multiply.golem',
           'docs/tasks/GOLEM-015/checked-divmod.golem',
           'docs/tasks/GOLEM-017/table.golem', 'docs/tasks/GOLEM-018/geometry.golem',
           'docs/tasks/GOLEM-019/bitmaps.golem', 'docs/tasks/GOLEM-019/review.golem',
           'docs/tasks/GOLEM-020/animation.golem']
assets = ['--reserve', '8195:8195', '--reserve', '8201:8202', '--reserve', '8463:8463']
for source in sources:
    for relocated in (False, True):
        options = ['--resident', '--emit-asm']
        if relocated:
            options += ['--reserve', '1:254', '--reserve', '260:264']
        if 'GOLEM-019' in source or 'GOLEM-020' in source:
            options += assets
        if source.endswith('table.golem'):
            options += ['--table-data', f'samples={ROOT}/docs/tasks/GOLEM-017/records.dat']
        work = BUILD / f'legacy-{len(legacy)}'
        for name, compiler in [('old', BEFORE), ('new', COMPILER)]:
            run([compiler, *options, ROOT / source, work / name])
        for artifact in ARTIFACTS:
            assert (work / 'old' / artifact).read_bytes() == (work / 'new' / artifact).read_bytes(), (source, relocated, artifact)
        legacy.append({'source': source, 'relocated': relocated,
                       'artifacts': {a: sha(work / 'new' / a) for a in ARTIFACTS}})
raw = []
for source in ['examples/hello_golem/hello.golem', 'examples/loop_golem/loop.golem']:
    work = BUILD / f'raw-{len(raw)}'
    work.mkdir(exist_ok=True)
    for name, compiler in [('old', BEFORE), ('new', COMPILER)]:
        run([compiler, ROOT / source, work / name])
    assert (work / 'old').read_bytes() == (work / 'new').read_bytes()
    raw.append({'source': source, 'sha256': sha(work / 'new')})
print(f'Legacy parity: {len(legacy)} six-artifact packages and {len(raw)} raw payloads', flush=True)

assembly_cases = []
for name, reserve in [('ordinary', []), ('relocated', ['--reserve', '1:254', '--reserve', '260:264'])]:
    work = BUILD / name
    pkg = work / 'package'
    run([COMPILER, '--resident', '--emit-asm', '--reserve', '50000:50000',
         *reserve, HERE / 'multi.golem', pkg])
    m = json.loads((pkg / 'interface.json').read_text())
    assert m['entry'] == m['entries'][0] and len(m['entries']) == 3
    assert m['input']['bytes'] == 5 and len({g['id'] for g in m['globals']}) == 1
    assert m['input']['id'] == m['globals'][0]['id']
    assert (pkg / 'invoke.bin').read_bytes() == bytes([23, 0, 160]) + m['entry']['id'].to_bytes(2, 'little') + b'\x01'
    assert (pkg / 'update-call.bin').read_bytes().endswith((pkg / 'invoke.bin').read_bytes())
    names, expected = [], []
    for entry in m['entries']:
        stem = 'golem_' + entry['name']
        names += [stem, stem + '_call', stem + '_call_len', stem + '_buffer']
    for g in m['globals']:
        names += ['golem_' + g['name'] + suffix for suffix in ['_buffer', '_offset', '_bytes', '_type']]
        expected += [g['id'], g['offset'], g['bytes'], {'uint8': 5, 'uint16': 1, 'sint16': 2}[g['type']]]
    names += ['golem_inputs_buffer', 'golem_inputs_bytes', 'golem_package_owned_count']
    expected += [m['input']['id'], m['input']['bytes'], len(m['owned'])]
    streams = [('golem_package_bootstrap', 'bootstrap.bin'),
               ('golem_package_update_call', 'update-call.bin'),
               ('golem_package_cleanup', 'cleanup.bin')]
    for label, _ in streams:
        names += [label, label + '_len']
    text = '.assume adl=1\n.org $40000\n' + ''.join('dl ' + n + '\n' for n in names)
    text += ''.join('call golem_' + e['name'] + '\n' for e in m['entries'])
    text += 'include "package/golem.inc"\n'
    (work / 'consumer.asm').write_text(text)
    run([ASSEMBLER, 'consumer.asm', 'consumer.bin', '-c'], cwd=work)
    data = (work / 'consumer.bin').read_bytes()
    words = [int.from_bytes(data[i*3:i*3+3], 'little') for i in range(len(names))]
    assert words[12:12+len(expected)] == expected
    calls_offset = len(names)*3
    for i, entry in enumerate(m['entries']):
        wrapper, packet, size, buffer = words[4*i:4*i+4]
        assert buffer == entry['id'] and size == 6
        offset = wrapper - 0x40000
        assert data[offset:offset+11] == b'\x21' + packet.to_bytes(3, 'little') + b'\x01\x06\0\0\x5b\xdf\xc9'
        offset = packet - 0x40000
        assert data[offset:offset+size] == bytes([23, 0, 160]) + buffer.to_bytes(2, 'little') + b'\x01'
        assert data[calls_offset+4*i:calls_offset+4*i+4] == b'\xcd' + wrapper.to_bytes(3, 'little')
    for i, (_, artifact) in enumerate(streams):
        address, size = words[12+len(expected)+2*i:14+len(expected)+2*i]
        assert data[address-0x40000:address-0x40000+size] == (pkg / artifact).read_bytes()
    assert (pkg / 'golem.inc').read_text().count('golem_package_bootstrap:\n') == 1
    assert len({e['id'] for e in m['entries']}) == 3
    assembly_cases.append({'name': name, 'entries': m['entries'], 'owned': len(m['owned']),
                           'input_bytes': m['input']['bytes'], 'binary_bytes': len(data),
                           'include_sha256': sha(pkg / 'golem.inc')})

# Assembly-only collisions remain valid source without requested exports.
bad_sources = [
    'Sub paint draw_point(1,0,0) EndSub Sub Paint draw_point(1,0,0) EndSub',
    'sint16 Second=0 Sub first draw_point(1,0,0) EndSub Sub second draw_point(1,0,0) EndSub',
    'Sub paint draw_point(1,0,0) EndSub Sub paint_call draw_point(1,0,0) EndSub',
    'Sub paint draw_point(1,0,0) EndSub Sub package_cleanup draw_point(1,0,0) EndSub',
    'Sub first draw_point(1,0,0) EndSub Sub ' + 'a'*60 + ' draw_point(1,0,0) EndSub',
]
preserve = BUILD / 'preserve'
preserve.mkdir(exist_ok=True)
failures = []
for i, source in enumerate(bad_sources + ['Sub first draw_point(1,0,0) EndSub Sub later x=1 EndSub']):
    path = BUILD / f'bad-{i}.golem'
    path.write_text(source)
    for artifact in ARTIFACTS:
        (preserve / artifact).write_bytes(b'preserve')
    p = run([COMPILER, '--resident', '--emit-asm', path, preserve], ok=False)
    assert p.returncode == 1 and all((preserve / a).read_bytes() == b'preserve' for a in ARTIFACTS), p.stderr
    missing = BUILD / f'missing-{i}'
    p_missing = run([COMPILER, '--resident', '--emit-asm', path, missing], ok=False)
    assert p_missing.returncode == 1 and not missing.exists()
    if i < len(bad_sources):
        run([COMPILER, '--resident', path, BUILD / f'without-asm-{i}'])
    failures.append(p.stderr.strip())

report = {'pass': True, 'scope': 'Host, byte parity and actual ez80asm; stock emulator separately qualified',
          'seconds': round(time.monotonic()-started, 3), 'baseline_compiler_sha256': sha(BEFORE),
          'maintained_contract_output': host, 'sanitizer_output': sanitizer,
          'legacy_packages': legacy, 'raw_payloads': raw, 'assembler_consumers': assembly_cases,
          'prepublication_rejections': failures, 'commands': commands,
          'source_sha256': {str(p.relative_to(ROOT)): sha(p) for p in
                           [ROOT/'src/resident_drawing.hpp', ROOT/'src/assembly_output.hpp',
                            ROOT/'tests/test_compiler.cpp', Path(__file__).resolve()]}}
(HERE / 'host-checks.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'GOLEM-022 host checks pass: {len(legacy)*6} legacy artifacts, {len(raw)} raw payloads, '
      f'{len(assembly_cases)} actual assembler consumers; {report["seconds"]}s', flush=True)
