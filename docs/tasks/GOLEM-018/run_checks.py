#!/usr/bin/env python3
"""Compiler contracts, sanitizer checks, prior-profile parity and real assembly."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

TASK = Path(__file__).resolve().parent
PROJECT = TASK.parents[2]
BUILD = TASK/'build'
BUILD.mkdir(exist_ok=True)
started = time.monotonic()
commands = []


def run(args,cwd=PROJECT,ok=True):
    tick = time.monotonic()
    p = subprocess.run(list(map(str,args)),cwd=cwd,capture_output=True,text=True)
    commands.append(dict(args=list(map(str,args)),seconds=round(time.monotonic()-tick,3)))
    if ok and p.returncode: raise RuntimeError(p.stdout+p.stderr)
    return p


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()


run(['make','-C','src','check','CXXFLAGS=-std=c++17 -Wall -Wextra -Werror -O2'])
sanitizer = BUILD/'test-sanitized'
run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-O1','-g','-fsanitize=address,undefined',
     '-fno-omit-frame-pointer',PROJECT/'tests/test_compiler.cpp','-o',sanitizer])
run([sanitizer])
compiler = PROJECT/'build/golemc'
baseline = BUILD/'baseline'; baseline.mkdir(exist_ok=True)
revision = run(['git','rev-parse','HEAD']).stdout.strip()
paths = run(['git','ls-tree','-r','--name-only','HEAD','src']).stdout.splitlines()
for path in paths:
    if path.endswith(('.hpp','.cpp')):
        (baseline/Path(path).name).write_bytes(subprocess.check_output(['git','show',f'{revision}:{path}'],cwd=PROJECT))
run(['g++','-std=c++17','-O2',baseline/'golemc.cpp','-o',baseline/'golemc'])
artifacts = ['bootstrap.bin','invoke.bin','update-call.bin','cleanup.bin','interface.json','golem.inc']
prior = []
for source in ['examples/resident/floor.golem','examples/resident/counted-floor.golem',
               'docs/tasks/GOLEM-013/checked-subtract.golem','docs/tasks/GOLEM-014/checked-multiply.golem',
               'docs/tasks/GOLEM-015/checked-divmod.golem','docs/tasks/GOLEM-017/table.golem']:
    for relocated in (False,True):
        options = ['--resident','--emit-asm']
        if relocated: options += ['--reserve','1:254','--reserve','260:264']
        if source.endswith('table.golem'): options += ['--table-data',f'samples={PROJECT}/docs/tasks/GOLEM-017/records.dat']
        work = BUILD/f'parity-{len(prior)}'
        for name,exe in [('old',baseline/'golemc'),('new',compiler)]:
            run([exe,*options,PROJECT/source,work/name])
        assert all((work/'old'/a).read_bytes()==(work/'new'/a).read_bytes() for a in artifacts),source
        prior.append(dict(source=source,relocated=relocated,artifacts=len(artifacts)))

# Assembly consumer independently checks all declared field offsets and widths,
# all package payloads, and the actual ADL wrapper instructions.
assembly_cases = []
assembler = PROJECT/'docs/tasks/GOLEM-007/build/ez80asm'
for name,options in [('default',[]),('relocated',['--reserve','1:254','--reserve','260:264']),
                     ('near-end',['--reserve','1:65521'])]:
    pkg = BUILD/f'host-{name}'
    run([compiler,'--resident','--emit-asm',*options,TASK/'geometry.golem',pkg])
    m = json.loads((pkg/'interface.json').read_text())
    entry = 'golem_'+m['entry']['name']
    names = [entry,entry+'_call',entry+'_call_len','golem_package_bootstrap','golem_package_bootstrap_len',
             'golem_package_update_call','golem_package_update_call_len','golem_package_cleanup','golem_package_cleanup_len']
    expected = []
    for g in m['globals']:
        names += ['golem_'+g['name']+suffix for suffix in ['_buffer','_offset','_bytes','_type']]
        expected += [g['id'],g['offset'],g['bytes'],5 if g['type']=='uint8' else 2]
    names += ['golem_inputs_buffer','golem_inputs_bytes']
    expected += [m['input']['id'],m['input']['bytes']]
    asm = BUILD/f'{name}-consumer.asm'; binary = BUILD/f'{name}-consumer.bin'
    asm.write_text('.assume adl=1\n.org $40000\n'+''.join('dl '+n+'\n' for n in names)+
                   f'include "host-{name}/golem.inc"\n')
    run([assembler,asm.name,binary.name,'-c'],cwd=BUILD)
    data = binary.read_bytes()
    values = [int.from_bytes(data[i*3:i*3+3],'little') for i in range(len(names))]
    assert values[9:]==expected
    offset = values[0]-0x40000
    assert data[offset:offset+11]==b'\x21'+values[1].to_bytes(3,'little')+b'\x01\x06\0\0\x5b\xdf\xc9'
    for i,artifact in zip((1,3,5,7),('invoke.bin','bootstrap.bin','update-call.bin','cleanup.bin')):
        assert data[values[i]-0x40000:values[i]-0x40000+values[i+1]]==(pkg/artifact).read_bytes()
    assembly_cases.append(dict(name=name,owned=len(m['owned']),installation=m['bootstrap_bytes'],
                               packed_inputs=m['input']['bytes'],call_bytes=m['call_bytes']))

# Unsupported source/options and export collisions preserve existing artifacts.
keep = BUILD/'preserve';keep.mkdir(exist_ok=True)
source = (TASK/'geometry.golem').read_text()
bad = BUILD/'bad.golem'
cases = [(['--reserve','1:65522'],source),([],source.replace('Sub drawGeometry','Sub package_bootstrap')),
         ([],source.replace('uint8 colour','uint8 inputs').replace('(colour,','(inputs,')),
         ([],source.replace('Bounds(0,63)','Bounds(0,255)')),([],source.replace('lineX1,','LineX1,')),
         (['--table-data','samples=missing'],source)]
for options,text in cases:
    bad.write_text(text)
    for a in artifacts: (keep/a).write_bytes(b'preserve')
    p = run([compiler,'--resident','--emit-asm',*options,bad,keep],ok=False)
    assert p.returncode==1 and all((keep/a).read_bytes()==b'preserve' for a in artifacts),p.stderr
report = dict(passed=True,seconds=round(time.monotonic()-started,3),baseline_revision=revision,
              prior_profiles=prior,assembly_cases=assembly_cases,prepublication_rejections=len(cases),
              sanitizers='address,undefined',commands=commands,
              source_sha256={str(p.relative_to(PROJECT)):sha(p) for p in
                             [*sorted((PROJECT/'src').glob('*.hpp')),PROJECT/'src/golemc.cpp',
                              PROJECT/'tests/test_compiler.cpp',TASK/'run_checks.py']})
(TASK/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Drawing host checks pass: {len(prior)} prior packages, {len(assembly_cases)} assembler consumers; {report["seconds"]}s')
