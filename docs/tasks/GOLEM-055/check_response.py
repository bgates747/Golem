#!/usr/bin/env python3
"""Check response-file reader boundaries/quoting independently of compilation."""
from pathlib import Path
import hashlib
import json
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PORT=ROOT/'docs/tasks/GOLEM-051/port'
BASE=HERE/'build/baseline'
SOFT=PORT.parent/'vendor/softfloat'

def main():
    out=HERE/'build/response-check';out.mkdir(parents=True,exist_ok=True)
    stub=out/'stub.cpp'
    stub.write_text('''#include <stdio.h>
#include <string.h>
int golemc_entry(int argc,char**argv){
    printf("argc=%d\\n",argc);
    for(int i=1;i<argc;++i)printf("arg=%lu:%s\\n",(unsigned long)strlen(argv[i]),argv[i]);
    return 0;
}
''')
    binary=out/'response-check'
    command=['g++','-O2','-std=c++17','-nostdinc++','-fno-exceptions','-fno-rtti',
        '-I'+str(BASE/'include'),'-I'+str(SOFT),'-I'+str(SOFT/'source/include'),
        '-I'+str(SOFT/'source/8086-SSE'),'-DSOFTFLOAT_FAST_INT64','-DINLINE_LEVEL=0',
        str(PORT/'entry.cpp'),str(BASE/'runtime.cpp'),str(stub),'-o',str(binary)]
    subprocess.run(command,check=True,capture_output=True)
    cases=[('empty','',0,'argc=1\n',''),
        ('quoted','--portable "two words" output',0,'argc=4\narg=10:--portable\narg=9:two words\narg=6:output\n',''),
        ('empty-quoted','""',0,'argc=2\narg=0:\n',''),
        ('limit','x'*8192,0,'argc=2\narg=8192:'+'x'*8192+'\n',''),
        ('too-large','x'*8193,1,'','golemc: response file exceeds 8192 bytes\n'),
        ('unclosed','"unclosed',1,'','golemc: unclosed response quote\n'),
        ('argument-limit',' '.join(['x']*127),0,'argc=128\n'+'arg=1:x\n'*127,''),
        ('too-many',' '.join(['x']*128),1,'','golemc: too many response arguments\n')]
    reports=[]
    for name,text,rc,stdout,stderr in cases:
        source=out/(name+'.args');source.write_text(text)
        result=subprocess.run([str(binary),'@'+str(source)],capture_output=True,text=True)
        if (result.returncode,result.stdout,result.stderr)!=(rc,stdout,stderr):
            raise AssertionError((name,result.returncode,result.stdout,result.stderr))
        reports.append({'case':name,'source_bytes':len(text),'exit_code':rc,'passed':True})
    result={'passed':True,'scope':'Host-adapted reader regression; actual MOS validation is separate.',
        'entry_sha256':hashlib.sha256((PORT/'entry.cpp').read_bytes()).hexdigest(),'cases':reports}
    (HERE/'response-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
