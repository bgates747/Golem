#!/usr/bin/env python3
"""Build an isolated consumer from frozen geometry and transform source."""
from pathlib import Path
import hashlib,importlib.util,json,os,re,shutil,struct,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];SDK=ROOT.parent/'agon-vdplet'
H=ROOT.parent/'agon-fsim/tests/0108-vdplet-wireframe';BUILD=HERE/'build'
sys.path.insert(0,str(ROOT.parent/'agon-utils/examples/agnb/api'))
from buffers import BufferRecord,build_container
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
word=lambda n:struct.pack('<H',n)
cmd=lambda n,op:bytes((23,0,160))+word(n)+bytes((op,))
EDGE=59020;BASELINE=59001;NATIVE=59021;STAGING=59000

def invoke(entry):
    body=struct.pack('<BBHBBHHH',1,2,0,0,0,0,entry,4)
    return bytes((23,0,216))+word(len(body))+body

def main():
    BUILD.mkdir(exist_ok=True)
    pins=json.loads((SDK/'docs/tasks/VDPL-003/fixture-pins.json').read_text())
    for path,wanted in pins['checked_file_sha256'].items():assert sha(H/path)==wanted,path
    shutil.copyfile(H/'cpp/transform.cpp',BUILD/'transform-source.cpp')
    shutil.copyfile(H/'corsair/cpp/build/layout.h',BUILD/'layout.h')
    (BUILD/'draw-layout.h').write_text(f'static constexpr unsigned EDGE_ID={EDGE}, EDGE_COUNT=4903;\n')
    subprocess.run([str(SDK/'.venv/bin/python'),str(SDK/'tools/build_module.py'),str(HERE/'worker.cpp'),'--math','--capabilities','5','--name','worker','--build-id','0x56445532','--output',str(BUILD)],check=True,stdout=subprocess.DEVNULL)
    manifest=json.loads((H/'corsair/build/manifest.json').read_text())
    source=manifest['source'];edges=source['edges'];assert len(edges)==4903
    data=b''.join(struct.pack('<HH',*e) for e in edges)
    assert hashlib.sha256(data).hexdigest()==pins['packed_inputs']['edge_pairs_uint16_le']['sha256']
    # Same exact reference generator, isolated import without executing its build.
    spec=importlib.util.spec_from_file_location('frozen_draw',H/'drawing.py');draw=importlib.util.module_from_spec(spec);spec.loader.exec_module(draw)
    installation,entries=draw.generate(edges,3,0)
    (BUILD/'baseline-install.bin').write_bytes(installation)
    (BUILD/'baseline-frame.bin').write_bytes(b''.join(cmd(e,1) for e in entries)+bytes((23,0,195)))
    (BUILD/'native-frame.bin').write_bytes(invoke(1)+bytes((23,0,195)))
    native=H/'corsair/build/native'
    common=[BufferRecord(1,(native/'state.bin').read_bytes()),BufferRecord(2,(native/'array-objectXYZ.bin').read_bytes()),BufferRecord(3,(native/'array-projectedXYZ.bin').read_bytes()),BufferRecord(4,word(1)),BufferRecord(EDGE,data),BufferRecord(STAGING,(BUILD/'worker.vdpl').read_bytes()),BufferRecord(NATIVE,(BUILD/'native-frame.bin').read_bytes())]
    (BUILD/'scene.agnb').write_bytes(build_container(common))
    # Decode only this frozen fixture writer's clear/write/consolidate envelope.
    # The bytes and IDs are source-owned test inputs, not compiler inference.
    at=0;buffers={}
    while at<len(installation):
        assert installation[at:at+3]==bytes((23,0,160))
        key,op=struct.unpack_from('<HB',installation,at+3);at+=6
        if op==2:buffers[key]=bytearray()
        elif op==0:
            size=struct.unpack_from('<H',installation,at)[0];at+=2
            buffers[key]+=installation[at:at+size];at+=size
        else:assert op==14,op
    baseline_records=[BufferRecord(key,bytes(value)) for key,value in buffers.items()]
    baseline_records.append(BufferRecord(BASELINE,(BUILD/'baseline-frame.bin').read_bytes()))
    (BUILD/'baseline.agnb').write_bytes(build_container(baseline_records))
    poses=[(20,30,0,0,0,0,3),(5,65,20,0,0,0,3),(20,30,0,15,20,0,3),(20,30,0,-20,-30,0,3),(20,30,0,0,0,0,2),(20,30,0,0,0,0,6)]
    include=HERE/'include/generated';include.mkdir(parents=True,exist_ok=True)
    rows=[','.join(repr(float(x))+'f' for x in (*pose[:6],1/pose[6],pose[6]*8192)) for pose in poses]
    owned=sorted(set([record.buffer_id for record in common]+[record.buffer_id for record in baseline_records]))
    config=f'#pragma once\nstatic constexpr uint16_t STATE_ID=1,CONTROLS_OFFSET=112,ARGUMENT_ID=4,MODULE_ID={STAGING},NATIVE_FRAME_ID={NATIVE},BASELINE_FRAME_ID={BASELINE};\nstatic constexpr uint32_t MODULE_BYTES={(BUILD/"worker.vdpl").stat().st_size};\nstatic constexpr unsigned REVIEW_FRAMES=24;\nstatic const uint16_t OWNED_IDS[]={{'+','.join(map(str,owned))+'};\nstatic const float REVIEW_POSES[6][8]={{'+'},{'.join(rows)+'}};\n'
    (include/'config.h').write_text(config)
    env=dict(os.environ,PATH='/Agon/agondev/release/bin:'+os.environ['PATH'])
    for name,bench,end in [('review',0,1),('benchmark-resident',1,0),('benchmark-submit',1,1)]:
        subprocess.run(['make','clean'],cwd=HERE,env=env,check=True,stdout=subprocess.DEVNULL)
        with (BUILD/(name+'-build.log')).open('w') as log:
            subprocess.run(['make',f'REVIEW_BENCHMARK={bench}',f'BENCH_END_SUBMIT={end}'],cwd=HERE,env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
        shutil.copyfile(HERE/'bin/vdu_review.bin',BUILD/(name+'.bin'))
    (BUILD/'transform-original.vdpl').write_bytes((H/'corsair/cpp/build/native/transform.vdpl').read_bytes())
    report={'geometry_and_transform_pins_checked':True,'vertices':2685,'edges':4903,'edge_bytes':len(data),'chunk_bytes':384,'submits':154,'raw_draw_bytes':4903*12,'baseline_resident_command_bytes':4903*36,'baseline_install_bytes':len(installation),'scene_bytes':(BUILD/'scene.agnb').stat().st_size,'transform_source_sha256':sha(BUILD/'transform-source.cpp'),'module':json.loads((BUILD/'worker.json').read_text())['module'],'source_owner_unchanged':True}
    (HERE/'build-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Built pinned Corsair consumer, module',report['module']['package_bytes'],'bytes; scene',report['scene_bytes'],'bytes.')
if __name__=='__main__':main()
