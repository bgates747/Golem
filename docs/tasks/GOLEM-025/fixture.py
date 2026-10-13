"""Build finite MOS qualification and the labelled visual review."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from host_checks import cases
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
            self.patched(update(wrapper,11,b'\0\0'),11)

    def named(self,entry,backend,count):
        self.data+=b'\x06'+bytes((entry+(count if backend=='native' else 0),))
        self.calls.append({'entry':entry,'backend':backend})

    def state(self,key,expected,label):
        if self.hybrid:
            for offset in range(0,len(expected),2):
                self.transaction(5,struct.pack('<HI',key,offset),4,struct.unpack_from('<H',expected,offset)[0],label+' word '+str(offset//2))
        else:
            self.data+=b'\x07'+word(key)+word(0)+word(len(expected))
            self.expect(expected,label)

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
    rectangle(21,20,164,218,173)
    meter=rectangle(11,20,165,20,172)
    square=rectangle(11,48,152,60,164)
    template.extend(bytes((31,0,0))+b'GOLEM meter.golem: grow / shrink\r\n'+
                    b'GOLEM: coordinates + tick count\r\n'+b'ASM: draws, selects, VSYNC pacing\r\n')
    label=len(template)
    template.extend(b'BUFFERED (cyan)     \r\n')
    code=(copy(TEMPLATE,meter+11,state,12,2)+copy(TEMPLATE,square+5,state,4,4)+
          copy(TEMPLATE,square+11,state,8,4)+call(TEMPLATE))
    return write(TEMPLATE,template)+write(RENDER,code), (meter+2,square+2,label)

def build(variant):
    hybrid=variant not in ('stock','meter-stock')
    source='meter' if variant in ('review','meter-stock') else ('arithmetic-relocated' if variant=='relocated' else 'arithmetic')
    directory=BUILD/source
    meta=json.loads((directory/'backends.json').read_text())
    interface=json.loads((directory/'interface.json').read_text())
    ast=json.loads((directory/'program.json').read_text())
    names=[entry['name'] for entry in interface['entries']]
    output=BUILD/variant
    output.mkdir(exist_ok=True)
    tape=Tape(hybrid)
    tape.send(bytes((22,8,12)))
    tape.send(write(QUERY,bytes((23,0,128,0))))
    tape.send((directory/'common-install.bin').read_bytes())
    tape.send((directory/'buffered-install.bin').read_bytes())
    if hybrid:
        tape.load(directory,meta)
    state=(directory/'state.bin').read_bytes()
    if variant=='review':
        install,offsets=renderer(meta['shared_state'])
        tape.send(install)
        tape.send(mode(136))
        # Twelve round trips plus a final climb: ten seconds at 60 Hz.
        # Both engines continue the same state; no eZ80 coordinate arithmetic.
        entries=([0]*24+[1]*24)*12+[0]*24
        for frame,entry in enumerate(entries):
            backend='buffered' if frame//12%2==0 else 'native'
            if frame%12==0:
                colour=11 if backend=='buffered' else 52
                for offset in offsets[:2]:
                    tape.send(update(TEMPLATE,offset,bytes((colour,))))
                caption=b'BUFFERED (cyan)     ' if backend=='buffered' else b'NATIVE (orange)     '
                assert len(caption)==20
                tape.send(update(TEMPLATE,offsets[2],caption))
            tape.named(entry,backend,len(names))
            state=reference(interface,ast,state,entry)
            tape.send(call(RENDER)+bytes((23,0,195)))
            if frame%12==11:
                tape.state(meta['shared_state'],state,f'continuous frame {frame+1} {backend}')
        # Restore single buffering for MOS without discarding the computed state.
        tape.send(mode(8)+call(RENDER))
        final=list(struct.unpack('<9H',state))
        assert final==[96,4,240,56,252,68,212,600,0],final
        for x,y,value in ((0,0,0),(240,56,52),(246,62,52),(253,62,0),(48,152,0),
                          (100,168,52),(212,168,52),(217,168,21),(219,168,0)):
            tape.pixel(x,y,value)
        tape.state(meta['shared_state'],state,'final shared state')
        tape.transaction(3,b'\0\0',3,0,'UNLOAD',handle=True)
        tape.send((directory/'buffered-cleanup.bin').read_bytes()+(directory/'native-cleanup.bin').read_bytes())
        tape.state(meta['shared_state'],state,'state survives both private cleanups')
        visual={'frames':len(entries),'final_words':final,'width':320,'height':240,'playfield_rows':176,
                'colours':{'buffered':11,'native':52,'rail':21},'private_cleanups_preserve_state':True}
    else:
        vectors=cases(directory)
        if variant=='relocated':
            vectors=[vectors[i] for i in (8,9,18,22,28,40,41,42,43,44,45)]
        for index,(entry,initial) in enumerate(vectors):
            want=reference(interface,ast,initial,entry)
            for backend in (('buffered','native') if hybrid else ('buffered',)):
                tape.send(write(meta['shared_state'],initial))
                tape.named(entry,backend,len(names))
                tape.state(meta['shared_state'],want,f'{backend} case {index}')
                state=want
        if hybrid:
            tape.transaction(3,b'\0\0',3,0,'UNLOAD',handle=True)
            tape.send((directory/'native-cleanup.bin').read_bytes())
        tape.send((directory/'buffered-cleanup.bin').read_bytes())
        tape.state(meta['shared_state'],state,'shared state survives cleanup')
        visual=None
    tape.data.append(0)
    assert len(tape.data)<65536 and len(tape.expected)<=24576,(len(tape.data),len(tape.expected))
    active=BUILD/'active';active.mkdir(exist_ok=True)
    shutil.copyfile(directory/'calls.inc',active/'calls.inc')
    dispatch='dispatch_table:\n'+''.join(' dl golem_'+backend+'_'+name+'\n' for backend in ('buffered','native') for name in names)
    (active/'dispatch.inc').write_text(dispatch)
    subprocess.run([str(ROOT/'docs/tasks/GOLEM-007/build/ez80asm'),'guest.asm',str(output.relative_to(HERE)/'probe.bin'),'-c'],cwd=HERE,check=True)
    (output/'script.bin').write_bytes(tape.data)
    (output/'expected.dat').write_bytes(tape.expected)
    (output/'mask.dat').write_bytes(tape.mask)
    expected={'variant':variant,'checks':tape.checks,'result_bytes':len(tape.expected),'module_loads':tape.loads,
              'calls':tape.calls,'visual':visual,'source':source,'state_id':meta['shared_state'],
              'probe_sha256':sha(output/'probe.bin'),'script_sha256':sha(output/'script.bin'),
              'source_sha256':sha(HERE/(source.removesuffix('-relocated')+'.golem')),
              'module_sha256':sha(directory/(source.removesuffix('-relocated')+'.vdpl'))}
    (output/'expected.json').write_text(json.dumps(expected,indent=2)+'\n')
    print(f'{variant}: {len(tape.calls)} actual named calls, {len(tape.checks)} observations, {len(tape.data)} tape bytes.')

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('variant',choices=('stock','meter-stock','hybrid','relocated','review','all'))
    args=parser.parse_args()
    for variant in (('stock','meter-stock','hybrid','relocated','review') if args.variant=='all' else (args.variant,)):
        build(variant)
