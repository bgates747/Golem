"""Agent-only retrieval after the Author closes the Oryx review."""
from pathlib import Path
import json
import subprocess

HERE = Path(__file__).resolve().parent
CODE = r'''from pathlib import Path
import hashlib,json,subprocess
root=Path.home()/'Agon/mystuff/golem';p=root/'.emulator/golem-055';d=p/'sdcard/golem/ram'
for proc in Path('/proc').glob('[0-9]*'):
 try:
  if str(p/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\0'):raise RuntimeError('Review is running; close it before retrieval')
 except OSError:pass
subprocess.run(['sha256sum','--check','--status','.runtime.sha256'],cwd=p,check=True)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
e=json.loads((d/'expected.json').read_text());data=(d/'results.dat').read_bytes()
assert data==bytes.fromhex(e['expected_observations'])
assert (d/'done.dat').read_bytes()==b'\0'
assert sha(d/'two-floats.golem')==e['source_sha256']
assert sha(d/'review.bin')==e['guest_sha256']
assert sha(p/'sdcard/golem/compiler/portable.bin')==e['worker_sha256']
ref=root/'docs/tasks/GOLEM-055/build/reference'
assert {f.name for f in ref.iterdir()}=={f.name for f in (d/'package').iterdir()}
for f in ref.iterdir():assert f.read_bytes()==(d/'package'/f.name).read_bytes(),f.name
print(json.dumps({'passed':True,'cases':len(e['cases']),'state_observations':len(data),'results_sha256':sha(d/'results.dat'),'matching_package_files':len(list(ref.iterdir())),'profile_stopped':True,'runtime_pins_match':True,'worker_sha256':e['worker_sha256'],'human_visual_report':'record separately','human_keyboard_report':'record separately'}))
'''

if __name__ == '__main__':
    result = subprocess.run(['ssh', '-o', 'BatchMode=yes', 'oryx-linuxlite', 'python3 -'],
                            input=CODE.encode(), stdout=subprocess.PIPE, check=True)
    report = json.loads(result.stdout)
    (HERE / 'review-retrieval.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
