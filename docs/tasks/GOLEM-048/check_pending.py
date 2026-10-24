#!/usr/bin/env python3
"""Differential pending-path tests using the task-only context preparation hook."""
from pathlib import Path
import argparse
import ctypes
import json
import struct
from qualify import old,SDK,HERE,BUILD,upload,word,plot,sha

p=argparse.ArgumentParser();p.add_argument('--firmware',type=Path,required=True)
p.add_argument('--engine',choices=('jit','interp'),default='jit');a=p.parse_args();a.firmware=a.firmware.resolve()
v=old.VDP(a.engine,BUILD/('pending-'+a.engine),120,firmware=a.firmware)
v.lib.flexe_vdp_frames.restype=ctypes.c_uint64
v.lib.copyVgaFramebuffer.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_float)]
rgb=(ctypes.c_uint8*(1024*768*3))()
def barrier():
 v.frames.clear();v.send(bytes((23,0,132,0,0,0,0)));v.wait(lambda:any(k==132 for k,_ in v.frames));v.frames.clear()
def capture():
 barrier();n=v.lib.flexe_vdp_frames();v.wait(lambda:v.lib.flexe_vdp_frames()>=n+3)
 w,h,f=ctypes.c_uint32(),ctypes.c_uint32(),ctypes.c_float()
 v.lib.copyVgaFramebuffer(ctypes.byref(w),ctypes.byref(h),rgb,ctypes.byref(f))
 assert(w.value,h.value)==(512,384)
 return bytes(rgb[:512*384*3])
def setup(graphics=False):
 v.send(bytes((22,20,23,1,0,23,0,192,0,26,18,0,15,12))+ (b'\x05' if graphics else b''));barrier()
def call(entry):v.transaction(2,struct.pack('<HHH',handle,entry,60022),3,0)
def pending():v.transaction(4,word(0x7f00),4,0)
cases=[]
try:
 data=(SDK/'build/modules/line-batch.vdpl').read_bytes();v.send(upload(60023,data))
 handle=v.transaction(1,struct.pack('<HIH',60023,len(data),0),2)
 pairs=[(140,30,180,90),(180,90,140,90)]
 packed=b''.join(struct.pack('<hhhh',*x) for x in pairs)
 direct=b''.join(plot(4,*x[:2])+plot(5,*x[2:]) for x in pairs)
 tail=plot(1,20,30)+plot(85,240,160)
 v.send(upload(60020,packed)+upload(60021,b'\0'*8)+upload(60022,word(60020)+word(60021)))
 for entry in (0,1):
  setup();pending();v.send(direct+tail);want=capture()
  setup();pending();call(entry);v.send(tail);assert capture()==want
  cases.append('prior pending path then '+('whole' if entry==0 else 'chunked')+' batch')
 setup();pending();v.send(direct+tail);want=capture()
 setup();pending();call(2);v.send(direct+tail);assert capture()==want
 cases.append('empty batch does not commit pending path')
 path=plot(0xdd,200,120)+plot(0xdd,160,150)+plot(4,230,160)
 setup();v.send(direct+path);want=capture()
 setup();call(0);v.send(path);assert capture()==want
 cases.append('batch followed by ordinary path/history drawing')
 setup(True);v.send(direct+b'Z');want=capture()
 setup(True);call(0);v.send(b'Z');assert capture()==want
 cases.append('graphics text observes same final Canvas cursor')
 v.transaction(3,word(handle),3,0)
finally:v.close()
stats=json.loads((BUILD/('pending-'+a.engine)/'stats.json').read_text())
assert not any(stats.get(k) for k in ('failed','unhandled','unregistered','unmapped'))
report={'passed':True,'engine':a.engine,'firmware_sha256':sha(a.firmware),'diagnostic_only':True,
        'cases':cases,'pixels_compared':len(cases)*512*384,'execution_stats':stats,
        'scope':'task hook prepares pending Context state; implemented service and literal PLOT pairs compared'}
(HERE/('pending-'+a.engine+'-evidence.json')).write_text(json.dumps(report,indent=2)+'\n')
print(a.engine,len(cases),'pending path/Canvas history cases pass.')
