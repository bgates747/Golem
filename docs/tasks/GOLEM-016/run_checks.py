#!/usr/bin/env python3
"""Assemble real consumers; compare bytes, exported locations and ADL wrappers."""
from pathlib import Path
import hashlib,json,subprocess,time

ROOT=Path(__file__).resolve().parents[3]
TASK=Path(__file__).resolve().parent
BUILD=TASK/'build'
BUILD.mkdir(exist_ok=True)
start=time.monotonic()
commands=[]
def run(args,cwd=ROOT,ok=True):
    tick=time.monotonic()
    p=subprocess.run([str(a) for a in args],cwd=cwd,capture_output=True,text=True)
    commands.append({'args':[str(a) for a in args],'seconds':round(time.monotonic()-tick,3)})
    if ok and p.returncode:raise RuntimeError(p.stdout+p.stderr)
    return p

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
sources=['src/golemc.cpp','src/resident.hpp','src/assembly_output.hpp','src/ordinary.hpp','src/scalar_backend.hpp','src/token_reader.hpp','src/buffer_allocator.hpp','tests/test_compiler.cpp']
before={p:digest(ROOT/p) for p in sources}
flags=['-std=c++17','-Wall','-Wextra','-Werror','-O2']
run(['make','-C','src','check','CXXFLAGS='+' '.join(flags)])
# Frozen compiler proves output-mode work changes none of the existing artifacts.
oldsrc=BUILD/'baseline/src';oldsrc.mkdir(parents=True,exist_ok=True)
for p in sources:
    if p=='src/assembly_output.hpp' or p.startswith('tests/'):continue
    (oldsrc/Path(p).name).write_bytes(subprocess.check_output(['git','show',f'e52a427:{p}'],cwd=ROOT))
old=BUILD/'baseline-golemc'
run(['g++',*flags,oldsrc/'golemc.cpp','-o',old])
compiler=ROOT/'build/golemc'
assembler=ROOT/'docs/tasks/GOLEM-007/build/ez80asm'
artifacts=['bootstrap.bin','invoke.bin','cleanup.bin','update-call.bin','interface.json']
fixtures=['examples/resident/floor.golem','examples/resident/counted-floor.golem',
          'docs/tasks/GOLEM-013/checked-subtract.golem','docs/tasks/GOLEM-014/checked-multiply.golem',
          'docs/tasks/GOLEM-015/checked-divmod.golem']
reports=[]
for ix,source in enumerate(fixtures):
 for relocated in (False,True):
    work=BUILD/f'case-{ix}-{int(relocated)}';work.mkdir(exist_ok=True)
    reserves=['--reserve','1:254','--reserve','260:264','--reserve','50000:50000'] if relocated else []
    run([old,'--resident',*reserves,ROOT/source,work/'old','-0.75'])
    run([compiler,'--resident',*reserves,ROOT/source,work/'plain','-0.75'])
    # Flag works before or after reservations.
    opts=[*reserves,'--emit-asm'] if relocated else ['--emit-asm',*reserves]
    run([compiler,'--resident',*opts,ROOT/source,work/'package','-0.75'])
    pkg=work/'package'
    for name in artifacts:
        assert (pkg/name).read_bytes()==(work/'plain'/name).read_bytes()==(work/'old'/name).read_bytes(),(source,name)
    assert not (work/'plain/golem.inc').exists()
    meta=json.loads((pkg/'interface.json').read_text());entry='golem_'+meta['entry']['name']
    streams=[(entry+'_call','invoke.bin'),('golem_package_bootstrap','bootstrap.bin'),
             ('golem_package_update_call','update-call.bin'),('golem_package_cleanup','cleanup.bin')]
    names=[entry]
    for label,_ in streams:names += [label,label+'_len']
    fields=[(meta['input'],3),(meta['output'],2),*[(g,1) for g in meta['globals']]]
    if 'runtime_error' in meta:
        fields.append((dict(meta['runtime_error'],name='runtime_error',offset=0,bytes=2),1))
    expected=[]
    for f,t in fields:
        names += ['golem_'+f['name']+suffix for suffix in ['_buffer','_offset','_bytes','_type']]
        expected += [f['id'],f['offset'],f['bytes'],t]
    names += [entry+'_buffer','golem_package_owned_count']
    expected += [meta['entry']['id'],len(meta['owned'])]
    consumer='.assume adl=1\n.org $40000\n'+''.join('    dl '+n+'\n' for n in names)+'    include "package/golem.inc"\n'
    (work/'consumer.asm').write_text(consumer)
    run([assembler,'consumer.asm','consumer.bin','-l','-c'],cwd=work)
    data=(work/'consumer.bin').read_bytes()
    words=[int.from_bytes(data[i*3:i*3+3],'little') for i in range(len(names))]
    assert words[9:]==expected
    wrapper=words[0]-0x40000
    packet=words[1]
    target=packet.to_bytes(3,'little')
    assert data[wrapper:wrapper+11]==b'\x21'+target+b'\x01\x06\x00\x00\x5b\xdf\xc9'
    for n,(_,binary) in enumerate(streams):
        offset=words[1+2*n]-0x40000;size=words[2+2*n]
        assert data[offset:offset+size]==(pkg/binary).read_bytes(),(source,binary)
    reports.append({'source':source,'relocated':relocated,'assembled_bytes':len(data),
                    'include_sha256':digest(pkg/'golem.inc'),'all_five_existing_artifacts_match_e52a427':True,
                    'payload_bytes_and_exported_fields_match':True,'wrapper_opcodes_match':True})
# Assemble the maintained usage example with a literal named eZ80 CALL.
run([compiler,'--resident','--emit-asm',TASK/'named-call.golem',BUILD/'demo'])
run([assembler,'consumer.asm','build/consumer.bin','-l','-c'],cwd=TASK)
# Name collisions must fail before any output changes. Same source still compiles
# without assembly emission; output-only constraints do not alter source semantics.
preserve=BUILD/'preserve';preserve.mkdir(exist_ok=True)
floor=(ROOT/fixtures[0]).read_text()
casefold='uint16 X=0\n'+floor
reserved=floor.replace('Sub compute','Sub package_bootstrap')
long='uint16 '+('a'*65)+'=0\n'+floor
failures=[]
for n,src in enumerate([casefold,reserved,long]):
    path=BUILD/f'bad-{n}.golem';path.write_text(src)
    for name in artifacts+['golem.inc']:(preserve/name).write_bytes(b'preserve')
    p=run([compiler,'--resident','--emit-asm',path,preserve],ok=False)
    assert p.returncode==1,p.stderr
    assert all((preserve/name).read_bytes()==b'preserve' for name in artifacts+['golem.inc'])
    run([compiler,'--resident',path,BUILD/f'no-asm-{n}'])
    failures.append(p.stderr.strip())
for args in [['--resident','--emit-asm','--emit-asm',ROOT/fixtures[0],preserve],['--emit-asm',ROOT/fixtures[0],preserve]]:
    p=run([compiler,*args],ok=False);assert p.returncode!=0
    failures.append(p.stderr.strip())
# Sanitizers cover maintained source and generated-symbol validation.
sanitized=BUILD/'test-sanitized'
run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-g','-O1','-fsanitize=address,undefined',ROOT/'tests/test_compiler.cpp','-o',sanitized])
sanitizer=run([sanitized]).stdout.strip()
assert before=={p:digest(ROOT/p) for p in sources}
report={'pass':True,'scope':'Host compiler and real ez80asm consumer; no emulator execution',
        'packages':reports,'failed_publication_cases':failures,'sanitizer_output':sanitizer,
        'source_sha256':before,'assembler_sha256':digest(assembler),'commands':commands,
        'seconds':round(time.monotonic()-start,3)}
(TASK/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Assembly interface checks pass: {len(reports)} packages; binary/manifest parity, symbols, wrapper bytes and preserved rejected outputs.')
print(sanitizer)
print(f'Total: {report["seconds"]} seconds.')
