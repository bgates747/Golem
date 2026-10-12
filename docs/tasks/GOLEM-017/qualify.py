#!/usr/bin/env python3
"""Stock Fab/MOS test of typed table selection; automatic headless execution only."""
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


sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK/'build'
PROFILE = PROJECT/'.emulator'
APP = PROFILE/'sdcard/golem-017'
QUERY, FOREIGN = 60000, 50000
spec=importlib.util.spec_from_file_location('golem007_transport',TASK.parent/'GOLEM-007/qualify.py')
transport=importlib.util.module_from_spec(spec)
spec.loader.exec_module(transport)
word, packet, update, write, Tape = (getattr(transport,n) for n in ('word','packet','update','write','Tape'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def configurations():
    return [('default',4,[]),
            ('relocated',4,['--reserve','1:254','--reserve','260:264']),
            ('wide',258,['--reserve','1:254','--reserve','260:264']),
            ('near-end',4,['--reserve','1:65510']),
            ('singleton',1,[])]


def rows(count):
    base=[(1,-2),(2,3),(3,-4),(4,5)]
    return b''.join(struct.pack('<Hh',*(base[i] if i<4 else ((i*251)&65535,((i*127)&65535)-32768))) for i in range(count))


def prepare_package(name,count,options):
    directory=BUILD/name
    source=BUILD/(name+'.golem')
    text=(TASK/'table.golem').read_text().replace('samples(0:3)',f'samples(0:{count-1})')
    source.write_text(text)
    data=BUILD/(name+'.dat');data.write_bytes(rows(count))
    command=[str(PROJECT/'build/golemc'),'--resident','--emit-asm','--table-data',f'samples={data}']
    if name!='near-end':command+=['--reserve','50000:50000','--reserve','60000:60000']
    subprocess.run([*command,*options,str(source),str(directory)],check=True)
    manifest=json.loads((directory/'interface.json').read_text())
    return directory,manifest


def generate():
    tape=Tape()
    tape.send(write(QUERY,bytes([23,0,128,0])))
    canaries={0:b'root-canary',FOREIGN:b'external-canary',254:b'gap-canary',260:b'reserve-canary'}
    packages=[]
    for name,count,options in configurations():
        directory,m=prepare_package(name,count,options)
        owned={v['id']:v for v in m['owned']}
        selected={k:v for k,v in canaries.items() if k not in owned}
        for key,data in selected.items():tape.send(write(key,data))
        assert not (set(owned)&{0,FOREIGN,QUERY})
        bootstrap=(directory/'bootstrap.bin').read_bytes()
        invoke=(directory/'invoke.bin').read_bytes()
        cleanup=(directory/'cleanup.bin').read_bytes()
        pos,blocks=7,{}
        while pos<len(bootstrap):
            assert bootstrap[pos:pos+3]==bytes([23,0,160])
            key=struct.unpack_from('<H',bootstrap,pos+3)[0];op=bootstrap[pos+5]
            if op==2:pos+=6
            else:
                assert op==0
                length=struct.unpack_from('<H',bootstrap,pos+6)[0]
                blocks[key]=bootstrap[pos+8:pos+8+length];pos+=8+length
        tape.send(bootstrap)
        status=m['runtime_error']['id']
        copier=next(v['id'] for v in m['owned'] if v['role']=='table.copy.template')
        def call():
            if name=='default':tape.data.append(3);tape.sent+=6
            else:tape.send(invoke)
        def observe(record,error,label):
            tape.read(m['output']['id'],0,record,f'{name} {label} record')
            tape.read(status,0,word(error),f'{name} {label} status')
        sentinel=struct.pack('<Hh',65535,-32768)
        indices=list(dict.fromkeys([0,count-1,1,2,3,count,255,256,257,258,511,512,32767,32768,65280,65534,65535]))
        numericJobs=0
        for index in indices:
            tape.send(update(m['input']['id'],word(index)))
            tape.send(update(m['output']['id'],sentinel))
            call();numericJobs+=1
            valid=index<count
            observe(rows(count)[index*4:index*4+4] if valid else sentinel,0 if valid else 5,f'index-{index}')
        # Observe persistent destination and status after repeated errors and recovery.
        record=rows(count)[:4]
        for index in [0,65535,65535,count-1]:
            tape.send(update(m['input']['id'],word(index)));call();numericJobs+=1
            if index<count:record=rows(count)[index*4:index*4+4]
            observe(record,0 if index<count else 5,f'persistent-{numericJobs}')
        # Trusted table contents are never modified by selection or index failure.
        for row in range(count):tape.read(m['table']['base_id']+row,0,rows(count)[row*4:row*4+4],f'{name} immutable row-{row}')
        for key,item in owned.items():
            if item['kind']=='program' and key!=copier:tape.read(key,0,blocks[key],f'{name} unchanged program')
        # The copy template is deliberately patched, and only its source ID changes.
        patched=bytearray(blocks[copier]);patched[11:13]=word(m['table']['base_id']+count-1)
        tape.read(copier,0,patched,f'{name} patched template')
        tape.send(cleanup)
        for key in owned:tape.read(key,0,b'\x55',f'{name} absent after cleanup')
        tape.send(bootstrap)
        tape.read(m['output']['id'],0,sentinel,f'{name} reinstall destination')
        tape.read(status,0,word(0),f'{name} reinstall status')
        tape.read(copier,0,blocks[copier],f'{name} reinstall template')
        tape.send(cleanup)
        for key,data in selected.items():tape.read(key,0,data,f'{name} external preserved')
        packages.append({'name':name,'manifest':m,'numeric_jobs':numericJobs,
                         'calls_generated_wrapper':name=='default',
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
           BUILD/'probe.bin',BUILD/'tape.bin',BUILD/'expected.dat',TASK/'guest.asm',TASK/'qualify.py',TASK/'records.dat',
           TASK/'table.golem',PROJECT/'docs/tasks/GOLEM-007/qualify.py',
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
        startup.write_bytes(b'SET KEYBOARD 1\r\ncd /golem-017\r\nload probe.bin\r\nrun .\r\n')
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
