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

def vectors(directory):
    iface,ast,root,meta=load(directory);fields={f['name']:f for f in iface['globals']};entries={e['name']:e['logical_id'] for e in iface['entries']}
    initial=(directory/'state.bin').read_bytes();out=[]
    def add(name,updates=None,mode=0,label=''):
        entry=entries[name];state=bytearray(initial);struct.pack_into('<H',state,28,entry)
        for key,n in (updates or {}).items():
            f=fields[key];struct.pack_into('<I' if f['bytes']==4 else '<H',state,f['offset'],n&((1<<(f['bytes']*8))-1))
        out.append((entry,mode,bytes(state),arrays_initial(directory,iface),label))
    unsigned=[0,1,65535,65536,0x7fffffff,0x80000000,0xfffffffe,0xffffffff]
    signed=[-2147483648,-2147483647,-65536,-38400,-1,0,1,38400,65535,2147483646,2147483647]
    for a in unsigned:
        for b in unsigned:
            for name in ('addUnsigned','subtractUnsigned'):add(name,{'ua':a,'ub':b},label='unsigned-arithmetic')
            for op in ('Equal','NotEqual','Less','Greater','LessEqual','GreaterEqual'):add(op+'Unsigned',{'ua':a,'ub':b},label='unsigned-comparison')
    for a in signed:
        for b in signed:
            for name in ('addSigned','subtractSigned'):add(name,{'sa':a,'sb':b},label='signed-arithmetic')
            for op in ('Equal','NotEqual','Less','Greater','LessEqual','GreaterEqual'):add(op+'Signed',{'sa':a,'sb':b},label='signed-comparison')
    for a in unsigned:
        for name in ('unsignedToSigned','narrowUnsigned','unsignedToNarrowSigned','identityUnsigned','indexConversion','skippedWideConversion'):
            add(name,{'ua':a},label='conversion')
    for a in signed:
        for name in ('signedToUnsigned','narrowSigned','signedToNarrowUnsigned','identitySigned'):add(name,{'sa':a},label='conversion')
    for a in (0,1,32767,32768,65535):add('widenUnsigned',{'un':a},label='widen')
    for a in (-32768,-1,0,1,32767):
        add('widenSigned',{'sn':a},label='widen');add('widenSignedToUnsigned',{'sn':a},label='widen')
    for index in (0,1,2,3,4,65535):
        add('readUnsigned',{'index':index},label='array-read')
        for a,b in ((0xffffffff,1),(65536,3276799),(0,0)):
            add('storeUnsigned',{'index':index,'ua':a,'ub':b},label='array-store')
    for index in (-3,-2,-1,0,1,2,32767):
        add('readSigned',{'signedIndex':index},label='array-read')
        for a,b in ((-2147483648,1),(-38400,38400),(2147483647,-1)):
            add('storeSigned',{'signedIndex':index,'sa':a,'sb':b},label='array-store')
    for n in (0,65535,65536,6553600,6553601,0xffffffff):add('boundedUnsigned',{'ua':n},label='bounds')
    for n in (-2147483648,-38401,-38400,0,38400,38401,2147483647):add('boundedSigned',{'sa':n},label='bounds')
    add('priorEffect',{'ua':0,'ub':65536},label='prior-effect')
    for mode in (1,2,3,4,5,6):add('addUnsigned',{'ua':65536,'ub':1},mode=mode,label='physical')
    rng=random.Random(3601)
    for n in range(240):
        if n%2:add('addUnsigned' if n%4==1 else 'subtractUnsigned',{'ua':rng.randrange(1<<32),'ub':rng.randrange(1<<32)},label='mixed')
        else:add('addSigned' if n%4==0 else 'subtractSigned',{'sa':rng.randrange(-(1<<31),1<<31),'sb':rng.randrange(-(1<<31),1<<31)},label='mixed')
    # Independent Rally primitive vectors; larger expressions/division remain later.
    rally=json.loads((HERE/'data/numeric-vectors.json').read_text())
    for case in rally['checked_arithmetic']:
        if case['operation'] not in ('add','sub'):continue
        signed=case['type']=='sint32';name=('add' if case['operation']=='add' else 'subtract')+('Signed' if signed else 'Unsigned')
        add(name,{('sa' if signed else 'ua'):case['left'],('sb' if signed else 'ub'):case['right']},label='rally-arithmetic')
    for case in rally['checked_conversions']:
        n=case['value'];target=case['destination_type']
        if n<-(1<<31) or n>(1<<32)-1:continue
        if target=='uint16':name='signedToNarrowUnsigned' if n<0 else 'narrowUnsigned'
        elif target=='sint16':name='narrowSigned' if n<0 else 'unsignedToNarrowSigned'
        elif target=='uint32':name='signedToUnsigned' if n<0 else 'identityUnsigned'
        elif target=='sint32':name='identitySigned' if n<0 else 'unsignedToSigned'
        else:continue
        add(name,{'sa' if n<0 else 'ua':n},label='rally-conversion')
    return out

def main():
    start=time.monotonic();BUILD.mkdir(exist_ok=True);verify_pin();receipts=[]
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-ffp-contract=off']
    run(['g++',*flags,'-O2',HERE/'compiler.cpp','-o',BUILD/'compiler'])
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'compiler.cpp','-o',BUILD/'compiler-asan'])
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1'
    for prefix in (0,254):
        directory=BUILD/('probe' if not prefix else 'probe-relocated');build(HERE/'probe.golem',directory,prefix,compiler=BUILD/'compiler-asan')
        iface,ast,root,meta=load(directory);cases=vectors(directory);runner=native_runner(directory);order=[a['buffer'] for a in iface['arrays']]
        stdin=b''.join(struct.pack('<HH',entry,mode)+state+b''.join(arrays[k] for k in order) for entry,mode,state,arrays,label in cases)
        observed=run([runner],input=stdin,env=env);size=4+meta['state_bytes']+sum(a['bytes'] for a in iface['arrays']);assert len(observed)==len(cases)*size
        machine=Machine();machine.execute((directory/'common-install.bin').read_bytes());machine.execute((directory/'buffered-install.bin').read_bytes())
        for n,(entry,mode,state,arrays,label) in enumerate(cases):
            want,data,rc=oracle(directory,entry,mode,state,arrays);expect=struct.pack('<i',rc)+want+b''.join(data[k] for k in order)
            assert observed[n*size:(n+1)*size]==expect,('native',n,entry,mode,label)
            if not mode:
                machine.memory[meta['shared_state']][:]=state
                for key,value in arrays.items():machine.memory[key][:]=value
                machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]))
                assert bytes(machine.memory[meta['shared_state']])==want,('buffered-root',n,entry,label,bytes(machine.memory[meta['shared_state']]).hex(),want.hex())
                assert all(machine.memory[k]==data[k] for k in order),('buffered-arrays',n,entry,label)
        retained={k:bytes(machine.memory[k]) for k in [meta['shared_state'],*order]};machine.execute((directory/'buffered-cleanup.bin').read_bytes())
        assert all(bytes(machine.memory[k])==v for k,v in retained.items())
        assert not set(meta['buffered_owned']+meta['native_owned'])&set(order)
        receipts.append({'prefix':prefix,'cases':len(cases),'wide_array_bytes':sum(a['bytes'] for a in iface['arrays']),'max_leases':3,'private_cleanup_retains_arrays':True})
    # Narrow packages remain byte-for-byte compatible, including float-capable native C.
    for name,prefix,source,mapping in [('default',0,'probe',{'sine':ROOT/'docs/tasks/GOLEM-035/data/sine.dat'}),
                                      ('relocated',254,'probe',{'sine':ROOT/'docs/tasks/GOLEM-035/data/sine.dat'}),
                                      ('animation',0,'animation',{key:ROOT/'docs/tasks/GOLEM-035/data'/(key+'.dat') for key in ('duration','speed','heading','rate','sine')})]:
        d=BUILD/('legacy-'+name);build(ROOT/'docs/tasks/GOLEM-035'/(source+'.golem'),d,prefix,array_data=mapping)
        old=ROOT/'docs/tasks/GOLEM-035/delivery/2156756263f8baff/packages'/name
        for p in old.iterdir():
            if p.suffix in ('.bin','.inc','.c'):assert (d/p.name).read_bytes()==p.read_bytes(),('legacy-parity',p.name)
    maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
    receipt={'base':verify_pin(),'packages':receipts,'checks':sum(r['cases'] for r in receipts),'prior_scalar_array_outputs_unchanged':True,'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3)}
    (HERE/'host-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Wide foundation host checks pass:',receipt['checks'])
if __name__=='__main__':main()
