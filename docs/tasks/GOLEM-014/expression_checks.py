#!/usr/bin/env python3
"""Check expression precedence, evaluation order and original-value aliasing."""
from pathlib import Path
import hashlib
import json
import subprocess
from command_model import Package, u16

TASK = Path(__file__).resolve().parent
ROOT = TASK.parents[2]
BUILD = TASK/'build/expressions'


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    # Explicit mathematical outcomes, independent of parsing/emission algorithms.
    cases = [
        ('a+b*c', (2, 3, 4, 0), 14, 0),
        ('a*b+c*d', (2, 3, 4, 5), 26, 0),
        ('a*b-c*d', (3, 4, 2, 5), 2, 0),
        ('2*a+3', (4, 0, 0, 0), 11, 0),
        ('a*2*3', (4, 0, 0, 0), 24, 0),
        ('a*b*0', (65535, 2, 0, 0), 48879, 3),
        ('a*0*b', (65535, 65535, 0, 0), 0, 0),
        ('a-b+c*d', (0, 1, 65535, 2), 48879, 2),
        ('a+b+c*d', (65535, 1, 65535, 2), 48879, 1),
        ('a-b*c', (0, 65535, 2, 0), 48879, 3),
        ('total*total', (0, 0, 0, 0), 48879, 3),
        ('total*0+total', (0, 0, 0, 0), 48879, 0),
        ('a*b+b*a', (1, 32767, 0, 0), 65534, 0),
        ('a*b+b*a', (1, 32768, 0, 0), 48879, 1),
    ]
    results = []
    for index, (expression, values, want, error) in enumerate(cases):
        declarations = ' '.join(f'uint16 {name}={value}' for name, value in zip('abcd', values))
        source = declarations + ' uint16 total=48879 uint16 done=0 ' + \
            'float32 x Range(-32768,32767)=-0.75 sint16 result Sub compute ' + \
            f'total={expression} done=1 result=sint16(floor(x)) EndSub'
        path = BUILD/f'{index}.golem'
        path.write_text(source)
        output = BUILD/str(index)
        subprocess.run([str(ROOT/'build/golemc'), '--resident', str(path), str(output)],
                       check=True, capture_output=True)
        manifest = json.loads((output/'interface.json').read_text())
        model = Package(output, manifest)
        model.execute(model.invoke)
        fields = {g['name']: g['id'] for g in manifest['globals']}
        actual = (u16(model.memory[fields['total']]),
                  u16(model.memory[manifest['runtime_error']['id']]))
        assert actual == (want, error), (expression, actual, want, error)
        assert u16(model.memory[fields['done']]) == (0 if error else 1)
        assert {name:u16(model.memory[fields[name]]) for name in 'abcd'} == dict(zip('abcd',values))
        results.append(dict(expression=expression, values=values, result=want, error=error))
    report = dict(passed=True, cases=results, scope='Independent command model; not VDP execution',
                  source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sorted((ROOT/'src').glob('*.hpp'))})
    (TASK/'expression-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Expression checks pass: {len(cases)} precedence/order/aliasing cases.')


if __name__ == '__main__':
    main()
