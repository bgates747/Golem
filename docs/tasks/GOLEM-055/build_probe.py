#!/usr/bin/env python3
"""Build isolated heap-sampled copies; never overwrite a maintained worker."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BASE=HERE/'build/baseline'
PORT=ROOT/'docs/tasks/GOLEM-051/port'
SOFT=PORT.parent/'vendor/softfloat'
TOOLS=Path(os.environ.get('AGONDEV_ROOT','/Agon/agondev/release'))

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def replace_once(text,old,new):
    if text.count(old)!=1:raise RuntimeError('Probe anchor count changed: '+old)
    return text.replace(old,new)

def build(which):
    freeze=json.loads((HERE/'baseline-freeze.json').read_text())
    for rel,digest in freeze['files'].items():
        if sha(BASE/rel)!=digest:raise RuntimeError('Frozen baseline changed: '+rel)
    dest=HERE/'build'/('probe-'+which);dest.mkdir(parents=True,exist_ok=True)
    for name in ('source','include'):
        shutil.copytree(BASE/name,dest/name,dirs_exist_ok=True)
    fixed={}
    if which=='fixed':
        spec=importlib.util.spec_from_file_location('golem_native_prepare',PORT/'prepare.py')
        prepare=importlib.util.module_from_spec(spec);spec.loader.exec_module(prepare)
        for rel in ('portable/root.hpp','portable/backends.hpp'):
            source=ROOT/'src'/rel;text=source.read_text()
            if rel=='portable/root.hpp':
                text=replace_once(text,'struct Binding {unsigned role=0,id=0,offset=0,bytes=0,access=0;};',
                    'struct Binding {unsigned role=0,id=0;uint32_t offset=0,bytes=0;unsigned access=0;};')
            else:text=replace_once(text,'return output_;','return std::move(output_);')
            (dest/'source'/rel).write_text(prepare.adapt(text));fixed[rel]=sha(source)
    runtime=(BASE/'runtime.cpp').read_text()
    runtime=replace_once(runtime,'void*memory(size_t n){void*p=malloc(n);',
        'extern "C" void heap_probe_sample(size_t,bool);\nvoid*memory(size_t n){void*p=malloc(n);heap_probe_sample(n,p!=nullptr);')
    (dest/'runtime.cpp').write_text(runtime)
    entry=(BASE/'entry.cpp').read_text()
    entry=replace_once(entry,'int golemc_entry(int,char**);',
        'int golemc_entry(int,char**);\nextern "C" void heap_probe_reset();\nextern "C" void heap_probe_report(const char*);')
    entry=replace_once(entry,'int rc=golemc_entry((int)count,args);free(text);return rc;',
        'heap_probe_reset();int rc=golemc_entry((int)count,args);heap_probe_report(path);free(text);return rc;')
    entry=replace_once(entry,'    return golemc_entry(argc,argv);',
        '    heap_probe_reset();int rc=golemc_entry(argc,argv);heap_probe_report(nullptr);return rc;')
    (dest/'entry.cpp').write_text(entry)
    common=['-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset',
        '-nostdinc','-isystem',str(TOOLS/'include'),'-DAGONDEV','-Oz','-Wa,-march=ez80+full']
    cpp=common+['-std=c++17','-ffp-contract=off','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics',
        '-fno-c++-static-destructors','-Wno-unqualified-std-cast-call','-I'+str(dest/'include'),
        '-I'+str(SOFT),'-I'+str(SOFT/'source/include'),'-I'+str(SOFT/'source/8086-SSE'),
        '-DSOFTFLOAT_FAST_INT64','-DINLINE_LEVEL=0']
    started=time.monotonic()
    def run(args,log):
        log.write('$ '+' '.join(map(str,args))+'\n');log.flush()
        subprocess.run(list(map(str,args)),stdout=log,stderr=subprocess.STDOUT,check=True)
    with (dest/'build.log').open('w') as log:
        objects=[]
        for source in (dest/'source/golemc.cpp',dest/'runtime.cpp',dest/'entry.cpp',HERE/'heap_probe.cpp'):
            obj=dest/(source.stem+'.o');objects.append(obj)
            run([TOOLS/'bin/ez80-none-elf-clang',*cpp,'-c',source,'-o',obj],log)
        linker=dest/'linker.conf'
        linker.write_text((TOOLS/'config/linker.conf').read_text().replace(
            '___heaptop           = RAM_START + RAM_SIZE;',
            '___heaptop           = RAM_START + RAM_SIZE - 0x4000;'))
        binary=dest/'golemc.bin';unnamed=dest/'golemc.noname.bin'
        run([TOOLS/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000',
            '-defsym=_has_exit_handler=0','-Map='+str(dest/'golemc.map'),'-T',linker,'--oformat','binary',
            '-o',unnamed,*objects,BASE/'real.o',BASE/'libnumeric.a','-L'+str(TOOLS/'lib'),'-lagon'],log)
        binary.write_bytes(unnamed.read_bytes());run([TOOLS/'bin/agondev-setname',binary],log)
    result={'variant':which,'worker':str(binary.relative_to(ROOT)),'bytes':binary.stat().st_size,
        'sha256':sha(binary),'baseline_worker_sha256':freeze['worker_sha256'],
        'fixed_maintained_headers':fixed,'probe_sha256':sha(HERE/'heap_probe.cpp'),
        'seconds':time.monotonic()-started,
        'scope':'Read-only free-list snapshots after ag::memory attempts; not every libc malloc/free or stack peak.',
        'allocator_sources':{str(p):sha(p) for p in [TOOLS.parent/'src/lib/libc/malloc.c.src',TOOLS.parent/'src/lib/libc/free.c.src',TOOLS.parent/'src/lib/libc/sbrk.src']}}
    (dest/'build.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2));return binary

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variant',choices=('baseline','fixed'))
    build(parser.parse_args().variant)
