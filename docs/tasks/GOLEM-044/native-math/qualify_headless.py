#!/usr/bin/env python3
"""Run actual ESP32 firmware and uploaded math through a windowless VDP adapter.

All state updates and observations use real wire commands. This does not boot
MOS, replace native instructions with host math, or stage a visual review.
"""
from pathlib import Path
import argparse
import ctypes
from collections import deque
import hashlib
import json
import math
import os
import struct
import threading
import time
from host_checks import ROOT,SDK,HERE,run,load,cases,compare,reference

word=lambda n:struct.pack('<H',n)
packet=lambda key,op:bytes((23,0,160))+word(key)+bytes((op,))
write=lambda key,data:packet(key,2)+packet(key,0)+word(len(data))+data
call=lambda key:packet(key,1)
update=lambda key,offset,data:packet(key,5)+b'\xc2'+word(offset)+word(len(data))+data
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()


class VDP:
    def __init__(self,engine,output,timeout,backend=None,firmware=None):
        self.output=output;output.mkdir(parents=True,exist_ok=True)
        self.firmware=firmware or SDK/'build/firmware/firmware-src/.pio/build/esp32dev/firmware.bin'
        self.elf=self.firmware.with_suffix('.elf')
        self.backend=backend or HERE/'build/headless-runtime/flexe-vdp.so'
        os.environ.update(AGON_FLEXE_FIRMWARE=str(self.firmware),AGON_FLEXE_ELF=str(self.elf),
                          AGON_FLEXE_ENGINE=engine,AGON_FLEXE_ARTIFACTS=str(output),
                          SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
        self.lib=ctypes.CDLL(str(self.backend))
        for name in ('z80_uart0_is_cts','z80_recv_from_vdp','flexe_vdp_failed'):
            getattr(self.lib,name).restype=ctypes.c_bool
        self.lib.z80_recv_from_vdp.argtypes=[ctypes.POINTER(ctypes.c_uint8)]
        self.lib.z80_send_to_vdp.argtypes=[ctypes.c_uint8]
        self.lib.flexe_vdp_cycles.restype=ctypes.c_uint64
        self.frames=deque();self.pending=bytearray();self.raw=bytearray();self.observed=bytearray()
        self.start=time.monotonic();self.timeout=timeout;self.token=0;self.sent=0;self.handle=0
        self.lib.vdp_setup();self.thread=threading.Thread(target=self.lib.vdp_loop);self.thread.start()
        self.send(bytes((23,0,128,0x42)));self.wait(lambda:any(key==128 and data==b'B' for key,data in self.frames));self.frames.clear()

    def poll(self):
        byte=ctypes.c_uint8()
        while self.lib.z80_recv_from_vdp(ctypes.byref(byte)):
            value=byte.value;self.raw.append(value)
            if not self.pending and value<128:continue
            self.pending.append(value)
            if len(self.pending)>=2 and len(self.pending)==self.pending[1]+2:
                self.frames.append((self.pending[0],bytes(self.pending[2:])));self.pending.clear()
        if self.lib.flexe_vdp_failed():raise RuntimeError('ESP32 execution stopped or faulted')
        if time.monotonic()-self.start>self.timeout:raise TimeoutError('Headless math qualification deadline')

    def wait(self,test):
        while True:
            self.poll()
            if test():return
            time.sleep(.0001)

    def send(self,data):
        for byte in data:
            self.wait(self.lib.z80_uart0_is_cts);self.lib.z80_send_to_vdp(byte);self.sent+=1

    def transaction(self,op,fields,kind,expected=None):
        self.token=(self.token+1)&65535
        body=struct.pack('<BBHBB',1,op,self.token,1,0)+fields
        self.send(bytes((23,0,216))+word(len(body))+body)
        while True:
            self.wait(lambda:bool(self.frames));key,data=self.frames.popleft()
            if key!=129 or not data.startswith(b'VDPL'):continue
            assert len(data)==10 and data[4]==1,(key,data)
            actual_kind,token,value=struct.unpack_from('<BHH',data,5)
            assert (actual_kind,token)==(kind,self.token),(op,kind,actual_kind,token,self.token,value)
            if expected is not None:assert value==expected,(op,value,expected)
            self.observed+=data
            return value

    def read(self,key,size):
        return b''.join(word(self.transaction(5,struct.pack('<HI',key,offset),4)) for offset in range(0,size,2))

    def install(self,directory,backend,name):
        self.send((directory/'common-install.bin').read_bytes())
        image=directory/(name+'.vdpl');self.send(write(60010,image.read_bytes()))
        self.handle=self.transaction(1,struct.pack('<HIH',60010,image.stat().st_size,0),2)
        assert self.handle
        address=self.transaction(4,word(6),4)|(self.transaction(4,word(7),4)<<16)
        assert 0x40080000<=address<0x400a0000,hex(address)
        self.send((directory/'native-install.bin').read_bytes())
        for entry in backend['native_wrappers']:self.send(update(entry,backend['native_handle_offset'],word(self.handle)))
        return address

    def close(self):
        self.lib.vdp_shutdown();self.thread.join(timeout=10)
        assert not self.thread.is_alive(),'VDP worker did not stop'
        (self.output/'results.dat').write_bytes(self.observed)
        (self.output/'wire-rx.dat').write_bytes(self.raw)


def build_packages():
    result=[]
    for name in ('probe','probe-relocated','flower'):
        directory=HERE/'build'/name;image_name='probe' if name.startswith('probe') else 'flower'
        run([os.sys.executable,SDK/'tools/build_module.py',directory/'native.c','--math','--name',image_name,
             '--build-id','0x474c3434','--output',directory])
        manifest=json.loads((directory/(image_name+'.json')).read_text())['module']
        admission=json.loads(run([HERE.parent/'build/math/admit',directory/(image_name+'.vdpl')]))
        assert admission['status']==0,(name,admission)
        result.append(dict(package=name,module=manifest))
    (HERE/'evidence/packages.json').write_text(json.dumps(result,indent=2)+'\n')


def qualify(engine,timeout,backend=None,firmware=None):
    (HERE/'evidence').mkdir(exist_ok=True)
    output=HERE/'build'/('target-'+engine);vdp=VDP(engine,output,timeout,backend,firmware)
    report={'engine':engine,'firmware_sha256':sha(vdp.firmware),'elf_sha256':sha(vdp.elf),
            'adapter_sha256':sha(vdp.backend),'scope':'actual ESP32 firmware/VDP-only ABI; no MOS/SD or physical hardware',
            'graphical_instance_launched':False,'visual_staging_deferred_by_author':True,'packages':[]}
    passed=False;failure=None
    try:
        for name in ('probe','probe-relocated','flower'):
            directory=HERE/'build'/name;interface,ast,root,backend=load(directory)
            initial=(directory/'state.bin').read_bytes();image_name='probe' if name.startswith('probe') else 'flower'
            address=vdp.install(directory,backend,image_name);stack_before=vdp.transaction(4,word(17),4)
            count=0;worst=0;coordinates=0;statuses=set()
            if name!='flower':
                for entry,state,label in cases(interface,initial,large=False):
                    vdp.send(write(backend['shared_state'],state));vdp.send(call(backend['native_wrappers'][entry]))
                    got=vdp.read(backend['shared_state'],len(initial))
                    wanted=reference(interface,ast,state,entry,root)
                    status=struct.unpack_from('<H',wanted,14)[0];statuses.add(status)
                    worst=max(worst,compare(wanted,got,interface,trig=label in ('sine','cosine','partial','nested','constants')))
                    vdp.transaction(4,word(3),4,1 if status else 0);count+=1
            else:
                fields={f['name']:f for f in interface['globals']}
                trials=[('defaults',{}),('fractional-angle',{'theta_init':22.5}),('full-circle',{'theta_init':360}),
                        ('integer-count',{'petals':3,'vectors':2,'periods':2}),('zero-shrink',{'shrink':0}),
                        ('full-shrink',{'shrink':1})]
                for trial,changes in trials:
                    state=bytearray(initial)
                    for field,value in changes.items():struct.pack_into('<f',state,fields[field]['offset'],value)
                    arrays={a['buffer']:bytearray((directory/a['initial_data']).read_bytes()) for a in interface['arrays']}
                    vdp.send(write(backend['shared_state'],state))
                    for key,data in arrays.items():vdp.send(write(key,data))
                    segments=0;entry=0
                    while True:
                        wanted_arrays={k:bytearray(data) for k,data in arrays.items()}
                        wanted=reference(interface,ast,state,entry,root,arrays=wanted_arrays)
                        vdp.send(call(backend['native_wrappers'][entry]));got=vdp.read(backend['shared_state'],len(initial))
                        worst=max(worst,compare(wanted,got,interface,flower=True))
                        for a in interface['arrays']:
                            actual=vdp.read(a['buffer'],a['bytes']);expected=wanted_arrays[a['buffer']]
                            aa,bb=struct.unpack('<32h',actual),struct.unpack('<32h',expected)
                            assert all(abs(x-y)<=1 for x,y in zip(aa,bb)),(trial,a['name'],aa,bb)
                            coordinates+=32;arrays[a['buffer']]=bytearray(actual)
                        state=got;count+=1
                        if entry==1:segments+=struct.unpack_from('<H',state,fields['batchCount']['offset'])[0]
                        if struct.unpack_from('<H',state,fields['remaining']['offset'])[0]==0:break
                        entry=1
                    assert segments==(13 if trial=='integer-count' else 396),(trial,segments)
                    print(engine,trial,segments,'segments verified',flush=True)
            stack_after=vdp.transaction(4,word(17),4)
            assert stack_after>=512,(name,stack_before,stack_after)
            before=vdp.read(backend['shared_state'],len(initial))
            vdp.transaction(3,word(vdp.handle),3,0);vdp.send((directory/'native-cleanup.bin').read_bytes())
            assert vdp.read(backend['shared_state'],len(initial))==before
            report['packages'].append({'name':name,'module_sha256':sha(directory/(image_name+'.vdpl')),
                                       'entry_address':hex(address),'calls':count,'max_float_ulps':worst,
                                       'coordinates_checked':coordinates,'stack_before':stack_before,
                                       'stack_after':stack_after,'language_statuses':sorted(statuses),
                                       'cleanup_preserves_shared_state':True})
            print(engine,name,count,'calls verified; stack floor',stack_after,flush=True)
        passed=True
    except BaseException as exc:
        failure=f'{type(exc).__name__}: {exc}';raise
    finally:
        vdp.close();report.update(passed=passed,failure=failure,elapsed_seconds=round(time.monotonic()-vdp.start,3),
                                  outbound_bytes=vdp.sent,replies=len(vdp.observed)//10,reply_bytes=len(vdp.observed))
        stats=json.loads((output/'stats.json').read_text());report['backend_stats']=stats
        if passed:assert not any(stats.get(key) for key in ('failed','unhandled','unregistered','unmapped')),stats
        (HERE/'evidence'/('target-'+engine+'.json')).write_text(json.dumps(report,indent=2)+'\n')
        if passed:print(json.dumps(report,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine',choices=('jit','interp'),default='jit');parser.add_argument('--timeout',type=float,default=600)
    parser.add_argument('--build-only',action='store_true')
    parser.add_argument('--skip-build',action='store_true');parser.add_argument('--backend',type=Path);parser.add_argument('--firmware',type=Path)
    args=parser.parse_args()
    if not args.skip_build:build_packages()
    if not args.build_only:qualify(args.engine,args.timeout,args.backend,args.firmware)
