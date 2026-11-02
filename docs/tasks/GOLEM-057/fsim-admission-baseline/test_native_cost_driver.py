"""Fake only device I/O; exercise the actual native driver orchestration."""
import copy,functools,json,struct,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
import native_cost_driver as d
import test_cost_driver as old
FIXED=(d.DIAG/'fixed-pose.bin').read_bytes();BUILD_ID=old.BUILD_ID

def request(op,token,body):return d.RUN+struct.pack('<HBBHBB',6+len(body),1,op,token,1,0)+body

def response(kind,token,value=0,protocol=1):return bytes([129,10])+b'VDPL'+struct.pack('<BBHH',protocol,kind,token,value)

def good_record(arm):
    v=d.ARMS.index(arm);w=[0]*160;w[:9]=[0x504e4331,1,640,1,v,v,2 if v else 6,4,64]
    w[9:15]=[68,68,68,0,0,68];w[20]=1;w[28:32]=[0,136,0,0]
    if v:
        w[21:28]=[32,10,10,0,100,200,0]
        for i in range(18):
            unit=60 if i==6 else 100 if i==17 else 10
            w[32+i*6:38+i*6]=[64,64*unit,0,unit,unit,0]
    return struct.pack('<160I',*w)

@functools.lru_cache(None)
def original_files():
    io=old.FakeIO('a');io.complete();return io.files

def outputs(arm,scenario):
    files=copy.deepcopy(original_files());raw=bytearray(good_record(arm))
    if scenario=='bad-record':raw[0]^=1
    if scenario=='bad-epoch':struct.pack_into('<I',raw,12,2)
    if scenario=='bad-reserved':struct.pack_into('<I',raw,636,1)
    if scenario=='bad-sum':struct.pack_into('<I',raw,132,123)
    if scenario=='bad-unwrap':struct.pack_into('<I',raw,112,1)
    if scenario=='bad-off-counts':struct.pack_into('<I',raw,128,64)
    if scenario=='short-record':raw=raw[:-2]
    record=good_record(arm);w=struct.unpack('<160I',record);v=d.ARMS.index(arm)
    stats=[]
    for i in range(18):
        n,lo,hi,mn,mx,z=w[32+i*6:38+i*6]
        stats.append(f'P066NC stat={i} count={n} sum_lo={lo} sum_hi={hi} min={mn} max={mx} zeros={z}')
    log=(f'P066NC variant={v} kind={v} record=nc001.bin structural=pass unwrap_qualified=0 physical_timing=0 counters={"conditional-modular" if v else "unavailable"}\r\n'+'\r\n'.join(stats)+'\r\nP066NC END status=complete readback=1 output=1 error=0 unsafe=0\r\n').encode()
    files.update({'nc001.bin':bytes(raw),'nc001.log':log})
    if scenario=='bad-native-summary':files['nc001.log']=log.replace(b'structural=pass',b'structural=invalid')
    if scenario=='bad-coarse-clock':files['cost001.log']=files['cost001.log'].replace(b'window_end=1142',b'window_end=1144')
    if scenario=='bad-state':files['state.bin']=struct.pack('<f',500)+files['state.bin'][4:]
    if scenario=='bad-stack-log':files['renderer.txt']=files['renderer.txt'].replace(b'stack_exit=1364',b'stack_exit=511')
    return files

class FakeIO(old.FakeIO):
    def native_trace(self):return b'VDPL LOAD handle=1\nVDPL UNLOAD status=0\n' if self.scenario!='missing-native-trace' else b''
    def complete(self):
        self.phase='flying';self.token=0
        self._reply+=bytes([134,8,64,1,240,0,40,30,64,136]);self.add_raw(b'\x17\x01\0'+d.GP+b'\0');self._reply+=bytes([128,1,0])
        if self.scenario=='RUN-timeout':
            p=bytearray(FIXED);struct.pack_into('<I',p,40,1)
            self.add_raw(d.base.POSE+p+request(2,1,struct.pack('<3H',1,0,30005)));return
        spacing=2 if self.scenario=='flight-timeout' else .005
        for i in range(68):self.schedule((i+1)*spacing,lambda i=i:self.frame(i))
        self.schedule(69*spacing,self.after_gp)
    def next(self,op,body,value=0,kind=None):
        self.token+=1;token=self.token
        tx=request(op,token,body);rx=response(kind if kind is not None else 4 if op in (4,5) else 3,token,value)
        return tx,rx
    def frame(self,i):
        if not self.live:return
        p=bytearray(FIXED);struct.pack_into('<I',p,4,i+1);struct.pack_into('<I',p,40,int(i==0))
        tx=d.base.POSE+p;rx=b''
        for entry in (0,1):
            a,b=self.next(2,struct.pack('<3H',1,entry,30005));tx+=a;rx+=b
            if i==0:
                a,b=self.next(4,struct.pack('<H',17),511 if self.scenario=='low-stack' else 1364);tx+=a;rx+=b
        tx+=d.SWAP+d.GP+bytes([i+1])
        gp=bytes([128,1,i+1])
        if self.scenario=='missing-GP' and i==3:gp=b''
        if self.scenario=='reordered-GP' and i==3:gp=bytes([128,1,2])
        if self.scenario=='extra-GP' and i==3:gp+=gp
        if self.scenario=='bad-token' and i==0:
            b=bytearray(rx);b[8]^=1;rx=bytes(b)
        if self.scenario=='bad-signature' and i==0:
            q=bytearray(rx);q[2]=ord('X');rx=bytes(q)
        if self.scenario=='bad-kind' and i==0:
            b=bytearray(rx);b[7]=4;rx=bytes(b)
        if self.scenario=='GP-before-DONE' and i==0:rx=gp+rx;gp=b''
        self.add_raw(tx);self._reply+=rx+gp
        if self.scenario=='READ-before-GP' and i==0:
            a,b=self.next(5,struct.pack('<HI',30003,15744));self.add_raw(a);self._reply+=b
    def after_gp(self):
        if not self.live:return
        self.add_raw(d.GP+bytes([69]));self._reply+=bytes([128,1,69])
        a,b=self.next(4,struct.pack('<H',17),1364);self.add_raw(a);self._reply+=b
        if self.scenario=='read-timeout':return
        for i in range(320):self.schedule(.001*(i+1),lambda i=i:self.read_word(i))
        self.schedule(.35,self.terminal)
    def read_word(self,i):
        if not self.live:return
        data=good_record(self.arm);value=struct.unpack_from('<H',data,i*2)[0]
        offset=15744+i*2
        if self.scenario=='missing-read' and i==150:return
        if self.scenario=='reordered-read' and i==3:offset+=2
        if self.scenario=='extra-read' and i==3:offset-=2
        if self.scenario=='read-wrong-buffer' and i==3:buffer=30004
        else:buffer=30003
        a,b=self.next(5,struct.pack('<HI',buffer,offset),value)
        if self.scenario=='read-bad-token' and i==2:
            q=bytearray(b);q[8]^=1;b=bytes(q)
        if self.scenario=='read-bad-kind' and i==2:
            q=bytearray(b);q[7]=3;b=bytes(q)
        self.add_raw(a);self._reply+=b
    def terminal(self):
        if not self.live:return
        if self.scenario=='cleanup-timeout':self.schedule(31,self.finish_guest);return
        self.finish_guest()
    def finish_guest(self):
        if not self.live:return
        self.files.update(outputs(self.arm,self.scenario))
        a,b=self.next(3,struct.pack('<H',1));self.add_raw(a+d.CLEAR+d.base.CLI);self._reply+=b
        self._reply+=bytes([134,8,128,2,224,1,80,60,16,0])
        self.add_raw(self.files['cost001.log']+self.files['nc001.log']+b'Flight ended ticks=106 error=0 OK\r\n/corsair-native-cost-'+self.arm.encode()+b' *');self.phase='mos'

class Checks(unittest.TestCase):
    def test_both_arms_observed_phases(self):
        fixtures=[]
        def factory(a):io=FakeIO(a);fixtures.append(io);return io
        result=d.cohort(factory,{a:BUILD_ID for a in d.ARMS},FIXED)
        self.assertTrue(all(r['passed'] for r in result),result);self.assertEqual([r['arm'] for r in result],list(d.ARMS))
        for r,io in zip(result,fixtures):
            self.assertLessEqual(r['ready_key_bracket'][1],r['GP68_observed']);self.assertLessEqual(r['GP68_observed'],r['READ16_complete_observed'])
            self.assertFalse(io.keys);self.assertFalse(io.signals);self.assertIn('proof.cfg',io.files);self.assertLess(io.t,450)
            self.assertFalse(r['native']['unwrap_qualified']);self.assertTrue(r['native']['structural_valid'])
    def test_first_failure_stops_no_on(self):
        scenarios=('missing-GP','extra-GP','reordered-GP','GP-before-DONE','bad-token','bad-kind','bad-signature','READ-before-GP',
            'missing-read','extra-read','reordered-read','read-wrong-buffer','read-bad-token','read-bad-kind',
            'bad-record','bad-epoch','bad-reserved','bad-unwrap','short-record','bad-off-counts','bad-native-summary',
            'bad-coarse-clock','bad-state','bad-stack-log','low-stack','missing-native-trace','wrong-build','wrong-assets','no-boot','no-ack')
        for scenario in scenarios:
            with self.subTest(scenario=scenario):
                made=[]
                def factory(a):made.append(a);return FakeIO(a,scenario)
                results=d.cohort(factory,{a:BUILD_ID for a in d.ARMS},FIXED)
                self.assertEqual(made,['off']);self.assertFalse(results[0]['passed'],(scenario,results))
    def test_phase_deadlines_and_key_cleanup(self):
        for scenario,label in [('RUN-timeout','2s RUN'),('flight-timeout','flight GP68'),('read-timeout','post-stop READ16'),('cleanup-timeout','terminal/MOS cleanup')]:
            io=FakeIO('off',scenario);r=d.run_arm(io,'off',BUILD_ID,FIXED)
            self.assertFalse(r['passed']);self.assertIn(label,r['error']);self.assertFalse(io.alive());self.assertFalse(io.keys);self.assertLessEqual(io.t,450.1)
    def test_record_on_sum_and_unavailable(self):
        for a in d.ARMS:self.assertTrue(d.validate_record(good_record(a),a)['structural_valid'])
        for scenario in ('bad-sum','bad-record','bad-epoch','bad-reserved','bad-unwrap','short-record'):
            with self.assertRaises(d.Fault):d.validate_record(outputs('on',scenario)['nc001.bin'],'on')
    def test_partial_and_reordered_observations(self):
        io=FakeIO('on');io.begin();io.events=[];io.live=True;io.complete()
        # Complete fake events then reveal one byte at a time, real parsers retained.
        io.sleep(2);io.sleep(1);trace=d.Trace(FIXED);tx=io.raw();rx=io.replies()
        # Trim unrelated initial mode announcement only; other packets are retained.
        at=0
        for end in range(1,len(tx)+1):trace.tx(tx[:end],.1)
        for end in range(1,len(rx)+1):trace.rx(rx[:end],.2)
        self.assertEqual(trace.complete(),good_record('on'))
    def test_failed_admission_never_stages(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);profile=root/'profile';(profile/'sdcard').mkdir(parents=True);out=root/'runs';out.mkdir()
            permit=root/'permit.json';freeze=root/'freeze.json';freeze.write_text('{}')
            permit.write_text(json.dumps(dict(owner='fsim-pingo',task='PINGO-066',scope='finite-pi-native-off-on',arms=list(d.ARMS),driver_freeze_sha256=d.sha(freeze))))
            (root/'cost-r1').mkdir();output=out/'attempt'
            with patch.object(d,'OUT',out),patch.object(d,'BUILD',root),patch.object(d,'PROFILE',profile),patch.object(d,'preflight',return_value={}),patch.object(d,'native_admission',side_effect=d.Fault('native busy')),patch.object(d,'HostIO',side_effect=AssertionError('must not stage')):
                with self.assertRaisesRegex(d.Fault,'native busy'):d.execute_selected(freeze,permit,output)
            self.assertFalse(output.exists());self.assertEqual(list((profile/'sdcard').iterdir()),[]);self.assertFalse(Path(str(permit)+'.consumed').exists())
    def test_source_drift_preflight_read_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'input';p.write_text('actual')
            with self.assertRaisesRegex(d.Fault,'driver/input drift'):d.preflight(dict(schema='pingo066-native-cost-driver-r1',files={str(p):dict(sha256='0'*64)}))
            self.assertEqual(list(Path(tmp).iterdir()),[p])
    def test_actual_host_begin_failed_admission_preserves_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);profile=root/'profile';(profile/'sdcard').mkdir(parents=True)
            output=root/'foreign';output.mkdir();sentinel=output/'keep';sentinel.write_text('unchanged')
            with patch.object(d,'PROFILE',profile),patch.object(d,'native_admission',side_effect=d.Fault('native busy')),patch.object(d.subprocess,'Popen',side_effect=AssertionError('must not launch')):
                io=d.HostIO('off',output,{})
                r=d.run_arm(io,'off',BUILD_ID,FIXED)
            self.assertFalse(r['passed']);self.assertEqual(sentinel.read_text(),'unchanged')
            self.assertEqual(list(output.iterdir()),[sentinel]);self.assertEqual(list((profile/'sdcard').iterdir()),[])
    def test_on_failure_keeps_off_result(self):
        made=[]
        def factory(a):made.append(a);return FakeIO(a,'bad-sum' if a=='on' else 'normal')
        r=d.cohort(factory,{a:BUILD_ID for a in d.ARMS},FIXED)
        self.assertEqual(made,['off','on']);self.assertTrue(r[0]['passed']);self.assertFalse(r[1]['passed'])
    def test_separate_file_observation_skew_keeps_unowned_tail(self):
        t=d.Trace(FIXED);reply=response(3,1)
        t.rx(reply,.1);self.assertEqual(t.rx_at,0)
        p=bytearray(FIXED);struct.pack_into('<I',p,40,1)
        tx=d.base.POSE+p+request(2,1,struct.pack('<3H',1,0,30005))
        t.tx(tx,.2);t.rx(reply,.2);self.assertEqual(len(t.done),1);self.assertEqual(t.rx_at,len(reply))
    def test_partial_terminal_and_clock_fault(self):
        t=d.Trace(FIXED);t.tx(d.RUN[:2],0);t.rx(b'\x81',0)
        with self.assertRaisesRegex(d.Fault,'partial terminal'):t.complete()
        io=FakeIO('off');io.live=True
        times=iter([10.,9.])
        with patch.object(io,'now',side_effect=lambda:next(times)):
            with self.assertRaisesRegex(d.Fault,'monotonic'):d.base.wait(io,lambda:False,20,'test')
    def test_owned_cleanup_escalation(self):
        # Reuse established owned-session signals with no new process mechanism.
        io=old.FakeIO('control','hard-hang');io.live=True
        result=d.cleanup(io,30,False)
        self.assertTrue(result['forced']);self.assertEqual(io.signals,['term','kill']);self.assertFalse(io.keys);self.assertFalse(io.alive())
if __name__=='__main__':unittest.main()
