"""Check that array reads retain destination Bounds/conversion semantics."""
import json,struct
from build_delivery import HERE,BUILD,build,run
from host_checks import load,arrays_initial,native_runner,call
from model import Machine,reference

def main():
    source=BUILD/'array-admission.golem';source.write_text('''uint16 index=0
uint16 bounded Bounds(10,20)=10
sint16 signedResult=0
float32 converted=0.0
ReadOnly Array uint16 words(0:1)=Values(65535,15)
ReadOnly Array sint16 signs(0:1)=Values(-1,15)
Sub boundedRead
 bounded=words(index)
EndSub
Sub toSigned
 signedResult=sint16(words(index))
EndSub
Sub toUnsigned
 bounded=uint16(signs(index))
EndSub
Sub toFloat
 converted=float32(words(index))
EndSub
''')
    directory=BUILD/'array-admission';build(source,directory)
    iface,ast,root,meta=load(directory);order=[a['buffer'] for a in iface['arrays']];runner=native_runner(directory);initial=(directory/'state.bin').read_bytes()
    cases=[];packed=bytearray();expected=bytearray()
    for entry in range(4):
        for index in (0,1):
            state=bytearray(initial);struct.pack_into('<H',state,28,entry);struct.pack_into('<H',state,112,index)
            arrays=arrays_initial(directory,iface);packed+=struct.pack('<HH',entry,0)+state+b''.join(arrays[k] for k in order)
            want=reference(iface,ast,state,entry,root,arrays=arrays);rc=1 if struct.unpack_from('<HH',want,12)!=(0,0) else 0
            expected+=struct.pack('<i',rc)+want+b''.join(arrays[k] for k in order)
            machine=Machine();machine.execute((directory/'common-install.bin').read_bytes());machine.execute((directory/'buffered-install.bin').read_bytes());machine.memory[meta['shared_state']][:]=state
            machine.execute(call(meta['buffered_entries'][entry]));assert bytes(machine.memory[meta['shared_state']])==want,(entry,index)
            assert all(machine.memory[k]==arrays[k] for k in order)
            cases.append({'entry':entry,'index':index,'language_status':struct.unpack_from('<H',want,14)[0]})
    assert run([runner],input=packed)==expected
    (HERE/'admission-checks.json').write_text(json.dumps({'cases':cases,'native_and_buffered_parity':True,'leases_released':True},indent=2)+'\n')
    print('Array destination/conversion admission checks pass:',len(cases))
if __name__=='__main__':main()
