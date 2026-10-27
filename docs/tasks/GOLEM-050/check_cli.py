#!/usr/bin/env python3
"""New layout/assembler/parity and read-only consumer source qualification."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
COMPILER=ROOT/'build/golemc'
ASSEMBLER=ROOT/'docs/tasks/GOLEM-044/native-math/build/assembler/bin/ez80asm'
PEER=Path('/Agon/mystuff/agon-fsim/tests/0114-stock-vdp-perspective')

def run(args,**kwargs):
    result=subprocess.run(list(map(str,args)),capture_output=True,text=True,**kwargs)
    assert not result.returncode,result.stdout+result.stderr
    return result.stdout

def main():
    # Reproduce the previously frozen68files without any loop-free output drift.
    run([ROOT/'.venv/bin/python',ROOT/'docs/tasks/GOLEM-049/check_cli.py'])
    assemblers=[]
    for name,span in [('default','1000:1003'),('relocated','1:1003')]:
        out=BUILD/('cli-'+name)
        run([COMPILER,'--stock-matrix','--emit-load-plan','--reserve',span,HERE/'example.golem',out])
        meta=json.loads((out/'interface.json').read_text());load=json.loads((out/'load-plan.json').read_text())
        assert meta['input']['bytes']==0 and len(meta['counters'])==1 and len(meta['loops'])==1
        counter=meta['counters'][0];loop=meta['loops'][0]
        records={r['buffer_id']:r for r in load['components'][0]['records']}
        assert counter['bytes']==2 and counter['storage_bytes']==3
        assert records[counter['buffer']]['bytes']==3 and records[loop['state_buffer']]['bytes']==loop['state_bytes']
        assert loop['state_bytes']==21 and loop['body_bytes']==324 and loop['cursors']==5
        assert not set(meta['owned'])&set(range(1000,1004))
        if name=='relocated':assert min(meta['owned'])>1003
        header=(out/'agnb.h').read_text();assert '#define GOLEM_FIELD_vertex_BYTES 2u' in header
        (out/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n jp start\n include "golem.inc"\nstart:\n call golem_project\n ret\n')
        run([ASSEMBLER,'consumer.asm','consumer.bin','-c'],cwd=out)
        assemblers.append(dict(layout=name,passed=True,owned_ids=meta['owned'],counter=counter,loop=loop))
    peer_reports=[]
    for model,n in [('heavytank',30),('corsair',2685)]:
        source=PEER/model/'transform.golem';raw=source.read_text();digest=hashlib.sha256(source.read_bytes()).hexdigest()
        header=raw.split('Sub projectChunk0',1)[0]
        body='''    For vertex In Range(0:END)
        matrix_set_value(a,baseMatrix,0,0,read_float32(clipVertices,12*vertex+8))
        matrix_set_value(a,a,1,1,read_float32(clipVertices,12*vertex+8))
        matrix_set_value(a,a,0,2,read_float32(clipVertices,12*vertex))
        matrix_set_value(a,a,1,2,read_float32(clipVertices,12*vertex+4))
        matrix_invert(a)
        transform_vertices(pixel,a,zero)
        copy_bytes(screenVertices,6*vertex,pixel,0,4)
    Next vertex'''.replace('END',str(n-1))
        replaced=False;lines=[]
        for line in header.splitlines():
            if re.fullmatch(r'\s*projectChunk\d+\(\)',line):
                if not replaced:lines.extend(body.splitlines());replaced=True
            else:lines.append(line)
        assert replaced
        converted='uint16 vertex = 0\n'+'\n'.join(lines)+'\n'
        local=BUILD/(model+'-loop.golem');local.write_text(converted)
        output=BUILD/('consumer-'+model)
        run([COMPILER,'--stock-matrix','--emit-load-plan','--reserve','1:1004','--reserve','65000:65001',local,output])
        meta=json.loads((output/'interface.json').read_text())
        assert meta['input']['bytes']==32 and len(meta['globals'])==8 and len(meta['entries'])==2 and len(meta['loops'])==1
        assert meta['loops'][0]['iterations']==n and meta['loops'][0]['body_operations']==7
        assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
        peer_reports.append(dict(model=model,source=str(source),source_sha256=digest,source_unchanged=True,output=str(output),
            bootstrap_bytes=(output/'bootstrap.bin').stat().st_size,owned_buffers=len(meta['owned']),control_bytes=32,
            body_bytes=meta['loops'][0]['body_bytes'],state_bytes=meta['loops'][0]['state_bytes'],entry_count=2,no_target_execution=True))
    assert peer_reports[0]['bootstrap_bytes']==peer_reports[1]['bootstrap_bytes']
    assert peer_reports[0]['owned_buffers']==peer_reports[1]['owned_buffers']
    rejected=BUILD/'cli-reject';rejected.mkdir(exist_ok=True);sentinel=rejected/'bootstrap.bin';sentinel.write_bytes(b'preserve existing output')
    source=BUILD/'bad-loop.golem';source.write_text((HERE/'example.golem').read_text().replace('12*vertex+8','12*vertex+9'))
    result=subprocess.run(list(map(str,[COMPILER,'--stock-matrix','--reserve','1000:1003',source,rejected])),capture_output=True,text=True)
    assert result.returncode and sentinel.read_bytes()==b'preserve existing output' and sorted(p.name for p in rejected.iterdir())==['bootstrap.bin']
    report=dict(passed=True,unchanged_legacy_files=68,actual_assembler_consumers=assemblers,peer_sources=peer_reports,
                rejected_output_preserved=True,contract_sha256=hashlib.sha256((HERE/'contract.md').read_bytes()).hexdigest())
    (HERE/'cli-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report),flush=True)

if __name__=='__main__':main()
