"""Build and check the exact human animation; no extra drawing scenes."""
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
from pathlib import Path
sys.dont_write_bytecode = True
TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK / 'build'
PROFILE = PROJECT / '.emulator'
ASSETS = TASK.parent / 'GOLEM-019'
APP = PROFILE / 'sdcard/golem-020'
spec = importlib.util.spec_from_file_location('wolf_qualification', ASSETS / 'qualify.py')
q = importlib.util.module_from_spec(spec)
spec.loader.exec_module(q)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def model(positions, velocities):
    maxima = [288,160,256,128,280,172]
    for i, limit in enumerate(maxima):
        positions[i] += velocities[i]
        if positions[i] < 0:
            positions[i] = -positions[i]; velocities[i] = -velocities[i]
        elif positions[i] > limit:
            positions[i] = 2*limit-positions[i]; velocities[i] = -velocities[i]

def picture(positions):
    screen = bytearray(320*240)
    assets = json.loads((ASSETS / 'assets.json').read_text())['assets']
    by_id = {a['buffer_id']:a for a in assets}
    for ident,x,y in [(8202,0,0),(8195,*positions[:2]),(8463,*positions[2:4]),(8201,*positions[4:])]:
        asset = by_id[ident]
        for i,p in enumerate((ASSETS / 'assets' / (asset['name']+'.rgba2')).read_bytes()):
            if p >> 6:
                screen[(y+i//asset['width'])*320+x+i%asset['width']] = ((p&3)<<4)|(p&12)|((p>>4)&3)
    return screen

def generate():
    BUILD.mkdir(exist_ok=True)
    subprocess.run([str(PROJECT/'build/golemc'),'--resident','--emit-asm',
        '--reserve','8195:8195','--reserve','8201:8202','--reserve','8463:8463',
        str(TASK/'animation.golem'),str(BUILD)],check=True)
    tape=q.Tape()
    tape.send(q.display_setup(136))
    tape.data.append(5)
    tape.send((BUILD/'bootstrap.bin').read_bytes())
    # Paint role labels into both buffers once; Golem clears only y=0..191.
    labels=bytes([17,63,28,0,29,39,24,31,0,0])+b'GOLEM: animation.golem draws each frame.\r\nASM: motion + vblank; assets loaded once.\r\nTen-second animation; then MOS returns.'
    for _ in range(2): tape.send(labels+bytes([23,0,195]))
    pos=[8,10,200,100,96,170]; vel=[3,2,-2,1,4,-3]
    checkpoints=[1,90,180,270,360,450,600]
    previous=0
    for frame in checkpoints:
        tape.data += bytes([6])+struct.pack('<H',frame-previous)
        for _ in range(frame-previous): model(pos,vel)
        state=struct.pack('<6h',*pos)+frame.to_bytes(3,'little')+bytes([136])
        start=len(tape.expected); tape.expected+=state
        tape.checks.append(dict(label=f'frame {frame} positions/count/mode',start=start,bytes=len(state)))
        screen=picture(pos)
        points={(x,y) for y in range(0,192,16) for x in range(0,320,16)}
        for x,y,w,h in [(pos[0],pos[1],32,32),(pos[2],pos[3],64,64),(pos[4],pos[5],40,20)]:
            points.update((x+px,y+py) for py in range(0,h,4) for px in range(0,w,4))
        if frame == 600:
            for x,y in sorted(points): tape.pixel(x,y,f'frame {frame} Golem pixel',screen[y*320+x])
        previous=frame
    # Return in ordinary single-buffered mode: typing dir must remain visible.
    tape.send(q.display_setup(8))
    tape.data.append(3)
    screen=picture(pos)
    for y in range(0,192,16):
        for x in range(0,320,16): tape.pixel(x,y,'final single-buffered frame',screen[y*320+x])
    tape.send((BUILD/'cleanup.bin').read_bytes())
    tape.data.append(0)
    assert len(tape.expected)<8192
    (BUILD/'tape.bin').write_bytes(tape.data)
    (TASK/'expected.dat').write_bytes(tape.expected)
    subprocess.run([str(PROJECT/'docs/tasks/GOLEM-007/build/ez80asm'),'animation.asm','build/probe.bin','-l','-c'],cwd=TASK,check=True)
    meta=dict(checks=len(tape.checks),observation_bytes=len(tape.expected),fixture=dict(checks=tape.checks),
        frames=600,checkpoints=checkpoints,expected_sha256=sha(TASK/'expected.dat'),
        frame_wire_bytes=32,position_payload_bytes=12,mode=136,final_mode=8,
        golem_source='animation.golem',assembly_source='animation.asm',direct_bitmap_draws=0,
        synchronization='stock VSYNC screen swap; UART flow control, no second host-side wait',
        target_seconds=10,human_validation='pending',graphical_emulator_launched=False)
    (TASK/'checks.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta

def execute(meta):
    runtime=(PROFILE/'fab-agon-emulator.bin').resolve()
    for p in Path('/proc').glob('[0-9]*'):
        try:
            if (p/'exe').resolve()==runtime: raise RuntimeError('Close emulator before qualification')
        except (FileNotFoundError,PermissionError): pass
    APP.mkdir(exist_ok=True)
    shutil.copyfile(BUILD/'probe.bin',APP/'probe.bin')
    shutil.copyfile(ASSETS/'assets/wolf-review.agnb',APP/'wolf-review.agnb')
    result=APP/'results.dat';result.unlink(missing_ok=True)
    startup=PROFILE/'sdcard/autoexec.txt'
    previous=startup.read_bytes() if startup.exists() else None
    paths=['.emulator/fab-agon-emulator','.emulator/fab-agon-emulator.bin','.emulator/firmware/vdp_platform.so','.emulator/mos_platform.bin','.emulator/mos_platform.map',
        'docs/tasks/GOLEM-020/animation.golem','docs/tasks/GOLEM-020/animation.asm','docs/tasks/GOLEM-020/prepare.py','docs/tasks/GOLEM-020/build/probe.bin','docs/tasks/GOLEM-020/build/tape.bin']
    hashes={p:sha(PROJECT/p) for p in paths}
    accepted=json.loads((ASSETS/'demo-checks.json').read_text())['runtime_identity']
    for p in paths:
        if p.startswith('.emulator/'): assert hashes[p]==accepted[p]
    env={k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD','BASH_ENV')}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    started=time.monotonic();process=None;mapped=set()
    try:
        startup.write_bytes(b'SET KEYBOARD 1\r\ncd /golem-020\r\nload probe.bin\r\nrun .\r\n')
        with (BUILD/'emulator.log').open('w') as log:
            process=subprocess.Popen(['./fab-agon-emulator','--renderer','sw'],cwd=PROFILE,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            while not result.exists() or result.stat().st_size<meta['observation_bytes']:
                if process.poll() is not None: raise RuntimeError('Emulator exited: inspect build/emulator.log')
                maps=Path(f'/proc/{process.pid}/maps').read_text()
                mapped.update(Path(line.split(maxsplit=5)[5]).resolve() for line in maps.splitlines() if '/vdp_' in line and '.so' in line)
                if mapped: assert mapped=={(PROFILE/'firmware/vdp_platform.so').resolve()}
                if time.monotonic()-started>60: raise TimeoutError('Inspect build/emulator.log')
                time.sleep(.05)
    finally:
        if process and process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
        if previous is None:startup.unlink(missing_ok=True)
        else:startup.write_bytes(previous)
    assert mapped
    assert all(sha(PROJECT/p)==h for p,h in hashes.items())
    actual=result.read_bytes();expected=(TASK/'expected.dat').read_bytes()
    (TASK/'observed.dat').write_bytes(actual)
    if actual!=expected:
        bad=[c['label'] for c in meta['fixture']['checks'] if actual[c['start']:c['start']+c['bytes']]!=expected[c['start']:c['start']+c['bytes']]]
        raise AssertionError(f'{len(bad)} mismatches: {bad[:10]}')
    meta.update(pass_headless=True,execution_seconds=round(time.monotonic()-started,3),runtime_identity=hashes,loaded_vdp=str(next(iter(mapped))))
    (TASK/'checks.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(f'Animation passes: {meta["frames"]} frames, {meta["checks"]} checks, {meta["execution_seconds"]}s.')
if __name__=='__main__': execute(generate())
