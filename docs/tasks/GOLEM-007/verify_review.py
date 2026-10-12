"""Verify the manually launched local review's complete guest observations."""
from pathlib import Path
import hashlib
import json

TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
result = PROJECT / '.emulator/sdcard/golem/results.dat'
expected = TASK / 'expected.dat'
report = json.loads((TASK / 'emulator-checks.json').read_text())

if not result.is_file():
    raise SystemExit('No review results.dat: run the fixture and wait for transport completion.')
reference = expected.read_bytes()
if hashlib.sha256(reference).hexdigest() != report['expected_sha256']:
    raise SystemExit('Expected-data fingerprint differs from the recorded qualification.')
observed = result.read_bytes()
if observed != reference:
    for check in report['fixture']['checks']:
        start, size = check['start'], check['bytes']
        if observed[start:start+size] != reference[start:start+size]:
            raise SystemExit('Review mismatch: ' + check['label'])
    raise SystemExit(f'Review length mismatch: {len(observed)} bytes, expected {len(reference)}.')
print(f'Review output matches: {report["checks"]} checks, {len(observed)} bytes.')
print('In the emulator window, type dir at the MOS prompt and report to the agent that it works.')
