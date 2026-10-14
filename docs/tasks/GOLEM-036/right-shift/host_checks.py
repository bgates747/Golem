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
 for bits in range(32):
  for n in (0,1,255,256,65535,65536,0x7fffffff,0x80000000,0xffffffff):add('shiftUnsigned',{'ua':n,'bits':bits})
  for n in (-2147483648,-2147483647,-65537,-65,-1,0,1,65,65536,2147483647):add('shiftSigned',{'sa':n,'bits':bits})
 for bits in (32,33,65535):
  add('shiftUnsigned',{'ua':0xffffffff,'bits':bits});add('shiftSigned',{'sa':-1,'bits':bits});add('priorEffect',{'ua':1,'bits':bits});add('skipped',{'ua':1,'bits':bits})
 rng=random.Random(3604)
 for _ in range(160):
  add('shiftUnsigned',{'ua':rng.randrange(1<<32),'bits':rng.randrange(32)})
  add('shiftSigned',{'sa':rng.randrange(-(1<<31),1<<31),'bits':rng.randrange(32)})
 for name,updates in [('chain',{'sa':-65,'bits':1,'more':1}),('precedence',{'ua':4,'ub':4,'bits':2}),
                      ('parentheses',{'ua':4,'ub':4,'bits':2}),('nested',{'ua':0xffffffff,'ub':256,'bits':1,'more':8}),
                      ('countExpression',{'ua':0xffffffff,'ub':0x80000000,'more':31}),
                      ('countExpression',{'ua':123,'ub':32,'more':0}),('boundedShift',{'sa':-65,'bits':6}),
                      ('boundedShift',{'sa':-2147483648,'bits':0}),
                      ('argumentOrder',{'ua':0xffffffff,'ub':2,'bits':65535}),('argumentOrder',{'ua':1,'ub':2,'bits':65535}),('constants',{})]:add(name,updates)
 for index in (0,1,2):
  for bits in (0,31,32):
   add('unsignedArray',{'index':index,'bits':bits});add('storeUnsigned',{'index':index,'bits':bits,'ua':0xffffffff})
  add('countArray',{'ua':0xffffffff,'index':index})
 for index in (-1,0,1):
  for bits in (0,31,32):add('signedArray',{'signedIndex':index,'bits':bits});add('storeSigned',{'signedIndex':index,'bits':bits,'sa':-65})
 for v in json.loads((HERE/'data/numeric-vectors.json').read_text())['signed_shift_examples']:add('shiftSigned',{'sa':v['value'],'bits':v['shift']},published=v)
 for mode in (1,2,3,4,5,6):add('shiftUnsigned',{'ua':5,'bits':2},mode=mode)
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
    value=struct.unpack_from('<i',want,fields['sr']['offset'])[0];assert struct.unpack_from('<H',want,14)[0]==0 and value==published['arithmetic_shift'];published_count+=1
   if not mode:
    machine.memory[meta['shared_state']][:]=state
    for k,v in arrays.items():machine.memory[k][:]=v
    machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]));assert bytes(machine.memory[meta['shared_state']])==want,('buffered',n,entry,bytes(machine.memory[meta['shared_state']]).hex(),want.hex());assert all(machine.memory[k]==data[k] for k in order)
  receipts.append({'prefix':prefix,'cases':len(cases),'direct_published_primitive_shift_checks':published_count,'all_counts_0_to_31':True})
 base=ROOT/'docs/tasks/GOLEM-036/division';pointer=json.loads((base/'delivery.json').read_text());bundle=base/pointer['bundle']
 for name,source,prefix in [('default','probe',0),('relocated','probe',254),('animation','animation',0)]:
  d=BUILD/('prior-'+name);build(base/(source+'.golem'),d,prefix)
  for p in (bundle/'packages'/name).iterdir():
   if p.suffix in ('.bin','.inc','.c'):assert (d/p.name).read_bytes()==p.read_bytes(),('prior-parity',name,p.name)
 maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
 (HERE/'host-checks.json').write_text(json.dumps({'packages':receipts,'checks':sum(r['cases'] for r in receipts),'prior_division_outputs_unchanged':True,'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3),'game_replay_performed':False},indent=2)+'\n');print('Typed right-shift host checks pass:',sum(r['cases'] for r in receipts))
if __name__=='__main__':main()
