"""Exercise four-byte offsets up to65528 in a maximum admitted array."""
import json,struct
from build_delivery import HERE,BUILD,build,run
from host_checks import load,arrays_initial,native_runner,call
from model import reference,Machine

def main():
    source=BUILD/'array-boundary.golem';source.write_text('''uint16 index=49153
uint32 value=4294967295
uint32 selected=17
Array uint32 large(49153:65535)=Fill(0)
Sub storeAndRead
 large(index)=value
 selected=large(index)
EndSub
''')
    d=BUILD/'array-boundary';build(source,d);iface,ast,root,meta=load(d);fields={f['name']:f for f in iface['globals']};a=iface['arrays'][0]
    assert a['count']==16383 and a['bytes']==65532
    machine=Machine();machine.execute((d/'common-install.bin').read_bytes());machine.execute((d/'buffered-install.bin').read_bytes())
    packed=bytearray();expected=bytearray();checks=[]
    for index in (49153,57345,65535,49152):
        state=bytearray((d/'state.bin').read_bytes());struct.pack_into('<H',state,fields['index']['offset'],index);arrays=arrays_initial(d,iface)
        packed+=struct.pack('<HH',0,0)+state+arrays[a['buffer']];want=reference(iface,ast,state,0,root,arrays=arrays);status=struct.unpack_from('<H',want,14)[0]
        if index>=49153:
            assert status==0 and struct.unpack_from('<I',arrays[a['buffer']],(index-49153)*4)[0]==4294967295
            assert struct.unpack_from('<I',want,fields['selected']['offset'])[0]==4294967295
            assert sum(v!=0 for v in arrays[a['buffer']])==4
        else:assert status==5 and struct.unpack_from('<I',want,fields['selected']['offset'])[0]==17 and not any(arrays[a['buffer']])
        expected+=struct.pack('<i',int(status!=0))+want+arrays[a['buffer']]
        machine.memory[meta['shared_state']][:]=state;machine.memory[a['buffer']][:]=arrays_initial(d,iface)[a['buffer']]
        machine.steps=0;machine.execute(call(meta['buffered_entries'][0]));assert bytes(machine.memory[meta['shared_state']])==want and machine.memory[a['buffer']]==arrays[a['buffer']]
        checks.append({'index':index,'byte_offset':(index-49153)*4 if index>=49153 else None,'language_status':status})
    assert run([native_runner(d)],input=packed)==expected
    (HERE/'array-boundary-checks.json').write_text(json.dumps({'count':16383,'array_bytes':65532,'checks':checks,'buffered_model_native_host_sanitizers':True},indent=2)+'\n')
    print('Maximum four-byte array offsets pass:',len(checks))
if __name__=='__main__':main()
