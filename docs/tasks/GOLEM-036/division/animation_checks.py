"""Qualify independent Rally columns/sine lookup and six-slot retained motion."""
import json,struct,time
from host_checks import load,arrays_initial,native_runner,call
from build_delivery import HERE,BUILD,run,sha
from model import Machine,reference

def cases(directory):
    iface,ast,root,meta=load(directory);state=(directory/'state.bin').read_bytes();arrays=arrays_initial(directory,iface);out=[]
    for frame in range(300):
        out.append((0,state,{k:bytearray(v) for k,v in arrays.items()},'wide-motion'))
        state=reference(iface,ast,state,0,root,arrays=arrays)
    return out,state,arrays

def main():
    start=time.monotonic();d=BUILD/'animation';iface,ast,root,meta=load(d)
    vectors,final,final_arrays=cases(d);order=[a['buffer'] for a in iface['arrays']];runner=native_runner(d)
    stdin=b''.join(struct.pack('<HH',entry,0)+state+b''.join(arrays[k] for k in order) for entry,state,arrays,label in vectors)
    got=run([runner],input=stdin);size=4+meta['state_bytes']+sum(a['bytes'] for a in iface['arrays']);assert len(got)==size*len(vectors)
    machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes())
    fields={f['name']:f for f in iface['globals']}
    for n,(entry,state,arrays,label) in enumerate(vectors):
        wanted={k:bytearray(v) for k,v in arrays.items()};want=reference(iface,ast,state,entry,root,arrays=wanted)
        rc=1 if struct.unpack_from('<HH',want,12)!=(0,0) else 0
        assert got[n*size:(n+1)*size]==struct.pack('<i',rc)+want+b''.join(wanted[k] for k in order),('native',n,label)
        machine.memory[meta['shared_state']][:]=state
        for k,v in arrays.items():machine.memory[k][:]=v
        machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]))
        assert bytes(machine.memory[meta['shared_state']])==want,('buffered',n,label)
        assert all(machine.memory[k]==wanted[k] for k in order),('arrays',n,label)
    receipt={'cases':len(vectors),'markers':3,'frames':300,'horizontal_divisor':64,
             'vertical_divisor':8, 'final_world_tick':struct.unpack_from('<I',final,fields['tick']['offset'])[0],
             'final_root':final.hex(),'final_arrays':{a['name']:bytes(final_arrays[a['buffer']]).hex() for a in iface['arrays']},
             'elapsed_seconds':round(time.monotonic()-start,3)}
    (HERE/'animation-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Wide animation host checks pass:',len(vectors))
if __name__=='__main__':main()
