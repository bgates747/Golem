#!/usr/bin/env python3
"""Compile default/relocated consumers; invalid syntax must not mutate outputs."""
from pathlib import Path
import hashlib
import json
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
COMPILER=ROOT/'build/golemc'
ASSEMBLER=ROOT/'docs/tasks/GOLEM-044/native-math/build/assembler/bin/ez80asm'

def run(args,**kwargs):
    result=subprocess.run(list(map(str,args)),capture_output=True,text=True,**kwargs)
    assert result.returncode==0,result.stdout+result.stderr
    return result.stdout

def main():
    BUILD.mkdir(exist_ok=True)
    reports=[]
    for name,spans in [('default',['1000:1001']),('relocated',['1:1001'])]:
        output=BUILD/name
        args=[COMPILER,'--stock-matrix','--emit-asm']
        for span in spans:args+=['--reserve',span]
        run(args+[HERE/'example.golem',output])
        meta=json.loads((output/'interface.json').read_text())
        assert meta['profile']=='stock-matrix-v1' and meta['target']=='stock-buffered'
        assert [f['offset'] for f in meta['globals']]==[0,4,8]
        assert meta['input']['bytes']==12
        assert {v['id'] for v in meta['borrowed']}=={1000,1001}
        assert not set(meta['owned'])&{1000,1001}
        if name=='relocated':assert min(meta['owned'])>1001
        else:assert meta['input']['id']==1
        assert not any((output/f).exists() for f in ('native.c','native-install.bin','flower.vdpl'))
        (output/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n jp start\n include "golem.inc"\nstart:\n call golem_setup\n call golem_transformFrame\n ret\n')
        run([ASSEMBLER,'consumer.asm','consumer.bin','-c'],cwd=output)
        reports.append({'layout':name,'actual_assembler':True,'bootstrap_bytes':(output/'bootstrap.bin').stat().st_size,
                        'owned_buffers':len(meta['owned']),'inputs':meta['input'],
                        'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in output.glob('*') if p.is_file()}})
    out=BUILD/'reject';out.mkdir(exist_ok=True)
    sentinel=out/'bootstrap.bin';sentinel.write_bytes(b'preserve existing output')
    bad=BUILD/'bad.golem';bad.write_text('Matrix(4,4) m = Identity Sub tick matrix_inverse(m) EndSub')
    result=subprocess.run([str(COMPILER),'--stock-matrix',str(bad),str(out)],capture_output=True,text=True)
    assert result.returncode and sentinel.read_bytes()==b'preserve existing output'
    assert sorted(p.name for p in out.iterdir())==['bootstrap.bin']
    (HERE/'cli-evidence.json').write_text(json.dumps({'passed':True,'packages':reports,'invalid_source_did_not_mutate_output':True},indent=2)+'\n')
    print('Stock matrix CLI, metadata, two actual assembler consumers and rejected-output preservation pass.')

if __name__=='__main__':main()
