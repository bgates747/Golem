"""Build finite MOS qualification and the labelled visual review."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from host_checks import load,arrays_initial,oracle,call
from model import reference

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
BUILD=HERE/'build'
QUERY,RENDER,TEMPLATE,STAGING=60000,60002,60003,60010
word=lambda value:struct.pack('<H',value)
packet=lambda key,op:bytes((23,0,160))+word(key)+bytes((op,))
call=lambda key:packet(key,1)
write=lambda key,data:packet(key,2)+packet(key,0)+word(len(data))+data
update=lambda key,offset,data:packet(key,5)+b'\xc2'+word(offset)+word(len(data))+data
copy=lambda key,offset,source,start,count:packet(key,5)+b'\xe2'+word(offset)+word(count)+word(source)+word(start)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()

class Tape:
    def __init__(self,hybrid):
        self.hybrid=hybrid
        self.data=bytearray()
        self.expected=bytearray()
        self.mask=bytearray()
        self.checks=[]
        self.token=0
        self.loads=0
        self.calls=[]

    def expect(self,data,label,mask=None):
        self.checks.append({'label':label,'offset':len(self.expected),'bytes':len(data)})
        self.expected+=data
        self.mask+=mask if mask is not None else b'\xff'*len(data)

    def send(self,data):
        assert len(data)
        for start in range(0,len(data),60000):
            part=data[start:start+60000];self.data+=b'\x01'+word(len(part))+part

    def patched(self,data,offset):
        self.data+=b'\x04'+word(offset)+word(len(data))+data

    def transaction(self,op,fields,kind,value,label,handle=False,capture=False):
        self.token+=1
        body=struct.pack('<BBHBB',1,op,self.token,1,0)+fields
        wire=bytes((23,0,0xd8))+word(len(body))+body
        flags=1 if capture else 2 if value is None else 0
        self.data+=b'\x02'+struct.pack('<HBBHHH',self.token,kind,flags,value or 0,11 if handle else 65535,len(wire))+wire
        self.expect(struct.pack('<4sBBHH',b'VDPL',1,kind,self.token,value or 0),label,
                    b'\xff'*8+(b'\0\0' if value is None else b'\xff\xff'))

    def load(self,directory,meta):
        image=directory/(directory.name.split('-relocated')[0]+'.vdpl')
        self.send(write(STAGING,image.read_bytes()))
        self.transaction(1,struct.pack('<HIH',STAGING,image.stat().st_size,0),2,None,'LOAD native',capture=True)
        self.loads+=1
        for field in (6,7):
            self.transaction(4,word(field),4,None,'native entry '+str(field))
        self.send((directory/'native-install.bin').read_bytes())
        for wrapper in meta['native_wrappers']:
            self.patched(update(wrapper,meta['native_handle_offset'],b'\0\0'),11)

    def named(self,entry,backend,count):
        self.data+=b'\x06'+bytes((entry+(count if backend=='native' else 0),))
        self.calls.append({'entry':entry,'backend':backend})

    def state(self,key,expected,label,offsets=None):
        offsets=range(0,len(expected),2) if offsets is None else sorted(set(offsets))
        if self.hybrid:
            for offset in offsets:
                self.transaction(5,struct.pack('<HI',key,offset),4,struct.unpack_from('<H',expected,offset)[0],label+' word '+str(offset//2))
        else:
            for offset in offsets:
                self.data+=b'\x07'+word(key)+word(offset)+word(2)
                self.expect(expected[offset:offset+2],label+' word '+str(offset//2))

    def pixel(self,x,y,value):
        self.data+=b'\x08'+struct.pack('<HH',x,y)
        self.expect(bytes((value,)),f'pixel {x},{y}')

def mode(number):
    return (bytes((22,number))+b''.join(bytes((19,i,i,0,0,0)) for i in range(64))+
            bytes((23,0,192,0,29,0,0,0,0,26,23,1,0,28,0,29,39,22,17,63)))

def renderer(interface):
    arrays={a['name']:a for a in interface['arrays']};template=bytearray()
    def rect(colour,x,y,right,bottom,relative=False):
        start=len(template);template.extend(bytes((18,0,colour,25,4))+struct.pack('<HH',x,y)+bytes((25,97 if relative else 101))+struct.pack('<HH',right,bottom));return start
    rect(0,0,0,319,175)
    for y in (53,97,141):rect(21,8,y,311,y)
    origins=[rect(12,32,y,14,10,True) for y in (48,92,136)]
    template.extend(bytes((31,0,0))+b'GOLEM-036S: 32-BIT RIGHT SHIFTS\r\n'+
                    b'GOLEM: lanes >>2/3/4; signed y >>2\r\n'+b'ASM: draw + VSYNC; red=negative y\r\n')
    caption=len(template);template.extend(b'BUFFERED            \r\n');code=bytearray()
    for slot,start in enumerate(origins):
        code+=copy(TEMPLATE,start+2,arrays['colour']['buffer'],slot*2,1)
        code+=copy(TEMPLATE,start+5,arrays['screenX']['buffer'],slot*2,2)
        code+=copy(TEMPLATE,start+7,arrays['screenY']['buffer'],slot*2,2)
    code+=call(TEMPLATE)
    return write(TEMPLATE,template)+write(RENDER,code),caption

def build(variant):
    hybrid=variant not in ('stock','meter-stock')
    source='animation' if variant in ('review','meter-stock') else 'probe-relocated' if variant=='relocated' else 'probe'
    directory=BUILD/source;iface,ast,root,meta=load(directory);output=BUILD/variant;output.mkdir(exist_ok=True)
    names=[e['name'] for e in iface['entries']];fields={f['name']:f for f in iface['globals']}
    initial=(directory/'state.bin').read_bytes();state=initial;arrays=arrays_initial(directory,iface)
    tape=Tape(hybrid);tape.send(bytes((22,8,12)));tape.send(write(QUERY,bytes((23,0,128,0))))
    tape.send((directory/'common-install.bin').read_bytes());tape.send((directory/'buffered-install.bin').read_bytes())
    if hybrid:tape.load(directory,meta)
    def observe(label,full=False):
        tape.state(meta['shared_state'],state,label, None if full else [12,14,20,22,24,28]+[f['offset']+n for f in iface['globals'] if source=='animation' or f['name'] in ('flag','ur','sr','bounded','effect') for n in range(0,f['bytes'],2)])
        for a in iface['arrays']:
            if full or a['access']==3:tape.state(a['buffer'],arrays[a['buffer']],label+' '+a['name'])
    def invoke(entry,backend):
        nonlocal state
        tape.named(entry,backend,len(names));state=reference(iface,ast,state,entry,root,arrays=arrays)
    if variant=='review':
        install,caption=renderer(iface);tape.send(install);tape.send(mode(136))
        for frame in range(300):
            backend='buffered' if frame//12%2==0 else 'native'
            if frame%12==0:tape.send(update(TEMPLATE,caption,b'BUFFERED            ' if backend=='buffered' else b'NATIVE              '))
            invoke(0,backend);tape.send(call(RENDER)+bytes((23,0,195)))
            if frame%60==59:observe('frame '+str(frame+1))
        tape.send(mode(8)+call(RENDER))
        get=lambda name:list(struct.unpack('<3H',arrays[next(a['buffer'] for a in iface['arrays'] if a['name']==name)]))
        rectangles=[{'x':x,'y':y,'right':x+14,'bottom':y+10,'colour':c} for x,y,c in zip(get('screenX'),get('screenY'),get('colour'))]
        for r in rectangles:tape.pixel(r['x'],r['y'],r['colour']);tape.pixel(r['right'],r['bottom'],r['colour'])
        visual={'frames':300,'successful_invocations':300,'rectangles':rectangles,'guides':[53,97,141],
                'width':320,'height':240,'playfield_rows':176,'horizontal_shift_counts':[2,3,4],'vertical_shift_count':2,
                'scheduler_implemented':False}
        observe('final full state',full=True)
    elif variant=='meter-stock':
        for n in range(60):invoke(0,'buffered')
        observe('sixty stock wide motion frames',full=True);visual=None
    else:
        selected=[]
        def case(name,updates=None):
            entry=names.index(name);s=bytearray(initial);struct.pack_into('<H',s,28,entry);data=arrays_initial(directory,iface)
            for name,v in (updates or {}).items():
                f=fields[name];struct.pack_into('<I' if f['bytes']==4 else '<H',s,f['offset'],v&((1<<(8*f['bytes']))-1))
            selected.append((entry,bytes(s),data))
        for n,c in [(4294967295,0),(4294967295,1),(4294967295,31),(2147483648,31),(65536,16),(1,32)]:
            case('shiftUnsigned',{'ua':n,'bits':c})
        for n,c in [(-2147483648,0),(-2147483648,31),(-1,31),(-65,6),(2147483647,31),(-1,65535)]:
            case('shiftSigned',{'sa':n,'bits':c})
        case('chain',{'sa':-65,'bits':1,'more':1})
        case('precedence',{'ua':4,'ub':4,'bits':2})
        case('parentheses',{'ua':4,'ub':4,'bits':2})
        case('nested',{'ua':4294967295,'ub':256,'bits':1,'more':8})
        case('countExpression',{'ua':4294967295,'ub':2147483648,'more':31})
        case('boundedShift',{'sa':-2147483648,'bits':0})
        case('priorEffect',{'ua':1,'bits':32})
        case('argumentOrder',{'ua':4294967295,'ub':2,'bits':65535})
        case('argumentOrder',{'ua':1,'ub':2,'bits':65535})
        case('storeUnsigned',{'index':0,'ua':4294967295,'bits':32})
        case('storeSigned',{'signedIndex':1,'sa':-1,'bits':32})
        case('countArray',{'index':1,'ua':4294967295})
        case('skipped',{'ua':1,'bits':32});case('constants')
        for n,(entry,start,data) in enumerate(selected):
            for backend in (('buffered','native') if hybrid else ('buffered',)):
                tape.send(write(meta['shared_state'],start))
                for key,value in data.items():tape.send(write(key,value))
                state=start;arrays={k:bytearray(v) for k,v in data.items()};invoke(entry,backend);observe(f'{backend} case {n}')
        if hybrid:
            a=iface['arrays'][-1];expected=arrays_initial(directory,iface)[a['buffer']]
            for label,data,rc in [('missing array',None,2),('short array',expected[:-1],3),('long array',expected+b'\0',3)]:
                tape.send(packet(a['buffer'],2) if data is None else write(a['buffer'],data))
                state=bytearray(initial);struct.pack_into('<H',state,14,77);tape.send(write(meta['shared_state'],state))
                tape.transaction(2,word(0)+word(0)+word(meta['native_arguments']),5,rc,label+' runtime',handle=True)
                struct.pack_into('<H',state,12,8);tape.state(meta['shared_state'],state,label+' unchanged source')
                tape.send(write(a['buffer'],expected))
            arrays=arrays_initial(directory,iface)
            for a in iface['arrays']:tape.send(write(a['buffer'],arrays[a['buffer']]))
        visual=None
    if hybrid:
        tape.transaction(3,b'\0\0',3,0,'UNLOAD',handle=True);tape.send((directory/'native-cleanup.bin').read_bytes())
    tape.send((directory/'buffered-cleanup.bin').read_bytes());observe('retained storage survives private cleanup',full=True)
    tape.data.append(0);assert len(tape.data)<131072 and len(tape.expected)<=24576,(variant,len(tape.data),len(tape.expected))
    active=BUILD/'active';active.mkdir(exist_ok=True);shutil.copyfile(directory/'calls.inc',active/'calls.inc')
    (active/'dispatch.inc').write_text('dispatch_table:\n'+''.join(' dl golem_'+backend+'_'+name+'\n' for backend in ('buffered','native') for name in names))
    subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'guest.asm',str(output.relative_to(HERE)/'probe.bin'),'-c'],cwd=HERE,check=True)
    for name,data in [('script.bin',tape.data),('expected.dat',tape.expected),('mask.dat',tape.mask)]:(output/name).write_bytes(data)
    expected={'variant':variant,'checks':tape.checks,'result_bytes':len(tape.expected),'module_loads':tape.loads,'calls':tape.calls,'visual':visual,
              'source':source,'state_id':meta['shared_state'],'probe_sha256':sha(output/'probe.bin'),'script_sha256':sha(output/'script.bin'),
              'source_sha256':sha(HERE/(source.removesuffix('-relocated')+'.golem')),'module_sha256':sha(directory/(source.removesuffix('-relocated')+'.vdpl'))}
    (output/'expected.json').write_text(json.dumps(expected,indent=2)+'\n');print(f'{variant}: {len(tape.calls)} calls, {len(tape.checks)} observations, {len(tape.data)} tape bytes.')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('variant',choices=('stock','meter-stock','hybrid','relocated','review','all'));args=parser.parse_args()
    for variant in (('stock','meter-stock','hybrid','relocated','review') if args.variant=='all' else (args.variant,)):build(variant)
