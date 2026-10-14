"""Compile native-only renderer bindings using the task-local C++ compiler."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,time
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
SDK=ROOT.parent/'agon-vdplet'
RALLY=ROOT.parent/'AgonArcade'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(args):return subprocess.check_output([str(x) for x in args],stderr=subprocess.STDOUT,text=True)
def build(source,output,bindings,prefix=0):
    started=time.monotonic();source=Path(source).resolve();output=Path(output).resolve();bindings=Path(bindings).resolve()
    output.mkdir(parents=True,exist_ok=True)
    pin=json.loads((HERE.parent/'helper-pin.json').read_text())
    helper=output/'helper';helper.mkdir(exist_ok=True)
    for name,digest in pin['copied_sources'].items():
        path=HERE.parent/'helper'/name;assert sha(path)==digest;shutil.copyfile(path,helper/name)
    command=[HERE/'build/compiler',source,output,prefix,'--target','native','--native-bindings',bindings]
    for array in ('routes','sine'):command+=['--array-data',array+'='+str(RALLY/'docs/tasks/RALLY-25/route-kernels'/(array+'.bin'))]
    print(run(command).strip(),flush=True)
    print(run([sys.executable,SDK/'tools/build_module.py',output/'native.c','--name','rally-render','--build-id','0x474c3731','--output',output]),flush=True)
    module=json.loads((output/'rally-render.json').read_text())
    print(run([SDK/'build/check_image',output/'rally-render.vdpl',module['module']['relative_relocations']]).strip())
    iface=json.loads((output/'interface.json').read_text())
    iface['native_bindings'].update(helper_header_sha256=sha(helper/'rally_render.h'),
        helper_symbols=['rally_render_bind_v1','rally_render_prepare_v1','rally_render_stop_v1'],
        context_owner='module_private_value_only',completion_owner='application_retained',
        completion=json.loads(bindings.read_text())['completion'],configuration_sha256=sha(bindings))
    (output/'interface.json').write_text(json.dumps(iface,indent=2)+'\n')
    frames=[]
    for line in (output/'rally-render.su').read_text().splitlines():
        name,size,kind=line.split('\t');assert kind=='static';frames.append(dict(function=name.rsplit(':',1)[-1],bytes=int(size)))
    receipt={'schema':1,'status':'buildable integration candidate; host/target/human qualification not implied',
        'source':str(source),'source_sha256':sha(source),'binding_sha256':sha(bindings),
        'compiler_inputs':{n:sha(HERE/n) for n in ('compiler.cpp','native_bindings.hpp','root.hpp','portable.hpp','backends.hpp')},
        'sdk_files':{n:sha(SDK/n) for n in ('include/vdplet.h','src/vdplet_policy.h','src/vdplet_image.h','src/vdplet_runtime.h','tools/build_module.py')},
        'module':module['module'],'frames':frames,'build_seconds':round(time.monotonic()-started,3),
        'artifacts':{p.name:dict(bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(output.iterdir())
                     if p.is_file() and p.name!='delivery-manifest.json' and p.suffix not in ('.o','.so')}}
    (output/'delivery-manifest.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Candidate:',output,flush=True)
    return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source');p.add_argument('output');p.add_argument('--native-bindings',required=True);p.add_argument('--prefix',type=int,default=0);a=p.parse_args()
    build(a.source,a.output,a.native_bindings,a.prefix)
