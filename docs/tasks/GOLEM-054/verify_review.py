"""Agent-only retrieval of the stopped Oryx review; no repeated guest instruction."""
from pathlib import Path
import json
import subprocess

HERE = Path(__file__).resolve().parent
REMOTE = 'Agon/mystuff/golem/.emulator/golem-054'
CODE = r'''from pathlib import Path
import hashlib,json,subprocess
p=Path.home()/'Agon/mystuff/golem/.emulator/golem-054';d=p/'sdcard/golem/floats'
for proc in Path('/proc').glob('[0-9]*'):
 try:
  if str(p/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\0'):raise RuntimeError('Review is still running; close it before retrieval')
 except OSError:pass
subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=p,check=True)
e=json.loads((d/'expected.json').read_text());data=(d/'results.dat').read_bytes()
assert data==bytes.fromhex(e['expected_observations'])
assert (d/'done.dat').read_bytes()==b'\0'
assert hashlib.sha256((d/'numbers.golem').read_bytes()).hexdigest()==e['source_sha256']
assert hashlib.sha256((d/'review.bin').read_bytes()).hexdigest()==e['guest_sha256']
print(json.dumps({'passed':True,'cases':len(e['cases']),'state_observations':len(data),'results_sha256':hashlib.sha256(data).hexdigest(),'profile_stopped':True,'runtime_pins_match':True,'human_visual_report':'record separately','human_keyboard_report':'record separately'}))
'''
if __name__ == '__main__':
    result = subprocess.run(['ssh', '-o', 'BatchMode=yes', 'oryx-linuxlite', 'python3 -'],
                            input=CODE.encode(), stdout=subprocess.PIPE, check=True)
    report = json.loads(result.stdout)
    (HERE / 'review-retrieval.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
