"""Exercise real generated source/helper/publication through bounded SDK mocks."""
from pathlib import Path
import ctypes as C,hashlib,json,os,struct,subprocess,sys
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
RALLY=ROOT.parent/'AgonArcade'
SDK=ROOT.parent/'agon-vdplet'
sys.path.insert(0,str(RALLY/'docs/tasks/RALLY-25/generated-state'))
from adapter import Interface
from host import Runtime,Request

def spec(directory):
    # The older Rally adapter explicitly requires two emitted targets. This
    # native-only profile instead supplies the same checked field/image surface.
    s=object.__new__(Interface);s.directory=Path(directory)
    s.interface=json.loads((s.directory/'interface.json').read_text());s.root=json.loads((s.directory/'root.json').read_text());s.backends=json.loads((s.directory/'backends.json').read_text())
    s.entries={e['name']:e['logical_id'] for e in s.interface['entries']};s.globals={g['name']:g for g in s.interface['globals']};s.arrays={a['name']:a for a in s.interface['arrays']};s.bindings={}
    s.images={s.interface['state_buffer']:(s.directory/'state.bin').read_bytes()}
    s.images.update({a['buffer']:(s.directory/a['initial_data']).read_bytes() for a in s.arrays.values()})
    assert s.interface['native_bindings']['configuration']['profile']=='rally-render-v1'
    assert not s.backends['buffered_entries'] and len(s.backends['native_wrappers'])==len(s.entries)
    return s

class Mock(Runtime):
    def __init__(self,*args):
        self.reject_release_id=None;self.refuse=False;self.cleanup_leases=0
        super().__init__(*args)
        self.store(self.root_id,8,struct.pack('<I',7))
        cfg=self.spec.interface['native_bindings']['configuration'];self.cfg=cfg
        asset=(RALLY/'docs/tasks/RALLY-25/integration-harness/fuji/fuji.r24').read_bytes()
        self.install(cfg['bindings']['asset']['buffer'],asset,1)
        for out in cfg['bindings']['outputs']:self.install(out['buffer'],bytes(out['bytes']))
        self.completion=cfg['completion']['buffer'];self.install(self.completion,bytes(48))
    def release(self,context,lease):
        if self.reject_release_id is not None and lease in self.live:
            pointer=self.live[lease][0]
            if pointer==self.pointer(self.reject_release_id):
                self.reject_release_id=None
                if not self.refuse:del self.live[lease]
                return 8
        return super().release(context,lease)
    def invoke(self,name=None,operation=2,entry=None,argument_bytes=2):
        assert not self.live
        if entry is None:entry=self.spec.entries[name] if name else 0
        if operation==2:
            self.store(self.root_id,28,struct.pack('<H',entry))
            if hasattr(self,'completion'):self.store(self.completion,44,b'\0\0')
        req=Request(C.sizeof(Request),operation,entry,self.argument_id,0,argument_bytes)
        rc=self.lib.vdplet_entry(C.byref(self.host),C.byref(req))
        if self.live:
            assert self.refuse,('unexpected leak',self.live)
            self.cleanup_leases+=len(self.live);self.live.clear() # runtime entry cleanup
        self.calls+=1;self.canaries();return rc
    def generation(self):return struct.unpack_from('<I',self.data(self.root_id),20)[0]

def check(directory):
    s=spec(directory);r=Mock(s,Path(directory)/'host.so');checks=[]
    assert r.invoke('Render30')==12 and r.generation()==0
    assert r.invoke('InitRace')==0 and r.generation()==1
    packets=bytearray();actual=bytearray()
    for frame in range(1,61):
        for _ in range(4):assert r.invoke('Tick120')==0
        assert r.invoke('Render30')==0
        root,arrays=r.snapshot();done=r.data(r.completion)
        assert struct.unpack_from('<HHii',done)==(48,1,0,0)
        assert struct.unpack_from('<III',done,12)==(7,frame,frame*4)
        assert struct.unpack_from('<H',done,44)[0]==1 and r.generation()==1+frame*5
        lengths=struct.unpack_from('<III',done,24);counts=struct.unpack_from('<HHH',done,36)
        lists=[r.data(out['buffer'])[:n] for out,n in zip(r.cfg['bindings']['outputs'],lengths)]
        assert all(b.endswith(b'\x17\0\xa0\xff\xff\7') for b in lists)
        actual+=struct.pack('<6I',*lengths,*counts)+b''.join(lists)
        raw=bytearray(112)
        struct.pack_into('<4sHHIIIiHhiHHHH',raw,0,b'R24S',1,112,7,frame,frame*4,s.get(root,'playerStation'),s.get(root,'renderPhase'),s.get(root,'renderSteering'),s.get(root,'playerLateral'),s.get(root,'renderFlags'),0,0,0)
        for i in range(6):
            if s.array_get(arrays,'active',i):struct.pack_into('<iihBB',raw,40+12*i,s.array_get(arrays,'position',i),s.array_get(arrays,'lateral',i),s.get(root,'routeHeading'),1,0)
        packets+=raw
    oracle=RALLY/'docs/tasks/RALLY-25/.work/scene-replay/oracle'
    road=RALLY/'docs/tasks/RALLY-25/.work/reference/r22/rally-production/data'
    # Use the frozen owner's oracle and road table paths from its actual recipe.
    if not (road/'fuji.road').exists():
        roads=list((RALLY/'docs/tasks/RALLY-25/.work/reference').rglob('fuji.road'));assert len(roads)==1,roads;road=roads[0].parent
    expected=subprocess.check_output([str(oracle),str(road),'fuji','B'],input=bytes(packets))
    assert bytes(actual)==expected,'Actual generated bindings/helper differ from independent owner renderer'
    checks.append('60 actual-source typed marshaling/helper/command-oracle frames')
    for label,mutate,clear,code in [
        ('phase domain',lambda:r.put('renderPhase',8000),lambda:r.put('renderPhase',0),1),
        ('heading domain',lambda:r.put('routeHeading',1921),lambda:r.put('routeHeading',0),1),
        ('completion acquire',lambda:setattr(r,'fault',(r.completion,'missing')),lambda:setattr(r,'fault',None),2),
        ('output acquire',lambda:setattr(r,'fault',(50301,'missing')),lambda:setattr(r,'fault',None),2),
        ('source overflow',lambda:(r.put('worldTick',4294967295)),lambda:r.put('worldTick',240),1)]:
        mutate();before=r.generation();rc=r.invoke('Tick120' if label=='source overflow' else 'Render30');assert rc==code,(label,rc);assert r.generation()==before;assert struct.unpack_from('<H',r.data(r.completion),44)[0]==0;clear();checks.append(label)
    for refuse in (False,True):
        r.reject_release_id=r.completion;r.refuse=refuse;before=r.generation();assert r.invoke('Render30')==8
        assert r.generation()==before and struct.unpack_from('<H',r.data(r.completion),44)[0]==1
        checks.append('completion release after ready '+str(refuse))
    r.refuse=False
    assert r.invoke('StopRace')==0
    before=r.generation();assert r.invoke('Render30')==12 and r.generation()==before
    assert r.invoke('ResetRace')==0
    r.reject_release_id=50301;r.refuse=True;before=r.generation();assert r.invoke('Render30')==8 and r.generation()==before
    r.refuse=False;assert r.invoke('Render30')==12 # helper invalidated after release failure
    assert r.invoke('ResetRace')==0
    r.reject_release_id=r.root_id;before=r.generation();assert r.invoke('Render30')==8
    assert r.generation()==before+1 # final root release failure needs runtime gate
    checks+=['stop/rebind','helper refused-release invalidates binding','root-release runtime gate']
    assert r.peak<=3 and not r.live;r.canaries()
    result={'schema':1,'scope':'real generated native-only binding/helper host integration; no peer game model rerun or target claim',
            'frames':60,'calls':r.calls,'peak_leases':r.peak,'modeled_runtime_cleanup_leases':r.cleanup_leases,'checks':checks,
            'commands_sha256':hashlib.sha256(actual).hexdigest(),'oracle_sha256':hashlib.sha256(expected).hexdigest()}
    print(json.dumps(result))

if __name__=='__main__':check(Path(sys.argv[1]))
