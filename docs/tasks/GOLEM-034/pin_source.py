"""Export the committed scalar compiler closure; never copy a moving task tree."""
from pathlib import Path
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PIN='25901e5'
FILES=['docs/tasks/GOLEM-032/portable.hpp','docs/tasks/GOLEM-032/backends.hpp',
       'docs/tasks/GOLEM-032/compiler.cpp','src/ordinary.hpp','src/token_reader.hpp','src/buffer_allocator.hpp']
def run(*args):return subprocess.check_output(args,cwd=ROOT)
def sha(data):return hashlib.sha256(data).hexdigest()
def export():
    commit=run('git','rev-parse',PIN).decode().strip()
    receipts=[]
    for name in FILES:
        data=run('git','show',commit+':'+name)
        output=HERE/'baseline'/name;output.parent.mkdir(parents=True,exist_ok=True)
        if output.exists() and output.read_bytes()!=data:raise RuntimeError('Pinned export changed: '+str(output))
        output.write_bytes(data);receipts.append({'path':name,'sha256':sha(data),'bytes':len(data)})
    sdk=ROOT.parent/'agon-vdplet'
    sdk_files={str(p.relative_to(sdk)):sha(p.read_bytes()) for p in [sdk/'include/vdplet.h',sdk/'src/vdplet_image.h',sdk/'src/vdplet_runtime.h',sdk/'tests/check_image.cpp',sdk/'build/check_image',sdk/'tools/build_module.py',sdk/'tools/pack_elf.py'] if p.exists()}
    receipt={'compiler_commit':commit,'source_scope':'GOLEM-032 frozen scalar frontend/backends and complete local include closure',
             'files':receipts,'sdk_commit':subprocess.check_output(['git','-C',str(sdk),'rev-parse','HEAD'],text=True).strip(),
             'sdk_files':sdk_files,'sdk_dirty':subprocess.check_output(['git','-C',str(sdk),'status','--porcelain'],text=True).splitlines(),
             'd0_layer':'root.hpp and compiler.cpp in GOLEM-034; retains committed scalar/float grammar; does not claim arrays/wide source arithmetic'}
    (HERE/'baseline-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Exported compiler closure at '+commit)
if __name__=='__main__':export()
