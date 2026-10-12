#!/usr/bin/env python3
"""Rerun Rally's import regression without writing into its reference checkout."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

TASK = Path(__file__).resolve().parent
RALLY = TASK.parents[2].parent / 'golem-rally19'
BUILD = TASK / 'build'
SOURCES = ['src/golemc.cpp', 'src/hosted.hpp', 'tests/test_imports.cpp',
           'examples/native_lookup/kernel.golem', 'examples/native_lookup/make_assets.py']

def fingerprints():
    return {p: hashlib.sha256((RALLY / p).read_bytes()).hexdigest() for p in SOURCES}

def main():
    BUILD.mkdir(exist_ok=True)
    before = fingerprints()
    start = time.monotonic()
    command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
               '-fsanitize=address,undefined', str(RALLY / 'tests/test_imports.cpp'),
               '-o', str(BUILD / 'test_imports')]
    subprocess.run(command, check=True, cwd=BUILD)
    result = subprocess.run([str(BUILD / 'test_imports')], check=True, cwd=BUILD,
                            capture_output=True, text=True)
    assert fingerprints() == before, 'Reference sources changed during audit'
    report = {
        'scope': 'AST-001 host import regression; no emulator or hardware execution',
        'pass': True,
        'rally_commit': subprocess.check_output(['git', '-C', str(RALLY), 'rev-parse', 'HEAD'], text=True).strip(),
        'rally_status_after': subprocess.check_output(['git', '-C', str(RALLY), 'status', '--porcelain'], text=True),
        'source_sha256': before,
        'command': command,
        'stdout': result.stdout.strip(),
        'seconds': round(time.monotonic() - start, 3),
        'reference_checkout_modified': False,
    }
    (TASK / 'import-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(result.stdout.strip())
    print(f'AST-001 import audit passed in {report["seconds"]} seconds.')

if __name__ == '__main__':
    main()
