#!/usr/bin/env python3
"""Build the ordinary eZ80 application with embedded, metadata-derived payloads."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import struct
import sys
import zlib

HERE=Path(__file__).resolve().parent
TASK=HERE.parent
ROOT=TASK.parents[2]
sys.path.insert(0,str(TASK));import generate_header
sys.path.insert(0,str(TASK/'renderer'));from build_renderer import generate,packet


def run(args,**kwargs):
    result=subprocess.run(list(map(str,args)),capture_output=True,text=True,**kwargs)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    return result.stdout


def build(relocated=False):
    out=HERE/'build'/('relocated' if relocated else 'default');out.mkdir(parents=True,exist_ok=True)
    reference=(TASK/'renderer/evidence/default-defaults-jit.png').read_bytes()
    at=8;compressed=b''
    while at<len(reference):
        size=struct.unpack_from('>I',reference,at)[0]
        if reference[at+4:at+8]==b'IDAT':compressed+=reference[at+8:at+8+size]
        at+=size+12
    rows=zlib.decompress(compressed);assert len(rows)==480*1921
    assert all(rows[y*1921]==0 for y in range(480))
    (HERE/'build/reference-default.rgb').write_bytes(b''.join(rows[y*1921+1:(y+1)*1921] for y in range(480)))
    shutil.copyfile(ROOT.parent/'agon-dev-env/scripts/setup_emulator.py',HERE/'build/setup_emulator.py')
    shutil.copyfile(ROOT.parent/'agon-dev-env/scripts/flexe_profile.py',HERE/'build/flexe_profile.py')
    firmware=ROOT.parent/'agon-vdplet/build/firmware/firmware-src/.pio/build/esp32dev'
    if (firmware/'firmware.bin').exists() and (firmware/'firmware.elf').exists():
        inputs=HERE/'build/runtime-inputs';inputs.mkdir(exist_ok=True)
        for name in ('firmware.bin','firmware.elf'):shutil.copyfile(firmware/name,inputs/name)
    args=[ROOT/'build/golemc','--portable','--target','native']
    if relocated:args+=['--reserve','1:4095']
    run(args+[TASK/'flower.golem',out])
    render=generate(out,out,(60002,60003,60004,60005))
    # The bridge uses logical coordinates; its command bytes are mode independent.
    render['screen']={'mode':19,'width':1024,'height':768,'foreground':'white','background':'black'}
    (out/'renderer.json').write_text(json.dumps(render,indent=2)+'\n')
    sdk=ROOT.parent/'agon-vdplet'
    run([sys.executable,sdk/'tools/build_module.py',out/'native.c','--math','--name','flower','--build-id','0x474c3434','--output',out])
    interface=json.loads((out/'interface.json').read_text());backend=json.loads((out/'backends.json').read_text())
    fields={f['name']:f for f in interface['globals']}
    names=['petals','vectors','depth','periods','shrink','radius_scale','theta_init']
    first=fields[names[0]]['offset']
    assert all(fields[name]['offset']==first+i*4 and fields[name]['type']=='float32' for i,name in enumerate(names))
    headers=HERE/'include/generated';headers.mkdir(parents=True,exist_ok=True)
    generate_header.generate(out,headers/'flower_golem.h')
    lines=['/* Generated application package: filenames and IDs resolved by this build. */',
           '#ifndef FLOWER_PACKAGE_H','#define FLOWER_PACKAGE_H','#include "flower_golem.h"',
           '#define FLOWER_STAGING 60010u',f'#define FLOWER_INPUT_OFFSET {first}u','#define FLOWER_INPUT_BYTES 28u',
           f'#define FLOWER_RENDER {render["entry"]}u',f'#define FLOWER_ARGUMENTS {backend["native_arguments"]}u',
           f'#define FLOWER_REMAINING_OFFSET {fields["remaining"]["offset"]}u',
           f'#define FLOWER_COUNT_OFFSET {fields["batchCount"]["offset"]}u',
           'static const uint16_t flower_wrappers[]={'+','.join(map(str,backend['native_wrappers']))+'};']
    blobs={name:(out/filename).read_bytes() for name,filename in [
        ('common_install','common-install.bin'),('native_install','native-install.bin'),
        ('native_cleanup','native-cleanup.bin'),('renderer_install','renderer-install.bin'),
        ('renderer_cleanup','renderer-cleanup.bin'),('module','flower.vdpl')]}
    blobs['common_cleanup']=b''.join(packet(key,2) for key in [backend['shared_state']]+[a['buffer'] for a in interface['arrays']]+[60010])
    for name,data in blobs.items():
        lines += [f'static const uint8_t flower_{name}[]={{']
        lines += [','.join(str(b) for b in data[i:i+24])+',' for i in range(0,len(data),24)]
        lines += ['};']
    lines += ['#endif',''];(headers/'flower_package.h').write_text('\n'.join(lines))
    env=dict(os.environ,PATH='/Agon/agondev/release/bin:'+os.environ['PATH'])
    run(['make','clean'],cwd=HERE,env=env)
    log=run(['make'],cwd=HERE,env=env);(out/'target-build.log').write_text(log)
    binary=HERE/'bin/flower.bin'
    (out/'flower.bin').write_bytes(binary.read_bytes())
    report={'layout':'relocated' if relocated else 'default','binary_bytes':binary.stat().st_size,
            'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
            'module_sha256':hashlib.sha256(blobs['module']).hexdigest(),
            'state_buffer':backend['shared_state'],'renderer':render,
            'embedded_module':True,'curve_math_on_eZ80':False}
    evidence=HERE/'evidence';evidence.mkdir(exist_ok=True)
    (evidence/(report['layout']+'-build.json')).write_text(json.dumps(report,indent=2)+'\n')
    if not relocated:
        # Qualification only: leave an unrelated slot occupied before the client.
        image=out/'flower.vdpl';size=image.stat().st_size
        load=bytes((23,0,216))+struct.pack('<HBBHBBHIH',14,1,1,0,0,0,60010,size,0)
        asm=''' .assume adl=1
 .org $40000
 jp start
 .align 64
 db "MOS",0,1
start:
 push af
 push bc
 push de
 push hl
 push ix
 push iy
 ld hl,wire
 ld bc,wire_end-wire
 rst.lil $18
 pop iy
 pop ix
 pop hl
 pop de
 pop bc
 pop af
 ld hl,0
 ret
wire:
'''
        data=packet(60010,2)+packet(60010,0)+struct.pack('<H',size)
        asm+=' db '+','.join(map(str,data))+'\n incbin "'+str(image)+'"\n'
        asm+=' db '+','.join(map(str,load))+'\nwire_end:\n'
        source=HERE/'build/busy.asm';source.write_text(asm)
        assembler=Path(os.environ.get('EZ80ASM',str(TASK/'native-math/build/assembler/bin/ez80asm')))
        run([assembler,source,HERE/'build/busy.bin','-c'])
    print(json.dumps({k:report[k] for k in ('layout','binary_bytes','binary_sha256','state_buffer')},indent=2))


if __name__=='__main__':build('--relocated' in sys.argv)
