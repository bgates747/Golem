#!/usr/bin/env python3
"""Build the small MOS dispatcher and standalone buffered runner with AgonDev."""
from pathlib import Path
import hashlib
import json
import subprocess
from build import HERE,TASK,TOOLS,run

def build_support():
    out=TASK/'build/support';out.mkdir(parents=True,exist_ok=True);reports=[]
    for name,source,start,size in [('golemc','dispatcher.c','0xb0000','0x8000'),('golemrun','runner.c','0x40000','0x70000')]:
        with (out/(name+'.log')).open('w') as log:
            obj=out/(name+'.o');binary=out/(name+'.bin');unnamed=out/(name+'.noname.bin')
            run([TOOLS/'bin/ez80-none-elf-clang','-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset',
                 '-nostdinc','-isystem',TOOLS/'include','-Oz','-DAGONDEV','-Wa,-march=ez80+full','-c',HERE/source,'-o',obj],log)
            run([TOOLS/'bin/ez80-none-elf-ld','-defsym=RAM_START='+start,'-defsym=RAM_SIZE='+size,'-defsym=_has_exit_handler=0',
                 '-Map='+str(out/(name+'.map')),'-T',TOOLS/'config/linker.conf','--oformat','binary','-o',unnamed,obj,'-L'+str(TOOLS/'lib'),'-lagon'],log)
            binary.write_bytes(unnamed.read_bytes());run([TOOLS/'bin/agondev-setname',binary],log)
            reports.append({'name':name,'bytes':binary.stat().st_size,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'address':start})
    (TASK/'support-build.json').write_text(json.dumps(reports,indent=2)+'\n');print(json.dumps(reports))
    return reports
if __name__=='__main__':build_support()
