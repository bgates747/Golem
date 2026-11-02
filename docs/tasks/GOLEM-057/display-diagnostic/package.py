#!/usr/bin/env python3
"""Package prepared matched artifacts and source delta; no execution/deployment."""
from pathlib import Path
import gzip
import hashlib
import io
import json
import tarfile

HERE = Path(__file__).resolve().parent
BUILD = HERE.parent / 'build/display-origin-diagnostic'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    files = {f'preparation/{p.name}': p.read_bytes() for p in HERE.iterdir()
             if p.is_file() and p.name != 'bundle-manifest.json'}
    changed = ['video/vdu.h', 'video/video.ino', '.pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgapalettedcontroller.cpp',
               '.pio/libdeps/esp32dev/vdp-gl/src/g057_diag.h']
    for variant in ['control', 'instrumented']:
        root = BUILD / variant
        for name in ['firmware.bin', 'firmware.elf', 'firmware.map', 'bootloader.bin', 'partitions.bin']:
            files[f'{variant}/{name}'] = (root / '.pio/build/esp32dev' / name).read_bytes()
        for name in changed:
            files[f'prepared-source/{variant}/{name}'] = (root / name).read_bytes()
    manifest = {'schema': 'g057-display-origin-prepared-bundle-v1',
        'scope': 'Source/build/host preparation only. No emulator/target execution or repair is authorized by this bundle.',
        'original_source': '/Agon/mystuff/agon-vdplet/build/vdpl-010/pixels/firmware-src',
        'note': 'Contains changed source files, exact patch and full source hash manifests; unchanged pinned dependencies remain in the original export and prepared source copies.',
        'files': {n: {'bytes': len(d), 'sha256': sha(d)} for n, d in sorted(files.items())}}
    raw_manifest = (json.dumps(manifest, indent=2) + '\n').encode()
    files['bundle-manifest.json'] = raw_manifest
    archive = io.BytesIO()
    with tarfile.open(fileobj=archive, mode='w', format=tarfile.PAX_FORMAT) as tar:
        for name, data in sorted(files.items()):
            info = tarfile.TarInfo(name)
            info.size, info.mode, info.mtime = len(data), 0o644, 0
            tar.addfile(info, io.BytesIO(data))
    target = BUILD / 'prepared-display-origin-r01.tar.gz'
    target.write_bytes(gzip.compress(archive.getvalue(), compresslevel=6, mtime=0))
    manifest['bundle'] = {'path': str(target), 'bytes': target.stat().st_size, 'sha256': sha(target.read_bytes())}
    (HERE / 'bundle-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest['bundle'], indent=2))


if __name__ == '__main__':
    main()
