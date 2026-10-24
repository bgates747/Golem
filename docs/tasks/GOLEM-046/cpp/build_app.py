#!/usr/bin/env python3
"""Compile the same Golem sources and the ordinary AgonDev C++ consumer."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
HERE=Path(__file__).resolve().parent
TASK=HERE.parent
ROOT=TASK.parents[2]
BUILD=HERE/'build'
API=ROOT.parent/'agon-utils/examples/agnb/api'
sys.path.insert(0,str(API))
from buffers import BufferRecord,build_container

def run(*args,**kw):
    r=subprocess.run(list(map(str,args)),text=True,capture_output=True,**kw)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)
    return r.stdout+r.stderr

def build(native=False):
    BUILD.mkdir(exist_ok=True)
    run(ROOT/'build/golemc','--portable','--emit-load-plan',TASK/'large.golem',BUILD/'large')
    if native:run(sys.executable,ROOT.parent/'agon-vdplet/tools/build_module.py',BUILD/'large/native.c','--name','large','--output',BUILD/'large')
    args=[sys.executable,ROOT/'tools/package_agnb.py',BUILD/'large']
    if native:args+=['--module',BUILD/'large/large.vdpl']
    run(*args)
    run(ROOT/'build/golemc','--resident','--emit-load-plan','--reserve','1:4095','--reserve','60000:60031',TASK/'review.golem',BUILD/'visual')
    run(sys.executable,ROOT/'tools/package_agnb.py',BUILD/'visual')
    include=HERE/'include/generated';include.mkdir(parents=True,exist_ok=True)
    for name in ('large','visual'):shutil.copyfile(BUILD/name/'agnb.h',include/(name+'.h'))
    expected=bytearray()
    def row(total,generation):return struct.pack('<HHHI',total,0,0,generation)
    for cycle in range(2):
        expected+=b'\0'+row(0,0)
        if native:expected+=b'\0\0'
        for total,generation in ((45,1),(90,2)):
            if native:expected+=b'\0\0'
            expected+=row(total,generation)
        expected+=b'\x55'*10
    expected+=b'\0'
    (BUILD/'expected.dat').write_bytes(expected)
    (include/'config.h').write_text('#pragma once\n#define REVIEW_NATIVE '+str(int(native))+'\n#define REVIEW_QUERY 60010u\n#define REVIEW_ERROR_BUFFER 60020u\n#define REVIEW_MODULE_BYTES '+str((BUILD/'large/large.vdpl').stat().st_size if native else 0)+'u\nstatic const uint8_t review_expected[]={'+','.join(map(str,expected))+'};\n')
    good=build_container([BufferRecord(60020,b'\x11'*7)])
    (BUILD/'probe.agnb').write_bytes(good)
    bad=bytearray(good);bad[56]=1;(BUILD/'bad.agnb').write_bytes(bad)
    (BUILD/'truncated.agnb').write_bytes(good[:-1])
    second=bytearray(build_container([BufferRecord(60021,b'Q')]));second[56]=1
    late=good+second[24:];late=late[:4]+struct.pack('<I',len(late)-8)+late[8:]
    (BUILD/'late.agnb').write_bytes(late)
    env=dict(os.environ,PATH='/Agon/agondev/release/bin:'+os.environ['PATH'])
    run('make','clean',cwd=HERE,env=env)
    log=run('make',cwd=HERE,env=env);(BUILD/'agondev-build.log').write_text(log)
    shutil.copyfile(HERE/'bin/agnb_cpp.bin',BUILD/'probe.bin')
    for name in ('large.golem','review.golem'): # profile preparation uses shared sources
        link=HERE/name
        if not link.exists():link.symlink_to('../'+name)
    memory_map=(HERE/'bin/agnb_cpp.map').read_text()
    bss_end=int(re.search(r'0x([0-9a-fA-F]+)\s+bss_end =',memory_map).group(1),16)
    report={'static_ram_bytes':bss_end-0x40000,'native':native,'binary_bytes':(BUILD/'probe.bin').stat().st_size,'binary_sha256':hashlib.sha256((BUILD/'probe.bin').read_bytes()).hexdigest(),
            'workspace_bytes':8192,'result_bytes':len(expected),'embedded_data':False,'sdk_and_firmware_changed':False,
            'sources':{str(p.relative_to(ROOT.parent)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [
                ROOT/'runtime/agondev/agnb_host.cpp',API/'agnb_buffers.hpp',API/'agnb_stdio.hpp',API/'agnb_agondev.hpp',HERE/'src/main.cpp',HERE/'src/visual.cpp',TASK/'large.golem',TASK/'review.golem']}}
    (HERE/'build-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    (HERE/('native-build-evidence.json' if native else 'stock-build-evidence.json')).write_text(json.dumps(report,indent=2)+'\n')
    print('Built C++ streamed consumer:',report['binary_bytes'],'bytes,',len(expected),'result bytes')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--native',action='store_true');a=p.parse_args();build(a.native)
