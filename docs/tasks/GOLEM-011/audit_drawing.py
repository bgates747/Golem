#!/usr/bin/env python3
"""Rerun existing Rally drawing checks without writing into that reference tree."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
RALLY = PROJECT.parent / 'golem-rally19'
BUILD = TASK / 'build'
TESTS = ['test_hosted', 'test_elements', 'test_bitmap', 'test_viewport']
SOURCES = ['src/golemc.cpp', 'src/hosted.hpp',
           'docs/design/rally19-hosted-v1.md',
           *[f'tests/{name}.cpp' for name in TESTS],
           'examples/native_vehicle_math/bitmap.golem',
           'examples/native_quantization/viewport.golem']
FIRMWARE = ['context/graphics.h', 'context/viewport.h', 'vdu.h',
            'vdu_sprites.h', 'buffers.h', 'types.h']


def fingerprints():
    paths = [RALLY / name for name in SOURCES]
    for release, root in [('1.2.4', PROJECT.parent / 'AgonJukebox'), ('1.2.5', PROJECT)]:
        base = root / f'.emulator/runtime/fab-{release}/src/vdp/vdp-console8/video'
        paths.extend(base / name for name in FIRMWARE)
    return {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}


def main():
    BUILD.mkdir(exist_ok=True)
    before = fingerprints()
    status = subprocess.check_output(['git', '-C', str(RALLY), 'status', '--porcelain'], text=True)
    start = time.monotonic()
    tests = []
    for name in TESTS:
        tick = time.monotonic()
        command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
                   '-fsanitize=address,undefined', str(RALLY / f'tests/{name}.cpp'),
                   '-o', str(BUILD / name)]
        subprocess.run(command, check=True, cwd=BUILD)
        result = subprocess.run([str(BUILD / name)], check=True, cwd=BUILD,
                                capture_output=True, text=True)
        tests.append({'test': name, 'command': command, 'stdout': result.stdout.strip(),
                      'seconds': round(time.monotonic() - tick, 3)})
        print(name + ': ' + result.stdout.strip(), flush=True)
    assert fingerprints() == before, 'Reference sources changed during audit'
    assert subprocess.check_output(['git', '-C', str(RALLY), 'status', '--porcelain'], text=True) == status
    report = {'pass': True, 'scope': 'DRW-001 existing host compiler drawing checks with ASan/UBSan; no graphics execution',
              'rally_commit': subprocess.check_output(['git', '-C', str(RALLY), 'rev-parse', 'HEAD'], text=True).strip(),
              'rally_status': status, 'source_sha256': before, 'tests': tests,
              'seconds': round(time.monotonic() - start, 3),
              'reference_checkout_modified': False, 'emulator_launched': False,
              'production_compiler_changed': False}
    (TASK / 'drawing-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'DRW-001 audit passes: {len(tests)} existing tests; {report["seconds"]}s.')


if __name__ == '__main__':
    main()
