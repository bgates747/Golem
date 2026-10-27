#!/usr/bin/env python3
"""Bounded actual-stock loop proof; all outputs stay in this task silo.

The imported transport is an immutable, hashed task-local copy of the peer's
VDP-only probe driver. No profile, firmware, application or GUI is changed.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import struct
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
COMPILER=ROOT/'build/golemc'
OLD=BUILD/'baseline-source/build/golemc'


def compile_program(name,count,start=0,end=None,old=False):
    end=count-1 if end is None else end
    output=1004 if old else 1003
    text=('' if old else 'uint16 vertex = 9\n')+f'''Matrix(3,3) base = Identity
Matrix(3,3) a = Identity
Borrowed XYZF32 clip = Buffer(1000,{count})
Borrowed XY16 zero = Buffer(1001,1)
Borrowed XY16 pixel = Buffer(1002,1)
Borrowed XYZ16 screen = Buffer({output},{count})
Sub project
'''
    def body(index):
        at=lambda stride,bias: str(stride*index+bias) if old else f'{stride}*vertex'+(f'+{bias}' if bias else '')
        return f'''matrix_set_value(a,base,0,0,read_float32(clip,{at(12,8)}))
matrix_set_value(a,a,1,1,read_float32(clip,{at(12,8)}))
matrix_set_value(a,a,0,2,read_float32(clip,{at(12,0)}))
matrix_set_value(a,a,1,2,read_float32(clip,{at(12,4)}))
matrix_invert(a)
transform_vertices(pixel,a,zero)
copy_bytes(screen,{at(6,0)},pixel,0,4)
'''
    if old:
        chunks=list(range(start,end+1,128))
        text+=''.join(f'chunk{i}()\n' for i in range(len(chunks)))+'EndSub\n'
        for i,first in enumerate(chunks):
            text+=f'Sub chunk{i}\n'+''.join(body(v) for v in range(first,min(first+128,end+1)))+'EndSub\n'
    else:text+=f'For vertex In Range({start}:{end})\n'+body(start)+'Next vertex\nEndSub\n'
    source=BUILD/(name+'.golem');source.write_text(text)
    destination=BUILD/name
    # Both packages and all shared borrowed/diagnostic data IDs are disjoint.
    args=[OLD if old else COMPILER,'--stock-matrix','--emit-load-plan','--reserve','1:3000' if old else '1:2000','--reserve','65000:65000',source,destination]
    subprocess.run(list(map(str,args)),check=True,capture_output=True)
    return destination,json.loads((destination/'interface.json').read_text())


def main():
    pins=json.loads((HERE/'target-inputs.json').read_text())
    for item in pins.values():assert hashlib.sha256(Path(item['path']).read_bytes()).hexdigest()==item['sha256']
    spec=importlib.util.spec_from_file_location('stock_transport',pins['transport_copy']['path'])
    mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
    mod.FIRMWARE=Path(pins['firmware']['path']);mod.BACKEND=Path(pins['adapter']['path'])
    vdp=mod.StockVDP(BUILD/'target',timeout=600)
    reports=[]
    try:
        for count,start,end in ((1,0,0),(30,0,29),(2685,0,2684),(30,3,17)):
            label=f'n{count}-{start}-{end}'
            loop,meta=compile_program(label+'-loop',count,start,end)
            old,legacy=compile_program(label+'-unrolled',count,start,end,True)
            vdp.send((loop/'bootstrap.bin').read_bytes());vdp.send((old/'bootstrap.bin').read_bytes());vdp.barrier()
            layout=meta['loops'][0]
            assert not set(meta['owned'])&set(legacy['owned'])
            per_case=[]
            for repeat in range(2):
                floats=[]
                for i in range(count):
                    w=1.0+(i%17)*0.125
                    u=100.25+(i%997)*0.5+repeat*0.25
                    v=80.75+(i%701)*0.5+repeat*0.25
                    floats.extend((-u*w,-v*w,w))
                original=b''.join(struct.pack('<HHh',0x1234,0x5678,(i*7)%30000) for i in range(count))
                vdp.send(mod.upload(1000,struct.pack('<'+'f'*len(floats),*floats)))
                for key,data in ((1001,b'\0'*4),(1002,b'\0'*4),(1003,original),(1004,original)):
                    vdp.send(mod.upload(key,data))
                if repeat:
                    # Direct qualification poison; production never changes these.
                    vdp.send(mod.upload(layout['counter_buffer'],b'\xff'*3))
                    state_bytes=layout['state_bytes']
                    vdp.send(mod.upload(layout['state_buffer'],b'\xff'*state_bytes))
                vdp.send((loop/'invoke.bin').read_bytes());vdp.barrier()
                observed=vdp.read(1003,count*6)
                vdp.send((old/'invoke.bin').read_bytes());vdp.barrier()
                reference=vdp.read(1004,count*6)
                assert observed==reference,(label,repeat,'loop/unrolled mismatch')
                for i in range(count):
                    assert observed[6*i+4:6*i+6]==original[6*i+4:6*i+6]
                    if not start<=i<=end:assert observed[6*i:6*i+6]==original[6*i:6*i+6]
                counter=vdp.read(layout['counter_buffer'],3)
                assert struct.unpack('<H',counter[:2])[0]==end and counter[2]==0
                state=vdp.read(layout['state_buffer'],layout['state_bytes'])
                assert all(state[8+3*i]==0 for i in range(layout['cursors']))
                per_case.append(dict(repeat=repeat,exact_output_bytes=len(observed),retained_z=True,counter=end,guards_zero=True))
            vdp.send((old/'cleanup.bin').read_bytes());vdp.send((loop/'cleanup.bin').read_bytes());vdp.barrier()
            # Borrowed data remains after both cleanups.
            assert vdp.read(1003,count*6)==observed
            reports.append(dict(records=count,start=start,end=end,loop_bootstrap_bytes=(loop/'bootstrap.bin').stat().st_size,
                                unrolled_bootstrap_bytes=(old/'bootstrap.bin').stat().st_size,owned_buffers=len(meta['owned']),
                                loop=layout,cases=per_case,cleanup_retains_borrowed=True))
        # Actual stock conditional comparison at UINT16_MAX, without a one-past update.
        source=BUILD/'endpoint.golem';source.write_text('uint16 i = 7 Matrix(3,3) m = Identity Sub tick For i In Range(65534:65535) matrix_identity(m) Next i matrix_identity(m) EndSub\n')
        output=BUILD/'endpoint';subprocess.run(list(map(str,[COMPILER,'--stock-matrix',source,output])),capture_output=True,check=True)
        meta=json.loads((output/'interface.json').read_text());vdp.send((output/'bootstrap.bin').read_bytes());vdp.send((output/'invoke.bin').read_bytes());vdp.barrier()
        assert vdp.read(meta['loops'][0]['counter_buffer'],3)==b'\xff\xff\0'
        vdp.send((output/'cleanup.bin').read_bytes());vdp.barrier()
        report=dict(passed=True,engine='actual unchanged stock2.16 / ARM64 VDP-only JIT',firmware_sha256=pins['firmware']['sha256'],
                    cases=reports,elapsed_seconds=round(time.monotonic()-vdp.started,3),uint16_endpoint_without_wrap=True,production_readback=False,physical=False,
                    mos_or_human_qualification=False,contract_sha256=hashlib.sha256((HERE/'contract.md').read_bytes()).hexdigest())
        (HERE/'stock-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report),flush=True)
    finally:vdp.close()

if __name__=='__main__':main()
