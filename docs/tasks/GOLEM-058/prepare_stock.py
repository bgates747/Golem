#!/usr/bin/env python3
"""Freeze native MOS example jobs, PC references and a test-only stock observer."""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
JOBS = [('hello', []), ('loops', []), ('cube', ['--resident', '--emit-asm']),
        ('counter', ['--portable']), ('arrays', ['--portable']),
        ('trig', ['--portable', '--target', 'native']),
        ('table', ['--resident', '--table-data', 'samples=examples/records.dat']),
        ('matrix', ['--stock-matrix', '--reserve', '1000:1001']),
        ('overflow', ['--portable']), ('precision', ['--portable']),
        ('bitwise', ['--portable']), ('float-print', ['--portable']),
        ('counter-with-load-plan', ['--portable', '--emit-load-plan', '--reserve', '1000:1001'])]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(archive, out):
    archive, out = archive.resolve(), out.resolve()
    assert not out.exists()
    out.mkdir(parents=True)
    shutil.copyfile(archive, out / 'distribution.zip')
    with zipfile.ZipFile(archive) as z:
        z.extractall(out / 'unpacked')
    unpacked = out / 'unpacked'
    manifest = {}
    for line in (unpacked / 'SHA256SUMS').read_text().splitlines():
        h, name = line.split('  ', 1)
        assert sha(unpacked / name) == h
        manifest[name] = h
    assert {str(p.relative_to(unpacked)) for p in unpacked.rglob('*') if p.is_file()} == set(manifest) | {'SHA256SUMS'}
    app = unpacked / 'sdcard/golem'
    (app / 'output').mkdir(exist_ok=True)
    reference = out / 'reference'
    reference.mkdir()
    jobs = []
    for name, options in JOBS:
        source = 'counter' if name == 'counter-with-load-plan' else name
        filename = name + ('.bin' if not options else '')
        args = [*options, 'examples/' + source + '.golem', 'output/' + filename]
        command = [str(ROOT / 'build/golemc'), *options, 'examples/' + source + '.golem', str(reference / filename)]
        subprocess.run(command, cwd=app, check=True, capture_output=True)
        response = 'g058-' + name + '.args'
        (app / response).write_text(' '.join(args) + '\n')
        jobs.append(dict(name=name, arguments=args, response=response, output=filename))
    (app / 'g058.list').write_text(''.join(j['response'] + '\n' for j in jobs))
    names = ['counter', 'arrays', 'overflow', 'precision', 'bitwise', 'float-print', 'table', 'matrix']
    declarations = [('counter', 'i', '<H', 10), ('counter', 'total', '<H', 55),
                    ('arrays', 'value', '<I', 4294967294), ('overflow', 'result', '<H', 17),
                    ('precision', 'maximum', '<I', 4294967295), ('precision', 'minimum', '<i', -2147483648),
                    ('precision', 'result', '<f', 0.4), ('precision', 'negative', '<f', -0.2),
                    ('bitwise', 'both', '<H', 1), ('bitwise', 'either', '<H', 7),
                    ('bitwise', 'different', '<H', 6), ('bitwise', 'inverted', '<H', 65530),
                    ('float-print', 'reading', '<f', 1.5), ('float-print', 'scale', '<f', 2.0)]
    checks, offsets = [], [0]

    def observe(label, bid, offset, data):
        checks.extend(dict(label=label, buffer=bid, offset=offset+i, expected=b) for i, b in enumerate(data))

    for name in names:
        if name in names[:6]:
            interface = json.loads((reference / name / 'interface.json').read_text())
            backends = json.loads((reference / name / 'backends.json').read_text())
            for n, field, fmt, value in declarations:
                if n == name:
                    variable = next(v for v in interface['globals'] if v['name'] == field)
                    observe(name + ' ' + field, backends['shared_state'], variable['offset'], struct.pack(fmt, value))
            observe(name + ' status', backends['shared_state'], 14, struct.pack('<H', 1 if name == 'overflow' else 0))
        elif name == 'table':
            interface = json.loads((reference / name / 'interface.json').read_text())
            observe('selected table record', interface['output']['id'], 0, struct.pack('<Hh', 30, -3))
        else:
            observe('transformed vertices', 1001, 0, b''.join(struct.pack('<hhh', *v) for v in [(100,120,0),(132,120,0),(100,152,0)]))
        offsets.append(len(checks))
    header = ['#define GROUPS 8', '#define PORTABLE_GROUPS 6', '#define OBSERVATIONS ' + str(len(checks)),
              'static const char *names[]={' + ','.join(json.dumps(n) for n in names) + '};',
              'static const unsigned starts[]={' + ','.join(map(str, offsets)) + '};',
              'struct Observation {uint16_t buffer,offset;uint8_t expected;};',
              'static const struct Observation observations[]={' + ','.join('{%d,%d,%d}' % (c['buffer'], c['offset'], c['expected']) for c in checks) + '};']
    (out / 'observations.h').write_text('\n'.join(header) + '\n')
    shutil.copyfile(HERE / 'stock_probe.c.in', out / 'probe.c')
    toolchain = Path(os.environ.get('AGONDEV_ROOT', '/Agon/agondev/release'))
    runner = unpacked / 'source/docs/tasks/GOLEM-051/port'
    subprocess.run([str(toolchain / 'bin/ez80-none-elf-clang'), '-target', 'ez80-none-elf', '-mllvm', '-z80-gas-style', '-mllvm', '-z80-print-zero-offset', '-nostdinc', '-isystem', str(toolchain / 'include'), '-I', str(runner), '-Oz', '-DAGONDEV', '-Wa,-march=ez80+full', '-c', str(out / 'probe.c'), '-o', str(out / 'probe.o')], check=True)
    subprocess.run([str(toolchain / 'bin/ez80-none-elf-ld'), '-defsym=RAM_START=0x40000', '-defsym=RAM_SIZE=0x70000', '-defsym=_has_exit_handler=0', '-T', str(toolchain / 'config/linker.conf'), '--oformat', 'binary', '-o', str(app / 'probe.bin'), str(out / 'probe.o'), '-L' + str(toolchain / 'lib'), '-lagon'], check=True)
    auto = 'SET KEYBOARD 1\nVDU 22 19\ncd /golem\ngolemc --batch g058.list\nload probe.bin\nrun .\ndir\nsave continuation.dat 40000 3\n'
    (out / 'autoexec.txt').write_bytes(auto.replace('\n', '\r\n').encode())
    for name in ('stage_stock.py', 'run_stock.py'):
        shutil.copyfile(HERE / name, out / name)
    shutil.copyfile(ROOT.parent / 'agon-dev-env/cross-agent/vdplet-rollout/inspect_oryx.py', out / 'inspect_oryx.py')
    shutil.copyfile(ROOT.parent / 'agon-dev-env/scripts/flexe_profile.py', out / 'canonical_flexe_profile.py')
    (out / 'checks.json').write_text(json.dumps(dict(checks=checks, expected=bytes(c['expected'] for c in checks).hex(), offsets=offsets), indent=2) + '\n')
    record = dict(scope='Stock MOS/native example compilation and execution; no physical or publication selection', archive_sha256=sha(archive), jobs=jobs, observations=len(checks), maximum_processes=1, retry=False)
    record['files'] = {str(p.relative_to(out)): sha(p) for p in out.rglob('*') if p.is_file()}
    (out / 'cohort.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k not in ('jobs','files')}))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--archive', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    prepare(a.archive, a.output)
