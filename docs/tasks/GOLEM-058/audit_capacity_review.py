#!/usr/bin/env python3
"""Bind the closed capacity review's preserved results to actual wire replies."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,struct
SDK=Path('/Agon/mystuff/agon-vdplet')

def audit(root):
    hashes=json.loads((root/'SHA256.json').read_text())
    for n,h in hashes.items():assert hashlib.sha256((root/n).read_bytes()).hexdigest()==h,n
    spec=importlib.util.spec_from_file_location('capacity_saved',SDK/'docs/tasks/VDPL-022/physical/verify_log.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    result=m.verify(root/'capacity.log',root/'capacity.done',SDK/'build/vdpl022-physical-client-r03/package',SDK/'docs/tasks/VDPL-022/physical/module-catalog.json',SDK/'docs/tasks/VDPL-022/generate.py')
    rx=(root/'runtime/vdp-to-mos.bin').read_bytes();assert rx==(root/'runtime/vdp-to-mos-delivered.bin').read_bytes()
    records=[];at=0
    while at<len(rx):
        if rx[at]<128:at+=1;continue
        assert at+2<=len(rx);n=rx[at+1];body=rx[at+2:at+2+n];assert len(body)==n
        if rx[at]==129 and body.startswith(b'VDPL'):
            assert n==10 and body[4]==1;records.append(struct.unpack_from('<BHH',body,5))
        at+=n+2
    outputs=[]
    for i,(kind,token,value)in enumerate(records):
        if(kind,value)!=(3,0):continue
        words=[]
        for k,t,v in records[i+1:]:
            if k!=4:break
            words.append(v)
        if len(words)==16:outputs.append(struct.pack('<16H',*words).hex())
    expected=[line.split()[3]for line in(root/'capacity.log').read_text().splitlines()if line.startswith('OUTPUT ')]
    assert outputs==expected and len(outputs)==result['calls']
    tx=(root/'runtime/mos-to-vdp.bin').read_bytes()
    for mode in(11,8,136):assert('VDPlet capacity / mode%d / 320x240'%mode).encode()in tx
    for i in range(5):assert('Module%d:'%i).encode()in tx
    assert b'Native VDPlet capacity review complete.'in tx
    assert b'C host: capacity.bin / main.c. Native workers:'in tx
    assert b'Directory: /capacity'in tx and b'/capacity *'in tx.partition(b'Directory: /capacity')[2]
    assert bytes([4,23,1,1,23,1,3])in tx
    stats=json.loads((root/'runtime/stats.json').read_text());assert not stats['failed']and all(stats[k]==0 for k in('unmapped','unhandled','unregistered'))
    return dict(passed=True,scope='Saved review log/oracle/native wire/MOS continuation; human report remains separate',files=len(hashes),modes=result['modes'],loads=result['loads'],calls=result['calls'],expected_MEMORY_refusals=result['expected_MEMORY_refusals'],sampled_free_recovery=True,MOS_DIR=True,explicit_flashing_cursor_commands=True,physical=False)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=audit(a.root);a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
