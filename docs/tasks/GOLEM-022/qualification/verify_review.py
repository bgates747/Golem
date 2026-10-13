"""Agent-side verification of saved human-review results; no extra user steps."""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent

def verify(result):
    expected = (HERE/'build/review/expected.dat').read_bytes()
    meta = json.loads((HERE/'build/review/expected.json').read_text())
    actual = result.read_bytes()
    if actual != expected:
        wrong = [c for c in meta['checks'] if actual[c['start']:c['start']+c['bytes']] !=
                 expected[c['start']:c['start']+c['bytes']]]
        raise AssertionError(f'{len(wrong)} review checks differ: {wrong[:8]}')
    print(f'Review output matches: {len(meta["checks"])} checks, {len(actual)} bytes.')
    return dict(checks=len(meta['checks']), bytes=len(actual),
                sha256=hashlib.sha256(actual).hexdigest())

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('result', nargs='?', type=Path, default=HERE/'.emulator/sdcard/golem022/results.dat')
    verify(p.parse_args().result)
