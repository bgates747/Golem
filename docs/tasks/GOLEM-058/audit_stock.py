#!/usr/bin/env python3
"""Independent returned-file, compiler-output and cube-pixel audit; no target."""
import argparse
import hashlib
import json
from pathlib import Path


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def audit(root, cohort):
    manifest = json.loads((root / 'SHA256.json').read_text())
    for name, expected in manifest.items():
        assert sha(root / name) == expected, name
    plan = json.loads((cohort / 'cohort.json').read_text())
    for name, expected in plan['files'].items():
        assert sha(cohort / name) == expected, name
    assert sha(cohort / 'distribution.zip') == plan['archive_sha256']
    batch = [line.split() for line in (root / 'batch.log').read_text().splitlines()]
    assert batch == [[str(i), '0', job['response']] for i, job in enumerate(plan['jobs'])]
    compared = []
    for job in plan['jobs']:
        expected, actual = cohort / 'reference' / job['output'], root / 'native-output' / job['output']
        if expected.is_file():
            assert actual.read_bytes() == expected.read_bytes()
            compared.append(job['output'])
        else:
            names = {str(p.relative_to(expected)) for p in expected.rglob('*') if p.is_file()}
            assert names == {str(p.relative_to(actual)) for p in actual.rglob('*') if p.is_file()}
            for name in names:
                assert (actual / name).read_bytes() == (expected / name).read_bytes(), (job['name'], name)
                compared.append(job['output'] + '/' + name)
    checks = json.loads((cohort / 'checks.json').read_text())
    assert (root / 'probe-results.dat').read_bytes() == bytes(c['expected'] for c in checks['checks'])
    assert (root / 'probe-status.dat').read_bytes() == (root / 'probe-done.dat').read_bytes() == b'\0'
    assert len((root / 'continuation.dat').read_bytes()) == 3
    wire = (root / 'runtime/mos-to-vdp.bin').read_bytes()
    assert b'Directory: /golem' in wire and b'/golem *' in wire.partition(b'Directory: /golem')[2]
    assert bytes((23,1,1)) in wire
    head, size, maximum, rgb = (root / 'runtime/final.ppm').read_bytes().split(b'\n', 3)
    assert (head, size, maximum, len(rgb)) == (b'P6', b'1024 768', b'255', 1024*768*3)
    anchors = [(256,288),(576,288),(576,608),(256,608),(448,448),(768,448),(768,288),(448,288),(416,288),(416,608),(256,448),(576,448),(608,448)]
    for x, y in anchors:
        assert rgb[(y*1024+x)*3:(y*1024+x)*3+3] == b'\xff\xff\xff', (x,y)
    stats = json.loads((root / 'runtime/stats.json').read_text())
    assert not stats['failed'] and all(stats[n] == 0 for n in ('unmapped','unhandled','unregistered'))
    return dict(passed=True, scope='Independent saved-file/native output/state/cube audit; human review remains separate', archive_sha256=plan['archive_sha256'], verified_files=len(manifest), native_compilations=len(batch), identical_output_files=len(compared), state_bytes=len(checks['checks']), independent_white_cube_anchors=len(anchors), actual_MOS_DIR=True, cursor_enable_command=True, human_review=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('root','cohort','output'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    result = audit(a.root, a.cohort)
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
