#!/usr/bin/env python3
"""Headless actual ESP32 service/context comparison, separate from full MOS."""
from pathlib import Path
import argparse
import ctypes
import hashlib
import importlib.util
import json
import struct
import subprocess
import sys
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SDK=ROOT.parent/'agon-vdplet'
OLD=ROOT/'docs/tasks/GOLEM-044/native-math'
sys.path.insert(0,str(OLD))
spec=importlib.util.spec_from_file_location('target_vdp',OLD/'qualify_headless.py')
old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
BUILD=HERE/'build'
word=lambda n:struct.pack('<H',n)
cmd=lambda n,op:bytes((23,0,160))+word(n)+bytes((op,))
plot=lambda op,x,y:bytes((25,op))+struct.pack('<hh',x,y)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def upload(key,data):
    assert data
    return cmd(key,2)+b''.join(cmd(key,0)+word(len(b))+b for at in range(0,len(data),4096) if (b:=data[at:at+4096]))+cmd(key,14)


def run(engine,firmware=None,backend=None):
    BUILD.mkdir(exist_ok=True)
    for name,source,cap,minor in [('legacy',HERE/'legacy_vdu.c',4,1),('probe',HERE/'line_probe.c',15,2),('undeclared',HERE/'line_probe.c',1,2)]:
        subprocess.run([str(SDK/'.venv/bin/python'),str(SDK/'tools/build_module.py'),str(source),'--name',name,
                        '--capabilities',str(cap),'--abi-minor',str(minor),'--output',str(BUILD)],check=True,stdout=subprocess.DEVNULL)
    output=BUILD/('target-'+engine)
    v=old.VDP(engine,output,300,backend,firmware)
    report={'engine':engine,'firmware_sha256':sha(v.firmware),'adapter_sha256':sha(v.backend),'checks':[]}
    v.lib.flexe_vdp_frames.restype=ctypes.c_uint64
    v.lib.copyVgaFramebuffer.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_float)]
    pixels=(ctypes.c_uint8*(1024*768*3))()
    def check(s):report['checks'].append(s)
    def load(path):
        data=path.read_bytes();v.send(upload(60023,data));return v.transaction(1,struct.pack('<HIH',60023,len(data),0),2)
    def unload(h):v.transaction(3,word(h),3,0)
    def call(h,entry=0,expected=0):v.transaction(2,struct.pack('<HHH',h,entry,60022),5 if expected else 3,expected)
    def barrier():
        v.frames.clear();v.send(bytes((23,0,132,0,0,0,0)));v.wait(lambda:any(k==132 for k,_ in v.frames));v.frames.clear()
    def capture():
        barrier();n=v.lib.flexe_vdp_frames();v.wait(lambda:v.lib.flexe_vdp_frames()>=n+3)
        w,h,f=ctypes.c_uint32(),ctypes.c_uint32(),ctypes.c_float()
        v.lib.copyVgaFramebuffer(ctypes.byref(w),ctypes.byref(h),pixels,ctypes.byref(f))
        assert (w.value,h.value)==(512,384),(w.value,h.value)
        return bytes(pixels[:512*384*3])
    def setup(prefix=b''):
        v.send(bytes((22,20,23,1,0,23,0,192,0,26,18,0,15,12))+prefix);barrier()
    lines=[(20,20,220,20),(220,20,220,160),(220,160,20,160),(20,160,20,20),
           (-30,100,400,100),(140,-30,140,440),(80,80,80,80),(20,20,220,20)]
    packed=b''.join(struct.pack('<hhhh',*line) for line in lines)
    direct=b''.join(plot(4,*line[:2])+plot(5,*line[2:]) for line in lines)
    tail=plot(1,35,-15)+plot(85,260,170)
    compared=0
    try:
        assert v.transaction(4,word(1),4)==15;check('line capability discovered')
        for name in ('numeric-1','numeric-2','packed-mono','vdu-plot'):
            assert (SDK/'build/modules'/f'{name}.vdpl').read_bytes()==(SDK/'docs/tasks/VDPL-004/build/baseline'/f'{name}.vdpl').read_bytes(),name
        check('all four prior package images unchanged')
        h=load(SDK/'docs/tasks/VDPL-004/build/baseline/numeric-1.vdpl')
        v.send(upload(60020,struct.pack('<f',2))+upload(60021,b'\0'*8)+upload(60022,word(60020)+word(60021)))
        call(h);assert v.read(60021,8)==struct.pack('<fI',3.25,1);unload(h)
        check('precompiled ABI1.0 numeric executes')
        h=load(BUILD/'legacy.vdpl');call(h);unload(h);check('ABI1.1 receives exact old extent/minor/mask')
        h=load(SDK/'docs/tasks/VDPL-004/build/baseline/vdu-plot.vdpl')
        setup();v.send(direct);want=capture()
        setup();v.send(upload(60020,direct));call(h);assert capture()==want;unload(h)
        check('precompiled ABI1.1 submitted drawing unchanged')
        h=load(SDK/'build/modules/line-batch.vdpl')
        prefixes=[('pixel',b''),('logical',bytes((23,0,192,1))),
                  ('origin',bytes((29,))+struct.pack('<hh',23,17)),
                  ('clip',bytes((24,))+struct.pack('<hhhh',40,160,200,20)),
                  ('xor',bytes((18,3,15))),('thick',bytes((23,23,3))),
                  ('graphics-cursor',bytes((5,))),
                  ('prior-pattern',bytes((23,6))+b'\xaa'*8+plot(4,5,5)+plot(21,240,10))]
        for name,prefix in prefixes:
            setup(prefix);v.send(direct+tail);want=capture();assert any(want),name
            for entry in (0,1):
                setup(prefix);v.send(upload(60020,packed));call(h,entry);v.send(tail)
                assert capture()==want,(name,entry);compared+=512*384
            check(name+' pixels and follow-on relative/triangle history, whole/chunked')
        # Empty preserves history; subsequent primitive uses the prior points.
        setup();v.send(direct+tail);want=capture()
        setup();v.send(direct);call(h,2);v.send(tail);assert capture()==want
        check('empty batch preserves history and graphics effects')
        # The queued primitives must retain coordinates, not pointers.
        setup();v.send(upload(60020,packed));call(h)
        v.send(upload(60020,b'\0'*len(packed))+tail);assert capture()==want
        check('caller endpoint data reusable after return')
        # Outer resident stream continues and still has its original identity.
        body=struct.pack('<BBHBBHHH',1,2,0,0,0,h,0,60022)
        invoke=bytes((23,0,216))+word(len(body))+body
        marker=cmd(60025,5)+b'\xc2'+word(0)+word(2)+b'\x34\x12'
        v.send(upload(60020,packed)+upload(60025,b'\0\0')+upload(60024,invoke+marker)+cmd(60024,1));barrier()
        assert v.read(60025,2)==b'\x34\x12';check('outer resident wrapper resumes after native batch')
        setup();v.send(upload(60020,struct.pack('<hhhh',15,15,15,15)*8191));call(h)
        check('maximum8191-line input executes')
        call(h,2)
        assert struct.unpack_from('<H',v.read(60021,8),2)[0]==9
        v.send(bytes((23,0,248))+word(0x110)+word(1));call(h,0,9)
        v.send(bytes((23,0,249))+word(0x110));call(h,2)
        check('INIT/echo rejection and successful recovery')
        unload(h);check('FINI rejection is contained and unload succeeds')
        # A runnable echo callback can remain registered with echo disabled.
        counter=cmd(60026,5)+b'\x03'+word(0)+b'\x01'
        v.send(upload(60026,b'\0\0')+upload(60027,counter)+cmd(60027,1)+cmd(60027,80)+word(0x10a))
        h=load(BUILD/'probe.vdpl');setup();v.send(upload(60020,packed));call(h)
        fields=struct.unpack('<HHHH',v.read(60021,8));assert fields[0]==0 and fields[2]>0 and fields[2]==fields[3],fields
        check('ABI1.2 exact table and four ordinary leases, no callback during batch')
        for entry,status in ((2,1),(3,13),(6,9)):
            empty=capture();call(h,entry,status);assert capture()==empty
        check('null/count/context failures preserve existing pixels')
        setup();v.send(direct);want=capture()
        setup();call(h,4,13);assert capture()==want
        check('earlier batch remains on later rejection; explicit raster drain')
        v.send(bytes((22,2,23,1,0)));barrier();call(h,1,7)
        assert struct.unpack('<HH',v.read(60021,4))==(7,0)
        unload(h);check('open packed-frame transaction rejects and closes')
        h=load(BUILD/'undeclared.vdpl');call(h,5,9);unload(h)
        check('image must declare the line capability')
        v.send(cmd(60027,81)+word(0x10a));barrier()
        report.update(passed=True,pixels_compared=compared,result_bytes=len(v.observed),
                      stack_free_bytes=v.transaction(4,word(17),4),elapsed_seconds=round(time.monotonic()-v.start,3),
                      scope='actual ESP32 instructions; headless VDP service checks, no MOS/physical claim')
    finally:v.close()
    report['result_bytes']=len(v.observed)
    report['execution_stats']=json.loads((output/'stats.json').read_text())
    assert not any(report['execution_stats'].get(k) for k in ('failed','unhandled','unregistered','unmapped'))
    (HERE/(engine+'-service-evidence.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(engine,len(report['checks']),'service/context checks pass;',compared,'pixels compared.')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--engine',choices=('jit','interp'),default='jit')
    p.add_argument('--firmware',type=Path);p.add_argument('--backend',type=Path)
    a=p.parse_args();run(a.engine,a.firmware,a.backend)
