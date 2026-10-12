#!/usr/bin/env python3
"""Verify saved observations after the user has closed the emulator."""
from pathlib import Path
import hashlib
import json

TASK=Path(__file__).resolve().parent
PROJECT=TASK.parents[2]
result=PROJECT/'.emulator/sdcard/golem-015/results.dat'
expected=(TASK/'expected.dat').read_bytes()
report=json.loads((TASK/'emulator-checks.json').read_text())
if not result.exists():
    raise SystemExit('No fresh result file. Run the prepared emulator first; see the task README.')
actual=result.read_bytes()
if actual!=expected:
    mismatches=[]
    for check in report['fixture']['checks']:
        start,size=check['start'],check['bytes']
        if actual[start:start+size]!=expected[start:start+size]:mismatches.append(check['label'])
    raise SystemExit(f'Review output differs: {len(actual)} bytes, expected {len(expected)}; first failures: {mismatches[:5]}')
assert hashlib.sha256(expected).hexdigest()==report['expected_sha256']
print(f'Review output matches: {report["checks"]} checks, {len(actual)} bytes.')
