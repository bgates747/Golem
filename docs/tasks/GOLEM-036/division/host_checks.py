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
    u=[0,1,2,65535,65536,0x7fffffff,0x80000000,0xfffffffe,0xffffffff]
    s=[-2147483648,-2147483647,-65536,-38400,-1,0,1,38400,65535,2147483646,2147483647]
    for a in u:
        for b in u:add('divideUnsigned',{'ua':a,'ub':b})
    for a in s:
        for b in s:add('divideSigned',{'sa':a,'sb':b})
    rng=random.Random(3603)
    for _ in range(160):
        add('divideUnsigned',{'ua':rng.randrange(1<<32),'ub':rng.randrange(1<<32)})
        add('divideSigned',{'sa':rng.randrange(-(1<<31),1<<31),'sb':rng.randrange(-(1<<31),1<<31)})
    rally=json.loads((HERE/'data/numeric-vectors.json').read_text())
    for v in rally['division']:
        signed=v['type']=='sint32';add('divideSigned' if signed else 'divideUnsigned',{'sa' if signed else 'ua':v['numerator'],'sb' if signed else 'ub':v['denominator']},published=v)
    for v in rally['route_products']:
        add('divideSigned',{'sa':v['product'],'sb':v['divisor']},published={'type':'sint32','numerator':v['product'],'denominator':v['divisor'],'quotient':v['delta_q8'],'route_tick':v['tick']})
    for name,updates in [('aliasUnsigned',{'ua':0xffffffff,'ub':65536}),('aliasUnsigned',{'ua':5,'ub':0}),
                         ('aliasSigned',{'sa':-2147483648,'sb':-1}),('aliasSigned',{'sa':-73,'sb':7}),
                         ('priorEffect',{'sa':1,'sb':0}),('priorEffect',{'sa':-2147483648,'sb':-1}),
                         ('argumentOrder',{'ua':0xffffffff,'ub':2,'uc':0xffffffff}),('argumentOrder',{'ua':1,'ub':2,'uc':0xffffffff}),
                         ('skipped',{'ua':1,'ub':0}),('repeated',{'ua':100,'ub':7,'uc':0}),('repeated',{'ua':100,'ub':7,'uc':3})]:add(name,updates)
    for a,b in [(73,7),(74,7),(77,7),(-74,7),(-73,-7),(1,0),(-2147483648,-1)]:add('boundedPair',{'sa':a,'sb':b})
    for index in (0,1,2):
        for d in (0,1,65536):add('unsignedArray',{'index':index,'ub':d})
    for index in (-1,0,1):
        for d in (-1,0,7):add('signedArray',{'signedIndex':index,'sb':d})
    for mode in (1,2,3,4,5,6):add('divideUnsigned',{'ua':5,'ub':2},mode=mode)
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
        machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes());published_count=0
        fields={f['name']:f for f in iface['globals']}
        for n,(entry,mode,state,arrays,published) in enumerate(cases):
            want,data,rc=oracle(d,entry,mode,state,arrays);assert got[n*size:(n+1)*size]==struct.pack('<i',rc)+want+b''.join(data[k] for k in order),('native',n)
            if published:
                signed=published['type']=='sint32';q=struct.unpack_from('<i' if signed else '<I',want,fields['sq' if signed else 'uq']['offset'])[0];r=struct.unpack_from('<i' if signed else '<I',want,fields['sr' if signed else 'ur']['offset'])[0];status=struct.unpack_from('<H',want,14)[0]
                if 'error' in published:assert status=={'division_by_zero':4,'division_overflow':6}[published['error']] and q==17 and r==19,(n,published,status,q,r)
                else:
                    assert status==0 and q==published['quotient'] and ('remainder' not in published or r==published['remainder']),(n,published,status,q,r)
                    assert published['numerator']==q*published['denominator']+r and abs(r)<abs(published['denominator'])
                    assert not signed or not r or (r<0)==(published['numerator']<0)
                published_count+=1
            if not mode:
                machine.memory[meta['shared_state']][:]=state
                for k,v in arrays.items():machine.memory[k][:]=v
                machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]));assert bytes(machine.memory[meta['shared_state']])==want,('buffered',n,entry,bytes(machine.memory[meta['shared_state']]).hex(),want.hex());assert all(machine.memory[k]==data[k] for k in order)
        receipts.append({'prefix':prefix,'cases':len(cases),'published_division_vectors':100,'published_route_product_divisions':600,'direct_published_checks':published_count})
    base=ROOT/'docs/tasks/GOLEM-036/multiplication';pointer=json.loads((base/'delivery.json').read_text());bundle=base/pointer['bundle']
    for name,source,prefix in [('default','probe',0),('relocated','probe',254),('animation','animation',0)]:
        d=BUILD/('prior-'+name);build(base/(source+'.golem'),d,prefix)
        for p in (bundle/'packages'/name).iterdir():
            if p.suffix in ('.bin','.inc','.c'):assert (d/p.name).read_bytes()==p.read_bytes(),('prior-parity',name,p.name)
    maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
    (HERE/'host-checks.json').write_text(json.dumps({'packages':receipts,'checks':sum(r['cases'] for r in receipts),'prior_multiplication_outputs_unchanged':True,'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3)},indent=2)+'\n');print('Wide paired division host checks pass:',sum(r['cases'] for r in receipts))
if __name__=='__main__':main()
