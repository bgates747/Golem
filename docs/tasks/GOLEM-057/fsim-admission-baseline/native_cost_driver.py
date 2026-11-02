"""Finite off/on native diagnostic supervisor. Default is read-only preflight."""
import argparse,collections,fcntl,hashlib,json,math,os,re,shutil,struct,subprocess,time
from pathlib import Path
import cost_driver as base
from build import ROOT,HERE,BUILD
Fault=base.Fault;require=base.require;sha=base.sha
OUT=BUILD/'native-driver-r1';DIAG=BUILD/'native-cost-r1'
PROFILE=base.PROFILE;HELPER=base.HELPER;ARMS=('off','on')
READY_SHA='c361e1545d6911adf5045e7e8b00b3b980c870047b16d54b88893986c439fe75'
GP=b'\x17\0\x80';RUN=b'\x17\0\xd8';SWAP=b'\x17\0\xc3'
CLEAR=b'\x17\0\xa0\xff\xff\x02'

def validate_record(raw,arm):
    require(len(raw)==640,'native record length')
    w=struct.unpack('<160I',raw);v=ARMS.index(arm)
    require(w[:9]==(0x504e4331,1,640,1,v,v,2 if v else 6,4,64),'native record schema/epoch/mode')
    require(w[9:14]==(68,68,68,0,0) and w[14]>0,'native pair/order counts')
    require(w[15:19]==(0,0,0,0) and w[19]<=136 and 0<w[20]<=w[14],'native errors/wraps/sequence')
    require(w[28:32]==(0,136,0,0) and not any(w[140:]),'native unwrap/publish/reserved')
    if v:
        require(w[21]==32 and w[22]<=w[23]<2**31 and w[24]<=32,'native calibration')
        require(0<(w[26]-w[25])%2**32<2**31,'native last call discontinuity')
    else:require(w[19]==0 and not any(w[21:28]) and not any(w[32:140]),'unavailable counts treated as timing')
    stats=[]
    for i in range(18):
        count,lo,hi,minimum,maximum,zeros=w[32+i*6:38+i*6];total=(hi<<32)|lo
        require(count==(64 if v else 0) and zeros<=count,'native aggregate counts')
        if v:
            require(minimum<=maximum<2**31 and minimum*count<=total<=maximum*count,'native aggregate range')
            require(not zeros or minimum==0,'native zero aggregate')
        stats.append(dict(count=count,sum_lo=lo,sum_hi=hi,min=minimum,max=maximum,zeros=zeros))
    for start,n in ((0,6),(7,10)):
        require(sum((s['sum_hi']<<32)|s['sum_lo'] for s in stats[start:start+n])==
            ((stats[start+n]['sum_hi']<<32)|stats[start+n]['sum_lo']),'native bin/full reconciliation')
    return dict(header=list(w[:32]),stats=stats,structural_valid=True,unwrap_qualified=False,
        timing='conditional-modular' if v else 'unavailable',physical_timing=False)

class Trace:
    """Incremental observations of two retained byte streams, not guest hooks."""
    def __init__(self,fixed):
        self.fixed=fixed;self.tx_at=self.rx_at=0;self.requests=[];self.native_at=0
        self.gps=[];self.gp_at=0;self.poses=[];self.runs=[];self.done=[];self.presented=0
        self.reads=[];self.words=[];self.unload=None;self.stack=[];self.handle=None
        self.tx_end=self.rx_end=0;self.gp68_at=self.read_done_at=None
    def tx(self,data,now):
        self.tx_end=len(data)
        prefixes=(base.POSE,RUN,GP)
        while self.tx_at<len(data):
            at=self.tx_at;tail=data[at:]
            if any(p.startswith(tail) for p in prefixes):break
            if data.startswith(base.POSE,at):
                if len(data)<at+len(base.POSE)+44:break
                pose=data[at+len(base.POSE):at+len(base.POSE)+44]
                p=bytearray(pose);seq=struct.unpack_from('<I',p,4)[0]
                require(len(self.poses)<68 and seq>0 and (not self.poses or seq>self.poses[-1]['sequence']),'pose count/order')
                struct.pack_into('<I',p,4,1);struct.pack_into('<I',p,40,0)
                fixed=bytearray(self.fixed);struct.pack_into('<I',fixed,4,1);struct.pack_into('<I',fixed,40,0)
                require(p==fixed and struct.unpack_from('<I',pose,40)[0]==int(not self.poses),'fixed visual pose/reset')
                self.poses.append(dict(sequence=seq,offset=at));self.tx_at=at+len(base.POSE)+44;continue
            if data.startswith(RUN,at):
                if len(data)<at+5:break
                n=struct.unpack_from('<H',data,at+3)[0]
                require(6<=n<=32,'native request width')
                if len(data)<at+5+n:break
                p=data[at+5:at+5+n];require(p[0]==1 and p[4:6]==b'\x01\0','native request protocol')
                op=p[1];token=struct.unpack_from('<H',p,2)[0];body=p[6:]
                require(token and (not self.requests or token==(self.requests[-1]['token']+1)&65535),'native request token order')
                req=dict(op=op,token=token,body=body,offset=at,observed=now)
                if op==2:
                    require(len(body)==6 and len(self.runs)<136 and not self.reads,'RUN width/count/order')
                    handle,entry,args=struct.unpack('<3H',body);require(handle and args==30005 and entry==len(self.runs)%2,'RUN entry/args order')
                    if self.handle is None:self.handle=handle
                    require(handle==self.handle,'RUN handle drift')
                    if entry==0:require(len(self.poses)==len(self.runs)//2+1,'PREPARE/pose pairing')
                    self.runs.append(req);req['entry']=entry
                elif op==4:require(body==struct.pack('<H',17),'runtime INFO field')
                elif op==5:
                    require(len(body)==6 and len(self.gps)>=68 and sum(g['frame'] for g in self.gps)==68,'READ16 before final presentation send')
                    buffer,offset=struct.unpack('<HI',body)
                    require(buffer==30003 and offset==15744+len(self.reads)*2 and len(self.reads)<320 and self.unload is None,'READ16 buffer/order/width')
                    self.reads.append(req)
                elif op==3:
                    require(len(body)==2 and struct.unpack('<H',body)[0]==self.handle and len(self.reads)==320 and self.unload is None,'unload before complete readback')
                    self.unload=req
                else:raise Fault('unexpected native runtime operation')
                self.requests.append(req);self.tx_at=at+5+n;continue
            if data.startswith(GP,at):
                if len(data)<at+4:break
                frame=at>=3 and data[at-3:at]==SWAP
                if frame:
                    count=sum(g['frame'] for g in self.gps)
                    require(count<68 and len(self.runs)==2*(count+1) and not self.reads,'GP/RUN send order')
                self.gps.append(dict(token=data[at+3],frame=frame,offset=at));self.tx_at=at+4;continue
            self.tx_at+=1
    def rx(self,data,now):
        self.rx_end=len(data)
        while self.rx_at+2<=len(data):
            kind,n=data[self.rx_at:self.rx_at+2];require(kind&128,'reply framing')
            if self.rx_at+2+n>len(data):break
            p=data[self.rx_at+2:self.rx_at+2+n]
            # Files are sampled separately. Retain a complete RX tail until its
            # earlier TX becomes visible; never consume/drop an unowned reply.
            if kind==129 and (n==10 or p.startswith(b'VDPL')) and self.native_at>=len(self.requests):break
            if kind==128 and self.gp_at>=len(self.gps):break
            self.rx_at+=2+n
            if kind==129 and (n==10 or p.startswith(b'VDPL')):
                require(p.startswith(b'VDPL'),'native reply signature')
                require(len(p)==10 and p[4]==1 and self.native_at<len(self.requests),'native reply schema/owner')
                req=self.requests[self.native_at];self.native_at+=1
                token,value=struct.unpack_from('<HH',p,6);op=req['op']
                require(token==req['token'] and p[5]==(4 if op in (4,5) else 3),'native reply token/kind')
                require(now-req['observed']<2 if op==2 else True,'2s RUN observation deadline')
                if op==2:require(value==0,'native RUN error');self.done.append(req)
                elif op==4:require(value>=512,'actual INFO stack below512');self.stack.append(value)
                elif op==5:
                    require(self.presented==68 and len(self.words)<320,'READ16 reply before final GP/extra')
                    self.words.append(value)
                    if len(self.words)==320:self.read_done_at=now
                elif op==3:require(value==0 and len(self.words)==320,'unload reply error/order');req['done']=True
            elif kind==128:
                require(n==1 and self.gp_at<len(self.gps),'GP reply schema/owner')
                gp=self.gps[self.gp_at];self.gp_at+=1;require(p[0]==gp['token'],'GP reply token/order')
                if gp['frame']:
                    self.presented+=1;require(len(self.done)==2*self.presented,'GP before matching RUN DONE')
                    if self.presented==68:self.gp68_at=now
    def update(self,tx,rx,now):
        self.tx(tx,now);self.rx(rx,now)
        for r in self.requests[self.native_at:]:
            if r['op']==2:require(now-r['observed']<2,'2s RUN observation deadline')
    def complete(self):
        require(self.tx_at==self.tx_end and self.rx_at==self.rx_end,'partial terminal wire/reply')
        require(len(self.poses)==68 and len(self.runs)==len(self.done)==136 and self.presented==68,'incomplete pose/RUN/GP cohort')
        require(len(self.reads)==len(self.words)==320 and self.native_at==len(self.requests),'incomplete native read/replies')
        require(self.unload and self.unload.get('done') and len(self.stack)>=3,'unload/stack proof missing')
        raw=struct.pack('<320H',*self.words)
        return raw

def validate_result(io,arm,tx,replies,trace):
    # Both clients retain original coarse A; do not conflate it with native OFF.
    result=base.validate_result(io,'a',tx,base.packets(replies))
    names=io.names();bins=[n for n in names if re.fullmatch(r'nc\d{3}\.bin',n)]
    logs=[n for n in names if re.fullmatch(r'nc\d{3}\.log',n)]
    require(len(bins)==len(logs)==1 and bins[0][:-4]==logs[0][:-4],'native unique output pair')
    raw=io.read(bins[0]);require(raw==trace.complete(),'saved native record differs from observed READ16')
    native=validate_record(raw,arm);head=native['header']
    require(head[20]==trace.poses[0]['sequence'] and head[14]==trace.poses[-1]['sequence'],'native record/pose sequence')
    lines=io.read(logs[0]).decode('ascii').splitlines()
    def one(prefix):
        found=[base.fields(s) for s in lines if s.startswith(prefix)];require(len(found)==1,'native summary schema');return found[0]
    h=one('P066NC variant=');end=one('P066NC END ');v=ARMS.index(arm)
    require(h['variant']==str(v) and h['kind']==str(v) and h['record']==bins[0] and h['structural']=='pass' and h['unwrap_qualified']==h['physical_timing']=='0' and h['counters']==('conditional-modular' if v else 'unavailable'),'native summary mode')
    require(end['status']=='complete' and end['readback']==end['output']=='1' and end['error']==end['unsafe']=='0','native terminal')
    rows=[base.fields(s) for s in lines if s.startswith('P066NC stat=')];require(len(rows)==18,'native summary stat count')
    for i,(f,s) in enumerate(zip(rows,native['stats'])):
        require(f=={k:str(value) for k,value in dict(stat=i,**s).items()},'native summary/binary disagreement')
    unload=trace.unload['offset'];require(CLEAR in tx[unload:] and tx.rfind(base.CLI)>unload,'unload/clear/CLI order')
    result.update(native=native,native_record_sha256=hashlib.sha256(raw).hexdigest(),native_stack_min=min(trace.stack),
        native_pairs=68,native_READ16=320,conditional_only=True)
    return result

def wait_trace(io,trace,tx_offset,rx_offset,predicate,deadline,phase):
    def observe():
        trace.update(io.raw(tx_offset),io.replies()[rx_offset:],io.now());return predicate()
    base.wait(io,observe,deadline,phase)

def cleanup(io,deadline,guest_done):
    # Retain owned process teardown; observe Escape release explicitly first.
    io.release_keys()
    if io.alive() and not guest_done and io.now()<deadline:
        try:
            base.tap(io,0x76,min(deadline,io.now()+2))
            base.wait(io,lambda:b'Flight ended ticks=' in io.raw(),min(deadline,io.now()+10),'guest stop')
        except Fault:io.release_keys()
    result=base.cleanup(io,deadline,True)
    result['guest_done']=guest_done
    return result

def run_arm(io,arm,build_id,fixed):
    started=io.now();guest_done=False;result={};error=None;cleanup_deadline=None
    try:
        io.begin();started=io.now();whole=started+450
        base.wait(io,lambda:base.BOOT in io.raw() and base.PROMPT in io.raw(),min(whole,started+75),'boot/prompt')
        boot_done=io.now();offset=len(io.raw());load_deadline=min(whole,boot_done+195)
        base.type_text(io,'cd /corsair-native-cost-'+arm+'\nload flight.bin\nrun\n',load_deadline)
        def ready():
            data=io.raw(offset);require(b'Flight ended ticks=' not in data,'early guest exit')
            if base.READY not in data or b'mass and aerodynamics unchanged.' not in data:return False
            require(b'FSIM flight '+build_id.encode()+b'\r' in data,'wrong build')
            at=0
            for name in (b'fsim.cfg',b'assets/renderer.vdpl',b'assets/native.scn',b'assets/corsair.rgba2',b'assets/bay.idx4',b'assets/native.vdu'):
                at=data.find(name,at);require(at>=0,'asset order');at+=len(name)
            return True
        base.wait(io,ready,load_deadline,'assets/Ready')
        prior=base.packets(io.replies());modes=[p for k,p in prior if k==134 and len(p)==8]
        require(modes and modes[-1]==bytes([128,2,224,1,80,60,16,0]),'incoming CLI mode')
        tx_at=len(io.raw());rx_at=len(io.replies());key_at=io.now();trace=Trace(fixed)
        base.tap(io,0x5a,min(whole,key_at+120));key_up=io.now()
        wait_trace(io,trace,tx_at,rx_at,lambda:trace.gp68_at is not None,min(whole,key_at+120),'flight GP68')
        # A polling batch can contain both endpoints. Keep their observed times.
        read_deadline=min(whole,trace.gp68_at+30)
        wait_trace(io,trace,tx_at,rx_at,lambda:trace.read_done_at is not None,read_deadline,'post-stop READ16')
        cleanup_deadline=min(whole,trace.read_done_at+30)
        wait_trace(io,trace,tx_at,rx_at,lambda:b'Flight ended ticks=' in io.raw(tx_at),cleanup_deadline,'terminal/MOS cleanup')
        guest_done=True
        base.wait(io,lambda:('cost001.log' in io.names() and b'/corsair-native-cost-'+arm.encode()+b' *' in io.raw(tx_at)),cleanup_deadline,'logs/MOS return')
        trace.update(io.raw(tx_at),io.replies()[rx_at:],io.now())
        result=validate_result(io,arm,io.raw(tx_at),io.replies()[rx_at:],trace)
        base.type_text(io,'copy fsim.cfg proof.cfg\n',cleanup_deadline)
        base.wait(io,lambda:'proof.cfg' in io.names(),cleanup_deadline,'MOS copy proof')
        require(io.read('proof.cfg')==io.read('fsim.cfg'),'MOS copy mismatch')
        result.update(ready_key_bracket=[key_at,key_up],GP68_observed=trace.gp68_at,READ16_complete_observed=trace.read_done_at,
            host_boundary='50ms host observation, separate TX/RX causal endpoints; not pixel or physical CPU timing')
    except Exception as exc:error=repr(exc)
    finally:
        deadline=min(started+450,io.now()+30,cleanup_deadline if cleanup_deadline is not None else started+450)
        try:teardown=cleanup(io,deadline,guest_done)
        except Exception as exc:teardown=dict(guest_done=guest_done,forced=True,alive=io.alive(),returncode=io.exit_code(),error=repr(exc))
        if not error and guest_done and not teardown['alive'] and not teardown['forced'] and teardown['returncode']==0:
            try:
                stats=io.stats();require(not stats['failed'] and all(stats[k]==0 for k in ('unhandled','unregistered','unmapped')),'firmware/frontend faults')
                native_trace=io.native_trace();require(b'VDPL LOAD handle=' in native_trace and b'VDPL UNLOAD status=0' in native_trace,'native load/unload trace')
                result['firmware_stats']=stats
            except Exception as exc:error=repr(exc)
        result.update(arm=arm,error=error,teardown=teardown,passed=not error and guest_done and not teardown['forced'] and not teardown['alive'] and teardown['returncode']==0)
        io.finish(result)
    return result

def cohort(factory,build_ids,fixed):
    results=[]
    for arm in ARMS:
        result=run_arm(factory(arm),arm,build_ids[arm],fixed);results.append(result)
        if not result['passed']:break
    return results

class HostIO(base.HostIO):
    def __init__(self,arm,output,manifest):
        super().__init__(arm,output,manifest);self.package=PROFILE/'sdcard'/('corsair-native-cost-'+arm)
    def native_trace(self):
        p=PROFILE/'last-run/esp32-uart0.log';return p.read_bytes() if p.exists() else b''
    def begin(self):
        if 'freeze' in self.manifest:preflight(self.manifest['freeze'])
        admission=native_admission()
        base.guarded_destination(self.output);base.guarded_destination(self.package)
        previous=PROFILE/'last-run';require(previous.is_dir() and not previous.is_symlink(),'last-run ownership')
        self.output.mkdir();self.owned_output=True
        (self.output/'native-admission.json').write_text(json.dumps(admission,indent=2)+'\n')
        self.package.mkdir()
        for name,record in self.manifest['arms'][self.arm].items():
            source=DIAG/self.arm/'client/textured'/name;dest=self.package/name
            require(sha(source)==record['sha256'],'candidate changed before copy');dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest);require(sha(dest)==record['sha256'],'candidate copy mismatch')
        require(not (self.package/'verify.tst').exists(),'unexpected verifier')
        shutil.move(previous,self.output/'prior-last-run');previous.mkdir()
        channel=self.output/'commands';channel.write_text('');self.commands=channel.open('a',buffering=1)
        self.stream=(self.output/'emulator.log').open('w')
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',LD_PRELOAD=str(HELPER),
            P023_VDP=str(PROFILE/'flexe-vdp.so'),P023_COMMANDS=str(channel))
        env.pop('DISPLAY',None);env.pop('WAYLAND_DISPLAY',None)
        self.process=subprocess.Popen(['./flexe-agon-emulator','--precise-interrupts'],cwd=PROFILE,env=env,
            stdout=self.stream,stderr=subprocess.STDOUT,start_new_session=True);self.group=self.process.pid

def native_admission():
    require(os.uname().nodename=='raspberrypi','Pi host required')
    pid1=Path('/proc/1/comm').read_text().strip();namespace=os.readlink('/proc/self/ns/pid')
    require(pid1=='systemd' and namespace==os.readlink('/proc/1/ns/pid'),'native whole-host PID visibility required')
    require(not base.local_emulators(),'Pi emulator host busy')
    compilers=[]
    for p in Path('/proc').iterdir():
        if p.name.isdigit():
            try:
                args=(p/'cmdline').read_bytes().split(b'\0');exe=Path(os.fsdecode(args[0])).name if args else ''
                if re.fullmatch(r'(?:cc1|cc1plus|cc|c\+\+|gcc|g\+\+|clang(?:\+\+)?|ld|ld.lld|cmake|ninja|make|xfoil|simpleFoam|pisoFoam)',exe):compilers.append(int(p.name))
            except (FileNotFoundError,ProcessLookupError,PermissionError):pass
    require(not compilers,'native compiler/solver work active')
    def cpu():return list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:9]))
    first=cpu();time.sleep(2);last=cpu();delta=[b-a for a,b in zip(first,last)]
    require(all(x>=0 for x in delta) and sum(delta)>0,'native CPU sample')
    busy=1-(delta[3]+delta[4])/sum(delta);require(busy<=.10,'native CPU quiet admission')
    require(not base.local_emulators(),'Pi host became busy')
    return dict(pid1=pid1,pid_namespace=namespace,CPU_two_second_busy_fraction=busy,quiet_threshold=.10,
        emulators=[],compilers=[],input='retained dummy video/audio, no DISPLAY/WAYLAND, acknowledged helper keyboard only')

def preflight(freeze):
    require(freeze['schema']=='pingo066-native-cost-driver-r1','native driver freeze schema')
    require(sha(DIAG/'readiness.json')==READY_SHA,'frozen diagnostics changed')
    for name,record in freeze['files'].items():
        p=Path(name) if name.startswith('/') else ROOT/name
        require(sha(p)==record['sha256'],'driver/input drift: '+name)
    old=json.loads((BUILD/'cost-r1/driver-readiness.json').read_text());base.preflight(old)
    require({key:os.environ.get(key) for key in freeze['host_environment']}==freeze['host_environment'],'host loader environment drift')
    require((Path.home()/'.local/lib/libSDL3.so.0').exists()==freeze['home_sdl_override_present'],'host SDL override drift')
    return dict(arms=freeze['packages'],freeze=freeze)

def execute_selected(freeze_path,permit_path,output,factory=None):
    freeze=json.loads(freeze_path.read_text());manifest=preflight(freeze)
    permit=json.loads(permit_path.read_text())
    require(permit.get('owner')=='fsim-pingo' and permit.get('task')=='PINGO-066' and
        permit.get('scope')=='finite-pi-native-off-on' and permit.get('arms')==list(ARMS),'execution permit scope')
    require(permit.get('driver_freeze_sha256')==sha(freeze_path),'execution permit freeze')
    require(not output.is_symlink() and output.resolve().parent==OUT.resolve(),'native output path')
    require(not Path(str(permit_path)+'.consumed').exists(),'execution permit already consumed')
    # Same lock as retained cost driver; no lock or writes in read-only mode.
    with (BUILD/'cost-r1/driver-run.lock').open('a') as lock:
        try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:raise Fault('another FSIM driver owns Pi profile')
        base.guarded_destination(output)
        for arm in ARMS:base.guarded_destination(PROFILE/'sdcard'/('corsair-native-cost-'+arm))
        admission=native_admission() # Failed admission never stages/launches.
        with Path(str(permit_path)+'.consumed').open('x') as used:used.write('finite off/on attempt selected\n')
        output.mkdir();(output/'admission.json').write_text(json.dumps(admission,indent=2)+'\n')
        fixed=(DIAG/'fixed-pose.bin').read_bytes()
        results=cohort(factory or (lambda a:HostIO(a,output/a,manifest)),freeze['build_ids'],fixed)
        require(not base.local_emulators(),'native stopped inventory failed')
        (output/'result.json').write_text(json.dumps(dict(passed=len(results)==2 and all(r['passed'] for r in results),
            arms=results,conditional_modular_only=True,physical_hardware=False,native_stopped_inventory=[]),indent=2)+'\n')
        require(len(results)==2 and all(r['passed'] for r in results),'native cohort stopped at first failure')
        return results

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',type=Path,required=True)
    p.add_argument('--permit',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    require(not a.output or a.permit,'output requires execution permit')
    if not a.permit:
        preflight(json.loads(a.freeze.read_text()));print('Native driver read-only source/runtime preflight; nothing staged/launched');return
    require(a.output is not None,'execution requires fresh output')
    execute_selected(a.freeze,a.permit,a.output)
if __name__=='__main__':main()
