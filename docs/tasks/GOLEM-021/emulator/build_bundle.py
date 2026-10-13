#!/usr/bin/env python3
"""Package owned pinned sources for Oryx, excluding host build products."""
from __future__ import annotations
from datetime import datetime, timezone
import gzip
import hashlib
import io
import json
from pathlib import Path
import tarfile

HERE = Path(__file__).resolve().parent
BUILD = HERE / 'build'
CORE = BUILD / 'flexe-source'
FRONTEND = BUILD / 'fab-source'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

identities = {}
for name, source, marker in [('core', CORE, '.golem-flexe-source.json'),
                             ('frontend', FRONTEND, '.golem-fab-source.json')]:
    identity = json.loads((source / marker).read_text())
    for path, digest in identity['patch_files'].items():
        if sha(source / path) != digest:
            raise ValueError(f'Changed pinned {name} source: {path}')
    identities[name] = identity

files = [p for p in HERE.iterdir() if p.is_file() and p.name != 'source-bundle.json']
for source in (CORE, FRONTEND):
    files.extend(p for p in source.rglob('*') if p.is_file() and
                 'target' not in p.relative_to(source).parts)
files.sort(key=lambda p: p.relative_to(HERE).as_posix())
manifest = dict(schema=1, identities=identities,
                files={p.relative_to(HERE).as_posix(): sha(p) for p in files})
manifest_bytes = (json.dumps(manifest, indent=2) + '\n').encode()
archive = BUILD / 'source-bundle.tar.gz'
with archive.open('wb') as output:
    with gzip.GzipFile(filename='', fileobj=output, mode='wb', mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode='w') as tar:
            for path in files:
                info = tar.gettarinfo(str(path), 'emulator/' + path.relative_to(HERE).as_posix())
                info.uid = info.gid = info.mtime = 0
                info.uname = info.gname = ''
                with path.open('rb') as data:
                    tar.addfile(info, data)
            info = tarfile.TarInfo('emulator/build/source-bundle.manifest.json')
            info.size = len(manifest_bytes)
            info.mode = 0o644
            tar.addfile(info, io.BytesIO(manifest_bytes))
receipt = dict(schema=1, created_utc=datetime.now(timezone.utc).isoformat(),
               archive='build/source-bundle.tar.gz', bytes=archive.stat().st_size,
               sha256=sha(archive), files=len(files), identities=identities,
               extraction_parent='~/Agon/mystuff/golem/docs/tasks/GOLEM-021',
               manifest_sha256=hashlib.sha256(manifest_bytes).hexdigest())
(HERE / 'source-bundle.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt, indent=2))
