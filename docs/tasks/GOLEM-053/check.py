"""Independent integer-bit-pattern cases for generated C and stock execution."""
from pathlib import Path
import hashlib,json,random,shutil,struct,subprocess,sys
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build';BUILD.mkdir(exist_ok=True)
SDK=ROOT.parent/'agon-vdplet'
word=lambda n:struct.pack('<H',n)
def run(args,**kw):return subprocess.run(list(map(str,args)),check=True,**kw)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def prepare():
    declarations=[];statements=[]
    for t,prefix in [('uint16','u'),('sint16','s'),('uint32','w'),('sint32','n')]:
        declarations += [f'{t} {prefix}a = 5',f'{t} {prefix}b = 3']
        declarations += [f'{t} {prefix}{suffix} = 0' for suffix in ('and','or','xor','not')]
        statements += [f'{prefix}and = {prefix}a & {prefix}b',f'{prefix}or = {prefix}a | {prefix}b',
                       f'{prefix}xor = {prefix}a ^ {prefix}b',f'{prefix}not = ~{prefix}a']
    declarations += ['uint16 one = 1','uint16 two = 2','uint16 three = 3','uint16 grouping = 0',
                     'bool flag = false','uint16 index = 1','uint16 selected = 0',
                     'uint16 limited Bounds(0,15) = 7',
                     'Array uint16 samples(0:2) = Values(0,43690,65535)']
    source='\n'.join(declarations)+ '\nSub compute\n'+'\n'.join('    '+s for s in statements)+'''
    grouping = one | two ^ three & one
    flag = Not one | two == 0 And one & two == 0
    selected = ~samples(index) & 255
EndSub
Sub boundsFailure
    limited = ua | one
EndSub
Sub eagerFailure
    limited = one & (ua + one)
EndSub
'''
    (HERE/'review.golem').write_text(source)
    packages=[]
    for label,opts in [('default',[]),('relocated',['--reserve','1:511'])]:
        out=BUILD/label;run([ROOT/'build/golemc','--portable',*opts,HERE/'review.golem',out],stdout=subprocess.DEVNULL)
        packages.append(out)
    return packages

def vectors(directory):
    meta=json.loads((directory/'backends.json').read_text());iface=json.loads((directory/'interface.json').read_text())
    fields={g['name']:g for g in iface['globals']};initial=(directory/'state.bin').read_bytes()
    array=(directory/iface['arrays'][0]['initial_data']).read_bytes()
    def put(state,name,bits):
        f=fields[name];size=4 if '32' in f['type'] else 2
        state[f['offset']:f['offset']+size]=(bits&((1<<(size*8))-1)).to_bytes(size,'little')
    cases=[]
    rng=random.Random(5301)
    pairs=[(0,0),(0,0xffffffff),(0xffffffff,0xffffffff),(0x55555555,0xaaaaaaaa),
           (0x80008000,1),(0x12345678,0x87654321),(5,3)]+[(rng.getrandbits(32),rng.getrandbits(32)) for _ in range(17)]
    for a,b in pairs:
        state=bytearray(initial);expected=bytearray(initial)
        for prefix,bits in [('u',16),('s',16),('w',32),('n',32)]:
            mask=(1<<bits)-1
            for target in (state,expected):put(target,prefix+'a',a);put(target,prefix+'b',b)
            for suffix,value in [('and',a&b),('or',a|b),('xor',a^b),('not',~a)]:put(expected,prefix+suffix,value&mask)
        put(expected,'grouping',3);put(expected,'flag',1);put(expected,'selected',85)
        cases.append({'entry':0,'initial':bytes(state),'expected':bytes(expected),'array':array,'status':0})
    # Reuse previously calculated outputs with different inputs, then repeat unchanged.
    repeat=dict(cases[6]);repeat['initial']=repeat['expected'];cases.append(repeat)
    for entry,status in [(1,32),(2,1)]:
        state=bytearray(initial);put(state,'ua',65535);struct.pack_into('<H',state,28,entry);expected=bytearray(state);struct.pack_into('<H',expected,14,status)
        cases.append({'entry':entry,'initial':bytes(state),'expected':bytes(expected),'array':array,'status':status})
    offsets=[12,13,14,15]+[g['offset']+i for g in iface['globals'] for i in range(4 if '32' in g['type'] else 2)]
    return meta,iface,cases,offsets

def native(directory):
    meta,iface,cases,offsets=vectors(directory)
    header=f"#define STATE_ID {meta['shared_state']}\n#define STATE_BYTES {meta['state_bytes']}\n#define ARGS_ID {meta['native_arguments']}\n#define ARRAY_COUNT 1\n"
    for name,values in [('ids',[a['buffer'] for a in iface['arrays']]),('bytes',[a['bytes'] for a in iface['arrays']]),('readonly',[int(a['access']==1) for a in iface['arrays']])]:
        header+='static const unsigned array_'+name+'[]={'+','.join(map(str,values))+'};\n'
    (directory/'fixture.h').write_text(header)
    exe=directory/'native-host'
    run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-O1','-g','-ffp-contract=off','-fsanitize=address,undefined',
        '-I'+str(SDK/'include'),'-I'+str(directory),'-DGENERATED_SOURCE="'+str(directory/'native.c')+'"',HERE/'native_host.c','-o',exe])
    payload=b''.join(word(c['entry'])+word(0)+c['initial']+c['array'] for c in cases)
    result=run([exe],input=payload,stdout=subprocess.PIPE).stdout
    step=4+meta['state_bytes']+len(cases[0]['array']);assert len(result)==len(cases)*step
    for i,c in enumerate(cases):
        block=result[i*step:(i+1)*step];rc=struct.unpack_from('<i',block)[0]
        assert (rc==0)==(c['status']==0),(i,rc)
        actual=block[4:4+meta['state_bytes']]
        assert all(actual[o]==c['expected'][o] for o in offsets),i
        assert block[4+meta['state_bytes']:]==c['array']
    return {'package':directory.name,'cases':len(cases),'observed_bytes':len(cases)*len(offsets),'native_sanitizers':True}

if __name__=='__main__':
    packages=prepare();reports=[native(p) for p in packages]
    (HERE/'host-evidence.json').write_text(json.dumps({'passed':True,'packages':reports},indent=2)+'\n')
    print(json.dumps(reports))
