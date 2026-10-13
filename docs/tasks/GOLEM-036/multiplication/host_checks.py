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
    iface,ast,root,meta=load(directory);fields={f['name']:f for f in iface['globals']};entries={e['name']:e['logical_id'] for e in iface['entries']};out=[]
    def add(name,updates=None,mode=0,published=None):
        entry=entries[name];state=bytearray((directory/'state.bin').read_bytes());struct.pack_into('<H',state,28,entry)
        for key,n in (updates or {}).items():
            f=fields[key];struct.pack_into('<I' if f['bytes']==4 else '<H',state,f['offset'],n&((1<<(f['bytes']*8))-1))
        out.append((entry,mode,bytes(state),arrays_initial(directory,iface),published))
    u=[0,1,2,65535,65536,65537,0x7fffffff,0x80000000,0xffffffff]
    s=[-2147483648,-2147483647,-65536,-46341,-46340,-1,0,1,2,46340,46341,65536,2147483647]
    for a in u:
        for b in u:add('multiplyUnsigned',{'ua':a,'ub':b})
    for a in s:
        for b in s:add('multiplySigned',{'sa':a,'sb':b})
    rng=random.Random(3602)
    for _ in range(240):
        add('multiplyUnsigned',{'ua':rng.randrange(1<<32),'ub':rng.randrange(1<<32)})
        add('multiplySigned',{'sa':rng.randrange(-(1<<31),1<<31),'sb':rng.randrange(-(1<<31),1<<31)})
    for bit in range(32):
        add('multiplyUnsigned',{'ua':1<<bit,'ub':(1<<(31-bit))+1})
        add('multiplyUnsigned',{'ua':(1<<bit)-1,'ub':(1<<(32-bit))-1})
    for name,updates in [('nestedUnsigned',{'ua':3,'ub':5,'uc':7}),('nestedUnsigned',{'ua':65536,'ub':65536,'uc':0}),
                         ('nestedSigned',{'sa':-3,'sb':5,'sc':7}),('nestedSigned',{'sa':2147483647,'sb':2,'sc':0}),
                         ('precedence',{'ua':3,'ub':5,'uc':7}),('priorEffect',{'ua':65536,'ub':65536}),
                         ('narrowFirst',{}),('wideFirst',{}),('boundedProduct',{'ua':180,'ub':65534}),
                         ('boundedProduct',{'ua':65536,'ub':181}),('constantSuccess',{}),
                         ('evaluationOrder',{'ua':0xffffffff,'ub':2,'sa':38400}),
                         ('evaluationOrder',{'ua':1,'ub':2,'sa':38400}),('evaluationOrder',{'ua':3,'ub':5,'sa':2})]:add(name,updates)
    for a,b in [(0xffffffff,2),(65535,65537),(0xffffffff,0)]:
        for index in (0,1,2):add('storeUnsigned',{'ua':a,'ub':b,'index':index})
        add('skipped',{'ua':a,'ub':b})
    for a,b in [(-2147483648,-1),(-2147483648,1),(-46340,46340)]:
        for index in (-1,0,1):add('storeSigned',{'sa':a,'sb':b,'signedIndex':index})
    rally=json.loads((HERE/'data/numeric-vectors.json').read_text())
    for case in rally['checked_arithmetic']:
        if case['operation']!='mul':continue
        signed=case['type']=='sint32';add('multiplySigned' if signed else 'multiplyUnsigned',{'sa' if signed else 'ua':case['left'],'sb' if signed else 'ub':case['right']},published=case)
    for mode in (1,2,3,4,5,6):add('multiplyUnsigned',{'ua':3,'ub':5},mode=mode)
    return out

def main():
    start=time.monotonic();BUILD.mkdir(exist_ok=True);verify_pin();receipts=[]
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-ffp-contract=off']
    run(['g++',*flags,'-O2',HERE/'compiler.cpp','-o',BUILD/'compiler'])
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'compiler.cpp','-o',BUILD/'compiler-asan'])
    for prefix in (0,254):
        d=BUILD/('probe' if not prefix else 'probe-relocated');build(HERE/'probe.golem',d,prefix,compiler=BUILD/'compiler-asan');iface,ast,root,meta=load(d);cases=vectors(d);runner=native_runner(d);order=[a['buffer'] for a in iface['arrays']]
        stdin=b''.join(struct.pack('<HH',entry,mode)+state+b''.join(arrays[k] for k in order) for entry,mode,state,arrays,published in cases)
        got=run([runner],input=stdin);size=4+meta['state_bytes']+sum(a['bytes'] for a in iface['arrays']);assert len(got)==size*len(cases)
        machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes());rally_count=0
        for n,(entry,mode,state,arrays,published) in enumerate(cases):
            want,data,rc=oracle(d,entry,mode,state,arrays);assert got[n*size:(n+1)*size]==struct.pack('<i',rc)+want+b''.join(data[k] for k in order),('native',n)
            if published:
                f=next(f for f in iface['globals'] if f['name']==('sr' if published['type']=='sint32' else 'ur'));value=struct.unpack_from('<i' if published['type']=='sint32' else '<I',want,f['offset'])[0];status=struct.unpack_from('<H',want,14)[0]
                assert (status==3 and value==17) if 'error' in published else (status==0 and value==published['result']),(published,status,value);rally_count+=1
            if not mode:
                machine.memory[meta['shared_state']][:]=state
                for k,v in arrays.items():machine.memory[k][:]=v
                machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]));assert bytes(machine.memory[meta['shared_state']])==want,('buffered',n,entry,bytes(machine.memory[meta['shared_state']]).hex(),want.hex());assert all(machine.memory[k]==data[k] for k in order)
        receipts.append({'prefix':prefix,'cases':len(cases),'direct_published_rally_products':rally_count})
    # Preserve accepted foundation executable/layout/C artifacts exactly.
    base=ROOT/'docs/tasks/GOLEM-036';pointer=json.loads((base/'delivery.json').read_text());bundle=base/pointer['bundle']
    for name,source,prefix in [('default','probe',0),('relocated','probe',254),('animation','animation',0)]:
        d=BUILD/('foundation-'+name);build(base/(source+'.golem'),d,prefix)
        for p in (bundle/'packages'/name).iterdir():
            if p.suffix in ('.bin','.inc','.c'):assert (d/p.name).read_bytes()==p.read_bytes(),('foundation-parity',name,p.name)
    maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
    (HERE/'host-checks.json').write_text(json.dumps({'packages':receipts,'checks':sum(r['cases'] for r in receipts),'foundation_outputs_unchanged':True,'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3)},indent=2)+'\n');print('Wide multiplication host checks pass:',sum(r['cases'] for r in receipts))
if __name__=='__main__':main()
