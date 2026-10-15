"""Native math/reference conformance; model trig accuracy is checked in ULPs."""
from pathlib import Path
import hashlib
import json
import math
import os
import random
import struct
import subprocess
import sys
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
SDK=ROOT.parent/'agon-vdplet'
ASSEMBLER=Path(os.environ.get('EZ80ASM',str(HERE/'build/assembler/bin/ez80asm'))).resolve()
sys.path.insert(0,os.environ.get('GOLEM_MATH_MODEL_DIR',str(ROOT/'tests/portable')))
from command_model import reference,float_bits,float_value


def run(args,**kwargs):
    result=subprocess.run(list(map(str,args)),capture_output=True,**kwargs)
    if result.returncode:raise RuntimeError(result.stdout.decode(errors='replace')+result.stderr.decode(errors='replace'))
    return result.stdout


def load(directory):
    return [json.loads((directory/name).read_text()) for name in ('interface.json','program.json','root.json','backends.json')]


def compare(want,got,interface,trig=False,flower=False):
    # Float outputs are bounded against an independent double-libm oracle.
    # Never hide root statuses/generations, inputs or integer arithmetic errors.
    assert len(got)==len(want)
    remaining=set(range(len(want)));worst=0
    for field in interface['globals']:
        at=field['offset'];size=field['bytes'];name=field['name']
        if field['type']=='float32' and ((trig and name=='result') or flower):
            a,b=struct.unpack_from('<I',want,at)[0],struct.unpack_from('<I',got,at)[0]
            fa,fb=float_value(a),float_value(b)
            assert math.isfinite(fb),(name,fb)
            ordered=lambda bits:~bits&0xffffffff if bits&0x80000000 else bits^0x80000000
            distance=abs(ordered(a)-ordered(b));worst=max(worst,distance)
            assert distance<=(8 if flower else 2),(name,fa,fb,distance)
            remaining.difference_update(range(at,at+size))
        elif flower and name in ('previousX','previousY','startX','startY'):
            a,b=struct.unpack_from('<h',want,at)[0],struct.unpack_from('<h',got,at)[0]
            assert abs(a-b)<=1,(name,a,b)
            remaining.difference_update(range(at,at+size))
    assert all(want[i]==got[i] for i in remaining),[(i,want[i],got[i]) for i in remaining if want[i]!=got[i]][:8]
    return worst


def cases(interface,initial,large=True):
    fields={f['name']:f for f in interface['globals']};entries={e['name']:e['logical_id'] for e in interface['entries']}
    result=[]
    def add(name,updates=None,raw=None):
        state=bytearray(initial);entry=entries[name];struct.pack_into('<H',state,28,entry)
        for field,value in (updates or {}).items():
            f=fields[field];bits=float_bits(value) if f['type']=='float32' else int(value)&65535
            struct.pack_into('<I' if f['bytes']==4 else '<H',state,f['offset'],bits)
        for field,bits in (raw or {}).items():struct.pack_into('<I',state,fields[field]['offset'],bits)
        result.append((entry,bytes(state),name))
    basis=[0.0,-0.0,0.5,-0.5,1.0,-1.0,math.pi/2,math.pi,2*math.pi,22.5,1000.0,1e20,3.4028234663852886e38,1.1754943508222875e-38]
    for name in ('sine','cosine','floorValue'):
        for a in basis:add(name,{'a':a})
    for a,b in [(1,3),(-1,3),(1,-3),(5,2),(0,2),(1,0),(1,-0.0),(1e38,0.5),
                (1e38,0.25),(1.1754943508222875e-38,2),(1e-30,1e30),(3,1.1754943508222875e-38)]:add('divide',{'a':a,'b':b})
    for a in [-32769,-32768.5,-32768,-1.25,-0.00006103515625,0,0.5,32767.5,32768,1e20]:add('floorConvert',{'a':a})
    for name in ('precedence','chain','boundedDivide','partial','operandOrder','skipped','nested','constants'):
        for updates in ({},{'a':8,'b':2,'c':2},{'a':1,'b':0},{'a':1e38,'b':4}):add(name,updates)
    for bits in (0x00000001,0x80000001,0x7f800000,0xff800000,0x7fc12345):add('sine',raw={'a':bits})
    add('partial',{'effect':65535});add('partial',{'a':1,'b':0,'whole':123})
    if large:
        rng=random.Random(44)
        for _ in range(128):
            a,b=rng.uniform(-10000,10000),rng.uniform(-1000,1000)
            add('divide',{'a':a,'b':b});add('sine',{'a':a});add('cosine',{'a':a});add('floorValue',{'a':a})
    return result


def fixture_header(out,interface,backend):
    arrays=interface.get('arrays',[])
    header=f"#define STATE_ID {backend['shared_state']}\n#define STATE_BYTES {interface['state_bytes']}\n#define ARGS_ID {backend['native_arguments']}\n#define ARRAY_COUNT {len(arrays)}\n"
    for name,values in [('ids',[a['buffer'] for a in arrays]),('bytes',[a['bytes'] for a in arrays]),('readonly',[a['access']==1 for a in arrays])]:
        header+='static const unsigned array_'+name+'[]={'+(','.join(str(int(x)) for x in values) or '0')+'};\n'
    (out/'fixture.h').write_text(header)


def build_host(out,interface,backend):
    fixture_header(out,interface,backend);runner=out/'math-host'
    run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-ffp-contract=off','-fno-builtin',
         '-fsanitize=address,undefined','-I'+str(SDK/'include'),'-I'+str(out),
         '-DGENERATED_SOURCE="'+str(out/'native.c')+'"',ROOT/'tests/portable/native_host.c','-lm','-o',runner])
    return runner


def check():
    started=time.monotonic();receipts=[]
    for name in ('probe','probe-relocated','flower'):
        out=HERE/'build'/name;out.mkdir(parents=True,exist_ok=True)
        command=[ROOT/'build/golemc','--portable','--target','native']
        if name.endswith('relocated'):command+=['--reserve','1:254']
        source=HERE/'probe.golem' if name.startswith('probe') else HERE.parent/'flower.golem'
        run(command+[source,out]);interface,ast,root,backend=load(out)
        runner=build_host(out,interface,backend);initial=(out/'state.bin').read_bytes()
        if name=='flower':
            arrays={a['buffer']:bytearray((out/a['initial_data']).read_bytes()) for a in interface['arrays']}
            state=initial;total=0;worst=0
            for entry in [0]+[1]*13:
                start=bytearray(state);struct.pack_into('<H',start,28,entry)
                wanted_arrays={k:bytearray(v) for k,v in arrays.items()}
                wanted=reference(interface,ast,start,entry,root,arrays=wanted_arrays)
                got=run([runner],input=struct.pack('<HH',entry,0)+start+b''.join(arrays.values()))
                assert struct.unpack_from('<i',got)[0]==0
                size=interface['state_bytes'];state=got[4:4+size]
                worst=max(worst,compare(wanted,state,interface,flower=True))
                at=4+size
                for a in interface['arrays']:
                    data=got[at:at+a['bytes']];at+=a['bytes']
                    actual=struct.unpack('<32h',data);expected=struct.unpack('<32h',wanted_arrays[a['buffer']])
                    assert all(abs(x-y)<=1 for x,y in zip(actual,expected))
                    arrays[a['buffer']]=bytearray(data)
                fields={f['name']:f for f in interface['globals']}
                if entry==1:total+=struct.unpack_from('<H',state,fields['batchCount']['offset'])[0]
            assert total==396 and struct.unpack_from('<H',state,fields['remaining']['offset'])[0]==0
            count=14
        else:
            selected=cases(interface,initial);payload=b''.join(struct.pack('<HH',entry,0)+state for entry,state,_ in selected)
            got=run([runner],input=payload);stride=4+interface['state_bytes'];assert len(got)==stride*len(selected)
            worst=0;statuses=set()
            for i,(entry,state,label) in enumerate(selected):
                wanted=reference(interface,ast,state,entry,root)
                result=got[i*stride:(i+1)*stride];runtime=struct.unpack_from('<i',result)[0]
                status=struct.unpack_from('<H',wanted,14)[0];statuses.add(status)
                assert runtime==(1 if status else 0),(label,status,runtime)
                worst=max(worst,compare(wanted,result[4:],interface,trig=label in ('sine','cosine','partial','nested','constants')))
            assert statuses=={0,1,4,32,33},statuses
            count=len(selected)
        consumer=' .assume adl=1\n .org $40000\n include "layout.inc"\n include "calls.inc"\n call golem_native_'+interface['entries'][0]['name']+'\n ret\n'
        (out/'consumer.asm').write_text(consumer)
        run([ASSEMBLER,'consumer.asm','consumer.bin','-c'],cwd=out)
        receipts.append({'package':name,'cases':count,'max_float_ulps':worst,'sanitized_generated_C':True,'actual_assembler':True})
    baseline=json.loads((HERE.parent/'evidence/math-baseline.json').read_text());parity=0
    for name in ('default','relocated','animation'):
        out=HERE/'build'/('parity-'+name);args=[ROOT/'build/golemc','--portable']
        if name=='relocated':args+=['--reserve','1:254']
        run(args+[ROOT/'tests/portable/fixtures'/name/'source.golem',out])
        for filename,digest in baseline[name].items():
            assert hashlib.sha256((out/filename).read_bytes()).hexdigest()==digest,(name,filename)
            parity+=1
    for name,src,cap,variant in [('numeric-1','numeric/numeric.c',1,1),('numeric-2','numeric/numeric.c',1,2),('packed-mono','packed_mono/packed_mono.cpp',3,1)]:
        out=HERE/'build/sdk-default'
        run([sys.executable,SDK/'tools/build_module.py',SDK/'examples'/src,'--name',name,'--capabilities',cap,'--variant',variant,'--output',out])
        assert hashlib.sha256((out/(name+'.vdpl')).read_bytes()).hexdigest()==baseline['sdk'][name+'.vdpl']
    report={'packages':receipts,'prior_files_unchanged':parity,'default_SDK_images_unchanged':3,
            'elapsed_seconds':round(time.monotonic()-started,3),'scope':'host/generated-C/assembler; not Xtensa execution'}
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True);(evidence/'host.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':check()
