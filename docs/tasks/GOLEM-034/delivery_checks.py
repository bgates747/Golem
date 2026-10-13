"""Check reproducible emitted bytes and the retained scalar/unbound profile."""
from pathlib import Path
import json,os,struct
from build_delivery import build,run,sha,SDK,HERE,BUILD
from model import Machine,reference

def main():
    receipts=[]
    for prefix in (0,254):
        original=BUILD/('probe' if not prefix else 'probe-relocated')
        repeated=BUILD/('reproduce/probe' if not prefix else 'reproduce/probe-relocated')
        previous={p.name:sha(p) for p in original.iterdir() if p.suffix in ('.bin','.inc','.c','.elf','.vdpl')}
        build(HERE/'probe.golem',original,prefix)
        build(HERE/'probe.golem',repeated,prefix)
        for name,digest in previous.items():
            assert sha(original/name)==digest,('original changed',name)
            assert sha(repeated/name)==digest,('path-dependent output',name)
        receipts.append({'prefix':prefix,'identical_executable_layout_files':len(previous),
                         'delivery_manifest_sha256':sha(original/'delivery-manifest.json')})
    unbound=BUILD/'unbound-bindings.json';unbound.write_text('{"schema":1,"bindings":[]}\n')
    output=BUILD/'scalar-compatibility'
    build(HERE/'scalar-compatibility.golem',output,bindings=unbound)
    meta=json.loads((output/'backends.json').read_text());iface=json.loads((output/'interface.json').read_text())
    ast=json.loads((output/'program.json').read_text());root=json.loads((output/'root.json').read_text());initial=(output/'state.bin').read_bytes()
    assert all(not b['buffer'] and not b['access'] for b in root['bindings'])
    want=reference(iface,ast,initial,0,root)
    assert struct.unpack_from('<HHHf',want,112)==(1,1,0,-0.5)
    machine=Machine();machine.execute((output/'common-install.bin').read_bytes());machine.execute((output/'buffered-install.bin').read_bytes())
    machine.execute(bytes((23,0,160))+struct.pack('<H',meta['buffered_entries'][0])+b'\x01')
    assert bytes(machine.memory[meta['shared_state']])==want
    runner=output/'native-host'
    run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-ffp-contract=off','-fsanitize=address,undefined',
         '-I'+str(SDK/'include'),'-DGENERATED_SOURCE="'+str(output/'native.c')+'"','-DSTATE_BYTES='+str(len(initial)),HERE/'native_host.c','-o',runner])
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1'
    assert run([runner],input=struct.pack('<HH',0,0)+initial,env=env)==struct.pack('<i',0)+want
    receipt={'rebuilds':receipts,'retained_scalar_types':['bool','uint16','sint16','float32'],
             'unaligned_float_offset':118,'unbound_descriptors':4,'native_and_buffered_source_parity':True}
    (HERE/'delivery-checks.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Delivery reproducibility and retained scalar checks pass.')
if __name__=='__main__':main()
