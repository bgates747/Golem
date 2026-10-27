#!/usr/bin/env python3
"""Build the same Golem source with the MOS library adapter, on host or eZ80."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from prepare import prepare

HERE=Path(__file__).resolve().parent
TASK=HERE.parent
SOFT=TASK/'vendor/softfloat'
TOOLS=Path(os.environ.get('AGONDEV_ROOT','/Agon/agondev/release'))

def sources():
    allfiles=list((SOFT/'source').glob('*.c'))+list((SOFT/'source/8086-SSE').glob('*.c'))
    names={p.stem:p for p in allfiles}
    todo='f64_add f64_sub f64_mul f64_div f64_eq f64_lt f64_le f64_roundToInt f64_to_f32 f64_to_i64_r_minMag f64_to_ui64_r_minMag f32_to_f64 i64_to_f64 ui64_to_f64 softfloat_state softfloat_raiseFlags'.split()
    chosen=set()
    while todo:
        name=todo.pop();key='s_'+name[len('softfloat_'):] if name.startswith('softfloat_') and name not in names else name
        if key not in names or names[key] in chosen:continue
        path=names[key];chosen.add(path)
        todo+=re.findall(r'\b(?:softfloat|f64|f32|i64|ui64)_\w+\b',path.read_text())
    return sorted(chosen)

def run(args,log):
    log.write('$ '+' '.join(map(str,args))+'\n');log.flush()
    result=subprocess.run(list(map(str,args)),stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise SystemExit(f'Build failed ({result.returncode}); see {log.name}')

def build(target=False,sanitize=False,profile=None):
    out,source=prepare(profile);dest=out/(('ez80-'+profile) if target and profile else 'ez80' if target else 'san' if sanitize else 'host');dest.mkdir(exist_ok=True)
    started=time.monotonic()
    includes=['-I'+str(out/'include'),'-I'+str(SOFT),'-I'+str(SOFT/'source/include'),'-I'+str(SOFT/'source/8086-SSE')]
    cflags=['-DSOFTFLOAT_FAST_INT64','-DINLINE_LEVEL=0']
    if target:
        cc=TOOLS/'bin/ez80-none-elf-clang';ar=TOOLS/'bin/ez80-none-elf-ar'
        common=['-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset',
                '-nostdinc','-isystem',str(TOOLS/'include'),'-DAGONDEV','-Oz','-Wa,-march=ez80+full']
        cpp=common+['-std=c++17','-ffp-contract=off','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-c++-static-destructors','-Wno-unqualified-std-cast-call']
    else:
        cc='gcc';ar='ar';common=['-O1' if sanitize else '-O2','-fvisibility=hidden']
        if sanitize:common+=['-g','-fsanitize=address,undefined']
        cpp=common+['-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti']
    with (dest/'build.log').open('w') as log:
        objects=[]
        for path in sources():
            obj=dest/(path.stem+'.o');objects.append(obj)
            if not obj.exists() or path.stat().st_mtime>obj.stat().st_mtime:
                run([cc,*common,*cflags,*includes,'-std=c11','-c',path,'-o',obj],log)
        archive=dest/'libnumeric.a';run([ar,'rcs',archive,*objects],log)
        objects=[]
        for path in (source/'golemc.cpp',HERE/'runtime.cpp',HERE/'real.cpp',HERE/'entry.cpp'):
            obj=dest/(path.stem+'.o');objects.append(obj)
            run([cc if target else 'g++',*cpp,*cflags,*includes,'-c',path,'-o',obj],log)
        binary=dest/('golemc.bin' if target else 'golemc')
        if target:
            unnamed=dest/'golemc.noname.bin'
            linker=dest/'linker.conf';linker.write_text((TOOLS/'config/linker.conf').read_text().replace('___heaptop           = RAM_START + RAM_SIZE;','___heaptop           = RAM_START + RAM_SIZE - 0x4000;'))
            run([TOOLS/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000',
                 '-defsym=_has_exit_handler=0','-Map='+str(dest/'golemc.map'),'-T',linker,
                 '--oformat','binary','-o',unnamed,*objects,archive,'-L'+str(TOOLS/'lib'),'-lagon'],log)
            binary.write_bytes(unnamed.read_bytes());run([TOOLS/'bin/agondev-setname',binary],log)
        else:
            run(['g++',*common,'-fvisibility=hidden','-o',binary,*objects,archive,'-lm'],log)
    receipt={'target':'eZ80/MOS' if target else 'host-adapted','sanitizers':sanitize,
             'binary_bytes':binary.stat().st_size,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
             'seconds':time.monotonic()-started,'source_pins':json.loads((out/'source-pins.json').read_text()),
             'numeric_sources':[str(p.relative_to(SOFT)) for p in sources()]}
    (dest/'build.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('source_pins','numeric_sources')}))
    return binary

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--target',action='store_true');p.add_argument('--sanitize',action='store_true');p.add_argument('--profile',choices=('raw','resident','portable','stock-matrix'))
    a=p.parse_args();build(a.target,a.sanitize,a.profile)
