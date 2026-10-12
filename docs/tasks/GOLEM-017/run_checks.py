#!/usr/bin/env python3
"""Table admission/lowering checks using an independent byte interpreter."""
from pathlib import Path
import hashlib,json,struct,subprocess,time
from command_model import Package
import qualify

ROOT=Path(__file__).resolve().parents[3]
TASK=Path(__file__).resolve().parent
BUILD=TASK/'build'
BUILD.mkdir(exist_ok=True)
started=time.monotonic()
def run(args,ok=True,cwd=ROOT):
    p=subprocess.run(list(map(str,args)),capture_output=True,text=True,cwd=cwd)
    if ok and p.returncode:raise RuntimeError(p.stdout+p.stderr)
    return p

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
files=[*sorted((ROOT/'src').glob('*.hpp')),ROOT/'src/golemc.cpp',ROOT/'tests/test_compiler.cpp',
       TASK/'run_checks.py',TASK/'command_model.py',TASK/'qualify.py']
identity={str(p.relative_to(ROOT)):sha(p) for p in files}
run(['make','-C','src','check','CXXFLAGS=-std=c++17 -Wall -Wextra -Werror -O2'])
packages=[]
for name,count,opts in qualify.configurations():
    directory,m=qualify.prepare_package(name,count,opts)
    model=Package(directory,m)
    original={key:bytes(model.memory[key]) for key in model.programs}
    copier=next(k for k,v in model.roles.items() if v=='table.copy.template')
    def select(index):
        model.execute(model.decode(qualify.update(m['input']['id'],qualify.word(index))))
        before=bytes(model.memory[m['output']['id']]);text=len(model.text)
        model.execute(model.invoke)
        expected=qualify.rows(count)[index*4:index*4+4] if index<count else before
        assert bytes(model.memory[m['output']['id']])==expected,(name,index)
        assert struct.unpack('<H',model.memory[m['runtime_error']['id']])[0]==(0 if index<count else 5)
        assert model.text[text:]==(b'selected\n' if index<count else b'')
        assert model.memory[m['input']['id']]==qualify.word(index)
        model.assert_external()
    # Exhaust every possible uint16 index for the four-row schema. Additional
    # sizes exercise both halves of the lexicographic bound and all valid rows.
    indices=range(65536) if name=='default' else sorted({*range(count+2),255,256,257,258,511,512,32767,32768,65280,65534,65535})
    for index in indices:select(index)
    select(0);select(65535);select(0)
    for key,data in original.items():
        if key!=copier:assert model.memory[key]==data
    for row in range(count):assert model.memory[m['table']['base_id']+row]==qualify.rows(count)[row*4:row*4+4]
    model.execute(model.cleanup)
    assert not set(model.owned)&set(model.memory)
    model.assert_external()
    model.execute(model.install)
    assert model.memory[m['output']['id']]==struct.pack('<Hh',65535,-32768)
    packages.append({'name':name,'indices_checked':len(indices)+3,'depth':model.depth,'owned':len(model.owned)})
# Also admit the documented maximum shape, with a full-width packed record.
source=(TASK/'table.golem').read_text()
maxsource=source.replace('samples(0:3)','samples(0:1023)')
maxsource=maxsource.replace('sint16 delta','sint16 delta '+ ' '.join(f'uint16 f{i}' for i in range(30)))
maxsource=maxsource.replace('Sample(65535, -32768)','Sample(65535, -32768,'+','.join(['0']*30)+')')
maxpath=BUILD/'max.golem';maxpath.write_text(maxsource)
maxdata=BUILD/'max.dat';maxdata.write_bytes(bytes(range(64))*1024)
run([ROOT/'build/golemc','--resident','--emit-asm','--table-data',f'samples={maxdata}',maxpath,BUILD/'max'])
maxmeta=json.loads((BUILD/'max/interface.json').read_text());maxmodel=Package(BUILD/'max',maxmeta)
for index in (0,255,256,1023,1024,65535):
    previous=bytes(maxmodel.memory[maxmeta['output']['id']])
    maxmodel.execute(maxmodel.decode(qualify.update(maxmeta['input']['id'],qualify.word(index))))
    maxmodel.execute(maxmodel.invoke)
    assert bytes(maxmodel.memory[maxmeta['output']['id']])==(bytes(range(64)) if index<1024 else previous)
    assert struct.unpack('<H',maxmodel.memory[maxmeta['runtime_error']['id']])[0]==(0 if index<1024 else 5)
# Semantic and option failures must precede output publication.
keep=BUILD/'preserve';keep.mkdir(exist_ok=True)
artifacts=['bootstrap.bin','invoke.bin','cleanup.bin','update-call.bin','interface.json','golem.inc']
compiler=ROOT/'build/golemc'
base=[compiler,'--resident','--emit-asm','--table-data',f'samples={TASK/"records.dat"}']
short=BUILD/'short.dat';short.write_bytes(b'\0')
long=BUILD/'long.dat';long.write_bytes((TASK/'records.dat').read_bytes()+b'\0')
bad=BUILD/'bad.golem';bad.write_text(source.replace('Sub fetch','Sub package_bootstrap'))
fails=[base+[TASK/'table.golem',keep,'65536'],
       base+['--reserve','1:65520',TASK/'table.golem',keep],
       [compiler,'--resident',TASK/'table.golem',keep],
       [compiler,'--resident','--table-data','broken',TASK/'table.golem',keep],
       base+['--table-data',f'samples={short}',TASK/'table.golem',keep],
       base+[bad,keep]]
for data in [short,long,TASK/'nonexistent.dat']:
    fails.append([compiler,'--resident','--table-data',f'samples={data}',TASK/'table.golem',keep])
fails.append([compiler,'--resident','--table-data',f'wrong={TASK/"records.dat"}',TASK/'table.golem',keep])
fails.append(base+[ROOT/'examples/resident/floor.golem',keep])
for args in fails:
    for name in artifacts:(keep/name).write_bytes(b'preserve')
    p=run(args,ok=False);assert p.returncode==1,p.stderr
    assert all((keep/name).read_bytes()==b'preserve' for name in artifacts)
# Check actual assembled field exports and the named wrapper/payload bytes.
assembler=ROOT/'docs/tasks/GOLEM-007/build/ez80asm'
for name,count,opts in qualify.configurations():
    pkg=BUILD/name;m=json.loads((pkg/'interface.json').read_text())
    labels=['golem_fetch','golem_fetch_call','golem_fetch_call_len','golem_package_bootstrap','golem_package_bootstrap_len',
            'golem_package_cleanup','golem_package_cleanup_len','golem_selected_delta_buffer','golem_selected_delta_offset',
            'golem_selected_delta_type','golem_samples_base_buffer','golem_samples_rows','golem_samples_row_bytes']
    asm=BUILD/(name+'-consumer.asm');binary=BUILD/(name+'-consumer.bin')
    asm.write_text('.assume adl=1\n.org $40000\n'+''.join('dl '+v+'\n' for v in labels)+f'include "{name}/golem.inc"\n')
    run([assembler,asm.name,binary.name,'-c'],cwd=BUILD)
    data=binary.read_bytes();values=[int.from_bytes(data[i*3:i*3+3],'little') for i in range(len(labels))]
    w=values[0]-0x40000
    assert data[w:w+11]==b'\x21'+values[1].to_bytes(3,'little')+b'\x01\x06\0\0\x5b\xdf\xc9'
    for offset,length,file in [(values[1],values[2],'invoke.bin'),(values[3],values[4],'bootstrap.bin'),(values[5],values[6],'cleanup.bin')]:
        assert data[offset-0x40000:offset-0x40000+length]==(pkg/file).read_bytes()
    assert values[7:]==[m['output']['id'],2,2,m['table']['base_id'],count,4]
assert identity=={str(p.relative_to(ROOT)):sha(p) for p in files}
report={'pass':True,'scope':'Independent byte decoder/interpreter and actual ez80asm; not stock firmware.',
        'packages':packages,'maximum_shape':{'rows':1024,'fields':32,'row_bytes':64,'indices_checked':6},
        'prepublication_rejections':len(fails),'all_uint16_indices_checked_for_four_rows':True,
        'source_sha256':identity,'seconds':round(time.monotonic()-started,3)}
(TASK/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Table host checks pass: {sum(p["indices_checked"] for p in packages)+6} jobs; {report["seconds"]}s')
