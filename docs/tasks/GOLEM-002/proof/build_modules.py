#!/usr/bin/env python3
"""Historical proof CLI; canonical module compilation lives in agon-vdplet."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import subprocess

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[4] / 'agon-vdplet'
spec = importlib.util.spec_from_file_location('vdplet_module_builder', SDK/'tools/build_module.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
build_one = builder.build_one
run = builder.run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path,
                        default=Path.home()/'.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gcc')
    parser.add_argument('--output', type=Path, default=HERE/'build')
    parser.add_argument('--packed', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    entries = [build_one(HERE/'numeric.c', output, args.compiler, f'numeric-{variant}',
                         variant, 0x4E554D30+variant, 1) for variant in (1, 2)]
    if args.packed:
        entries.append(build_one(HERE.parent/'packed-mono/packed_mono.cpp', output,
                                 args.compiler, 'packed-mono', 1, 0x5041434B, 3))
    manifest = {
        'compiler': subprocess.check_output([str(args.compiler), '--version'], text=True).splitlines()[0],
        'abi_header_sha256': hashlib.sha256((SDK/'include/vdplet.h').read_bytes()).hexdigest(),
        'modules': entries,
    }
    (output/'modules.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
