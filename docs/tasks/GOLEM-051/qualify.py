#!/usr/bin/env python3
"""Own Pi headless profile for the actual MOS-native compiler and examples."""
from pathlib import Path
import importlib.util
import hashlib
import json
import os
import shutil
import signal
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PROFILE=ROOT/'.emulator/golem-051'
BUILD=HERE/'build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def stage():
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            if str(PROFILE/'sdcard').encode() in (proc/'cmdline').read_bytes().split(b'\0'):
                raise RuntimeError('Owned GOLEM-051 profile is running; stop it before staging')
        except OSError:pass
    spec=importlib.util.spec_from_file_location('setup',ROOT.parent/'agon-dev-env/scripts/setup_emulator.py')
    setup=importlib.util.module_from_spec(spec);spec.loader.exec_module(setup)
    # The Pi has a qualified ARM64 Fab runtime. No complete Pi Flexe frontend
    # was found in the maintained build, so use the Author-approved fallback.
    mos=PROFILE/'sdcard/mos';owned=PROFILE/'.mos-local'
    preserve=mos.is_dir() and not mos.is_symlink()
    if preserve:mos.rename(owned)
    try:sd=setup.link_shared_runtime(PROFILE,ROOT/'.emulator/runtime/fab-1.2.5')
    finally:
        if preserve:
            if mos.is_symlink():mos.unlink()
            owned.rename(mos)
    app=sd/'golem';app.mkdir(exist_ok=True)
    for d in ('compiler','cases','out','cache'):(app/d).mkdir(exist_ok=True)
    inputs=['fab-agon-emulator','fab-agon-emulator.bin','mos_platform.bin','mos_platform.map','firmware/vdp_platform.so']
    (PROFILE/'.runtime.sha256').write_text(''.join(sha(PROFILE/p)+'  '+p+'\n' for p in inputs))
    (BUILD/'profile.json').write_text(json.dumps({'profile':str(PROFILE),'runtime':{p:sha(PROFILE/p) for p in inputs},'human':'pending','physical':False},indent=2)+'\n')
    shim=HERE/'sdl_io.c'
    if not shim.exists():
        shim.write_text((ROOT/'docs/tasks/GOLEM-044/app/sdl_input.c').read_text().replace('FLOWER_EVENTS','GOLEM051_EVENTS').replace('FLOWER_ADAPTER','GOLEM051_VDP'))
    subprocess.run(['cc','-shared','-fPIC','-I'+str(Path.home()/'.local/include'),str(shim),'-ldl','-o',str(BUILD/'sdl-io.so')],check=True)
    return app

class Session:
    def __init__(self,autoexec,label):
        self.label=label;self.app=stage();self.started=time.monotonic()
        self.art=BUILD/'runs'/label;self.art.mkdir(parents=True,exist_ok=True)
        (PROFILE/'sdcard/autoexec.txt').write_bytes(autoexec.replace('\r','').replace('\n','\r\n').encode())
        self.events=self.art/'events.txt';self.events.write_text('')
        self.log=(self.art/'frontend.log').open('w')
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',LD_PRELOAD=str(BUILD/'sdl-io.so'),
                 GOLEM051_EVENTS=str(self.events),GOLEM051_VDP=str((PROFILE/'firmware/vdp_platform.so').resolve()))
        env.pop('BASH_ENV',None)
        self.process=subprocess.Popen(['./fab-agon-emulator','--renderer','sw'],cwd=PROFILE,env=env,stdout=self.log,stderr=subprocess.STDOUT,start_new_session=True)
    def emit(self,line):
        with self.events.open('a') as f:f.write(line+'\n')
    def type(self,text):
        for char in text:
            if 'a'<=char<='z':scan=4+ord(char)-ord('a')
            elif '1'<=char<='9':scan=30+ord(char)-ord('1')
            else:scan={'0':39,'.':55,'-':45,' ':44,'/':56,'\n':40}[char]
            self.emit(f'key {scan} 1');time.sleep(.035);self.emit(f'key {scan} 0');time.sleep(.035)
    def capture(self,name='screen'):
        dest=self.art/(name+'.ppm');dest.unlink(missing_ok=True);self.emit('capture '+str(dest))
        deadline=time.monotonic()+10
        while True:
            complete=False
            if dest.exists():
                with dest.open('rb') as f:
                    magic=f.readline();size=f.readline();scale=f.readline();header=f.tell()
                if magic==b'P6\n' and scale==b'255\n':
                    width,height=map(int,size.split());complete=dest.stat().st_size==header+width*height*3
            if complete:break
            if time.monotonic()>deadline:raise TimeoutError('framebuffer capture')
            time.sleep(.05)
        return dest
    def wait(self,path,seconds=180):
        deadline=time.monotonic()+seconds
        while not path.exists():
            if self.process.poll() is not None:raise RuntimeError('Emulator exited')
            if time.monotonic()>deadline:
                self.capture('timeout');raise TimeoutError(str(path))
            time.sleep(.05)
        return path
    def close(self):
        self.emit('quit')
        try:self.process.wait(timeout=15)
        except subprocess.TimeoutExpired:
            os.killpg(self.process.pid,signal.SIGTERM);self.process.wait(timeout=5)
        self.log.close()
        return self.process.returncode

def smoke():
    app=stage()
    shutil.copyfile(BUILD/'port/ez80-portable/golemc.bin',app/'compiler/portable.bin')
    (app/'cases/smoke.golem').write_text('uint16 x=2 uint16 y=0 Sub step y=x+3 EndSub\n')
    (app/'smoke.args').write_text('--portable cases/smoke.golem out/smoke\n')
    (app/'smoke.list').write_text('smoke.args\n')
    for name in ('done.dat','batch.log','batch-errors.log'):(app/name).unlink(missing_ok=True)
    s=Session('SET KEYBOARD 1\nVDU 22 0\ncd /golem\nload compiler/portable.bin\nrun . --batch smoke.list\nsave done.dat 40000 3\ndir\n','smoke')
    try:
        s.wait(app/'batch.log');s.wait(app/'done.dat');time.sleep(.5);image=s.capture()
        assert (app/'batch.log').read_text().split()[1]=='0'
        report={'passed':True,'seconds':time.monotonic()-s.started,'actual_eZ80_compile':True,
                'normal_MOS_continuation':True,'picture':str(image),'binary_sha256':sha(app/'compiler/portable.bin'),
                'outputs':{p.name:sha(p) for p in (app/'out/smoke').iterdir()},'human':'pending'}
    finally:rc=s.close()
    report['frontend_exit']=rc
    (HERE/'smoke.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':smoke()
