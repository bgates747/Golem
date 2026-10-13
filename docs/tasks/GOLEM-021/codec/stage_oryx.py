"""Qualify supplied codec cases on Oryx without changing the main review profile."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
PROJECT = HERE.parents[3]
BUILD = HERE / 'build'
HOST = 'oryx-linuxlite'
REMOTE = 'Agon/mystuff/golem/docs/tasks/GOLEM-021/codec'
SSH = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=8', HOST]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def command(args, *, script=None, timeout=60):
    result = subprocess.run(args, input=script, text=True, capture_output=True,
                            timeout=timeout)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def bundle():
    files = {}
    for path in sorted((BUILD / 'cases').rglob('*')):
        if path.is_file():
            files[str(path.relative_to(HERE))] = path.read_bytes()
    for path in sorted((BUILD / 'supplied').rglob('*')):
        if path.is_file():
            files[str(path.relative_to(HERE))] = path.read_bytes()
    for name in ('cases.json', 'source-pins.json'):
        files['build/' + name] = (BUILD / name).read_bytes()
    for name in ('profile.py', 'qualify.py', 'verify.py'):
        files['support/' + name] = (TASK / 'consumer' / name).read_bytes()
    files['support/setup_emulator.py'] = (
        PROJECT.parent / 'agon-dev-env/scripts/setup_emulator.py').read_bytes()
    for name in ('esp32-firmware.bin', 'esp32-firmware.elf'):
        files['build/' + name] = (TASK / '.emulator' / name).read_bytes()
    files['review.sh'] = (TASK / 'review.sh').read_bytes()
    for name in ('PLAN.md', 'probe.asm', 'build.py', 'stage_oryx.py'):
        files[name] = (HERE / name).read_bytes()
    manifest = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                    files={name:dict(bytes=len(data),sha256=sha(data))
                           for name,data in sorted(files.items())})
    files['build/bundle-manifest.json'] = (json.dumps(manifest,indent=2)+'\n').encode()
    archive = BUILD / 'codec-bundle.tar.gz'
    with tarfile.open(archive, 'w:gz') as target:
        for name,data in sorted(files.items()):
            info = tarfile.TarInfo(name)
            info.size = len(data)
            info.mode = 0o755 if name == 'review.sh' else 0o644
            target.addfile(info,io.BytesIO(data))
    manifest.update(archive_sha256=sha(archive.read_bytes()), archive_bytes=archive.stat().st_size)
    (BUILD / 'bundle.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Codec bundle: {len(files)} files, {archive.stat().st_size} bytes.',flush=True)
    return archive,manifest


REMOTE_SCRIPT = r'''
set -euo pipefail
remote_project="$HOME/Agon/mystuff/golem"
"$remote_project/.venv/bin/python" - "$remote_project" "$1" "$2" <<'PY'
from datetime import datetime,timezone
import hashlib,importlib.util,io,json,os,platform,shutil,socket,sys,tarfile
from pathlib import Path
p=Path(sys.argv[1]); t=p/'docs/tasks/GOLEM-021'; c=t/'codec'; b=c/'build'
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert socket.gethostname()=='oryx-linuxlite' and platform.machine()=='x86_64'
archive=b/'codec-bundle.tar.gz.new'
assert sha(archive)==sys.argv[2], 'Bundle transfer hash mismatch'
with tarfile.open(archive) as source: source.extractall(c,filter='data')
manifest=json.loads((b/'bundle-manifest.json').read_text())
for name,identity in manifest['files'].items():
    path=c/name
    assert path.stat().st_size==identity['bytes'] and sha(path)==identity['sha256'],name
archive.rename(b/'codec-bundle.tar.gz')
frontend=t/'emulator/build/fab-source/target/release/fab-agon-emulator'
adapter=t/'emulator/build/flexe-vdp.so'
def stopped():
    for child in Path('/proc').glob('[0-9]*'):
        try:
            if (child/'exe').resolve()==frontend.resolve():
                raise RuntimeError('Dedicated hybrid frontend is already running')
        except (FileNotFoundError,PermissionError,ProcessLookupError): pass
stopped()
main=t/'.emulator'
main_id=(main/'.runtime.sha256').read_bytes()
sys.path.insert(0,str(c/'support'))
import profile,qualify
qualify.TASK=c
evidence=c/'evidence'; evidence.mkdir(exist_ok=True)
receipts=[]; names=[]
cases=json.loads((b/'cases.json').read_text())
for case in cases:
    name=case['fixture']; profile.HERE=b/'cases'/name
    profile.prepare(c/'.emulator',p/'.emulator/runtime/fab-1.2.5',adapter,
                    b/'esp32-firmware.bin',b/'esp32-firmware.elf',frontend,
                    b/'supplied',c/'support/setup_emulator.py')
    for engine in ('jit','interp'):
        started=datetime.now(timezone.utc).isoformat()
        result=qualify.qualify((c/'.emulator').resolve(),engine,float(sys.argv[3]))
        result.update(codec_entry=case['codec_entry'],sequence_prefix=case['sequence_prefix'],
                      unsupported_mode_case=case['unsupported_mode_case'],
                      retained_bytes_observed_completely=case['retained_bytes_observed_completely'],
                      retained_sha256=case['retained_sha256'],started_utc=started,
                      module_sha256=case['modules']['apple-codecs.vdpl']['sha256'])
        receipt=evidence/f'{name}-{engine}.json'
        receipt.write_text(json.dumps(result,indent=2)+'\n')
        for suffix in ('.json','.dat','-expected.json'): names.append(f'{name}-{engine}{suffix}')
        for artifact in ('final.ppm','stats.json','esp32-uart0.log','frontend.log'):
            output=f'{name}-{engine}-{artifact}'
            shutil.copyfile(c/'.emulator/last-run'/artifact,evidence/output); names.append(output)
        receipts.append(dict(fixture=name,engine=engine,checks=result['checks'],
            bytes=result['bytes'],pixel_checks=result['pixel_checks'],
            elapsed_seconds=result['elapsed_seconds'],
            command_task_minimum_free_stack_bytes=result['command_task_minimum_free_stack_bytes'],
            retained_bytes_observed_completely=case['retained_bytes_observed_completely']))
        print(f'PASS {name} {engine}: {result["checks"]} replies, {result["pixel_checks"]} pixels.',flush=True)
stopped()
assert (main/'.runtime.sha256').read_bytes()==main_id, 'Main review identities changed'
for line in main_id.decode().splitlines():
    digest,name=line.split('  ',1)
    assert sha(main/name)==digest, 'Main review payload changed: '+name
# Leave a representative F3 image staged, with no old human result present.
profile.HERE=b/'cases/f3-step07'
profile.prepare(c/'.emulator',p/'.emulator/runtime/fab-1.2.5',adapter,
                b/'esp32-firmware.bin',b/'esp32-firmware.elf',frontend,
                b/'supplied',c/'support/setup_emulator.py')
summary=dict(passed=True,host=socket.gethostname(),architecture=platform.machine(),
    created_utc=datetime.now(timezone.utc).isoformat(),cases=receipts,
    total_runs=len(receipts),total_reply_checks=sum(r['checks'] for r in receipts),
    total_saved_bytes=sum(r['bytes'] for r in receipts),
    total_pixel_checks=sum(r['pixel_checks'] for r in receipts),
    completely_observed_retained_bytes=sum(r['retained_bytes_observed_completely'] for r in receipts),
    minimum_command_task_free_stack_bytes=min(r['command_task_minimum_free_stack_bytes'] for r in receipts),
    frontend_sha256=sha(frontend),adapter_sha256=sha(adapter),
    firmware_sha256=sha(b/'esp32-firmware.bin'),bundle_sha256=sys.argv[2],
    main_review_unchanged=True,graphical_instance_launched=False,emulator_stopped=True)
(evidence/'summary.json').write_text(json.dumps(summary,indent=2)+'\n'); names.append('summary.json')
index={name:sha(evidence/name) for name in names}
output=b/'oryx-evidence.tar.gz'
with tarfile.open(output,'w:gz') as target:
    for name in names: target.add(evidence/name,arcname=name,recursive=False)
    data=(json.dumps(dict(files=index),indent=2)+'\n').encode()
    info=tarfile.TarInfo('manifest.json');info.size=len(data);info.mode=0o644
    target.addfile(info,io.BytesIO(data))
print(json.dumps(dict(archive_sha256=sha(output),total_runs=len(receipts),stopped=True)),flush=True)
PY
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle-only',action='store_true')
    parser.add_argument('--timeout',type=float,default=180)
    args=parser.parse_args()
    if args.timeout<=0: parser.error('--timeout must be positive')
    archive,manifest=bundle()
    if args.bundle_only: return
    command([*SSH,'mkdir','-p',REMOTE+'/build'])
    command(['scp','-o','BatchMode=yes','-o','ConnectTimeout=8',str(archive),
             HOST+':'+REMOTE+'/build/codec-bundle.tar.gz.new'],timeout=180)
    output=command([*SSH,'bash','-s','--',manifest['archive_sha256'],str(args.timeout)],
                   script=REMOTE_SCRIPT,timeout=36*args.timeout+600)
    (BUILD/'oryx-run.log').write_text(output)
    remote=json.loads(output.splitlines()[-1])
    retrieved=BUILD/'oryx-evidence.tar.gz'
    command(['scp','-o','BatchMode=yes','-o','ConnectTimeout=8',
             HOST+':'+REMOTE+'/build/oryx-evidence.tar.gz',str(retrieved)],timeout=180)
    assert sha(retrieved.read_bytes())==remote['archive_sha256']
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True)
    with tarfile.open(retrieved) as source:
        assert all(m.isfile() and Path(m.name).name==m.name for m in source.getmembers())
        source.extractall(evidence,filter='data')
    index=json.loads((evidence/'manifest.json').read_text())
    for name,digest in index['files'].items():
        assert Path(name).name==name and sha((evidence/name).read_bytes())==digest
    summary=json.loads((evidence/'summary.json').read_text())
    print(json.dumps({k:v for k,v in summary.items() if k!='cases'},indent=2))


if __name__=='__main__':
    main()
