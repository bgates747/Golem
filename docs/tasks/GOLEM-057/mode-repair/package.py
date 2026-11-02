#!/usr/bin/env python3
"""Seal offline control/candidate artifacts and source evidence; no deployment."""
from pathlib import Path
import gzip
import hashlib
import io
import json
import tarfile

HERE = Path(__file__).resolve().parent
BUILD = HERE.parent / 'build/display-mode-repair'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    files = {f'preparation/{p.name}': p.read_bytes() for p in HERE.iterdir()
             if p.is_file() and p.name != 'bundle-manifest.json'}
    for name in ['mode-fallback-release-plan-v1.md', 'mode-fallback-release-plan-v1.freeze.json']:
        files[f'preparation/{name}'] = (HERE.parent / name).read_bytes()
    for variant in ['control00', 'candidate']:
        root = BUILD / variant
        for name in ['firmware.bin', 'firmware.elf', 'firmware.map', 'bootloader.bin', 'partitions.bin']:
            files[f'{variant}/{name}'] = (root / '.pio/build/esp32dev' / name).read_bytes()
        for name in ['video/vdu.h', 'video/agon_screen.h']:
            files[f'prepared-source/{variant}/{name}'] = (root / name).read_bytes()
    manifest = {'schema': 'g057-mode-repair-prepared-bundle-v1',
        'scope': 'Offline prepared failure-only disposal candidate. No execution/deployment grant or maintained SDK promotion.',
        'original_source': '/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src',
        'source_note': 'Includes source delta, exact patch and full original-source hashes; unchanged pinned dependencies remain in original export and separate copied roots.',
        'files': {n: {'bytes': len(d), 'sha256': sha(d)} for n, d in sorted(files.items())}}
    files['bundle-manifest.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    stream = io.BytesIO()
    with tarfile.open(fileobj=stream, mode='w', format=tarfile.PAX_FORMAT) as tar:
        for name, data in sorted(files.items()):
            info = tarfile.TarInfo(name)
            info.size, info.mode, info.mtime = len(data), 0o644, 0
            tar.addfile(info, io.BytesIO(data))
    target = BUILD / 'prepared-mode-repair-r01.tar.gz'
    target.write_bytes(gzip.compress(stream.getvalue(), compresslevel=6, mtime=0))
    manifest['bundle'] = {'path': str(target), 'bytes': target.stat().st_size, 'sha256': sha(target.read_bytes())}
    (HERE / 'bundle-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    with tarfile.open(target, 'r:gz') as tar:
        for name, expected in manifest['files'].items():
            data = tar.extractfile(name).read()
            assert len(data) == expected['bytes'] and sha(data) == expected['sha256'], name
    print(json.dumps(manifest['bundle'], indent=2))


if __name__ == '__main__':
    main()
