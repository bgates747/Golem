#!/usr/bin/env python3
"""Exercise the real eZ80 application through headless SDL/PS2 input on Oryx."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import signal
import shutil
import subprocess
import time
import profile as staged

HERE=Path(__file__).resolve().parent
TASK=HERE.parent


class Session:
    def __init__(self,name,engine='jit',layout='default',stock=False,mode=8,legacy=False,busy=False):
        assert os.uname().nodename=='oryx-linuxlite'
        staged.prepare(layout,stock,mode,legacy,busy)
        self.profile=TASK/('.emulator-stock' if stock else '.emulator')
        self.art=self.profile/'last-run';self.art.mkdir(exist_ok=True)
        self.events=self.art/'events.txt';self.events.write_text('')
        self.log=self.profile/'sdcard/flower/flower.log'
        self.name=name;self.engine=engine;self.mode=mode;self.stock=stock;self.start=time.monotonic()
        self.dest=HERE/'evidence/mode19'/name;self.dest.mkdir(parents=True,exist_ok=True)
        self.images={};self.checks=[]
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',AGON_FLEXE_ENGINE=engine,
                 LD_PRELOAD=str(HERE/'build/sdl-input.so'),FLOWER_EVENTS=str(self.events),
                 FLOWER_ADAPTER=str(self.profile/('firmware/vdp_platform.so' if stock else 'flexe-vdp.so')))
        self.out=(self.art/'frontend.log').open('w')
        self.process=subprocess.Popen(['./fab-agon-emulator' if stock else './flexe-agon-emulator'],cwd=self.profile,env=env,
                                      stdout=self.out,stderr=subprocess.STDOUT,start_new_session=True)

    def content(self):return self.log.read_text() if self.log.exists() else ''
    def wire(self):
        path=self.art/'mos-to-vdp.bin'
        return path.read_bytes() if path.exists() else b''
    def wait(self,needle,count=1,timeout=40):
        start=time.monotonic()
        while time.monotonic()-start<timeout:
            content=self.content()
            if sum(needle in line for line in content.splitlines(keepends=True) if line.endswith('\n'))>=count:return content
            if self.process.poll() is not None:raise RuntimeError('Frontend exited before '+needle)
            time.sleep(.03)
        raise TimeoutError(needle+'\n'+self.content())
    def emit(self,line):
        with self.events.open('a') as out:out.write(line+'\n')
    def edge(self,scan,down):self.emit(f'key {scan} {int(down)}');time.sleep(.06)
    def press(self,scan):self.edge(scan,True);self.edge(scan,False)
    def type(self,text):
        for char in text:
            if char.isalpha():scan=4+ord(char.lower())-ord('a')
            elif '1'<=char<='9':scan=30+ord(char)-ord('1')
            else:scan={'0':39,'.':55,'-':45,'+':46}[char]
            shift=char.isupper() or char=='+'
            if shift:self.edge(225,True)
            self.press(scan)
            if shift:self.edge(225,False)
    def accept(self,field,text='',cycle=1):
        self.wait('PROMPT '+str(field)+' ',cycle)
        self.type(text);self.press(40)
        return self.wait('ACCEPT '+str(field)+' ',cycle)
    def capture(self,label,width=1024,height=768):
        # Prompt text is asynchronous too; allow its queued glyphs to settle on
        # the slower interpreter before comparing the complete review image.
        time.sleep(1.0)
        path=self.art/(label+'.ppm');path.unlink(missing_ok=True);self.emit('capture '+str(path))
        start=time.monotonic()
        header_expected=f'P6\n{width} {height}'.encode()
        while not path.exists() or path.stat().st_size<width*height*3+len(header_expected)+5:
            if time.monotonic()-start>10:raise TimeoutError('No complete framebuffer capture')
            time.sleep(.03)
        raw=path.read_bytes();header,image=raw.split(b'\n255\n',1)
        assert header==header_expected,header
        assert len(image)==width*height*3
        shutil.copyfile(path,self.dest/(label+'.ppm'));self.images[label]=image
        return image
    def check_white_curve(self,image):
        assert 'READY mode=19' in self.content()
        play=image[:1024*736*3]
        assert all(image[i:i+3] in (b'\0\0\0',b'\xff\xff\xff') for i in range(0,len(image),3))
        points=[(i//3%1024,i//3//1024) for i in range(0,len(play),3) if play[i:i+3]==b'\xff\xff\xff']
        assert len(points)>2000,len(points)
        xs,ys=zip(*points)
        assert max(xs)-min(xs)>500 and max(ys)-min(ys)>350,(min(xs),max(xs),min(ys),max(ys))
        self.checks.append({'check':'mode 19 white curve on black at full 1024x768 resolution','white_pixels':len(points),'bounds':[min(xs),min(ys),max(xs),max(ys)]})
    def compare_playfield(self,a,b,label):
        limit=1024*736*3
        bad=sum(a[i:i+3]!=b[i:i+3] for i in range(0,limit,3));assert bad==0,(label,bad)
        self.checks.append({'check':label,'exact_pixels':1024*736})
    def exit(self,curves=None,esc=True):
        if esc:self.press(41)
        content=self.wait('EXIT ')
        match=re.search(r'EXIT saved=(\d+) restored=(\d+) cleanup=(\d+) overflow=(\d+) curves=(\d+)',content)
        assert match and list(map(int,match.groups()[:4]))==[self.mode,self.mode,1,0],content
        if curves is not None:assert int(match[5])==curves,content
        if self.stock:
            # The stock native VDP does not export wire transcripts. The native
            # cases independently qualify live MOS typing after this same exit.
            self.checks.append({'check':'unavailable runtime leaves original mode and controlled cleanup','saved_mode':self.mode})
            return
        # Use the restored normal MOS handler, rather than another terminal/verifier.
        time.sleep(.2);self.type('dir');self.press(40)
        start=time.monotonic()
        while time.monotonic()-start<10:
            tail=self.wire().partition(b'Type dir at the MOS prompt')[2]
            if b'flower.bin' in tail and tail.count(b'/flower *')>=2:break
            if b'Flower:' in self.wire() and b'flower.bin' in self.wire().rsplit(b'Flower:',1)[-1]:break
            time.sleep(.05)
        else:raise AssertionError('MOS dir did not complete')
        self.checks.append({'check':'saved mode, owned cleanup and live MOS dir','saved_mode':self.mode})
    def finish(self,passed,failure=None):
        self.emit('quit')
        try:self.process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            os.killpg(self.process.pid,signal.SIGTERM)
            try:self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:os.killpg(self.process.pid,signal.SIGKILL);self.process.wait()
        self.out.close()
        for file in ('flower.log',):
            if (self.profile/'sdcard/flower'/file).exists():shutil.copyfile(self.profile/'sdcard/flower'/file,self.dest/file)
        stats={}
        if not self.stock:
            stats=json.loads((self.art/'stats.json').read_text())
            assert not any(stats.get(k) for k in ('failed','unhandled','unregistered','unmapped')),stats
        for name in ('stats.json','frontend.log','mos-to-vdp.bin','vdp-to-mos-delivered.bin','final.ppm'):
            if (self.art/name).exists():shutil.copyfile(self.art/name,self.dest/name)
        report={'passed':passed,'failure':failure,'scenario':self.name,'engine':self.engine,'checks':self.checks,
                'elapsed_seconds':time.monotonic()-self.start,'stock':self.stock,'backend_stats':stats,
                'frontend_exit_status':self.process.returncode,
                'graphical_launch':False,'returned_to_MOS_and_dir':passed and not self.stock,
                'identities':(self.profile/'.runtime.sha256').read_text().splitlines(),
                'images':{key:hashlib.sha256(value).hexdigest() for key,value in self.images.items()}}
        (self.dest/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
        print(self.name,'PASS' if passed else 'FAIL',round(report['elapsed_seconds'],2),'seconds',flush=True)


def workflow(session):
    for field in range(7):session.accept(field)
    session.wait('DONE 1 segments=396')
    initial=session.capture('defaults')
    session.check_white_curve(initial)
    session.edge(21,True) # Hold the key which opens the editor beyond typematic delay.
    session.wait('PROMPT 0 ',2);time.sleep(.8)
    assert 'EDIT 0 r' not in session.content()
    returned=session.capture('reentered')
    session.compare_playfield(initial,returned,'re-entry preserves curve outside prompt pane')
    session.edge(21,False)
    session.type('3.031');session.press(42);session.type('00');session.press(40);session.wait('ACCEPT 0 3.0300 ')
    session.accept(1,cycle=2)
    session.wait('PROMPT 2 ',2);session.type('nan');session.press(40);session.wait('REJECT 2 nan')
    for _ in range(3):session.press(42)
    session.type('.6000');session.press(40);session.wait('ACCEPT 2 .6000 ')
    session.accept(3,'6.6e1',2);session.accept(4,'0.8000',2);session.accept(5,'512.00',2);session.accept(6,'22.5000',2)
    session.wait('DONE 2 segments=396');angled=session.capture('fractional')
    assert initial[:1024*736*3]!=angled[:1024*736*3]
    session.press(58);session.wait('PROMPT 0 ',3) # F1 has no ASCII byte, but is still a key.
    assert 'PROMPT 0 3.0300' in session.content()
    session.accept(0,'1',3);session.accept(1,'1',3);session.accept(2,cycle=3)
    session.accept(3,'70000',3);session.accept(4,cycle=3);session.accept(5,cycle=3);session.accept(6,cycle=3)
    session.wait('MATH_ERROR runtime=1 interface=0 language=32')
    session.wait('PROMPT 0 1',1)
    failed=session.capture('failed-prepare');session.compare_playfield(angled,failed,'failed preparation preserves previous curve')
    assert 'DRAW 3' not in session.content()
    for needle in ('PROMPT 2 .6000','PROMPT 3 6.6e1','PROMPT 4 0.8000','PROMPT 5 512.00','PROMPT 6 22.5000'):
        assert needle in session.content(),needle
    session.checks.append({'check':'retained numeric strings, blank Enter, corrections, NaN rejection, fractional degrees and held-trigger suppression'})
    session.exit(2)


def run_case(name,engine,kind='workflow',field=0,layout='default',mode=8,legacy=False,stock=False,busy=False):
    session=Session(name,engine,layout,stock,mode,legacy,busy);passed=False;failure=None
    try:
        if stock or busy:
            session.wait('EXIT ')
            tail=session.wire()
            message=b'module slot is busy' if busy else b'compatible VDPlet-enabled VDP'
            start=time.monotonic()
            if not stock:
                while message not in tail and time.monotonic()-start<10:time.sleep(.05);tail=session.wire()
                assert message in tail
            assert 'READY' not in session.content()
            session.exit(0,False)
            if stock:session.capture('native-unavailable',320,240)
        elif kind=='workflow':workflow(session)
        elif kind=='escape':
            for index in range(field):session.accept(index)
            session.wait('PROMPT '+str(field)+' ');session.exit(0)
        elif kind=='startup':
            session.wait('START');session.exit(0)
        elif kind=='drawing':
            for index,text in enumerate(('3','2','','10000','','','')):session.accept(index,text)
            session.wait('BATCH 32 ');session.exit(0)
            assert 'DONE' not in session.content()
            session.checks.append({'check':'Escape stops a 60001-segment curve between finite batches'})
        elif kind=='length':
            session.wait('PROMPT 0 ');session.type('0'*30+'1');session.type('2');session.wait('LIMIT 0')
            session.press(42);session.type('2');session.press(40)
            session.wait('ACCEPT 0 '+'0'*30+'2'+' ')
            session.checks.append({'check':'31-character limit and backspace correction on actual target'})
            session.exit(0)
        elif kind in ('default','fullcircle'):
            for index in range(7):session.accept(index,'360.000' if kind=='fullcircle' and index==6 else '')
            session.wait('DONE 1 segments=396');image=session.capture('defaults')
            session.check_white_curve(image)
            assert 'READY mode=19' in session.content()
            session.exit(1)
        passed=True
    except BaseException as exc:
        failure=f'{type(exc).__name__}: {exc}';raise
    finally:session.finish(passed,failure)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--engine',default='jit',choices=('jit','interp'))
    parser.add_argument('--scenario',default='workflow',choices=('workflow','escape','startup','drawing','length','default','fullcircle','stock','busy'))
    parser.add_argument('--field',default=0,type=int);parser.add_argument('--layout',default='default',choices=('default','relocated'))
    parser.add_argument('--mode',type=int,default=8);parser.add_argument('--legacy',action='store_true')
    args=parser.parse_args();name='-'.join([args.scenario,str(args.field) if args.scenario=='escape' else args.layout,
        'legacy' if args.legacy else str(args.mode),args.engine])
    run_case(name,args.engine,args.scenario,args.field,args.layout,args.mode,args.legacy,args.scenario=='stock',args.scenario=='busy')
