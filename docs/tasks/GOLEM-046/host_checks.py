#!/usr/bin/env python3
"""Optional integration: compiler/AGNB/embedded-state parity and real consumers."""
from pathlib import Path
import importlib.util
import json
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
sys.path.insert(0,str(ROOT/'tools'))
from package_agnb import package
sys.path.insert(0,str(ROOT.parent/'agon-utils/examples/agnb/api'))
from buffers import read_container


def run(*args,**kw):
    return subprocess.run([str(a) for a in args],check=True,**kw)


def install(raw,state=None):
    """Independent installer-only model; never interpret arbitrary procedure data."""
    state={} if state is None else state
    at=0;effects=[]
    while at<len(raw):
        if raw[at:at+7]==bytes((23,0,248,1,0,1,0)):
            effects.append(('affine',));at+=7;continue
        assert raw[at:at+3]==bytes((23,0,160)),(at,raw[at:at+30].hex())
        id=int.from_bytes(raw[at+3:at+5],'little');op=raw[at+5];at+=6
        if op==2:state.pop(id,None)
        elif op==0:
            n=int.from_bytes(raw[at:at+2],'little');at+=2
            assert n and at+n<=len(raw)
            state[id]=state.get(id,b'')+raw[at:at+n];at+=n
        elif op==5:
            assert raw[at]==0xc2
            offset=int.from_bytes(raw[at+1:at+3],'little');n=int.from_bytes(raw[at+3:at+5],'little');at+=5
            b=bytearray(state[id]);assert offset+n<=len(b)
            b[offset:offset+n]=raw[at:at+n];state[id]=bytes(b);at+=n
        elif op==33:
            assert raw[at]==0;at+=1
            state[id]=b'typed-identity-4x4'
        else:raise AssertionError((id,op,at))
    return state,effects


def main():
    BUILD.mkdir(exist_ok=True)
    floor='uint16 calls=3 float32 x Range(-32768,32767)=-0.75 sint16 result Sub compute calls=calls+1 result=sint16(floor(x)) EndSub'
    table='Record Sample uint16 tag sint16 delta EndRecord Table Sample samples(0:3) Sample selected=Sample(65535,-32768) uint16 index=0 Sub fetch selected=samples(index) EndSub'
    draw='uint8 colour Bounds(0,63)=15 sint16 x=40 sint16 y=60 Sub paint draw_line(colour,x,y,200,180) EndSub'
    bitmap='uint16 bitmapBuffer Bounds(1,65534)=8202 sint16 x=96 sint16 y=16 Sub show draw_bitmap(bitmapBuffer,x,y) EndSub'
    (BUILD/'records.dat').write_bytes(bytes(range(16)))
    cases=[('scalar',['--resident'],floor),('table',['--resident','--table-data','samples='+str(BUILD/'records.dat')],table),
           ('drawing',['--resident'],draw),('bitmap',['--resident','--reserve','8202:8202'],bitmap),
           ('matrix',['--stock-matrix','--reserve','1000:1001'],(ROOT/'docs/tasks/GOLEM-045/example.golem').read_text()),
           ('portable',['--portable'],(ROOT/'examples/portable/counter.golem').read_text()),
           ('arithmetic',['--portable'],'uint16 a=900 uint16 b=7 uint16 q=0 uint16 r=0 uint16 p=0 uint32 wide=100000 uint32 divisor=17 uint32 wq=0 uint32 wr=0 Sub tick p=a*b (q,r)=divmod(a,b) (wq,wr)=divmod(wide,divisor) wide=wide*divisor EndSub'),
           ('native',['--portable','--target','native'],'float32 x=1 float32 y=0 Sub tick y=sin(x) EndSub')]
    comparisons=0
    for name,flags,source in cases:
        path=BUILD/(name+'.golem');path.write_text(source)
        for relocated in (False,True):
            extra=['--reserve','1:254'] if relocated else []
            label=name+('-relocated' if relocated else '')
            old=BUILD/(label+'-embedded');new=BUILD/label
            run(ROOT/'build/golemc',*flags,*extra,path,old,stdout=subprocess.DEVNULL)
            run(ROOT/'build/golemc',*flags,*extra,'--emit-load-plan',path,new,stdout=subprocess.DEVNULL)
            for artifact in old.iterdir():
                assert artifact.read_bytes()==(new/artifact.name).read_bytes(),artifact.name
                comparisons+=1
            receipt=package(new)
            plan=json.loads((new/'load-plan.json').read_text())
            assert 'package_bootstrap' not in (new/'agnb.inc').read_text()
            assert 'common-install.bin' not in (new/'agnb.inc').read_text()
            for c in plan['components']:
                record_list=read_container((new/('program-'+c['name']+'.agnb')).read_bytes())
                records={r.buffer_id:r.data for r in record_list}
                assert records=={r['buffer_id']:(new/r['path']).read_bytes() for r in c['records']}
                setup=records.get(c['setup_entry'],b'')
                state={id:b for id,b in records.items() if id not in (c['setup_entry'],c['cleanup_entry'])}
                state,effects=install(setup,state)
                embedded=(old/'bootstrap.bin').read_bytes() if plan['profile']!='portable' else (old/'common-install.bin').read_bytes()+(old/(c['name']+'-install.bin')).read_bytes()
                wanted,wanted_effects=install(embedded)
                assert state==wanted,(label,c['name'],set(state)^set(wanted))
                assert effects==wanted_effects,(label,effects,wanted_effects)
                cleaned,_=install(records[c['cleanup_entry']],dict(state))
                assert not cleaned,(label,cleaned.keys())
            comparisons+=len(plan['components'])
    # A real six-byte wrapper + generic loader, no payloads embedded in app.
    assembler=ROOT/'docs/tasks/GOLEM-044/native-math/build/assembler/bin/ez80asm'
    guest=''' .assume adl=1
 .org $40000
 call golem_agnb_buffered_load
 ret nz
 call golem_agnb_buffered_setup
 call golem_buffered_step
 call golem_agnb_buffered_cleanup
 ret
 include "portable/agnb.inc"
 include "../../../../../agon-utils/examples/agnb/api/agnb_dependencies.inc"
 include "../../../../../agon-utils/examples/agnb/api/agnb_api.inc"
'''
    (BUILD/'consumer.asm').write_text(guest)
    run(assembler,'consumer.asm','consumer.bin','-c',cwd=BUILD,stdout=subprocess.DEVNULL)
    (BUILD/'consumer.cpp').write_text('#include "portable/agnb.h"\nstatic_assert(GOLEM_FIELD_total_BYTES==2);\nvoid calls(){golem_native_step();golem_agnb_native_bind(9); }\nint main(){return golem_agnb_buffered_load();}\n')
    run('c++','-std=c++17','-Wall','-Wextra','-Werror','-c','consumer.cpp','-o','consumer.o',cwd=BUILD)
    # Invalid record/package admission leaves existing container bytes untouched.
    directory=BUILD/'portable';path=directory/'load-plan.json';original=path.read_text()
    before=(directory/'program-buffered.agnb').read_bytes();rejected=0
    def reject(change):
        nonlocal rejected
        p=json.loads(original);change(p);path.write_text(json.dumps(p))
        try:package(directory)
        except (ValueError,OSError):rejected+=1
        else:raise AssertionError('invalid package accepted')
        assert (directory/'program-buffered.agnb').read_bytes()==before
    try:
        reject(lambda p:p.update(schema=9))
        reject(lambda p:p.update(components=[]))
        reject(lambda p:p['components'][0]['records'][0].update(path='../../../../README.md'))
        reject(lambda p:p['components'][0]['records'][0].update(bytes=999999))
        reject(lambda p:p['components'][0]['records'][0].update(buffer_id=65534))
        reject(lambda p:p['components'][0]['records'].append(p['components'][0]['records'][0]))
        reject(lambda p:p['components'][0]['owned'].append(p['components'][0]['owned'][0]))
    finally:path.write_text(original)
    # The packager treats an SDK image as opaque bytes at the assigned staging ID.
    # Actual Xtensa admission/execution is the separate target qualification.
    image=BUILD/'opaque-image.dat';image.write_bytes(b'opaque-image-fixture')
    receipt=package(directory,module=image,backend='native')
    c=next(c for c in json.loads(original)['components'] if c['name']=='native')
    records={r.buffer_id:r.data for r in read_container((directory/'program-native.agnb').read_bytes())}
    assert records[c['module_buffer']]==image.read_bytes() and receipt['packages'][0]['module_included']
    package(directory) # Restore the module-free host fixture.
    evidence={'embedded_artifact_and_component_comparisons':comparisons,'profiles':len(cases),'relocated':True,'package_rejections':rejected,'real_assembler':True,'cxx_header':True}
    (HERE/'host-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print('AGNB compiler integration passes:',evidence)


if __name__=='__main__':main()
