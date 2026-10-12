#!/usr/bin/env python3
"""Stock Fab/MOS test of uint16 divmod; automatic headless execution only."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import signal
import shutil
import struct
import subprocess
import sys
import time

from cases import cases, initial, expected

sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK/'build'
PROFILE = PROJECT/'.emulator'
APP = PROFILE/'sdcard/golem-015'
QUERY, FOREIGN = 60000, 50000
spec=importlib.util.spec_from_file_location('golem007_transport',TASK.parent/'GOLEM-007/qualify.py')
transport=importlib.util.module_from_spec(spec)
spec.loader.exec_module(transport)
word, packet, update, write, Tape = (getattr(transport,n) for n in ('word','packet','update','write','Tape'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate():
    tape=Tape()
    tape.send(write(QUERY,bytes([23,0,128,0])))
    canaries={0:b'root-canary',FOREIGN:b'external-canary'}
    for key,data in canaries.items():tape.send(write(key,data))
    packages=[]
    for name,options in [('default',[]),('relocated',['--reserve','1:254','--reserve','260:264','--reserve','50000:50000'])]:
        directory=BUILD/name
        subprocess.run([str(PROJECT/'build/golemc'),'--resident',*options,str(TASK/'checked-divmod.golem'),str(directory)],check=True)
        m=json.loads((directory/'interface.json').read_text())
        fields={g['name']:g['id'] for g in m['globals']}
        owned={v['id']:v for v in m['owned']}
        assert not (set(owned)&{*canaries,QUERY})
        bootstrap=(directory/'bootstrap.bin').read_bytes()
        invoke=(directory/'invoke.bin').read_bytes()
        cleanup=(directory/'cleanup.bin').read_bytes()
        # Extract every installed program from the host compiler's package, not a model.
        pos,blocks=7,{}
        while pos<len(bootstrap):
            assert bootstrap[pos:pos+3]==bytes([23,0,160])
            key=struct.unpack_from('<H',bootstrap,pos+3)[0];op=bootstrap[pos+5]
            if op==2:pos+=6
            elif op==0:
                length=struct.unpack_from('<H',bootstrap,pos+6)[0]
                blocks[key]=bootstrap[pos+8:pos+8+length];pos+=8+length
            elif op==5:
                # Initialized scalar input, or a constant-folded global initializer.
                assert bootstrap[pos+6]==0xc2
                length=struct.unpack_from('<H',bootstrap,pos+9)[0];pos+=11+length
            else:raise AssertionError((pos,op))
        tape.send(bootstrap)
        status=m['runtime_error']['id']
        def observe(state,result,error,label):
            for field,value in state.items():tape.read(fields[field],0,word(value),f'{name} {label} {field}')
            tape.read(m['output']['id'],0,struct.pack('<h',result),f'{name} {label} floor')
            tape.read(status,0,word(error),f'{name} {label} status')
        for index,values in enumerate(cases()):
            for field,value in initial(values).items():tape.send(update(fields[field],word(value)))
            tape.send(update(m['output']['id'],word(4660)))
            tape.send(invoke)
            observe(*expected(values),f'case-{index}')
        # First pair survives failure of the second pair; status resets per call.
        values=dict(numerator=17,denominator=5,nextDivisor=2)
        state=initial(values)
        for field,value in state.items():tape.send(update(fields[field],word(value)))
        state.update(before=17,quotient=3,remainder=2,secondQuotient=1,secondRemainder=1,after=2,completed=1)
        tape.send(invoke);observe(state,-1,0,'repeat-success')
        for index in range(2):
            tape.send(update(fields['nextDivisor'],word(0)));state['nextDivisor']=0
            tape.send(update(fields['completed'],word(0)));state['completed']=0
            tape.send(invoke);observe(state,-1,4,f'repeated-failure-{index}')
        tape.send(update(fields['nextDivisor'],word(2)));state.update(nextDivisor=2,completed=1)
        tape.send(invoke);observe(state,-1,0,'recovery')
        for key,item in owned.items():
            if item['kind']=='program':tape.read(key,0,blocks[key],f'{name} unchanged program')
        # Seed data only after arithmetic checks, so missing-buffer probes are unambiguous.
        for key,item in owned.items():
            if item['kind']=='data':tape.send(update(key,b'\xa6'))
            tape.read(key,0,b'\xa6' if item['kind']=='data' else blocks[key][:1],f'{name} present')
        tape.send(cleanup)
        for key in owned:tape.read(key,0,b'\x55',f'{name} absent after cleanup')
        tape.send(bootstrap)
        tape.read(status,0,word(0),f'{name} reinstall status')
        for field,value in {'quotient':48879,'denominator':1,'completed':0}.items():
            tape.read(fields[field],0,word(value),f'{name} reinstall {field}')
        tape.send(cleanup)
        for key,data in canaries.items():tape.read(key,0,data,f'{name} external preserved')
        packages.append({'name':name,'manifest':m,'numeric_jobs':len(cases())+4,
                         'artifacts_sha256':{p.name:sha(p) for p in sorted(directory.iterdir())}})
    tape.data.append(0)
    assert len(tape.expected)<=24000
    (BUILD/'tape.bin').write_bytes(tape.data)
    (BUILD/'expected.dat').write_bytes(tape.expected)
    meta={'packages':packages,'checks':tape.checks,'observation_bytes':len(tape.expected),
          'tape_bytes':len(tape.data),'tape_sent_vdu_bytes':tape.sent,'query_vdu_bytes_per_observed_byte':102}
    (BUILD/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n')
    subprocess.run([str(PROJECT/'docs/tasks/GOLEM-007/build/ez80asm'),'guest.asm','build/probe.bin','-l','-c'],cwd=TASK,check=True)
    return meta


def identity():
    paths=[PROFILE/'fab-agon-emulator',PROFILE/'fab-agon-emulator.bin',PROFILE/'firmware/vdp_platform.so',
           PROFILE/'mos_platform.bin',PROFILE/'mos_platform.map',PROFILE/'.mos-release.sha256',
           BUILD/'probe.bin',BUILD/'tape.bin',BUILD/'expected.dat',TASK/'guest.asm',TASK/'qualify.py',TASK/'cases.py',
           TASK/'checked-divmod.golem',PROJECT/'docs/tasks/GOLEM-007/qualify.py',
           PROJECT/'docs/tasks/GOLEM-007/build/ez80asm',PROJECT/'build/golemc',
           *sorted((PROJECT/'src').glob('*.hpp')),PROJECT/'src/golemc.cpp']
    return {str(p.relative_to(PROJECT)):sha(p) for p in paths}


def execute(meta):
    assert (PROFILE/'fab-agon-emulator').is_file() and not (PROFILE/'fab-agon-emulator').is_symlink()
    assert 'Generated by agon-dev-env setup_emulator.py' in (PROFILE/'fab-agon-emulator').read_text()
    runtime=(PROFILE/'fab-agon-emulator.bin').resolve()
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            if (proc/'exe').resolve()==runtime:raise RuntimeError('Close existing shared-runtime emulator before qualification')
        except (PermissionError,FileNotFoundError):pass
    APP.mkdir(exist_ok=True)
    shutil.copyfile(BUILD/'probe.bin',APP/'probe.bin')
    result=APP/'results.dat';result.unlink(missing_ok=True)
    startup=PROFILE/'sdcard/autoexec.txt'
    previous=startup.read_bytes() if startup.exists() else None
    before=identity()
    env={k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD','BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    command=['./fab-agon-emulator','--renderer','sw']
    started=time.monotonic();mapped=set();actual=None;process=None
    try:
        startup.write_bytes(b'SET KEYBOARD 1\r\ncd /golem-015\r\nload probe.bin\r\nrun .\r\n')
        with (BUILD/'emulator.log').open('w') as log:
            process=subprocess.Popen(command,cwd=PROFILE,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while not result.exists() or result.stat().st_size<meta['observation_bytes']:
                if process.poll() is not None:raise RuntimeError('Emulator exited before complete results')
                maps=Path(f'/proc/{process.pid}/maps').read_text()
                mapped.update(Path(line.split(maxsplit=5)[5]).resolve() for line in maps.splitlines() if '/vdp_' in line and '.so' in line)
                if mapped:
                    assert mapped=={(PROFILE/'firmware/vdp_platform.so').resolve()},mapped
                    actual=Path(f'/proc/{process.pid}/exe').resolve();assert actual==runtime
                if time.monotonic()-started>180:raise TimeoutError('Guest progress; inspect build/emulator.log')
                time.sleep(0.05)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
        if previous is None:startup.unlink(missing_ok=True)
        else:startup.write_bytes(previous)
    assert mapped and actual,'Native process identity was not observed'
    assert identity()==before,'Sources/runtime changed during qualification'
    observed=result.read_bytes();expected_=(BUILD/'expected.dat').read_bytes()
    failures=[]
    for check in meta['checks']:
        start,size=check['start'],check['bytes']
        if observed[start:start+size]!=expected_[start:start+size]:
            failures.append({**check,'observed':observed[start:start+size].hex(),'expected':expected_[start:start+size].hex()})
    report={'pass':not failures and len(observed)==len(expected_),'checks':len(meta['checks']),
            'resident_jobs':sum(p['numeric_jobs'] for p in meta['packages']),'observed_bytes':len(observed),
            'execution_seconds':round(time.monotonic()-started,3),'command':command,'architecture':os.uname().machine,
            'executed_fab':str(actual),'loaded_vdp':str(next(iter(mapped))),'runtime_identity':before,
            'fab_tag':'1.2.5','mos_release':'v3.0.2 Arthur','fixture':meta,'failures':failures,
            'observed_sha256':hashlib.sha256(observed).hexdigest(),'expected_sha256':hashlib.sha256(expected_).hexdigest(),
            'scope':'Stock VDP through MOS/eZ80 UART; headless. GP observations establish VDP parser progress after job exit.',
            'human_validation':'pending; Oryx human review not staged by this runner','hardware_qualified':False}
    (TASK/'observed.dat').write_bytes(observed);(TASK/'expected.dat').write_bytes(expected_)
    (TASK/'emulator-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    if not report['pass']:raise AssertionError(f'{len(failures)} failed checks; see emulator-checks.json')
    print(f'Stock emulator passes: {report["checks"]} checks, {report["resident_jobs"]} jobs, {len(observed)} bytes, {report["execution_seconds"]}s')


def main():
    started=time.monotonic()
    BUILD.mkdir(exist_ok=True)
    subprocess.run(['make','-C',str(PROJECT/'src')],check=True)
    meta=generate();print(f'Prepared fixture in {time.monotonic()-started:.3f}s',flush=True)
    execute(meta)


if __name__=='__main__':main()
