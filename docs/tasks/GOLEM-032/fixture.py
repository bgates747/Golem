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
    rectangle(21,128,8,128,166)
    square=rectangle(48,32,60,48,90)
    template.extend(bytes((31,0,0))+b'GOLEM-032: FRACTIONAL MOTION\r\n'+
                    b'GOLEM: float motion + casts\r\n'+b'ASM: origin + draw + VSYNC, 600 calls\r\n')
    label=len(template);template.extend(b'BUFFERED            \r\n')
    code=(copy(TEMPLATE,square+2,state,18,1)+copy(TEMPLATE,square+5,state,8,2)+
          copy(TEMPLATE,square+11,state,10,2)+call(TEMPLATE))
    return write(TEMPLATE,template)+write(RENDER,code),label

def build(variant):
    hybrid=variant not in ('stock','meter-stock')
    source='drift' if variant in ('review','meter-stock') else ('floats-relocated' if variant=='relocated' else 'floats')
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
        # A visibly distinct traffic light; Golem owns every lamp decision.
        entries=[0]*600
        for frame,entry in enumerate(entries):
            backend='buffered' if frame//12%2==0 else 'native'
            if frame%12==0:
                caption=b'BUFFERED            ' if backend=='buffered' else b'NATIVE              '
                assert len(caption)==20
                tape.send(update(TEMPLATE,offsets,caption))
            tape.named(entry,backend,len(names))
            state=reference(interface,ast,state,entry)
            tape.send(call(RENDER)+bytes((23,0,195)))
            if frame%12==11:
                tape.state(meta['shared_state'],state,f'continuous frame {frame+1} {backend}')
        # Restore single buffering for MOS without discarding the computed state.
        tape.send(mode(8)+call(RENDER))
        final=list(struct.unpack('<12H',state))
        assert state==struct.pack('<ffHHHfHHH',-30.0,0.375,98,114,600,0.75,48,1,0),final
        for x,y,value in ((0,0,0),(128,10,21),(128,160,21),(98,60,48),
                          (114,90,48),(97,60,0),(115,90,0),(180,80,0),(180,91,0)):
            tape.pixel(x,y,value)
        tape.state(meta['shared_state'],state,'final shared state')
        tape.transaction(3,b'\0\0',3,0,'UNLOAD',handle=True)
        tape.send((directory/'buffered-cleanup.bin').read_bytes()+(directory/'native-cleanup.bin').read_bytes())
        tape.state(meta['shared_state'],state,'state survives both private cleanups')
        visual={'frames':len(entries),'final_words':final,'width':320,'height':240,'playfield_rows':176,
                'colours':{'negative':48,'nonnegative':12,'origin':21},'private_cleanups_preserve_state':True}
    else:
        vectors=cases(directory)
        if variant=='meter-stock':
            vectors=vectors[:21]+[vectors[21+i] for i in (0,15,31,32,63,95,127,599)]
        else:
            fields={f['name']:f['offset'] for f in interface['globals']}
            def get(state,name):return struct.unpack_from('<I' if name in ('a','b','result','bounded') else '<H',state,fields[name])[0]
            selected=[];seen=set()
            pairs={0:{(0x3f800000,0x33800000),(0x7f7fffff,0x7f7fffff),(0x00800000,0x80800001),(0x80000000,0)},
                   1:{(0x7f7fffff,0xff7fffff),(0x00800001,0x00800000),(0x3fc00000,0x3f800000)},
                   2:{(0x7f7fffff,0x40000000),(0x00800000,0x3f000000),(0x00800000,0x3f7fffff),(0x3fc00000,0xbf000000)},
                   3:{(0x4b800000,0x3f800000)}}
            conversion={10:{0xc70000ff,0x46ffffff,0x47000000},
                        11:{0xbf000000,0x477fffff,0x47800000},
                        14:{0x4121999a,0x41219999,0xc121999a,0xc1219999},15:{0xbf800000},16:{0x80000000}}
            for entry,state in vectors:
                if (entry,state) in seen:continue
                seen.add((entry,state));a,b=get(state,'a'),get(state,'b');u,flag=get(state,'u'),get(state,'flag')
                signed=get(state,'signedResult')
                take=(entry<4 and (a,b) in pairs[entry] or
                      4<=entry<10 and (a,b) in ((0xbf800000,0),(0,0)) or
                      entry in conversion and a in conversion[entry] and u==0 and signed==0 and (flag==0 or entry==15) or
                      entry==12 and a==0 and u==32767 and signed==32768 and flag==0 or
                      entry==13 and a==0 and u==65535 and signed==65535 and flag==0 or
                      entry==0 and (a in (1,0x7f800000,0x7fc00000) or get(state,'result')==0x7fc00000))
                if take:selected.append((entry,state))
            vectors=selected
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
    assert len(tape.data)<131072 and len(tape.expected)<=24576,(len(tape.data),len(tape.expected))
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
