"""Qualify independent Rally columns/sine lookup and six-slot retained motion."""
import json,struct,time
from host_checks import load,arrays_initial,native_runner,call
from build_delivery import HERE,BUILD,run,sha
from model import Machine,reference

def cases(directory):
    iface,ast,root,meta=load(directory);initial=(directory/'state.bin').read_bytes();data=arrays_initial(directory,iface)
    fields={f['name']:f for f in iface['globals']};out=[]
    def add(entry,updates=None,state=None,arrays=None,label=''):
        state=bytearray(initial if state is None else state);struct.pack_into('<H',state,28,entry)
        for name,value in (updates or {}).items():struct.pack_into('<H',state,fields[name]['offset'],value)
        arrays={k:bytearray(v) for k,v in (data if arrays is None else arrays).items()}
        out.append((entry,bytes(state),arrays,label));want=reference(iface,ast,state,entry,root,arrays=arrays)
        return want,arrays
    for i in range(7):add(2,{'routeIndex':i},label='route')
    for i in range(258):add(3,{'waveIndex':i},label='sine')
    state,arrays=add(0,label='init')
    for i in range(600):state,arrays=add(1,state=state,arrays=arrays,label='motion')
    return out,state,arrays

def main():
    start=time.monotonic();d=BUILD/'animation';iface,ast,root,meta=load(d)
    vectors,final,final_arrays=cases(d);order=[a['buffer'] for a in iface['arrays']];runner=native_runner(d)
    stdin=b''.join(struct.pack('<HH',entry,0)+state+b''.join(arrays[k] for k in order) for entry,state,arrays,label in vectors)
    got=run([runner],input=stdin);size=4+meta['state_bytes']+sum(a['bytes'] for a in iface['arrays']);assert len(got)==size*len(vectors)
    machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes())
    rally=json.loads((HERE/'data/rally-data-receipt.json').read_text())
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
        if label=='route' and n<6:
            assert [struct.unpack_from('<h',want,fields[name]['offset'])[0] for name in ('durationResult','speedResult','headingResult','rateResult')]==rally['records'][n]
        if label=='sine' and n-7<257:
            assert struct.unpack_from('<h',want,fields['sineResult']['offset'])[0]==rally['sine'][n-7]
    receipt={'cases':len(vectors),'route_records':6,'sine_values':257,'opponents':6,'frames':600,
             'final_root':final.hex(),'final_arrays':{a['name']:bytes(final_arrays[a['buffer']]).hex() for a in iface['arrays']},
             'rally_data_sha256':rally['sha256'],'elapsed_seconds':round(time.monotonic()-start,3)}
    (HERE/'animation-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Rally array/animation host checks pass:',len(vectors))
if __name__=='__main__':main()
