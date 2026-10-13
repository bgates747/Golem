"""Assert published Rally results directly, and explicit widening order."""
import json,struct
from build_delivery import HERE,BUILD,build,run,sha
from host_checks import load,arrays_initial,native_runner,vectors,call
from model import reference,Machine

def main():
    d=BUILD/'probe';iface,ast,root,meta=load(d);fields={f['name']:f for f in iface['globals']}
    rally=json.loads((HERE/'data/numeric-vectors.json').read_text())
    arithmetic=[v for v in rally['checked_arithmetic'] if v['operation'] in ('add','sub')]
    conversions=[v for v in rally['checked_conversions'] if -(1<<31)<=v['value']<(1<<32)]
    selected=[v for v in vectors(d) if v[-1] in ('rally-arithmetic','rally-conversion')]
    assert len(selected)==len(arithmetic)+len(conversions)
    stdin=bytearray();expected=bytearray();checks=[]
    machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes())
    order=[a['buffer'] for a in iface['arrays']]
    for n,((entry,mode,state,arrays,label),published) in enumerate(zip(selected,arithmetic+conversions)):
        stdin+=struct.pack('<HH',entry,mode)+state+b''.join(arrays[k] for k in order)
        want=reference(iface,ast,state,entry,root,arrays=arrays);status=struct.unpack_from('<H',want,14)[0]
        if label=='rally-arithmetic':
            name='sr' if published['type']=='sint32' else 'ur'
            error=1 if published['operation']=='add' else 2
        else:
            name={'sint32':'sr','uint32':'ur','sint16':'sn','uint16':'un'}[published['destination_type']];error=32
        f=fields[name];fmt=('<i' if f['type']=='sint32' else '<I') if f['bytes']==4 else ('<h' if f['type']=='sint16' else '<H')
        value=struct.unpack_from(fmt,want,f['offset'])[0]
        if 'error' in published:
            assert status==error and published['preserve_destination'] and value==17,(n,published,status,value)
        else:assert status==0 and value==published['result'],(n,published,status,value)
        expected+=struct.pack('<i',int(status!=0))+want+b''.join(arrays[k] for k in order)
        machine.memory[meta['shared_state']][:]=state
        for k,v in arrays_initial(d,iface).items():machine.memory[k][:]=v
        machine.steps=0;machine.execute(call(meta['buffered_entries'][entry]))
        assert bytes(machine.memory[meta['shared_state']])==want
        checks.append({'label':label,'published':published,'language_status':status,'result':value})
    assert run([native_runner(d)],input=stdin)==expected
    source=BUILD/'widening.golem';source.write_text('''uint16 narrow=65535
uint32 result=17
Array uint32 retained(0:0)=Fill(0)
Sub narrowFirst
 result=uint32(narrow+1)
EndSub
Sub wideFirst
 result=uint32(narrow)+1
EndSub
''')
    d=BUILD/'widening';build(source,d);iface,ast,root,meta=load(d);initial=(d/'state.bin').read_bytes();arrays=arrays_initial(d,iface);root_offset=next(f['offset'] for f in iface['globals'] if f['name']=='result');packed=bytearray();expected=bytearray()
    for entry,status,value in [(0,1,17),(1,0,65536)]:
        state=bytearray(initial);struct.pack_into('<H',state,28,entry);want=reference(iface,ast,state,entry,root,arrays=arrays)
        assert struct.unpack_from('<H',want,14)[0]==status and struct.unpack_from('<I',want,root_offset)[0]==value
        packed+=struct.pack('<HH',entry,0)+state+b''.join(arrays.values());expected+=struct.pack('<i',int(status!=0))+want+b''.join(arrays.values())
        machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes());machine.memory[meta['shared_state']][:]=state;machine.execute(call(meta['buffered_entries'][entry]));assert bytes(machine.memory[meta['shared_state']])==want
    assert run([native_runner(d)],input=packed)==expected
    receipt={'published_oracle_sha256':sha(HERE/'data/numeric-vectors.json'),'arithmetic_cases':len(arithmetic),'conversion_cases':len(conversions),'conversion_vectors_outside_source_32bit_domain_deferred':len(rally['checked_conversions'])-len(conversions),'checks':checks,'explicit_widening_order_cases':2,'buffered_and_native_host_sanitizers':True,'full_route_contact_replay':False}
    (HERE/'rally-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Published Rally primitive results match:',len(checks),'plus two widening-order checks.')
if __name__=='__main__':main()
