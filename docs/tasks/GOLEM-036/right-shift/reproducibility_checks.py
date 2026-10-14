"""Rebuild executable/storage artifacts in independent directories."""
from build_delivery import HERE,BUILD,build,sha
import json

def main():
    receipts=[]
    for name,prefix,source,mappings in [('probe',0,'probe',[]),('probe-relocated',254,'probe',[]),
                                        ('animation',0,'animation',[])]:
        original=BUILD/name;again=BUILD/'reproduce'/name
        previous={p.name:sha(p) for p in original.iterdir() if p.suffix in ('.bin','.inc','.c','.elf','.vdpl')}
        data={key:HERE/'data'/(key+'.dat') for key in mappings}
        build(HERE/(source+'.golem'),original,prefix,array_data=data)
        build(HERE/(source+'.golem'),again,prefix,array_data=data)
        for filename,digest in previous.items():
            assert sha(original/filename)==digest,('changed executable',name,filename)
            assert sha(again/filename)==digest,('path-dependent output',name,filename)
        for expected in (HERE/'evidence').glob('*-expected.json'):
            manifest=json.loads(expected.read_text())
            if manifest['source']==name:
                assert manifest['module_sha256']==sha(original/(source+'.vdpl'))
                assert manifest['source_sha256']==sha(HERE/(source+'.golem'))
        receipts.append({'package':name,'identical_files':len(previous),'manifest_sha256':sha(original/'delivery-manifest.json')})
    (HERE/'reproducibility-checks.json').write_text(json.dumps({'packages':receipts,'target_package_identities_preserved':True},indent=2)+'\n')
    print('Three independent package/storage rebuilds match qualified bytes.')
if __name__=='__main__':main()
