#!/usr/bin/env python3
"""Promotion parity checks; generated fixtures stay in this task's build silo."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[3]
TASK = Path(__file__).resolve().parent
BUILD = TASK / 'build'
BUILD.mkdir(exist_ok=True)
started = time.monotonic()
commands = []
def run(args):
    tick = time.monotonic()
    result = subprocess.run([str(a) for a in args], cwd=ROOT, text=True, capture_output=True)
    commands.append({'command': [str(a) for a in args], 'seconds': round(time.monotonic()-tick,3)})
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

# Freeze the accepted checkpoint, including its original relative include layout.
checkpoint = 'ec91999'
paths = ['src/golemc.cpp', 'src/token_reader.hpp', 'src/buffer_allocator.hpp',
         'docs/tasks/GOLEM-006/golemc_integrated.cpp',
         'docs/tasks/GOLEM-006/integration.hpp', 'docs/tasks/GOLEM-006/scalar_backend.hpp']
for name in paths:
    destination = BUILD / 'baseline' / name
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(subprocess.check_output(['git','show',f'{checkpoint}:{name}'],cwd=ROOT))
flags = ['-std=c++17','-Wall','-Wextra','-Werror','-O2']
old = BUILD/'baseline-resident'
raw = BUILD/'baseline-raw'
run(['g++',*flags,BUILD/'baseline/docs/tasks/GOLEM-006/golemc_integrated.cpp','-o',old])
run(['g++',*flags,BUILD/'baseline/src/golemc.cpp','-o',raw])
run(['make','-C','src','check','CXXFLAGS='+' '.join(flags)])
compiler = ROOT/'build/golemc'
review = json.loads((ROOT/'docs/tasks/GOLEM-007/emulator-checks.json').read_text())
parity = []
for package in review['fixture']['packages']:
    args = package['command'][2:-2]
    source = ROOT/'examples/resident'/Path(package['command'][-2]).name
    output = BUILD/package['name']
    run([compiler,'--resident',*args,source,output])
    for name, expected in package['artifacts_sha256'].items():
        if name == 'interface.json':
            continue
        assert digest(output/name) == expected, (package['name'],name)
    metadata = json.loads((output/'interface.json').read_text())
    previous = dict(package['manifest'])
    previous.pop('experimental_task')
    previous['profile'] = 'resident-scalar'
    assert metadata == previous
    # Exercise example-input updates independently of the frozen review package.
    for value in ['-32768','-1.5','-0','0','1.401298464324817e-45','32767']:
        run([compiler,'--resident',*args,source,output,value])
        reference = BUILD/'reference'
        run([old,'--resident',*args,source,reference,value])
        for name in ['bootstrap.bin','cleanup.bin','invoke.bin','update-call.bin']:
            assert (output/name).read_bytes() == (reference/name).read_bytes()
    parity.append(package['name'])
raw_cases = ['print "hello"','uint16 x','uint16 x=65535 x=x+1 print "done\\n"',
             'uint16 i For i=1 To 3 print "x" Next i',
             'UINT16 i UINT16 j For i=1 To 2 For j=1 To 3 print "x" Next j Next i']
for index, source in enumerate(raw_cases):
    path=BUILD/f'raw-{index}.golem'; path.write_text(source)
    run([raw,path,BUILD/'old.bin']); run([compiler,path,BUILD/'new.bin'])
    assert (BUILD/'old.bin').read_bytes() == (BUILD/'new.bin').read_bytes()
# Semantic rejection must not mutate an existing package.
output=BUILD/'reject'; output.mkdir(exist_ok=True)
marker=output/'bootstrap.bin'; marker.write_bytes(b'preserve')
source=ROOT/'examples/resident/floor.golem'
invalid = [['--reserve','0:1'],['--reserve','65535:65535'],['--reserve','9:1'],
           ['--reserve','1:65534'],['--reserve','1:5','--reserve','3:7'],
           ['--reserve','1:x'],['--reserve','1'],['--reserve','1:2:3']]
for args in invalid:
    result=subprocess.run([str(compiler),'--resident',*args,str(source),str(output)],capture_output=True)
    assert result.returncode != 0 and marker.read_bytes()==b'preserve'
for value in ['nan','inf','32768','-32769','1 garbage','1e9999']:
    result=subprocess.run([str(compiler),'--resident',str(source),str(output),value],capture_output=True)
    assert result.returncode != 0 and marker.read_bytes()==b'preserve'
assert sorted(p.name for p in output.iterdir()) == ['bootstrap.bin']
# Standalone production build has no task implementation available.
standalone=BUILD/'standalone/src'; standalone.mkdir(parents=True,exist_ok=True)
for path in (ROOT/'src').iterdir():
    if path.suffix in {'.cpp','.hpp'}: shutil.copy2(path,standalone/path.name)
run(['g++',*flags,standalone/'golemc.cpp','-o',BUILD/'standalone-golemc'])
run([BUILD/'standalone-golemc','--resident',source,BUILD/'standalone-output'])
run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-O1','-g',
     '-fsanitize=address,undefined','-fno-omit-frame-pointer',ROOT/'tests/test_compiler.cpp','-o',BUILD/'sanitized'])
sanitizer_output=run([BUILD/'sanitized'])
report={'pass':True,'baseline_checkpoint':checkpoint,'review_package_parity':parity,
        'resident_boundary_layout_comparisons':24,'raw_comparisons':len(raw_cases),
        'cli_rejections_preserve_output':len(invalid)+6,'sanitizer_output':sanitizer_output.strip(),
        'standalone_source_build':True,'commands':commands,
        'production_sha256':{str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'src').glob('*')) if p.is_file()},
        'seconds':round(time.monotonic()-started,3)}
(TASK/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in {'commands','production_sha256'}},indent=2))
