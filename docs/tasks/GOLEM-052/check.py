"""Exercise emitted stock bytes against an independent decoder and decimal oracle."""
from pathlib import Path
import json,random,struct,subprocess
from machine import Machine
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=HERE/'build';BUILD.mkdir(exist_ok=True)
word=lambda n:struct.pack('<H',n)
def call(id):return bytes((23,0,160))+word(id)+b'\x01'
def run(args,**kw):return subprocess.run(list(map(str,args)),check=True,**kw)
def program():
    return '''uint16 u = 0
sint16 s = 0
uint32 w = 0
sint32 n = 0
uint16 i = 0
uint16 total = 0
Sub numbers
    print "[" print str(u) print "," print str(s) print ","
    print str(w) print "," print str(n) print "]"
EndSub
Sub accumulate
    For i In Range(1:10)
        total = total + i
    Next i
    print "i=" print str(i) print " total=" print str(total)
EndSub
Sub fail
    print "before:" print str(u + 1) print ":after"
EndSub
Sub branch
    For i In Range(0:2)
        If i == 1 Then print "x" Else print str(i) EndIf
    Next i
EndSub
'''
def cases():
    values=[(0,0,0,0),(65535,-32768,4294967295,-2147483648),(0,-1,0,-1),
            (1,32767,2147483648,2147483647)]
    for p in (1,10,100,1000,10000):
        for d in (-1,0,1):
            v=p+d
            if 0<=v<=65535:values.append((v,-min(v,32768),v,-min(v,2147483648)))
    for p in (100000,1000000,10000000,100000000,1000000000):
        for d in (-1,0,1):
            v=p+d;values.append((v&65535,-(v%32768),v,-v))
    rng=random.Random(5201)
    values += [(rng.randrange(65536),rng.randrange(-32768,32768),rng.randrange(1<<32),rng.randrange(-(1<<31),1<<31)) for _ in range(24)]
    # Large-to-small sequences expose stale digits and sign-state reuse.
    values += [(65535,-32768,4294967295,-2147483648),(0,0,0,0),(9,-9,9,-9),(1,1,1,1)]
    return values

def compile_source(source,folder,extra=()):
    run([ROOT/'build/golemc','--portable',*extra,source,folder],stdout=subprocess.DEVNULL)
    return json.loads((folder/'backends.json').read_text()),json.loads((folder/'interface.json').read_text())
def load(directory):
    m=Machine();m.execute((directory/'common-install.bin').read_bytes());m.execute((directory/'buffered-install.bin').read_bytes());return m

def check(directory):
    meta=json.loads((directory/'backends.json').read_text());iface=json.loads((directory/'interface.json').read_text())
    assert meta['available_backends']==['buffered'] and not meta['native_wrappers']
    assert not (directory/'native.c').exists()
    machine=load(directory);state=machine.memory[meta['shared_state']];fields={g['name']:g for g in iface['globals']}
    def setvalue(name,value):
        f=fields[name];size=f['bytes'];state[f['offset']:f['offset']+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
    temporaries=[int.from_bytes(bytes(p)[3:5],'little') for p in meta['programs'].values() if len(p)==9 and p[:3]==[23,0,160] and p[5]==11]
    assert len(temporaries)==8
    texts=[]
    for values in cases():
        for name,value in zip(('u','s','w','n'),values):setvalue(name,value)
        before=bytes(state[112:]);start=len(machine.text);machine.steps=0
        machine.execute(call(meta['buffered_entries'][0]))
        actual=bytes(machine.text[start:]);expected=('['+','.join(map(str,values))+']').encode()
        assert actual==expected,(values,actual,expected)
        assert all(not any(machine.memory[t]) for t in temporaries)
        assert state[112:]==before
        assert struct.unpack_from('<HH',state,12)==(0,0)
        texts.append(actual.decode())
    for total in (55,110):
        start=len(machine.text);machine.steps=0;machine.execute(call(meta['buffered_entries'][1]))
        assert machine.text[start:]==f'i=10 total={total}'.encode()
    setvalue('u',65535);start=len(machine.text);machine.steps=0;machine.execute(call(meta['buffered_entries'][2]))
    assert machine.text[start:]==b'before:' and struct.unpack_from('<H',state,14)[0]==1
    machine.steps=0;start=len(machine.text);machine.execute(call(meta['buffered_entries'][3]));assert machine.text[start:]==b'0x2'
    assert struct.unpack_from('<HH',state,12)==(0,0)
    machine.execute((directory/'buffered-cleanup.bin').read_bytes())
    assert not any(id in machine.memory for id in meta['buffered_owned'])
    assert meta['shared_state'] in machine.memory
    return {'cases':len(texts)+4,'decimal_values':len(texts)*4,'install_bytes':(directory/'buffered-install.bin').stat().st_size,
            'private_buffers':len(meta['buffered_owned']),'observed_text':texts}

if __name__=='__main__':
    source=HERE/'numbers.golem';source.write_text(program());reports=[]
    for label,extra in [('default',()),('relocated',('--reserve','1:511'))]:
        folder=BUILD/label;compile_source(source,folder,extra);reports.append(check(folder))
    # CLI selection is explicit; failures must not create a package.
    for target in ('native','both'):
        out=BUILD/('rejected-'+target)
        r=subprocess.run([ROOT/'build/golemc','--portable','--target',target,source,out],capture_output=True)
        assert r.returncode==1 and not out.exists()
    compile_source(HERE/'review.golem',BUILD/'review')
    r={'passed':True,'packages':reports,'target_rejections':2}
    (HERE/'model-evidence.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({k:v for k,v in r.items() if k!='packages'}))
    print('Decoder checks:',sum(p['cases'] for p in reports),'calls,',sum(p['decimal_values'] for p in reports),'formatted integers.')
