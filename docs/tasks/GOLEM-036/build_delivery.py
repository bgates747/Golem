"""Build a hash-identified D0 delivery using the frozen scalar compiler closure."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SDK=Path(os.environ.get('GOLEM_VDPLET_SDK',ROOT.parent/'agon-vdplet')).resolve()
BUILD=HERE/'build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def run(args,**kw):return subprocess.check_output([str(x) for x in args],**kw)
def verify_pin():
    pin=json.loads((HERE/'baseline-receipt.json').read_text())
    for f in pin['files']:
        if sha(ROOT/f['path'])!=f['sha256']:raise RuntimeError('Frozen compiler input changed: '+f['path'])
    for name,digest in pin['sdk_files'].items():
        if sha(SDK/name)!=digest:raise RuntimeError('Pinned SDK input changed: '+name)
    return pin
def build(source,output,prefix=0,bindings=None,compiler=None,array_data=None):
    pin=verify_pin();source=Path(source).resolve();output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    compiler=compiler or BUILD/'compiler';binding=bindings or HERE/'bindings.json'
    config=json.loads(Path(binding).read_text());assert config['schema']==1
    command=[compiler,source,output,prefix]
    for row in config['bindings']:command+=['--bind',row['role'],row['buffer'],row['offset'],row['bytes']]
    for name,path in (array_data or {}).items():command+=['--array-data',name+'='+str(Path(path).resolve())]
    print(run(command).decode().strip(),flush=True)
    print(run([sys.executable,SDK/'tools/build_module.py',output/'native.c','--name',output.name.removesuffix('-relocated'),
               '--build-id','0x474c3336','--output',output]).decode(),flush=False)
    module=json.loads((output/(output.name.removesuffix('-relocated')+'.json')).read_text())
    run([SDK/'build/check_image',output/(output.name.removesuffix('-relocated')+'.vdpl'),module['module']['relative_relocations']])
    toolchain=Path.home()/'.platformio/packages/toolchain-xtensa-esp32/bin'
    tools={name:sha(Path(shutil.which(name)).resolve()) for name in ('g++','gcc')}
    tools.update({name:sha(toolchain/name) for name in ('xtensa-esp32-elf-gcc','xtensa-esp32-elf-ld','xtensa-esp32-elf-strip')})
    frames=[]
    for line in (output/(output.name.removesuffix('-relocated')+'.su')).read_text().splitlines():
        name,size,kind=line.split('\t');assert kind=='static'
        frames.append({'function':name.rsplit(':',1)[-1],'bytes':int(size)})
    assert sum(f['bytes'] for f in frames)<=512
    resources={'module':module['module'],'stack_frames':frames,
               'conservative_sum_module_static_frames':sum(f['bytes'] for f in frames),
               'stack_scope':'module frames only; excludes firmware/service caller frames',
               'max_live_leases':3,'admitted_stack_budget':512,
               'admission_policy':'existing SDK limits; no capacity probe or limit change'}
    (output/'resources.json').write_text(json.dumps(resources,indent=2)+'\n')
    inputs={'source':{'path':str(source),'sha256':sha(source)},'bindings':{'path':str(binding),'sha256':sha(Path(binding))},
            'd0_layer':{n:sha(HERE/n) for n in ('compiler.cpp','root.hpp','portable.hpp','backends.hpp')},'baseline':pin,
            'host_compiler':run(['g++','--version']).decode().splitlines()[0],
            'tool_executable_sha256':tools,
            'native_toolchain':module['compiler'],'abi_header_sha256':module['abi_header_sha256'],
            'reservations_prefix':prefix,
            'array_data':{name:{'path':str(Path(path).resolve()),'sha256':sha(Path(path))} for name,path in (array_data or {}).items()}}
    manifest={'family':'R25-G0/root-v1','kind':'wide-integer-foundation-D1b','schema':1,'inputs':inputs,
              'interface':json.loads((output/'interface.json').read_text()),'root':json.loads((output/'root.json').read_text()),
              'backends':{k:v for k,v in json.loads((output/'backends.json').read_text()).items() if k!='programs'},
              'native_module':module['module'],'resources':resources,
              'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in sorted(output.iterdir())
                           if p.is_file() and p.suffix in ('.json','.inc','.bin','.c','.elf','.vdpl','.su') and p.name!='delivery-manifest.json'}}
    (output/'delivery-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--prefix',type=int,default=0);p.add_argument('--bindings',type=Path)
    p.add_argument('--array-data',action='append',default=[])
    a=p.parse_args();BUILD.mkdir(exist_ok=True)
    run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-O2',HERE/'compiler.cpp','-o',BUILD/'compiler'])
    build(a.source,a.output,a.prefix,a.bindings,array_data=dict(item.split("=",1) for item in a.array_data))
