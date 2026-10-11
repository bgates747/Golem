"""Host-only qualification of checked allocation and experimental integration."""
from pathlib import Path
import hashlib
import json
import resource
import subprocess
import sys
import time
sys.dont_write_bytecode = True
from wire_checks import Package

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[2]
BUILD = ROOT / 'build'
BASELINE = '5cb1756'
FLAGS = ['-std=c++17', '-Wall', '-Wextra', '-Werror']
SANITIZERS = ['-O1', '-g', '-fsanitize=address,undefined']


def hashes(paths):
    return {str(path.relative_to(PROJECT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def run(command, **kwargs):
    start = time.monotonic()
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, **kwargs)
    assert result.returncode == 0, (command, result.stdout, result.stderr)
    return {'command': command, 'stdout': result.stdout.strip(), 'seconds': round(time.monotonic() - start, 3)}


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    BUILD.mkdir(exist_ok=True)
    sources = [PROJECT / 'src/golemc.cpp', PROJECT / 'src/buffer_allocator.hpp', PROJECT / 'src/token_reader.hpp',
               *ROOT.glob('*.cpp'), *ROOT.glob('*.hpp'), *ROOT.glob('*.py'), *ROOT.glob('examples/*.golem')]
    frozen = [path for name in ('GOLEM-003', 'GOLEM-004', 'GOLEM-005')
              for path in (ROOT.parent / name).rglob('*')
              if path.is_file() and not {'build', '__pycache__'}.intersection(path.relative_to(PROJECT).parts)]
    before, historical = hashes(sources), hashes(frozen)
    report = {'baseline_commit': subprocess.check_output(['git', 'rev-parse', BASELINE], cwd=PROJECT).decode().strip(),
              'builds': {}, 'cli_rejections': [], 'firmware_executed': False}
    # Reuse all GOLEM-005 semantic/parser expectations without editing its
    # historical files or depending on its generated build directory.
    old_main = subprocess.check_output(['git', 'show', '6a2017c:src/golemc.cpp'], cwd=PROJECT)
    (BUILD / 'legacy.cpp').write_bytes(old_main)
    parser_test = (ROOT.parent / 'GOLEM-005/test_parsers.cpp').read_text()
    for old, new in [('../../../src/golemc.cpp', str(PROJECT / 'src/golemc.cpp')),
                     ('build/legacy.cpp', str(BUILD / 'legacy.cpp')),
                     ('../GOLEM-003/scalar_frontend.hpp', str(ROOT.parent / 'GOLEM-003/scalar_frontend.hpp')),
                     ('"scalar_frontend.hpp"', '"' + str(ROOT.parent / 'GOLEM-005/scalar_frontend.hpp') + '"')]:
        parser_test = parser_test.replace(old, new)
    (BUILD / 'parser-regression.cpp').write_text(parser_test)
    for name, source, flags in [
        ('parser-regression', BUILD / 'parser-regression.cpp', SANITIZERS),
        ('integration', ROOT / 'test_integration.cpp', SANITIZERS),
        ('golemc-integrated', ROOT / 'golemc_integrated.cpp', ['-O2']),
    ]:
        print('Building ' + name, flush=True)
        report['builds'][name] = run(['g++', *FLAGS, *flags, str(source), '-o', str(BUILD / name)])
        if name != 'golemc-integrated':
            report[name] = run([str(BUILD / name)])
            print(report[name]['stdout'], flush=True)

    start = time.monotonic()
    packages, models = {}, {}
    for directory in sorted((BUILD / 'fixtures').iterdir()):
        manifest = json.loads((directory / 'interface.json').read_text())
        package = Package(directory, manifest)
        packages[directory.name] = package
        models[directory.name] = {'numeric_jobs': package.numeric_jobs(), 'programs': len(package.programs),
                                  'decoded_commands': sum(len(code) for code in package.code.values()),
                                  'owned_buffers': len(package.owned), 'logical_call_depth': package.depth,
                                  'installation_bytes': manifest['bootstrap_bytes'], 'recurring_update_call_bytes': 21,
                                  'cleanup_bytes': manifest['cleanup_bytes'],
                                  'persistent_state_and_reinstall_reset': True, 'external_assets_preserved': True}
    reference = packages['compat'].normalized()
    for name in ('floor-default', 'floor-reserved'):
        assert packages[name].normalized() == reference, name
    assert packages['mixed-default'].normalized() == packages['mixed-reserved'].normalized()
    report['decoder_and_host_model'] = {'seconds': round(time.monotonic() - start, 3), 'fixtures': models,
                                       'normalized_scalar_relocations': 2, 'normalized_mixed_relocations': 1,
                                       'description': 'Bounded independent host model; no firmware execution or timing claim'}
    cli = str(BUILD / 'golemc-integrated')
    output = BUILD / 'cli-output'
    valid = ROOT / 'examples/counted-floor.golem'
    report['cli_success'] = run([cli, '--resident', '--reserve', '1:8', '--reserve', '20:24', str(valid), str(output), '-0.75'])
    manifest = json.loads((output / 'interface.json').read_text())
    package = Package(output, manifest)
    assert (output / 'update-call.bin').stat().st_size == 21
    package.execute(package.decode((output / 'update-call.bin').read_bytes()))
    assert int.from_bytes(package.memory[manifest['output']['id']], 'little', signed=True) == -1
    assert {g['name']: int.from_bytes(package.memory[g['id']], 'little') for g in manifest['globals']} == {'calls': 1, 'previous': 0}
    saved = {p.name: p.read_bytes() for p in output.iterdir()}
    invalid = BUILD / 'invalid.golem'
    invalid.write_text(valid.read_text().replace('previous = calls', 'For calls=1 To 2 Next calls'))
    cases = [
        (['--resident', str(invalid), str(output)], 'not qualified'),
        (['--resident', str(valid), str(output), '32768'], 'declared Range'),
        (['--resident', str(valid), str(output), '1e9999'], 'representable range'),
        (['--resident', '--reserve', '1:65534', str(valid), str(output)], 'namespace exhausted'),
        (['--resident', '--reserve', '5:10', '--reserve', '8:12', str(valid), str(output)], 'already claimed'),
    ]
    for text in ('0:1', '1:65535', '1:65536', '999999999999999999999:999999999999999999999', '20:10',
                 '5', '1:', ':10', '1:2:3', '-1:2', 'x:2'):
        cases.append((['--resident', '--reserve', text, str(valid), str(output)], None))
    for args, message in cases:
        result = subprocess.run([cli, *args], cwd=ROOT, capture_output=True, text=True)
        assert result.returncode == 1 and 'golemc-integrated:' in result.stderr
        if message: assert message in result.stderr, result.stderr
        assert {p.name: p.read_bytes() for p in output.iterdir()} == saved
        report['cli_rejections'].append({'args': args, 'returncode': result.returncode, 'stderr': result.stderr.strip()})
    # Raw mode keeps the exact ordinary contract and rejects resident source.
    raw_source = PROJECT / 'examples/loop_golem/loop.golem'
    raw_output = BUILD / 'raw.bin'
    run([cli, str(raw_source), str(raw_output)])
    baseline_cpp = subprocess.check_output(['git', 'show', f'{BASELINE}:src/golemc.cpp'], cwd=PROJECT)
    (BUILD / 'baseline.cpp').write_bytes(baseline_cpp)
    report['builds']['baseline'] = run(['g++', *FLAGS, '-O2', '-I', str(PROJECT / 'src'), str(BUILD / 'baseline.cpp'), '-o', str(BUILD / 'baseline')])
    run([str(BUILD / 'baseline'), str(raw_source), str(BUILD / 'baseline.bin')])
    assert raw_output.read_bytes() == (BUILD / 'baseline.bin').read_bytes()
    before_raw = raw_output.read_bytes()
    rejected = subprocess.run([cli, str(valid), str(raw_output)], cwd=ROOT, capture_output=True, text=True)
    assert rejected.returncode == 1 and raw_output.read_bytes() == before_raw
    assert before == hashes(sources), 'Source changed during checks'
    assert historical == hashes(frozen), 'Historical artifacts changed'
    report['source_sha256'] = before
    report['historical_sha256'] = historical
    report['ordinary_payload_preserved'] = True
    report['rejection_preserves_outputs'] = True
    (ROOT / 'host-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Host checks passed; task-local host-checks.json written.', flush=True)


if __name__ == '__main__':
    main()
