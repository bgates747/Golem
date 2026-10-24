#!/usr/bin/env python3
"""Actual ESP32 generic service checks, separate from MOS/human qualification."""
from pathlib import Path
import argparse,ctypes,hashlib,importlib.util,json,os,struct,subprocess,sys,time
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3];SDK=ROOT.parent/'agon-vdplet';BUILD=HERE/'build'
OLD=ROOT/'docs/tasks/GOLEM-044/native-math'
sys.path.insert(0,str(OLD))
spec=importlib.util.spec_from_file_location('qualified_math_vdp',OLD/'qualify_headless.py')
old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
word=lambda n:struct.pack('<H',n)
cmd=lambda key,op:bytes((23,0,160))+word(key)+bytes((op,))
plot=lambda op,x,y:bytes((25,op))+struct.pack('<hh',x,y)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def upload(key,data):
    assert data
    return cmd(key,2)+b''.join(cmd(key,0)+word(len(b))+b for at in range(0,len(data),4096) if (b:=data[at:at+4096]))+cmd(key,14)
def invoke(handle,entry,args=60022):
    body=struct.pack('<BBHBBHHH',1,2,0,0,0,handle,entry,args)
    return bytes((23,0,216))+word(len(body))+body

def run(engine,firmware=None,backend=None):
    BUILD.mkdir(exist_ok=True)
    subprocess.run([str(SDK/'.venv/bin/python'),str(SDK/'tools/build_module.py'),str(HERE/'legacy_strict.c'),'--name','legacy','--output',str(BUILD)],check=True,stdout=subprocess.DEVNULL)
    subprocess.run([str(SDK/'.venv/bin/python'),str(SDK/'tools/build_module.py'),str(HERE/'service_probe.c'),'--name','probe','--capabilities','7','--output',str(BUILD)],check=True,stdout=subprocess.DEVNULL)
    output=BUILD/engine
    v=old.VDP(engine,output,180,backend,firmware)
    report={'engine':engine,'firmware_sha256':sha(v.firmware),'adapter_sha256':sha(v.backend),'checks':[]}
    pixels=(ctypes.c_uint8*(1024*768*3))()
    v.lib.flexe_vdp_frames.restype=ctypes.c_uint64
    v.lib.copyVgaFramebuffer.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_float)]
    def check(name):report['checks'].append(name)
    def loaded(path):
        data=path.read_bytes();v.send(upload(60023,data));return v.transaction(1,struct.pack('<HIH',60023,len(data),0),2)
    def unload(handle):v.transaction(3,word(handle),3,0)
    def submit(handle,entry=0,expect=0):
        v.transaction(2,struct.pack('<HHH',handle,entry,60022),5 if expect else 3,expect)
    def barrier():
        v.frames.clear();v.send(bytes((23,0,132,0,0,0,0)));v.wait(lambda:any(k==132 for k,_ in v.frames));v.frames.clear()
    def capture():
        barrier();first=v.lib.flexe_vdp_frames();v.wait(lambda:v.lib.flexe_vdp_frames()>=first+3)
        w,h,f=ctypes.c_uint32(),ctypes.c_uint32(),ctypes.c_float()
        v.lib.copyVgaFramebuffer(ctypes.byref(w),ctypes.byref(h),pixels,ctypes.byref(f))
        assert (w.value,h.value)==(512,384),(w.value,h.value)
        return bytes(pixels[:w.value*h.value*3])
    def setup():v.send(bytes((22,20,23,1,0,23,0,192,0,26,18,0,15,12)));barrier()
    try:
        assert v.transaction(4,word(1),4)==7;check('discovery capability mask includes VDU')
        for name in ('numeric-1','numeric-2','packed-mono'):
            p=SDK/'build/modules'/f'{name}.vdpl';b=SDK/'docs/tasks/VDPL-003/build/baseline/modules'/p.name
            assert p.read_bytes()==b.read_bytes(),name
        check('all three default images unchanged')
        # Actual precompiled ABI1.0 execution, not merely loader admission.
        h=loaded(SDK/'docs/tasks/VDPL-003/build/baseline/modules/numeric-1.vdpl')
        v.send(upload(60020,struct.pack('<f',2.0))+upload(60021,b'\0'*8)+upload(60022,word(60020)+word(60021)))
        submit(h);assert v.read(60021,8)==struct.pack('<fI',3.25,1);unload(h)
        check('precompiled legacy numeric module executes unchanged')
        h=loaded(BUILD/'legacy.vdpl');submit(h);unload(h)
        check('legacy strict extent/minor/capabilities table retained')
        h=loaded(SDK/'build/modules/vdu-plot.vdpl')
        v.send(upload(60021,b'\0'*8)+upload(60022,word(60020)+word(60021)))
        setup()
        shapes=[plot(4,20,20)+plot(5,220,20)+plot(5,220,160)+plot(5,20,160)+plot(5,20,20),
                plot(4,-30,100)+plot(5,400,100)+plot(4,140,-30)+plot(5,140,440),
                plot(4,10,10)+plot(5,230,190)+plot(4,230,10)+plot(5,10,190)]
        compared=0
        for n,data in enumerate(shapes):
            setup();v.send(data);direct=capture();assert any(direct)
            for entry in (0,1):
                setup();v.send(upload(60020,data));submit(h,entry)
                assert capture()==direct,(n,entry);compared+=512*384
            check(f'shape{n} complete-span and six-byte chunk context/pixels')
        # Native invocation inside a buffered wrapper must return to its tail.
        setup();data=shapes[0];v.send(upload(60020,data)+upload(60025,b'\0\0'))
        tail=cmd(60025,5)+b'\xc2'+word(0)+word(2)+b'\x34\x12'
        v.send(upload(60024,invoke(h,0)+tail)+cmd(60024,1));barrier()
        assert v.read(60025,2)==b'\x34\x12';check('outer resident wrapper continues after span')
        setup();v.send(upload(60020,plot(4,1,1)*10922));submit(h)
        check('maximum65532-byte admitted span executes without outer reads')
        for name,data,expected in [('truncated',plot(4,1,1)[:-1],4),('unsupported',plot(4,1,1)+bytes((12,0,0,0,0,0)),9),('limit',plot(4,1,1)*10923,13)]:
            setup();empty=capture();v.send(upload(60020,data));submit(h,0,expected)
            assert capture()==empty,name
            assert int.from_bytes(v.read(60021,2),'little')==expected
            check(name+' rejected before drawing; outer protocol remains usable')
        setup();v.send(upload(60020,shapes[0]));submit(h);submit(h,2)
        result=v.read(60021,8);assert struct.unpack_from('<H',result,2)[0]==9
        check('valid retry/empty span/INIT submission rejection')
        # Echo is explicitly unsupported; registration alone is compatible.
        v.send(bytes((23,0,248))+word(0x110)+word(1));submit(h,0,9)
        v.send(bytes((23,0,249))+word(0x110));submit(h)
        check('echo rejected; recovery with console tracing')
        unload(h);assert v.transaction(4,word(2),4)==0;barrier()
        check('FINI submission rejection, unload and clean outer stream')
        # Exercise the callback body normally, then register it with echo off.
        # Activating stock echo callbacks can recurse in flushEcho itself; that
        # unsupported state is not part of the submission service.
        counter=cmd(60026,5)+b'\x03'+word(0)+b'\x01'
        v.send(upload(60026,b'\0\0')+upload(60027,counter)+cmd(60027,1)+cmd(60027,80)+word(0x10a))
        h=loaded(BUILD/'probe.vdpl')
        setup();v.send(upload(60020,shapes[0]));submit(h,0)
        fields=struct.unpack('<HHHH',v.read(60021,8));assert fields[0]==0 and fields[2]>0 and fields[2]==fields[3],fields
        check('all four buffer leases active; no echo callback during submit')
        for entry,status in ((2,1),(3,13)):
            submit(h,entry,status)
        check('null pointer and oversize service arguments rejected')
        setup();v.send(shapes[0]);want=capture()
        setup();submit(h,4,9);assert capture()==want
        check('earlier successful submit remains on later failure; explicit raster drain')
        v.send(bytes((22,2,23,1,0)));barrier();submit(h,1,7)
        assert struct.unpack('<HH',v.read(60021,4))==(7,0)
        check('open framebuffer transaction rejects submit and closes cleanly')
        unload(h);v.send(cmd(60027,81)+word(0x10a));barrier()
        report['minimum_free_stack_bytes']=v.transaction(4,word(17),4)
        report.update(elapsed_seconds=round(time.monotonic()-v.start,3),passed=True,pixels_compared=compared,result_bytes=len(v.observed),scope='actual ESP32 firmware and uploaded code; no MOS/hardware claim')
    finally:v.close()
    report['execution_stats']=json.loads((output/'stats.json').read_text())
    assert not any(report['execution_stats'].get(k) for k in ('failed','unhandled','unregistered','unmapped'))
    (HERE/(engine+'-evidence.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(f'{engine}: {len(report["checks"])} service cases, {report["pixels_compared"]} compared pixels pass.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--engine',choices=('jit','interp'),default='jit');p.add_argument('--firmware',type=Path);p.add_argument('--backend',type=Path);a=p.parse_args();run(a.engine,a.firmware,a.backend)
