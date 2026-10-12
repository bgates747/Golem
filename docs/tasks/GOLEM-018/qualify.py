#!/usr/bin/env python3
"""Exercise actual stock MOS/VDP geometry and compare direct/resident pixels."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import shutil
import signal
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK/'build'
PROFILE = PROJECT/'.emulator'
APP = PROFILE/'sdcard/golem-018'
spec = importlib.util.spec_from_file_location('golem007_transport', TASK.parent/'GOLEM-007/qualify.py')
transport = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transport)
word, packet, write = (getattr(transport, n) for n in ('word','packet','write'))
QUERY, FOREIGN = 60000, 50000


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def update(key, data, offset=0):
    return packet(key,5)+b'\xc2'+word(offset)+word(len(data))+data


def plot(code,x,y):
    return bytes([25,code])+struct.pack('<hh',x,y)


def direct(values):
    """Independent stock sequence: spell the two quad triangles explicitly."""
    colour,px,py,lx1,ly1,lx2,ly2,ax,ay,bx,by,cx,cy,dx,dy = values
    data = bytes([18,0,colour])+plot(69,px,py)
    for x,y in [(27,50),(30,50),(24,53),(27,53),(30,53),(24,56),(27,56),(30,56)]:
        data += plot(69,x,y)
    data += plot(4,lx1,ly1)+plot(5,lx2,ly2)
    data += plot(4,ax,ay)+plot(4,bx,by)+plot(85,dx,dy)
    data += plot(4,bx,by)+plot(4,dx,dy)+plot(85,cx,cy)
    return data


class Tape(transport.Tape):
    def pixel(self,x,y,label,expected=None,reference=None):
        self.data += b'\x04'+word(x)+word(y)
        start = len(self.expected)
        self.expected.append(0 if expected is None else expected)
        self.checks.append(dict(label=label,start=start,bytes=1,pixel=[x,y],expected=expected,reference=reference))
        return start


def prepare_package(name,options):
    directory = BUILD/name
    command = [str(PROJECT/'build/golemc'),'--resident','--emit-asm','--reserve','50000:50000',
               '--reserve','60000:60000',*options,str(TASK/'geometry.golem'),str(directory)]
    subprocess.run(command,check=True)
    return directory,json.loads((directory/'interface.json').read_text())


def installed(directory):
    raw = (directory/'bootstrap.bin').read_bytes()
    pos,blocks = 0,{}
    while pos<len(raw):
        assert raw[pos:pos+3]==bytes([23,0,160])
        key = struct.unpack_from('<H',raw,pos+3)[0]
        if raw[pos+5]==2:
            pos += 6
        else:
            assert raw[pos+5]==0
            size = struct.unpack_from('<H',raw,pos+6)[0]
            blocks[key] = raw[pos+8:pos+8+size]
            pos += 8+size
    return blocks


def generate():
    BUILD.mkdir(exist_ok=True)
    tape = Tape()
    # 64 unique palette entries, pixel coordinates and default origin/viewport.
    setup = bytes([22,8,23,0,192,0,29,0,0,0,0,26,23,1,0])+plot(4,0,0)
    tape.send(setup)
    tape.send(write(QUERY,bytes([23,0,128,0])))
    initial = [11,24,50,60,50,130,100,180,60,260,60,280,120,160,120]
    packages = []
    for name,options in [('default',[]),('relocated',['--reserve','1:254','--reserve','260:264'])]:
        directory,m = prepare_package(name,options)
        blocks = installed(directory)
        owned = {v['id']:v for v in m['owned']}
        canaries = {0:b'root',FOREIGN:b'borrowed',260:b'reserved'}
        canaries = {k:v for k,v in canaries.items() if k not in owned}
        for k,v in canaries.items(): tape.send(write(k,v))
        tape.send((directory/'bootstrap.bin').read_bytes())
        def call():
            if name=='default':
                tape.data.append(3); tape.sent+=6
            else: tape.send((directory/'invoke.bin').read_bytes())
        grids = [(x,y) for y in range(44,61) for x in range(18,35)]
        grids += [(x,y) for y in range(46,105,4) for x in range(56,149,4)]
        grids += [(x,y) for y in range(56,125,4) for x in range(156,285,4)]
        grids += [(27,53),(220,90),(180,100),(150,90),(24,50),(33,50),(60,50),(146,100)]
        clipped = [6,-1,2,-10,30,24,46,-8,4,16,4,20,20,-12,20]
        cases = [('initial',initial),('partial',initial.copy()),('clipped',clipped)]
        cases[1][1][0]=6; cases[1][1][1]=33; cases[1][1][5]=146
        for label,values in cases:
            raw = struct.pack('<B14h',*values)
            if label=='partial':
                # Real partial updates preserve all other inputs without reinstall.
                for field,value in [('colour',6),('pointX',33),('lineX2',146)]:
                    g = next(g for g in m['globals'] if g['name']==field)
                    tape.send(update(g['id'],struct.pack('<B' if g['bytes']==1 else '<h',value),g['offset']))
            elif label=='clipped': tape.send(update(m['input']['id'],raw))
            tape.send(b'\x10') # CLG before each comparison; no old drawing residue.
            call()
            if label=='initial': call() # unchanged repeat, no input upload.
            tape.read(m['input']['id'],0,raw,f'{name} {label} persistent packed inputs')
            points = grids if label!='clipped' else [(x,y) for y in range(0,49,2) for x in range(0,27,2)]
            known = {(27,53):values[0],(220,90):values[0],(180,100):values[0],(150,90):0}
            refs = []
            for x,y in points:
                expected = known.get((x,y)) if label!='clipped' else (values[0] if (x,y)==(0,10) else None)
                refs.append(tape.pixel(x,y,f'{name} {label} resident pixel',expected))
            tape.send(b'\x10'+direct(values))
            for (x,y),reference in zip(points,refs):
                tape.pixel(x,y,f'{name} {label} direct pixel',reference=reference)
        tape.read(m['entry']['id'],0,blocks[m['entry']['id']],f'{name} unchanged entry')
        tape.send((directory/'cleanup.bin').read_bytes())
        for key in owned: tape.read(key,0,b'\x55',f'{name} cleanup absence')
        tape.send((directory/'bootstrap.bin').read_bytes())
        tape.read(m['input']['id'],0,struct.pack('<B14h',*initial),f'{name} reinstall initial values')
        tape.send((directory/'cleanup.bin').read_bytes())
        for key,data in canaries.items(): tape.read(key,0,data,f'{name} external preserved')
        packages.append(dict(name=name,manifest=m,jobs=4,calls_generated_wrapper=name=='default',
                             artifacts_sha256={p.name:sha(p) for p in directory.iterdir()}))
    # Leave the human-facing display clean and useful after automated probes.
    default = BUILD/'default'
    tape.send(b'\x10'+(default/'bootstrap.bin').read_bytes())
    tape.data.append(3);tape.sent+=6
    tape.pixel(220,90,'final demo trapezoid',11)
    tape.send((default/'cleanup.bin').read_bytes())
    tape.data.append(0)
    assert len(tape.expected)<=24000
    (BUILD/'tape.bin').write_bytes(tape.data)
    (BUILD/'fixed-expected.dat').write_bytes(tape.expected)
    meta = dict(packages=packages,checks=tape.checks,observation_bytes=len(tape.expected),
                tape_bytes=len(tape.data),tape_sent_vdu_bytes=tape.sent,
                pixel_pairs=sum(c.get('reference') is not None for c in tape.checks))
    (BUILD/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n')
    subprocess.run([str(PROJECT/'docs/tasks/GOLEM-007/build/ez80asm'),'guest.asm','build/probe.bin','-l','-c'],cwd=TASK,check=True)
    return meta


def identity():
    paths = [PROFILE/'fab-agon-emulator',PROFILE/'fab-agon-emulator.bin',PROFILE/'firmware/vdp_platform.so',
             PROFILE/'mos_platform.bin',PROFILE/'mos_platform.map',BUILD/'probe.bin',BUILD/'tape.bin',
             TASK/'guest.asm',TASK/'geometry.golem',TASK/'qualify.py',PROJECT/'build/golemc',
             *sorted((PROJECT/'src').glob('*.hpp')),PROJECT/'src/golemc.cpp']
    return {str(p.relative_to(PROJECT)):sha(p) for p in paths}


def validate(meta,actual):
    assert len(actual)==meta['observation_bytes'],(len(actual),meta['observation_bytes'])
    expected = (BUILD/'fixed-expected.dat').read_bytes()
    failures = []
    for c in meta['checks']:
        start,size = c['start'],c['bytes']
        got = actual[start:start+size]
        if 'pixel' not in c: want = expected[start:start+size]
        elif c.get('reference') is not None: want = actual[c['reference']:c['reference']+1]
        elif c['expected'] is not None: want = bytes([c['expected']])
        else: continue
        if got!=want: failures.append(dict(**c,observed=got.hex(),wanted=want.hex()))
    return failures


def execute(meta):
    assert not (PROFILE/'fab-agon-emulator').is_symlink()
    assert 'Generated by agon-dev-env setup_emulator.py' in (PROFILE/'fab-agon-emulator').read_text()
    runtime = (PROFILE/'fab-agon-emulator.bin').resolve()
    for p in Path('/proc').glob('[0-9]*'):
        try:
            if (p/'exe').resolve()==runtime: raise RuntimeError('Close the emulator before qualification')
        except (FileNotFoundError,PermissionError): pass
    APP.mkdir(exist_ok=True)
    shutil.copyfile(BUILD/'probe.bin',APP/'probe.bin')
    result = APP/'results.dat'; result.unlink(missing_ok=True)
    startup = PROFILE/'sdcard/autoexec.txt'
    previous = startup.read_bytes() if startup.exists() else None
    before = identity()
    env = {k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD','BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    started = time.monotonic(); process = None; mapped = set(); actual_exe = None
    try:
        startup.write_bytes(b'SET KEYBOARD 1\r\ncd /golem-018\r\nload probe.bin\r\nrun .\r\n')
        with (BUILD/'emulator.log').open('w') as log:
            process = subprocess.Popen(['./fab-agon-emulator','--renderer','sw'],cwd=PROFILE,env=env,
                                       stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while not result.exists() or result.stat().st_size<meta['observation_bytes']:
                if process.poll() is not None: raise RuntimeError('Emulator exited before results')
                maps = Path(f'/proc/{process.pid}/maps').read_text()
                mapped.update(Path(line.split(maxsplit=5)[5]).resolve() for line in maps.splitlines()
                              if '/vdp_' in line and '.so' in line)
                if mapped:
                    assert mapped=={(PROFILE/'firmware/vdp_platform.so').resolve()}
                    actual_exe = Path(f'/proc/{process.pid}/exe').resolve(); assert actual_exe==runtime
                if time.monotonic()-started>180: raise TimeoutError('Inspect build/emulator.log')
                time.sleep(.05)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: os.killpg(process.pid,signal.SIGKILL);process.wait()
        if previous is None: startup.unlink(missing_ok=True)
        else: startup.write_bytes(previous)
    assert mapped and actual_exe
    assert identity()==before
    actual = result.read_bytes()
    failures = validate(meta,actual)
    report = dict(pass_headless=not failures,checks=len(meta['checks']),pixel_pairs=meta['pixel_pairs'],
                  observed_bytes=len(actual),execution_seconds=round(time.monotonic()-started,3),
                  fab_tag='1.2.5',mos_release='v3.0.2 Arthur',architecture=os.uname().machine,
                  runtime_identity=before,loaded_vdp=str(next(iter(mapped))),executed_fab=str(actual_exe),
                  fixture=meta,failures=failures,scope='Stock MOS/VDP; direct/resident pixel comparisons and buffer observations.',
                  human_validation='pending',hardware_qualified=False)
    report['pass'] = not failures
    (TASK/'observed.dat').write_bytes(actual)
    if not failures:
        (TASK/'expected.dat').write_bytes(actual)
        report['expected_sha256'] = sha(TASK/'expected.dat')
    (TASK/'emulator-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    if failures: raise AssertionError(f'{len(failures)} failed checks: {failures[:3]}')
    print(f'Stock pixels pass: {meta["pixel_pairs"]} pairs, {len(meta["checks"])} checks, {report["execution_seconds"]}s')


def main():
    subprocess.run(['make','-C',str(PROJECT/'src')],check=True)
    meta = generate()
    execute(meta)


if __name__=='__main__': main()
