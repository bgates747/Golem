"""Freeze qualified D1b foundation packages and their complete local build source closure."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    payload={};pin=json.loads((HERE/'baseline-receipt.json').read_text())
    source=['compiler.cpp','root.hpp','portable.hpp','backends.hpp','build_delivery.py','baseline-receipt.json','probe.golem','animation.golem','bindings.json']
    for name in source:payload['source/docs/tasks/GOLEM-036/'+name]=(HERE/name).read_bytes()
    for f in pin['files']:payload['source/'+f['path']]=(ROOT/f['path']).read_bytes()
    for name in ('ordinary.hpp','token_reader.hpp','buffer_allocator.hpp'):payload['source/src/'+name]=(ROOT/'src'/name).read_bytes()
    for p in sorted((HERE/'data').iterdir()):
        if p.is_file():payload['source/docs/tasks/GOLEM-036/data/'+p.name]=p.read_bytes()
    packages={}
    for name,basename in [('default','probe'),('relocated','probe-relocated'),('animation','animation')]:
        d=HERE/'build'/basename;manifest=json.loads((d/'delivery-manifest.json').read_text())
        for filename,receipt in manifest['artifacts'].items():
            data=(d/filename).read_bytes();assert len(data)==receipt['bytes'] and sha(data)==receipt['sha256']
            payload['packages/'+name+'/'+filename]=data
        data=(d/'delivery-manifest.json').read_bytes();payload['packages/'+name+'/delivery-manifest.json']=data
        packages[name]={'manifest':'packages/'+name+'/delivery-manifest.json','sha256':sha(data)}
    for name in ('host-checks.json','animation-checks.json','rally-checks.json','array-boundary-checks.json','frontend-checks.json','reproducibility-checks.json',
                 'review-screen.png'):
        payload['receipts/'+name]=(HERE/name).read_bytes()
    for p in sorted((HERE/'evidence').glob('*')):
        if p.suffix in ('.json','.dat'):payload['receipts/target/'+p.name]=p.read_bytes()
    manifest={'schema':1,'kind':'R25-G0/root-v1 D1b immutable wide integer foundation delivery',
              'base_commit':pin['compiler_commit'],'status':'automatic qualification passes; Author human review/publication pending',
              'packages':packages,'files':{name:{'bytes':len(data),'sha256':sha(data)} for name,data in sorted(payload.items())}}
    content=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode();identity=sha(content);payload['bundle.json']=content
    d=HERE/'delivery'/identity[:16];d.mkdir(parents=True,exist_ok=True)
    for name,data in payload.items():
        p=d/name;p.parent.mkdir(parents=True,exist_ok=True)
        if p.exists():assert p.read_bytes()==data,'Immutable delivery changed'
        else:p.write_bytes(data)
    assert {str(p.relative_to(d)) for p in d.rglob('*') if p.is_file()}==set(payload)
    pointer={'bundle':str(d.relative_to(HERE)),'sha256':identity,'packages':packages}
    (HERE/'delivery.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(pointer,indent=2))
if __name__=='__main__':main()
