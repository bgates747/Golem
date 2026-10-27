#!/usr/bin/env python3
"""Task qualification: frozen legacy output, integrated sizes, actual assembler."""
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

def hashes(folder):
    return {str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(folder.rglob('*')) if p.is_file()}

def main():
    BUILD.mkdir(exist_ok=True)
    baseline=json.loads((HERE/'legacy-baseline.json').read_text())
    legacy_count=0
    for case in baseline['cases']:
        source=ROOT/case['source']
        if case['name'].startswith('minimal'):
            source=BUILD/'legacy-minimal.golem'
            source.write_text('Matrix(4,4) model = Identity Sub tick matrix_identity(model) EndSub\n')
        args=[COMPILER,'--stock-matrix']
        if case['load']: args+=['--emit-load-plan']
        for span in case['reservations']:args+=['--reserve',span]
        out=BUILD/'parity'/case['name']
        run(args+[source,out])
        assert hashes(out)==case['sha256'],case['name']
        legacy_count+=len(case['sha256'])
    reports=[]
    for name,span in [('default','1000:1005'),('relocated','1:1005')]:
        output=BUILD/name
        run([COMPILER,'--stock-matrix','--emit-load-plan','--reserve',span,HERE/'example.golem',output])
        meta=json.loads((output/'interface.json').read_text())
        component=json.loads((output/'load-plan.json').read_text())['components'][0]
        assert [(m['rows'],m['bytes']) for m in meta['matrices']]==[(4,64),(4,64),(3,36),(3,36)]
        assert [(v['type'],v['bytes']) for v in meta['borrowed']]==[('XYZF32',24)]*3+[('XY16',4)]*2+[('XYZ16',12)]
        assert [e['name'] for e in meta['entries']]==['projectFrame','projectFirst','projectSecond']
        assert not set(component['owned'])&set(range(1000,1006))
        records={record['buffer_id']:record for record in component['records']}
        for m in meta['matrices']:assert m['id'] not in records
        if name=='relocated':assert min(meta['owned'])>1005
        header=(output/'agnb.h').read_text()
        for matrix in ('base','work'):assert '#define GOLEM_FIELD_'+matrix+'_BYTES 36u' in header
        for record in ('vertices','camera','clipped'):assert '#define GOLEM_FIELD_'+record+'_BYTES 24u' in header
        (output/'consumer.asm').write_text(' .assume adl=1\n .org $40000\n jp start\n include "golem.inc"\nstart:\n call golem_projectFrame\n ret\n')
        run([ASSEMBLER,'consumer.asm','consumer.bin','-c'],cwd=output)
        reports.append(dict(layout=name,actual_assembler=True,bootstrap_bytes=(output/'bootstrap.bin').stat().st_size,
                            owned_ids=meta['owned'],matrices=meta['matrices'],borrowed=meta['borrowed']))
    # Full Corsair-sized SOURCE only: finite chunks avoid a new runtime loop or
    # a host per-vertex renderer. This verifies the public generation boundary.
    count=2685
    text='''Matrix(3,3) base = Identity
Matrix(3,3) work = Identity
Borrowed XYZF32 clipped = Buffer(1000,2685)
Borrowed XY16 zero = Buffer(1001,1)
Borrowed XY16 pixel = Buffer(1002,1)
Borrowed XYZ16 projected = Buffer(1003,2685)
Sub projectFrame
'''
    chunks=list(range(0,count,128))
    text+=''.join(f'chunk_{i:03}()\n' for i in range(len(chunks)))+'EndSub\n'
    for i,start in enumerate(chunks):
        text+=f'Sub chunk_{i:03}\n'
        for vertex in range(start,min(start+128,count)):
            offset=vertex*12
            text+=f'''matrix_set_value(work,base,0,0,read_float32(clipped,{offset+8}))
matrix_set_value(work,work,1,1,read_float32(clipped,{offset+8}))
matrix_set_value(work,work,0,2,read_float32(clipped,{offset}))
matrix_set_value(work,work,1,2,read_float32(clipped,{offset+4}))
matrix_invert(work)
transform_vertices(pixel,work,zero)
copy_bytes(projected,{vertex*6},pixel,0,4)
'''
        text+='EndSub\n'
    large=BUILD/'large.golem';large.write_text(text)
    output=BUILD/'large'
    run([COMPILER,'--stock-matrix','--emit-load-plan','--reserve','1000:1003',large,output])
    meta=json.loads((output/'interface.json').read_text())
    assert len(meta['entries'])==22 and max(e['bytes'] for e in meta['entries'])<=65535
    assert meta['entries'][0]['bytes']==21*6
    # Bad static inputs must fail before an existing package is touched.
    rejected=BUILD/'reject';rejected.mkdir(exist_ok=True)
    sentinel=rejected/'bootstrap.bin';sentinel.write_bytes(b'preserve existing output')
    bad=BUILD/'bad.golem';bad.write_text('Matrix(3,3) m = Identity Sub tick missing() EndSub')
    result=subprocess.run([str(COMPILER),'--stock-matrix','--emit-load-plan',str(bad),str(rejected)],capture_output=True,text=True)
    assert result.returncode and sentinel.read_bytes()==b'preserve existing output'
    assert sorted(p.name for p in rejected.iterdir())==['bootstrap.bin']
    report=dict(passed=True,legacy_packages=len(baseline['cases']),unchanged_legacy_files=legacy_count,
                packages=reports,large_source_vertices=count,large_source_subs=len(meta['entries']),
                large_bootstrap_bytes=(output/'bootstrap.bin').stat().st_size,
                largest_entry_bytes=max(e['bytes'] for e in meta['entries']),
                invalid_source_preserved_output=True,no_target_execution=True)
    (HERE/'cli-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report),flush=True)

if __name__=='__main__':main()
