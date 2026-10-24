#!/usr/bin/env python3
"""Agent-side verification after the Author closes the emulator."""
from pathlib import Path
import subprocess
HERE=Path(__file__).resolve().parent
PROFILE=HERE.parents[2]/'.emulator/golem-046'
subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=PROFILE,check=True)
actual=(PROFILE/'sdcard/golem046/results.dat').read_bytes()
expected=(HERE/'build/expected.dat').read_bytes()
assert actual==expected,(actual.hex(),expected.hex())
assert not (PROFILE/'sdcard/golem046/failure.dat').exists()
print(f'Streamed Golem review matches all {len(expected)} result bytes.')
