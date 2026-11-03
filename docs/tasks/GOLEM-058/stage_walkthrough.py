#!/usr/bin/env python3
"""Stage a stopped consumer walkthrough using unchanged packaged examples."""
import argparse
import json
from pathlib import Path
from stage_stock import stage, sha


def prepare(reference, profile, cohort):
    stage(reference, profile, cohort)
    auto = '''SET KEYBOARD 1
VDU 22 19
cd /golem
golemc examples/hello.golem output/hello.bin
golemc --portable examples/counter.golem output/counter
golemc --portable examples/float-print.golem output/float-print
golemc --portable examples/bitwise.golem output/bitwise
golemc --resident --emit-asm examples/cube.golem output/cube
VDU 12
VDU 28 0 95 127 81
echo Golem on Agon: compiled here, running on stock VDP
echo Source: hello, counter, float-print, bitwise and cube in examples/
golemrun output/hello.bin
golemrun --portable output/counter
golemrun --portable output/float-print
golemrun --portable output/bitwise
echo All 12 white edges: examples/cube.golem, Sub drawCube
golemrun --review --resident output/cube
save review-done.dat 40000 3
'''
    (profile / 'sdcard/autoexec.txt').write_bytes(auto.replace('\n', '\r\n').encode())
    record = json.loads((profile / 'stock-profile.json').read_text())
    record['pins']['sdcard/autoexec.txt'] = sha(profile / 'sdcard/autoexec.txt')
    record['scope'] = 'Packaged Golem examples compile natively; only cube.golem draws; MOS sets mode/footer and loads/calls.'
    record['human_review'] = 'pending'
    (profile / 'stock-profile.json').write_text(json.dumps(record, indent=2) + '\n')
    (profile / '.runtime.sha256').write_text(''.join(h + '  ' + n + '\n' for n, h in record['pins'].items()))
    print(json.dumps(dict(stopped=True, profile=str(profile), expected='white cube, Hello world, i=10 total=55, reading=1.5 scaled=3, and=1 or=7 xor=6 not=65530', archive_sha256=record['archive_sha256'])))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('reference','profile','cohort'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    prepare(a.reference.resolve(), a.profile.resolve(), a.cohort.resolve())
