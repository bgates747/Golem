#!/usr/bin/env python3
"""Independent paired-result aliasing, argument-order and error checks."""
from pathlib import Path
import hashlib
import json
import subprocess
from command_model import Package, u16

TASK=Path(__file__).resolve().parent
ROOT=TASK.parents[2]
BUILD=TASK/'build/arguments'


def main():
    BUILD.mkdir(parents=True,exist_ok=True)
    # args, destinations, initial a/b/q/r, mathematical results, resident error
    cases=[
        ('a,b',('q','r'),(17,5,400,500),(3,2),0),
        ('a,b',('a','b'),(17,5,400,500),(3,2),0),
        ('a,b',('b','a'),(17,5,400,500),(3,2),0),
        ('q+r,r',('q','r'),(0,0,17,5),(4,2),0),
        ('a+1,b*2',('q','r'),(16,2,400,500),(4,1),0),
        ('a*b,b+1',('q','r'),(17,5,400,500),(14,1),0),
        ('a+1,b',('q','r'),(65535,0,400,500),None,1),
        ('a,b+1',('q','r'),(17,65535,400,500),None,1),
        ('a*b,b',('q','r'),(65535,2,400,500),None,3),
        ('a-b,b',('q','r'),(0,1,400,500),None,2),
        ('a,0',('q','r'),(17,5,400,500),None,4),
        ('a,1-1',('q','r'),(17,5,400,500),None,4),
        ('a,b',('q','r'),(65535,65535,400,500),(1,0),0),
        ('a,b',('q','r'),(0,65535,400,500),(0,0),0),
        ('a*b,b',('q','r'),(0,0,400,500),None,4),
        ('a,b',('a','b'),(17,0,400,500),None,4),
    ]
    evidence=[]
    for index,(args,destinations,values,results,error) in enumerate(cases):
        initial=dict(zip('abqr',values));initial['done']=0
        declarations=' '.join(f'uint16 {name}={value}' for name,value in initial.items())
        statement=f'({destinations[0]},{destinations[1]})=divmod({args})'
        source=declarations+' float32 x Range(-32768,32767)=-0.75 sint16 result '+\
            f'Sub compute {statement} done=1 result=sint16(floor(x)) EndSub'
        path=BUILD/f'{index}.golem';path.write_text(source);output=BUILD/str(index)
        subprocess.run([str(ROOT/'build/golemc'),'--resident',str(path),str(output)],check=True,capture_output=True)
        manifest=json.loads((output/'interface.json').read_text())
        model=Package(output,manifest);model.execute(model.invoke)
        fields={g['name']:g['id'] for g in manifest['globals']}
        want=initial.copy()
        if not error:want.update(zip(destinations,results));want['done']=1
        actual={name:u16(model.memory[id_]) for name,id_ in fields.items()}
        assert actual==want,(statement,actual,want)
        assert u16(model.memory[manifest['runtime_error']['id']])==error
        floor=int.from_bytes(model.memory[manifest['output']['id']],'little',signed=True)
        assert floor==(0 if error else -1)
        evidence.append(dict(statement=statement,initial=initial,expected=want,error=error))
    report=dict(passed=True,cases=evidence,scope='Independent command model; not VDP execution',
                source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in sorted((ROOT/'src').glob('*.hpp'))})
    (TASK/'argument-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Paired argument/alias/error checks pass: {len(cases)} cases.')


if __name__=='__main__':main()
