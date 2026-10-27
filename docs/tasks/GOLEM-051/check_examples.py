#!/usr/bin/env python3
"""Build witnesses and execute native-compiled packages using the actual runner."""
from pathlib import Path
import json
import shutil
import struct
import subprocess
import time
from qualify import HERE,ROOT,BUILD,PROFILE,Session,stage,sha
from ppm_png import read,convert

JOBS=[('hello',[]),('loops',[]),('cube',['--resident','--emit-asm']),('counter',['--portable']),
      ('arrays',['--portable']),('trig',['--portable','--target','native']),
      ('table',['--resident','--table-data','samples=examples/records.dat']),
      ('matrix',['--stock-matrix','--reserve','1000:1001']),('overflow',['--portable']),('precision',['--portable'])]

def run(args,**kwargs):return subprocess.run(list(map(str,args)),check=True,**kwargs)
def prepare():
    app=stage();sd=PROFILE/'sdcard'
    # Canonical profiles link shared MOS utilities. Replace ONLY this profile's
    # directory link with individual read-only links before adding our moslet.
    mos=sd/'mos'
    if mos.is_symlink():
        original=mos.resolve();mos.unlink();mos.mkdir()
        for p in original.iterdir():(mos/p.name).symlink_to(p)
    if (mos/'golemc.bin').is_symlink():(mos/'golemc.bin').unlink()
    shutil.copyfile(BUILD/'support/golemc.bin',mos/'golemc.bin')
    if (sd/'bin/golemrun.bin').is_symlink():(sd/'bin/golemrun.bin').unlink()
    shutil.copyfile(BUILD/'support/golemrun.bin',sd/'bin/golemrun.bin')
    for mode in ('raw','resident','portable','stock-matrix'):
        shutil.copyfile(BUILD/'port'/('ez80-'+mode)/'golemc.bin',app/'compiler'/(mode+'.bin'))
    examples=app/'examples';examples.mkdir(exist_ok=True)
    for p in (HERE/'examples').iterdir():shutil.copyfile(p,examples/p.name)
    for folder in ('output','cache'):(app/folder).mkdir(exist_ok=True)
    reference=BUILD/'examples-reference';reference.mkdir(exist_ok=True)
    expected=bytearray();checks=[];job_commands=[]
    for name,opts in JOBS:
        output=reference/(name+'.bin' if not opts else name)
        args=[str(ROOT/'build/golemc'),*opts,'examples/'+name+'.golem',str(output)]
        run(args,cwd=app,stdout=subprocess.DEVNULL)
        native_output='output/'+name+('.bin' if not opts else '')
        job_commands.append('golemc '+' '.join([*opts,'examples/'+name+'.golem',native_output]))
    declarations=[]
    def observe(label,id,offset,data):
        for i,b in enumerate(data):
            checks.append({'label':label,'buffer':id,'offset':offset+i,'expected':b});expected.append(b)
    for name,field,value in [('counter','i',struct.pack('<H',10)),('counter','total',struct.pack('<H',55)),
                             ('arrays','value',struct.pack('<I',4294967294)),('overflow','result',struct.pack('<H',17)),
                             ('precision','maximum',struct.pack('<I',4294967295)),('precision','minimum',struct.pack('<i',-2147483648)),
                             ('precision','result',struct.pack('<f',0.4)),('precision','negative',struct.pack('<f',-0.2))]:
        meta=json.loads((reference/name/'interface.json').read_text());back=json.loads((reference/name/'backends.json').read_text())
        g=next(g for g in meta['globals'] if g['name']==field)
        declarations.append((name,field,back['shared_state'],g['offset'],value))
    order=['counter','arrays','overflow','precision','table','matrix']
    offsets=[0]
    for name in order:
        for n,field,id,off,value in declarations:
            if n==name:observe(name+' '+field,id,off,value)
        if name in ('counter','arrays','overflow','precision'):
            meta=json.loads((reference/name/'backends.json').read_text());observe(name+' status',meta['shared_state'],14,struct.pack('<H',1 if name=='overflow' else 0))
        elif name=='table':
            meta=json.loads((reference/name/'interface.json').read_text());observe('selected record',meta['output']['id'],0,struct.pack('<Hh',30,-3))
        else:
            observe('transformed borrowed vertices',1001,0,b''.join(struct.pack('<hhh',*v) for v in [(100,120,0),(132,120,0),(100,152,0)]))
        offsets.append(len(checks))
    (BUILD/'example-checks.json').write_text(json.dumps({'checks':checks,'offsets':offsets,'expected':expected.hex()},indent=2)+'\n')
    lines=['#include <stdint.h>','#include <stdio.h>','#define GOLEM_QUALIFY 1','#define main runner_main',
           '#include "../port/runner.c"','#undef main',
           'static const unsigned starts[]={'+','.join(map(str,offsets))+'};',
           'struct Observation {uint16_t buffer,offset;uint8_t expected;};',
           'static const struct Observation observations[]={'+','.join('{'+f'{c["buffer"]},{c["offset"]},{c["expected"]}'+'}' for c in checks)+'};',
           f'static uint8_t observed[{len(checks)}];static unsigned current,errors;']
    lines.append(r'''
void g_observe(void){
 if(current>=6)return;
 for(unsigned i=starts[current];i<starts[current+1];++i){
  const struct Observation*o=observations+i;
  uint8_t header[8]={23,0,160,0,0,0,4,0},query[4]={23,0,128,0};query[3]=(uint8_t)~o->expected;
  command(0,2);send(header,8);send(query,4);
  uint8_t patch[15]={23,0,160,0,0,5,0xe2,3,0,1,0,0,0,0,0};word(patch+11,o->buffer);word(patch+13,o->offset);send(patch,15);
  volatile uint8_t*gp=mos_sysvars()+sysvar_gp;*gp=(uint8_t)~o->expected;command(0,1);
  uint32_t start=getsysvar_time();while(*gp!=o->expected&&(uint32_t)(getsysvar_time()-start)<600ul){}
  observed[i]=*gp;if(*gp!=o->expected)++errors;
 }
}
int main(void){
 static const char*names[]={"counter","arrays","overflow","precision","table","matrix"};
 current=6;
 char*hello[]={"golemrun","output/hello.bin"};if(runner_main(2,hello))++errors;
 char*loops[]={"golemrun","output/loops.bin"};if(runner_main(2,loops))++errors;
 // Borrowed input/output are application-owned, separately from Golem code.
 const int16_t xyz[]={0,0,0,32,0,0,0,32,0};uint8_t h[8]={23,0,160,0,0,0,18,0};
 for(unsigned id=1000;id<=1001;++id){command(id,2);word(h+3,id);send(h,8);send(xyz,18);}if(!echo())return 1;
 for(current=0;current<6;++current){char path[40];snprintf(path,sizeof path,"output/%s",names[current]);
  char*args[]={"golemrun",current<4?"--portable":"--resident",path};if(runner_main(3,args))++errors;
 }
 command(1000,2);command(1001,2);echo();
 FILE*f=fopen("probe-results.dat","wb");if(!f)return 1;fwrite(observed,1,sizeof observed,f);fclose(f);
 f=fopen("probe-status.dat","wb");if(!f)return 1;uint8_t status=(uint8_t)errors;fwrite(&status,1,1,f);fclose(f);
 // The only picture comes from cube.golem compiled above on the emulated Agon.
 char*draw[]={"golemrun","--review","--resident","output/cube"};int rc=runner_main(4,draw);
 f=fopen("probe-done.dat","wb");if(!f)return 1;uint8_t done=(uint8_t)rc;fwrite(&done,1,1,f);fclose(f);return rc;
}
''')
    probe=BUILD/'probe.c';probe.write_text('\n'.join(lines)+'\n')
    tools=Path('/Agon/agondev/release')
    run([tools/'bin/ez80-none-elf-clang','-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset',
         '-nostdinc','-isystem',tools/'include','-Oz','-DAGONDEV','-Wa,-march=ez80+full','-c',probe,'-o',BUILD/'probe.o'])
    run([tools/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000','-defsym=_has_exit_handler=0',
         '-T',tools/'config/linker.conf','--oformat','binary','-o',BUILD/'probe.bin',BUILD/'probe.o','-L'+str(tools/'lib'),'-lagon'])
    shutil.copyfile(BUILD/'probe.bin',app/'probe.bin')
    auto='SET KEYBOARD 1\nVDU 22 19\ncd /golem\n'+'\n'.join(job_commands)+'\nload probe.bin\nrun .\n'
    return app,reference,expected,auto

def execute():
    app,reference,expected,auto=prepare()
    for name in ('probe-results.dat','probe-status.dat','probe-done.dat','examples-proof.dat'):(app/name).unlink(missing_ok=True)
    s=Session(auto,'examples')
    try:
        s.wait(app/'probe-done.dat',240);s.wait(app/'probe-status.dat',20);s.wait(app/'probe-results.dat',20)
        deadline=time.monotonic()+20
        while (app/'probe-results.dat').stat().st_size<len(expected):
            if time.monotonic()>deadline:raise TimeoutError('incomplete results')
            time.sleep(.05)
        assert (app/'probe-done.dat').read_bytes()==b'\0'
        assert (app/'probe-status.dat').read_bytes()==b'\0', (app/'probe-status.dat').read_bytes().hex()
        assert (app/'probe-results.dat').read_bytes()==expected
        time.sleep(.5);s.type('save examples-proof.dat 40000 3\n');s.wait(app/'examples-proof.dat',20)
        image=s.capture();w,h,rgb=read(image);assert (w,h)==(1024,768)
        anchors=[(256,288),(576,288),(576,608),(256,608),(448,448),(768,448),
                 (768,288),(448,288),(416,288),(416,608),(256,448),(576,448),(608,448)]
        for x,y in anchors:assert rgb[(y*w+x)*3:(y*w+x)*3+3]==b'\xff\xff\xff',(x,y)
        picture=convert(image)
        report={'passed':True,'seconds':time.monotonic()-s.started,'observations':len(expected),'picture':str(picture),'white_cube_anchors':len(anchors),
                                 'actual_native_compilation':True,'stock_VDP_execution':True,'MOS_keyboard_continuation':True,'human':'pending'}
        mismatches=[]
        for name,opts in JOBS:
            ref=reference/(name+'.bin' if not opts else name);native=app/'output'/(name+'.bin' if not opts else name)
            if ref.is_file():
                if ref.read_bytes()!=native.read_bytes():mismatches.append(name)
            else:
                for p in ref.iterdir():
                    if p.read_bytes()!=(native/p.name).read_bytes():mismatches.append(name+'/'+p.name)
        assert not mismatches,mismatches
        report['example_packages']=len(JOBS)
        report['compiler_binaries']={m:sha(BUILD/'port'/('ez80-'+m)/'golemc.bin') for m in ('raw','resident','portable','stock-matrix')}
        report['dispatcher_sha256']=sha(BUILD/'support/golemc.bin');report['runner_sha256']=sha(BUILD/'support/golemrun.bin')
    finally:exitcode=s.close()
    report['frontend_exit']=exitcode;(HERE/'examples-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':execute()
