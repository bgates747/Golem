#!/usr/bin/env python3
"""Host qualification of checked resident additions; no emulator launch."""
from pathlib import Path
import hashlib
import json
import subprocess
import time

from cases import cases, initial, expected
from command_model import Package, u16

TASK = Path(__file__).resolve().parent
ROOT = TASK.parents[2]
BUILD = TASK / 'build'
FLAGS = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-O2']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    BUILD.mkdir(exist_ok=True)
    started = time.monotonic()
    commands = []
    def run(args):
        tick = time.monotonic()
        result = subprocess.run([str(a) for a in args], cwd=ROOT, capture_output=True, text=True)
        commands.append({'command': [str(a) for a in args], 'seconds': round(time.monotonic()-tick, 3)})
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout.strip()
    source_paths = [*sorted((ROOT/'src').glob('*.hpp')), ROOT/'src/golemc.cpp', ROOT/'tests/test_compiler.cpp']
    before = {str(p.relative_to(ROOT)): digest(p) for p in source_paths}
    run(['make', '-C', 'src', 'check', 'CXXFLAGS='+' '.join(FLAGS)])
    cli = ROOT/'build/golemc'
    packages, models = [], []
    for name, options in [('default', []), ('relocated', ['--reserve', '1:254', '--reserve', '260:264', '--reserve', '50000:50000'])]:
        directory = BUILD/name
        run([cli, '--resident', *options, TASK/'checked-subtract.golem', directory])
        manifest = json.loads((directory/'interface.json').read_text())
        model = Package(directory, manifest)
        globals_ = {g['name']: g['id'] for g in manifest['globals']}
        status = manifest['runtime_error']['id']
        code_before = {key: bytes(model.memory[key]) for key in model.programs}
        for index, values in enumerate(cases()):
            for key, value in initial(values).items():
                model.memory[globals_[key]][:] = value.to_bytes(2, 'little')
            model.memory[manifest['output']['id']][:] = (4660).to_bytes(2, 'little')
            # Do not reset error status: the next invocation must clear a prior failure.
            model.text.clear()
            model.execute(model.invoke)
            want, result, error = expected(values)
            assert {key: u16(model.memory[id_]) for key, id_ in globals_.items()} == want, (name,index)
            assert int.from_bytes(model.memory[manifest['output']['id']], 'little', signed=True) == result
            assert u16(model.memory[status]) == error
            assert bytes(model.text) == (b'' if error else b'completed')
            assert all(bytes(model.memory[key]) == data for key,data in code_before.items())
            model.assert_external()
        # Calls without host updates: late tail overflow, repeated failure, then recovery.
        for key,value in initial(dict(a=3,b=2,c=1,selfValue=0,extra=0,tail=1)).items():
            model.memory[globals_[key]][:] = value.to_bytes(2,'little')
        model.execute(model.invoke)
        assert u16(model.memory[status]) == 0 and u16(model.memory[globals_['tail']]) == 0
        for _ in range(2):
            model.memory[globals_['completed']][:] = b'\x00\x00'
            model.execute(model.invoke)
            assert u16(model.memory[status]) == 2 and u16(model.memory[globals_['tail']]) == 0
            assert u16(model.memory[globals_['completed']]) == 0
        model.memory[globals_['tail']][:] = b'\xff\xff'
        model.execute(model.invoke)
        assert u16(model.memory[status]) == 0 and u16(model.memory[globals_['completed']]) == 1
        model.execute(model.cleanup)
        assert not any(key in model.memory for key in model.owned)
        model.assert_external()
        model.execute(model.install)
        assert u16(model.memory[status]) == 0 and u16(model.memory[globals_['total']]) == 48879
        assert u16(model.memory[globals_['tail']]) == 9
        models.append(model)
        packages.append({'name':name,'manifest':manifest,'arithmetic_jobs':len(cases())+4,
                         'artifacts_sha256':{p.name:digest(p) for p in sorted(directory.iterdir())}})
    assert models[0].normalized() == models[1].normalized(), 'Relocation altered operations'
    # Pure floor command artifacts stay identical to accepted stock evidence.
    run([cli, '--resident', ROOT/'examples/resident/floor.golem', BUILD/'pure-floor'])
    original = json.loads((ROOT/'docs/tasks/GOLEM-009/syntax-additions.json').read_text())
    for name,sha in original['floor_artifact_sha256'].items():
        assert digest(BUILD/'pure-floor'/name) == sha, name
    # Raw compatibility is intentional until a separate checked-raw tranche.
    baseline = BUILD/'baseline/src'
    baseline.mkdir(parents=True,exist_ok=True)
    for p in (ROOT/'src').iterdir():
        if p.suffix in ('.hpp','.cpp'):
            (baseline/p.name).write_bytes(subprocess.check_output(['git','show',f'ddb5e87:src/{p.name}'],cwd=ROOT))
    run(['g++',*FLAGS,baseline/'golemc.cpp','-o',BUILD/'baseline-golemc'])
    raw_cases=['print "hello"','uint16 x=65535 x=x+1 print "done"',
               'uint16 i For i=1 To 3 print "x" Next i',
               'uint16 i uint16 j For i=1 To 2 For j=1 To 3 print "x" Next j Next i']
    for index,source in enumerate(raw_cases):
        path=BUILD/f'raw-{index}.golem';path.write_text(source)
        run([cli,path,BUILD/'new.bin']);run([BUILD/'baseline-golemc',path,BUILD/'old.bin'])
        assert (BUILD/'new.bin').read_bytes()==(BUILD/'old.bin').read_bytes()
    # Failed validation leaves a pre-existing package untouched.
    output=BUILD/'rejected';output.mkdir(exist_ok=True)
    marker=output/'bootstrap.bin';marker.write_bytes(b'preserve')
    base=(TASK/'checked-subtract.golem').read_text()
    for source in [base.replace('a - b + c','0 - 1 + 1'), 'uint16 bad=0-1 '+base,
                   'uint16 earlier=1 uint16 bad=earlier-1 '+base]:
        path=BUILD/'invalid.golem';path.write_text(source)
        result=subprocess.run([str(cli),'--resident',str(path),str(output)],capture_output=True)
        assert result.returncode and marker.read_bytes()==b'preserve'
    assert sorted(p.name for p in output.iterdir())==['bootstrap.bin']
    run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-O1','-g','-fsanitize=address,undefined',
         '-fno-omit-frame-pointer',ROOT/'tests/test_compiler.cpp','-o',BUILD/'sanitized'])
    sanitized=run([BUILD/'sanitized'])
    after={str(p.relative_to(ROOT)):digest(p) for p in source_paths}
    assert before==after,'Sources changed during host qualification'
    report={'pass':True,'packages':packages,'host_model_jobs':sum(p['arithmetic_jobs'] for p in packages),
            'source_sha256':before,'pure_floor_five_artifact_parity':True,'raw_parity_cases':len(raw_cases),
            'rejected_packages_preserved':3,'sanitizer_output':sanitized,'commands':commands,
            'seconds':round(time.monotonic()-started,3),'scope':'Host command-byte model and compiler checks; not VDP execution.'}
    (TASK/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Host checks pass: {report["host_model_jobs"]} jobs, relocation, cleanup/reinstall, pure/raw parity, sanitizers; {report["seconds"]}s')


if __name__=='__main__':
    main()
