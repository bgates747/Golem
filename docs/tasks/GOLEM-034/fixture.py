"""Build finite MOS qualification and the labelled visual review."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from host_checks import vectors,expected
from model import reference

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
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
        assert 0<len(data)<=65535
        self.data+=b'\x01'+word(len(data))+data

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

def renderer(state):
    template=bytearray()
    def rectangle(colour,x,y,right,bottom):
        start=len(template)
        template.extend(bytes((18,0,colour,25,4))+struct.pack('<HH',x,y)+bytes((25,101))+struct.pack('<HH',right,bottom))
        return start
    rectangle(0,0,0,319,175)
    rectangle(21,128,8,128,166)
    square=rectangle(12,32,60,48,90)
    template.extend(bytes((31,0,0))+b'GOLEM-034: D0 SCALAR PROBE\r\n'+
                    b'GOLEM: five named scalar Subs\r\n'+b'ASM: calls/draw/VSYNC; no scheduler\r\n')
    label=len(template);template.extend(b'BUFFERED            \r\n')
    code=(copy(TEMPLATE,square+2,state,122,1)+copy(TEMPLATE,square+5,state,116,2)+
          copy(TEMPLATE,square+11,state,118,2)+call(TEMPLATE))
    return write(TEMPLATE,template)+write(RENDER,code),label

def build(variant):
    hybrid=variant not in ('stock','meter-stock')
    source='probe-relocated' if variant=='relocated' else 'probe'
    directory=BUILD/source
    meta=json.loads((directory/'backends.json').read_text())
    interface=json.loads((directory/'interface.json').read_text())
    root=json.loads((directory/'root.json').read_text())
    ast=json.loads((directory/'program.json').read_text())
    names=[entry['name'] for entry in interface['entries']]
    output=BUILD/variant;output.mkdir(exist_ok=True)
    tape=Tape(hybrid)
    tape.send(bytes((22,8,12)))
    tape.send(write(QUERY,bytes((23,0,128,0))))
    for binding in root['bindings']:
        if binding['buffer']:tape.send(write(binding['buffer'],bytes(binding['installed_min_bytes'])))
    tape.send((directory/'common-install.bin').read_bytes())
    tape.send((directory/'buffered-install.bin').read_bytes())
    if hybrid:tape.load(directory,meta)
    state=(directory/'state.bin').read_bytes()
    sampled=list(range(8,26,2))+[28,32,34]+[f['offset'] for f in interface['globals']]
    if variant=='review':
        install,label=renderer(meta['shared_state']);tape.send(install);tape.send(mode(136))
        def invoke(entry,backend):
            nonlocal state
            tape.named(entry,backend,len(names));state=reference(interface,ast,state,entry,root)
        invoke(0,'buffered')
        for frame in range(600):
            backend='buffered' if frame//12%2==0 else 'native'
            if frame%12==0:
                caption=b'BUFFERED            ' if backend=='buffered' else b'NATIVE              '
                tape.send(update(TEMPLATE,label,caption))
            if frame in (180,240,420):invoke({180:4,240:3,420:0}[frame],backend)
            invoke(1,backend);invoke(2,backend)
            tape.send(call(RENDER)+bytes((23,0,195)))
            if frame%12==11:tape.state(meta['shared_state'],state,f'frame {frame+1} {backend}',sampled)
        invoke(4,'native')
        tape.send(mode(8)+call(RENDER))
        final=[struct.unpack_from('<H',state,i)[0] for i in range(112,len(state),2)]
        assert final==[56,0,56,72,180,48,0],final
        assert struct.unpack_from('<I',state,20)[0]==1205
        for x,y,value in ((0,0,0),(128,10,21),(128,160,21),(56,60,48),
                          (72,90,48),(55,60,0),(73,90,0),(180,80,0),(180,91,0)):tape.pixel(x,y,value)
        tape.state(meta['shared_state'],state,'final root')
        visual={'frames':600,'successful_invocations':1205,'final_words':final,'width':320,'height':240,
                'playfield_rows':176,'colours':{'forward':12,'reverse':48,'marker':21},
                'pause_frames':[181,240],'reset_frame':241,'restart_frame':421,
                'private_cleanups_preserve_state':True,'scheduler_implemented':False}
    else:
        all_cases=vectors(directory);selected=[];labels=set()
        # All five entries, one rejection in each metadata class, source failure,
        # opaque session/runtime fields, generation wrap and unknown dispatch.
        for item in all_cases:
            entry,physical,initial,label=item
            if physical not in (0,7):continue
            key=(label,entry) if label in ('normal','opaque-wrap','source-admission') else (label,)
            if label=='metadata':
                changed=[i for i in range(112) if initial[i]!=(directory/'state.bin').read_bytes()[i] and i not in range(14,26) and i not in (28,29)]
                at=changed[0];key=(label,1 if at<2 else 2 if at<4 else 5 if at>=48 else 6 if at in (30,31) or 40<=at<48 else 3)
            if label=='sequence':continue
            if key not in labels:labels.add(key);selected.append(item)
        for index,(entry,physical,initial,label) in enumerate(selected):
            want,_=expected(directory,entry,physical,initial)
            changed=[i//2*2 for i in range(len(initial)) if initial[i]!=(directory/'state.bin').read_bytes()[i]]
            offsets=sampled+changed
            for backend in (('buffered','native') if hybrid else ('buffered',)):
                tape.send(write(meta['shared_state'],initial))
                if physical==7:
                    if backend=='buffered':tape.send(call(meta['buffered_dispatch']))
                    else:tape.transaction(2,word(0)+word(0)+word(meta['native_arguments']),5,1,'unknown requested runtime',handle=True)
                else:tape.named(entry,backend,len(names))
                tape.state(meta['shared_state'],want,f'{backend} {label} case {index}',offsets);state=want
        if hybrid:
            # Observe actual runtime results independently from root.runtime_result.
            for name,bound,rc in (('missing input',None,2),('short input',17,3)):
                if bound is None:tape.send(packet(20000,2))
                else:tape.send(write(20000,bytes(bound)))
                initial=bytearray((directory/'state.bin').read_bytes());struct.pack_into('<HH',initial,12,0,77)
                tape.send(write(meta['shared_state'],initial))
                tape.transaction(2,word(0)+word(0)+word(meta['native_arguments']),5,rc,name+' runtime',handle=True)
                struct.pack_into('<H',initial,12,8);state=bytes(initial)
                tape.state(meta['shared_state'],state,name+' rejected',sampled)
                tape.send(write(20000,bytes(18)))
            initial=(directory/'state.bin').read_bytes()
            tape.send(write(meta['shared_state'],initial[:-2]))
            tape.transaction(2,word(0)+word(0)+word(meta['native_arguments']),5,3,'short root runtime',handle=True)
            tape.state(meta['shared_state'],initial[:-2],'short root unchanged')
            tape.send(write(meta['shared_state'],initial))
            tape.transaction(2,word(0)+word(65535)+word(meta['native_arguments']),5,9,'unknown SDK entry runtime',handle=True)
            tape.state(meta['shared_state'],initial,'unknown SDK unchanged');state=initial
        visual=None
    if hybrid:
        tape.transaction(3,b'\0\0',3,0,'UNLOAD',handle=True)
        tape.send((directory/'native-cleanup.bin').read_bytes())
    tape.send((directory/'buffered-cleanup.bin').read_bytes())
    tape.state(meta['shared_state'],state,'root survives both private cleanups')
    for binding in root['bindings']:
        if binding['buffer']:tape.state(binding['buffer'],bytes(binding['installed_min_bytes']),'borrowed role '+str(binding['role']))
    tape.data.append(0)
    assert len(tape.data)<131072 and len(tape.expected)<=24576,(len(tape.data),len(tape.expected))
    active=BUILD/'active';active.mkdir(exist_ok=True)
    shutil.copyfile(directory/'calls.inc',active/'calls.inc')
    dispatch='dispatch_table:\n'+''.join(' dl golem_'+backend+'_'+name+'\n' for backend in ('buffered','native') for name in names)
    (active/'dispatch.inc').write_text(dispatch)
    subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'guest.asm',str(output.relative_to(HERE)/'probe.bin'),'-c'],cwd=HERE,check=True)
    (output/'script.bin').write_bytes(tape.data);(output/'expected.dat').write_bytes(tape.expected);(output/'mask.dat').write_bytes(tape.mask)
    data={'variant':variant,'checks':tape.checks,'result_bytes':len(tape.expected),'module_loads':tape.loads,
          'calls':tape.calls,'visual':visual,'source':source,'state_id':meta['shared_state'],
          'probe_sha256':sha(output/'probe.bin'),'script_sha256':sha(output/'script.bin'),
          'source_sha256':sha(HERE/'probe.golem'),'module_sha256':sha(directory/(source.removesuffix('-relocated')+'.vdpl'))}
    (output/'expected.json').write_text(json.dumps(data,indent=2)+'\n')
    print(f'{variant}: {len(tape.calls)} named calls, {len(tape.checks)} observations, {len(tape.data)} tape bytes.')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('variant',choices=('stock','meter-stock','hybrid','relocated','review','all'))
    args=parser.parse_args()
    for variant in (('stock','meter-stock','hybrid','relocated','review') if args.variant=='all' else (args.variant,)):build(variant)
