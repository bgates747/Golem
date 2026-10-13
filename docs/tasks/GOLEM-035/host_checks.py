"""Check typed retained-array semantics with independent model and native C."""
from pathlib import Path
import hashlib,json,os,random,struct,subprocess,time
from build_delivery import build,run,verify_pin,SDK,HERE,BUILD,ROOT,sha
from model import Machine,reference
word=lambda v:struct.pack('<H',v)
call=lambda key:bytes((23,0,160))+word(key)+b'\x01'
def load(directory):
    return [json.loads((directory/name).read_text()) for name in ('interface.json','program.json','root.json','backends.json')]
def arrays_initial(directory,interface):
    return {a['buffer']:bytearray((directory/a['initial_data']).read_bytes()) for a in interface.get('arrays',[])}
def vectors(directory):
    iface,ast,root,meta=load(directory);fields={f['name']:f for f in iface['globals']};initial=(directory/'state.bin').read_bytes();out=[]
    def add(entry,updates=None,words=None,mode=0,label=''):
        state=bytearray(initial);struct.pack_into('<H',state,28,entry);arrays=arrays_initial(directory,iface)
        for name,v in (updates or {}).items():struct.pack_into('<H',state,fields[name]['offset'],v&65535)
        if words is not None:
            a=next(a for a in iface['arrays'] if a['name']=='mutable');arrays[a['buffer']][:]=struct.pack('<6H',*words)
        out.append((entry,mode,bytes(state),arrays,label))
    for entry in range(10):
        for index in (0,1,4,5,6,255,256,257,65530,65533,65534,65535):
            for signed in (-3,-2,-1,0,1,2):add(entry,{'index':index,'signedIndex':signed},label='boundary')
    for index in range(257):add(5,{'index':index},label='all-sine')
    for index,value in ((0,65535),(6,65535),(65535,1),(4,2),(0,2)):
        add(7,{'index':index,'value':value},label='index-first')
        add(8,{'index':index,'value':value},label='index-arithmetic')
    for values in ([65535,0,0,0,0,0],[65535,1,0,0,0,0],[1,2,3,4,5,6]):
        add(3,{'index':0,'value':1},values,label='store-failure');add(9,words=values,label='loop-arithmetic')
    for mode in (1,2,3,4,5,6):add(2,mode=mode,label='physical')
    rng=random.Random(3501)
    for n in range(120):
        add(rng.randrange(10),{'index':rng.randrange(7),'signedIndex':rng.randrange(-3,3),'value':rng.choice((0,1,2,32767,65535))},[rng.randrange(65536) for _ in range(6)],label='mixed')
    # Backend-independent persistent state sequence. Each invocation is independently checked.
    state=initial;arrays=arrays_initial(directory,iface)
    for n in range(300):
        entry=2;state=bytearray(state);struct.pack_into('<H',state,28,entry)
        struct.pack_into('<H',state,fields['index']['offset'],n%6);struct.pack_into('<H',state,fields['value']['offset'],n)
        out.append((entry,0,bytes(state),{k:bytearray(v) for k,v in arrays.items()},'sequence'))
        state=reference(iface,ast,state,entry,root,arrays=arrays)
    return out

def oracle(directory,entry,mode,state,arrays):
    iface,ast,root,meta=load(directory);arrays={k:bytearray(v) for k,v in arrays.items()}
    if mode in (3,5):return state,arrays,{3:3,5:1}[mode]
    if mode in (1,2,4,6):
        result=bytearray(state);struct.pack_into('<H',result,12,8);return bytes(result),arrays,{1:2,2:3,4:7,6:3}[mode]
    result=reference(iface,ast,state,entry,root,arrays=arrays)
    return result,arrays,1 if struct.unpack_from('<HH',result,12)!=(0,0) else 0

def native_runner(directory):
    iface,ast,root,meta=load(directory)
    header=f"#define STATE_ID {meta['shared_state']}\n#define STATE_BYTES {meta['state_bytes']}\n#define ARGS_ID {meta['native_arguments']}\n#define ARRAY_COUNT {len(iface['arrays'])}\n"
    for name,values in [('ids',[a['buffer'] for a in iface['arrays']]),('bytes',[a['bytes'] for a in iface['arrays']]),('readonly',[int(a['access']==1) for a in iface['arrays']])]:
        header+='static const unsigned array_'+name+'[]={'+','.join(map(str,values))+'};\n'
    (directory/'fixture.h').write_text(header)
    runner=directory/'native-host'
    run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-ffp-contract=off','-fsanitize=address,undefined',
         '-I'+str(SDK/'include'),'-I'+str(directory),'-DGENERATED_SOURCE="'+str(directory/'native.c')+'"',HERE/'native_host.c','-o',runner])
    return runner

def main():
    start=time.monotonic();BUILD.mkdir(exist_ok=True);verify_pin();receipts=[]
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-ffp-contract=off']
    run(['g++',*flags,'-O2',HERE/'compiler.cpp','-o',BUILD/'compiler'])
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'compiler.cpp','-o',BUILD/'compiler-asan'])
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1'
    for prefix in (0,254):
        directory=BUILD/('probe' if not prefix else 'probe-relocated')
        build(HERE/'probe.golem',directory,prefix,compiler=BUILD/'compiler-asan',array_data={'sine':HERE/'data/sine.dat'})
        iface,ast,root,meta=load(directory);cases=vectors(directory);runner=native_runner(directory)
        order=[a['buffer'] for a in iface['arrays']]
        stdin=b''.join(struct.pack('<HH',entry,mode)+state+b''.join(arrays[k] for k in order) for entry,mode,state,arrays,label in cases)
        observed=run([runner],input=stdin,env=env);size=4+meta['state_bytes']+sum(a['bytes'] for a in iface['arrays']);assert len(observed)==len(cases)*size
        machine=Machine();machine.execute((directory/'common-install.bin').read_bytes());machine.execute((directory/'buffered-install.bin').read_bytes())
        for n,(entry,mode,state,arrays,label) in enumerate(cases):
            want,data,rc=oracle(directory,entry,mode,state,arrays)
            expect=struct.pack('<i',rc)+want+b''.join(data[k] for k in order)
            assert observed[n*size:(n+1)*size]==expect,('native',n,entry,mode,label)
            if not mode:
                machine.memory[meta['shared_state']][:]=state
                for key,value in arrays.items():machine.memory[key][:]=value
                machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]))
                assert bytes(machine.memory[meta['shared_state']])==want,('buffered-root',n,entry,label,bytes(machine.memory[meta['shared_state']]).hex(),want.hex())
                assert all(machine.memory[k]==data[k] for k in order),('buffered-arrays',n,entry,label)
        retained={k:bytes(machine.memory[k]) for k in [meta['shared_state'],*order]}
        machine.execute((directory/'buffered-cleanup.bin').read_bytes())
        assert all(bytes(machine.memory[k])==v for k,v in retained.items())
        assert not set(meta['buffered_owned'])&set(order)
        assert not set(meta['native_owned'])&set(order)
        receipts.append({'prefix':prefix,'cases':len(cases),'arrays':len(order),'sine_values':257,'private_cleanup_preserves_arrays':True,'max_leases':3})
    # The pinned D0 scalar source emits exactly the same scalar layouts/commands/C.
    for prefix in (0,254):
        d=BUILD/('scalar-parity' if not prefix else 'scalar-parity-relocated');build(ROOT/'docs/tasks/GOLEM-034/probe.golem',d,prefix,bindings=ROOT/'docs/tasks/GOLEM-034/bindings.json')
        old=ROOT/'docs/tasks/GOLEM-034/delivery/b7a6be65ce06789e/packages'/('default' if not prefix else 'relocated')
        for p in old.iterdir():
            if p.suffix in ('.bin','.inc','.c'):assert (d/p.name).read_bytes()==p.read_bytes(),('scalar-parity',p.name)
    maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
    receipt={'base':verify_pin(),'packages':receipts,'checks':sum(r['cases'] for r in receipts),'scalar_executable_layout_parity':True,'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3)}
    (HERE/'host-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Array host checks pass:',receipt['checks'])
if __name__=='__main__':main()
