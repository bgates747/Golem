#!/usr/bin/env python3
"""Build the isolated unchanged-transform Corsair line-batch consumer."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SDK=ROOT.parent/'agon-vdplet'
PEER=ROOT.parent/'agon-fsim/tests/0108-vdplet-wireframe'
BUILD=HERE/'build'
sys.path.insert(0,str(ROOT.parent/'agon-utils/examples/agnb/api'))
from buffers import BufferRecord,build_container
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
word=lambda n:struct.pack('<H',n)


def invoke(entry):
    body=struct.pack('<BBHBBHHH',1,2,0,0,0,0,entry,4)
    return bytes((23,0,216))+word(len(body))+body+bytes((23,0,195))


def main():
    BUILD.mkdir(exist_ok=True)
    pins=json.loads((SDK/'docs/tasks/VDPL-003/fixture-pins.json').read_text())
    for path,wanted in pins['checked_file_sha256'].items():assert sha(PEER/path)==wanted,path
    shutil.copyfile(PEER/'cpp/transform.cpp',BUILD/'transform-source.cpp')
    shutil.copyfile(PEER/'corsair/cpp/build/layout.h',BUILD/'layout.h')
    (BUILD/'draw-layout.h').write_text('static constexpr unsigned EDGE_ID=59020, EDGE_COUNT=4903;\n')
    subprocess.run([str(SDK/'.venv/bin/python'),str(SDK/'tools/build_module.py'),str(HERE/'worker.cpp'),
                    '--math','--capabilities','13','--name','worker','--build-id','0x4c494e32','--output',str(BUILD)],check=True,stdout=subprocess.DEVNULL)
    manifest=json.loads((PEER/'corsair/build/manifest.json').read_text())
    edges=manifest['source']['edges'];assert len(edges)==4903
    packed=b''.join(struct.pack('<HH',*edge) for edge in edges)
    assert hashlib.sha256(packed).hexdigest()==pins['packed_inputs']['edge_pairs_uint16_le']['sha256']
    native=PEER/'corsair/build/native'
    records=[BufferRecord(1,(native/'state.bin').read_bytes()),BufferRecord(2,(native/'array-objectXYZ.bin').read_bytes()),
             BufferRecord(3,(native/'array-projectedXYZ.bin').read_bytes()),BufferRecord(4,word(1)),
             BufferRecord(59020,packed),BufferRecord(59000,(BUILD/'worker.vdpl').read_bytes()),
             BufferRecord(59021,invoke(1)),BufferRecord(59022,invoke(2))]
    (BUILD/'scene.agnb').write_bytes(build_container(records))
    poses=[(20,30,0,0,0,0,3),(5,65,20,0,0,0,3),(20,30,0,15,20,0,3),
           (20,30,0,-20,-30,0,3),(20,30,0,0,0,0,2),(20,30,0,0,0,0,6)]
    rows=[','.join(repr(float(x))+'f' for x in (*pose[:6],1/pose[6],pose[6]*8192)) for pose in poses]
    include=HERE/'include/generated';include.mkdir(parents=True,exist_ok=True)
    config='''#pragma once
static constexpr uint16_t STATE_ID=1,CONTROLS_OFFSET=112,ARGUMENT_ID=4,MODULE_ID=59000,NATIVE_FRAME_ID=59022,BASELINE_FRAME_ID=59021;
static constexpr unsigned REVIEW_FRAMES=120;
'''
    config+=f'static constexpr uint32_t MODULE_BYTES={(BUILD/"worker.vdpl").stat().st_size};\n'
    config+='static const uint16_t OWNED_IDS[]={'+','.join(str(x.buffer_id) for x in records)+'};\n'
    config+='static const float REVIEW_POSES[6][8]={{'+'},{'.join(rows)+'}};\n'
    (include/'config.h').write_text(config)
    env=dict(os.environ,PATH='/Agon/agondev/release/bin:'+os.environ['PATH'])
    for name,bench,end in [('review',0,1),('benchmark-vdu',1,0),('benchmark-lines',1,1)]:
        subprocess.run(['make','clean'],cwd=HERE,env=env,check=True,stdout=subprocess.DEVNULL)
        with (BUILD/(name+'-build.log')).open('w') as log:
            subprocess.run(['make',f'REVIEW_BENCHMARK={bench}',f'BENCH_END_LINES={end}'],cwd=HERE,env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
        shutil.copyfile(HERE/'bin/lines_review.bin',BUILD/(name+'.bin'))
    report={'fixture_pins_verified':True,'source_owner_unchanged':True,'vertices':2685,'edges':4903,
            'chunk_edges':32,'submits_per_frame':154,'four_draw_leases':True,'scene_bytes':(BUILD/'scene.agnb').stat().st_size,
            'transform_sha256':sha(BUILD/'transform-source.cpp'),
            'module':json.loads((BUILD/'worker.json').read_text())['module']}
    (HERE/'build-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Built original-transform worker and all three isolated application variants.')


if __name__=='__main__':main()
