"""Freeze D0 source, packages and qualification receipts in a content-addressed silo."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    payload={}
    source=['compiler.cpp','root.hpp','build_delivery.py','baseline-receipt.json','probe.golem','bindings.json']
    for name in source:payload['source/docs/tasks/GOLEM-034/'+name]=(HERE/name).read_bytes()
    for p in sorted((HERE/'baseline').rglob('*')):
        if p.is_file():payload['source/docs/tasks/GOLEM-034/'+str(p.relative_to(HERE))]=p.read_bytes()
    packages={}
    for name,directory in [('default',HERE/'build/probe'),('relocated',HERE/'build/probe-relocated')]:
        manifest=json.loads((directory/'delivery-manifest.json').read_text())
        for filename,receipt in manifest['artifacts'].items():
            data=(directory/filename).read_bytes();assert len(data)==receipt['bytes'] and sha(data)==receipt['sha256']
            payload['packages/'+name+'/'+filename]=data
        payload['packages/'+name+'/delivery-manifest.json']=(directory/'delivery-manifest.json').read_bytes()
        packages[name]={'manifest':'packages/'+name+'/delivery-manifest.json','sha256':sha(payload['packages/'+name+'/delivery-manifest.json'])}
    for name in ('host-checks.json','delivery-checks.json','interface-acceptance.json','rally25-float-baseline-accepted-eight.json',
                 'rally-oracle-handoff-v2.json','review-screen.png'):
        payload['receipts/'+name]=(HERE/name).read_bytes()
    for p in sorted((HERE/'evidence').glob('*')):
        if p.suffix in ('.json','.dat'):payload['receipts/target/'+p.name]=p.read_bytes()
    manifest={'schema':1,'kind':'R25-G0/root-v1 D0 immutable scalar delivery',
              'compiler_baseline':json.loads((HERE/'baseline-receipt.json').read_text())['compiler_commit'],
              'status':'host/stock/actual-Xtensa qualified; Author visual review and commit approval pending',
              'packages':packages,'files':{name:{'bytes':len(data),'sha256':sha(data)} for name,data in sorted(payload.items())}}
    content=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode();identity=sha(content)
    destination=HERE/'delivery'/identity[:16];destination.mkdir(parents=True,exist_ok=True)
    payload['bundle.json']=content
    for name,data in payload.items():
        p=destination/name;p.parent.mkdir(parents=True,exist_ok=True)
        if p.exists():assert p.read_bytes()==data,'Immutable delivery changed: '+str(p)
        else:p.write_bytes(data)
    assert {str(p.relative_to(destination)) for p in destination.rglob('*') if p.is_file()}==set(payload)
    pointer={'bundle':str(destination.relative_to(HERE)),'sha256':identity,'package_manifests':packages}
    (HERE/'delivery.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(pointer,indent=2))
if __name__=='__main__':main()
