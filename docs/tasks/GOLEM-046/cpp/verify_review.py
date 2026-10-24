#!/usr/bin/env python3
"""Agent-only retrieval check after the Author closes the C++ profile."""
from pathlib import Path
import subprocess
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PROFILE=ROOT/'.emulator/golem-046-cpp'
subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=PROFILE,check=True)
app=PROFILE/'sdcard/golem046'
actual=(app/'results.dat').read_bytes();expected=(HERE/'build/expected.dat').read_bytes()
assert actual==expected,(actual.hex(),expected.hex())
assert not (app/'failure.dat').exists()
log=(app/'cpp.log').read_text();assert log.split() and log.split()[-1]=='PASS',log
print(f'C++ AGNB review matches all {len(expected)} result bytes and its completed application log.')
