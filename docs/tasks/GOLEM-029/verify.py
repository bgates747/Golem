"""Verify saved observations; never repeat the guest's keyboard instruction."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import zlib

def screen(ppm,preview=None):
    magic,dimensions,maximum,pixels=ppm.read_bytes().split(b'\n',3)
    assert (magic,dimensions,maximum)==(b'P6',b'320 240',b'255')
    assert len(pixels)==320*240*3
    checked=0
    for y in range(176):
        for x in range(320):
            colour=(0,0,0)
            if 112<=x<=208 and 8<=y<=166:colour=(85,85,85)
            if 116<=x<=204 and 12<=y<=162:colour=(0,0,0)
            if 132<=x<=188 and (20<=y<=56 or 120<=y<=156):colour=(85,85,85)
            if 132<=x<=188 and 70<=y<=106:colour=(255,85,0)
            offset=(y*320+x)*3
            assert tuple(pixels[offset:offset+3])==colour,(x,y,colour,tuple(pixels[offset:offset+3]))
            checked+=1
    if preview:
        def chunk(kind,data):
            return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
        raw=b''.join(b'\0'+pixels[y*960:(y+1)*960] for y in range(240))
        preview.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',320,240,8,2,0,0,0))+
                            chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
    return checked

def verify(app):
    expected=json.loads((app/'expected.json').read_text())
    got=(app/'results.dat').read_bytes()
    want=(app/'expected.dat').read_bytes();mask=(app/'mask.dat').read_bytes()
    if len(got)!=len(want):raise RuntimeError(f'Saved {len(got)} bytes; expected {len(want)}')
    failures=[check['label'] for check in expected['checks']
              if any((got[i]^want[i])&mask[i] for i in range(check['offset'],check['offset']+check['bytes']))]
    if failures:raise RuntimeError('Observation mismatch: '+', '.join(failures[:12]))
    dynamic={check['label']:struct.unpack_from('<H',got,check['offset']+8)[0]
             for check in expected['checks'] if check['label'] in ('LOAD native','native entry 6','native entry 7')}
    if dynamic:
        assert dynamic['LOAD native']!=0,'Zero module handle'
        entry=dynamic['native entry 6'] | dynamic['native entry 7']<<16
        assert 0x40080000<=entry<0x400a0000,hex(entry)
        dynamic['entry_address']=hex(entry)
    return {'variant':expected['variant'],'observations':len(expected['checks']),
            'calls':len(expected['calls']),'result_bytes':len(got),
            'result_sha256':hashlib.sha256(got).hexdigest(),'native':dynamic,
            'visual':expected['visual']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('app',type=Path)
    print(json.dumps(verify(parser.parse_args().app),indent=2))
