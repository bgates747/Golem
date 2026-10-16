#!/usr/bin/env python3
"""Verify final receipts/wire budgets and encode unchanged framebuffer pixels."""
from pathlib import Path
import binascii
import hashlib
import json
import struct
import zlib

HERE=Path(__file__).resolve().parent
EVIDENCE=HERE/'evidence/mode19'
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()


def png(path,width,height,rgb):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',binascii.crc32(kind+data))
    rows=b''.join(b'\0'+rgb[y*width*3:(y+1)*width*3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
                     +chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))


def records(data):
    result=[];at=0
    while at<len(data):
        start=at
        if data[at:at+3]==bytes((23,0,160)):
            key,op=struct.unpack_from('<HB',data,at+3)
            if op in (1,2):size=6;kind='call' if op==1 else 'clear'
            elif op==0:size=8+struct.unpack_from('<H',data,at+6)[0];kind='write'
            elif op==5:
                assert data[at+6]==0xc2,'unexpected host buffer operation'
                size=11+struct.unpack_from('<H',data,at+9)[0];kind='update'
            else:raise AssertionError(('unexpected host buffer command',op))
            result.append((kind,key,start,size));at+=size
        elif data[at:at+3]==bytes((23,0,216)):
            size=5+struct.unpack_from('<H',data,at+3)[0]
            result.append(('request',data[at+6],start,size));at+=size
        elif data[at:at+3]==bytes((23,0,132)):
            result.append(('plot-barrier',0,start,7));at+=7
        else:at+=1
    return result


def summarize():
    builds={name:json.loads((HERE/'evidence'/(name+'-build.json')).read_text()) for name in ('default','relocated')}
    receipts=[];pixels=0;wire_batches=0
    for path in sorted(EVIDENCE.glob('*/receipt.json')):
        receipt=json.loads(path.read_text());assert receipt['passed'] and receipt['frontend_exit_status']==0,path
        layout='relocated' if 'relocated' in path.parent.name else 'default'
        identities=dict(line.split('  ',1)[::-1] for line in receipt['identities'])
        assert identities['sdcard/flower/flower.bin']==builds[layout]['binary_sha256'],path
        assert identities['sdcard/flower/main.cpp']==sha(HERE/'src/main.cpp'),path
        pixels+=sum(c.get('exact_pixels',0) for c in receipt['checks'])
        wire=path.parent/'mos-to-vdp.bin'
        parsed=records(wire.read_bytes()) if wire.exists() and not receipt['stock'] else []
        wrapper=builds[layout]['renderer']['batch_native_wrapper']
        render=builds[layout]['renderer']['entry'];count=0
        for index,record in enumerate(parsed):
            if record[:2]!=('call',wrapper):continue
            block=parsed[index:index+5]
            if [(r[0],r[1]) for r in block]==[('call',wrapper),('request',4),('call',render),('request',5),('plot-barrier',0)]:
                assert sum(r[3] for r in block)==49
                assert block[-1][2]+7-record[2]==49,'extra wire bytes inside batch'
                count+=1
        if path.parent.name.startswith('workflow'):assert count==26,(path,count)
        if path.parent.name.startswith('busy'):
            assert sum(r[:2]==('request',1) for r in parsed)==1
            assert not any(r[:2]==('request',3) for r in parsed),'busy slot was unloaded'
        wire_batches+=count
        receipts.append({'scenario':path.parent.name,'engine':receipt['engine'],'elapsed_seconds':receipt['elapsed_seconds'],
                         'complete_49_byte_batches_observed':count,'native_MOS_dir_witness':receipt['returned_to_MOS_and_dir']})
        for image in path.parent.glob('*.ppm'):
            header,rgb=image.read_bytes().split(b'\n255\n',1)
            magic,dimensions=header.split(b'\n');assert magic==b'P6'
            width,height=map(int,dimensions.split());assert len(rgb)==width*height*3
            png(image.with_suffix('.png'),width,height,rgb)
    jit=json.loads((EVIDENCE/'workflow-default-8-jit/receipt.json').read_text())
    interp=json.loads((EVIDENCE/'workflow-default-8-interp/receipt.json').read_text())
    assert jit['images']==interp['images'],'JIT/interpreter application pictures differ'
    assert (EVIDENCE/'workflow-default-8-jit/flower.log').read_bytes()==(EVIDENCE/'workflow-default-8-interp/flower.log').read_bytes()
    # The same curve must remain identical across ID relocation, legacy mode
    # family selection, and the inclusive 360-degree endpoint at mode 19.
    reference=jit['images']['defaults']
    for name in ('default-relocated-8-jit','default-default-legacy-jit','fullcircle-default-8-jit'):
        receipt=json.loads((EVIDENCE/name/'receipt.json').read_text())
        assert receipt['images']['defaults']==reference,(name,'default image differs')
    report={'passed':True,'scenarios':receipts,'scenario_count':len(receipts),'exact_playfield_pixel_comparisons':pixels,
            'complete_49_byte_batches_observed':wire_batches,'per_batch_reply_bytes':30,
            'JIT_interpreter_workflow_images_and_application_log_identical':True,
            'default_binary':builds['default']['binary_sha256'],'default_binary_bytes':builds['default']['binary_bytes'],
            'relocated_binary':builds['relocated']['binary_sha256'],'native_module':builds['default']['module_sha256'],
            'production_compiler_SDK_firmware_emulator_source_changed':False,
            'scope':'real eZ80/MOS app plus actual Xtensa firmware; SDL/PS2; Oryx headless',
            'human_review':'staged, not launched by agent; acceptance pending',
            'unqualified':['physical hardware','deliberate missing-ACK/hung-module/mode-failure injection'],
            'source_hashes':{str(p.relative_to(HERE)):sha(p) for p in [HERE/'src/main.cpp',HERE/'src/packets.src',HERE/'include/parameters.hpp',HERE/'build_app.py',HERE/'profile.py',HERE/'qualify.py',HERE/'sdl_input.c']}}
    assert len(receipts)==17,len(receipts)
    (EVIDENCE/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('scenario_count','exact_playfield_pixel_comparisons','complete_49_byte_batches_observed','default_binary_bytes')},indent=2))


if __name__=='__main__':summarize()
