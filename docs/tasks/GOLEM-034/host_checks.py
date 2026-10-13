"""Qualify D0 envelope, delivery identity and lease cleanup on host models/C."""
from pathlib import Path
import hashlib,json,os,struct,subprocess,time
from build_delivery import build,run,verify_pin,SDK
from model import Machine,reference
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2];BUILD=HERE/'build'
word=lambda v:struct.pack('<H',v)
call=lambda key:bytes((23,0,160))+word(key)+b'\x01'
def vectors(directory):
    meta=json.loads((directory/'backends.json').read_text());iface=json.loads((directory/'interface.json').read_text());root=json.loads((directory/'root.json').read_text())
    initial=(directory/'state.bin').read_bytes();fields={f['name']:f for f in iface['globals']};out=[]
    def add(entry,updates=None,offsets=None,mode=0,label=''):
        state=bytearray(initial);struct.pack_into('<H',state,28,entry)
        for name,v in (updates or {}).items():struct.pack_into('<H',state,fields[name]['offset'],v)
        for at,fmt,v in (offsets or []):struct.pack_into('<'+fmt,state,at,v)
        out.append((entry,mode,bytes(state),label))
    for entry in range(5):
        for position in (32,34,128,222,224):
            for direction in (0,1):add(entry,{'position':position,'direction':direction,'running':1},label='normal')
    for entry in range(5):
        for at in list(range(8))+[26,27,30,31]+list(range(36,48))+list(range(48,112)):
            add(entry,offsets=[(at,'B',initial[at]^1),(14,'H',77),(20,'I',999),(24,'H',3)],label='metadata')
        add(entry,offsets=[(8,'I',0),(14,'H',77)],label='zero-session')
        add(entry,offsets=[(8,'I',0x80000000),(16,'I',0x87654321),(20,'I',0xffffffff)],label='opaque-wrap')
        for updates in ({'position':31},{'position':225},{'direction':2},{'running':2},{'colour':64}):add(entry,updates,label='source-admission')
    for ticks in (65534,65535):add(1,{'ticks':ticks,'running':1},offsets=[(20,'I',12),(24,'H',2)],label='source-failure')
    add(1,{'running':0},label='stopped')
    for mode in (1,2,3,4,5):
        add(1,{'running':1},offsets=[(14,'H',77),(20,'I',99),(24,'H',2)],mode=mode,label='physical-runtime')
    add(0,offsets=[(28,'H',65535),(14,'H',77)],mode=7,label='unknown-requested')
    add(65535,offsets=[(14,'H',77)],mode=8,label='unknown-sdk')
    state=initial
    for i in range(600):
        entry=0 if i==0 else 1
        state=bytearray(state);struct.pack_into('<H',state,28,entry);state=bytes(state)
        out.append((entry,0,state,'sequence'));state=reference(iface,json.loads((directory/'program.json').read_text()),state,entry,root)
    return out

def expected(directory,entry,mode,state):
    iface=json.loads((directory/'interface.json').read_text());ast=json.loads((directory/'program.json').read_text());root=json.loads((directory/'root.json').read_text())
    if mode in (3,5,8):return state,{3:3,5:1,8:9}[mode]
    if mode in (1,2,4):
        result=bytearray(state);struct.pack_into('<H',result,12,8);return bytes(result),{1:2,2:3,4:7}[mode]
    result=reference(iface,ast,state,entry,root,named=mode!=7)
    rc=1 if struct.unpack_from('<H',result,12)[0] or struct.unpack_from('<H',result,14)[0] else 0
    return result,rc

def main():
    start=time.monotonic();BUILD.mkdir(exist_ok=True);verify_pin()
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-ffp-contract=off']
    run(['g++',*flags,'-O2',HERE/'compiler.cpp','-o',BUILD/'compiler'])
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'compiler.cpp','-o',BUILD/'compiler-asan'])
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1'
    receipts=[]
    for prefix in (0,254):
        directory=BUILD/('probe' if not prefix else 'probe-relocated')
        manifest=build(HERE/'probe.golem',directory,prefix,compiler=BUILD/'compiler-asan')
        meta=json.loads((directory/'backends.json').read_text());initial=(directory/'state.bin').read_bytes()
        machine=Machine();machine.execute((directory/'common-install.bin').read_bytes());machine.execute((directory/'buffered-install.bin').read_bytes())
        runner=directory/'native-host'
        run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-ffp-contract=off','-fsanitize=address,undefined',
             '-I'+str(SDK/'include'),'-DGENERATED_SOURCE="'+str(directory/'native.c')+'"','-DSTATE_BYTES='+str(len(initial)),HERE/'native_host.c','-o',runner])
        cases=vectors(directory);stdin=b''.join(struct.pack('<HH',entry,mode)+state for entry,mode,state,_ in cases)
        observed=run([runner],input=stdin,env=env);size=len(initial)+4;assert len(observed)==len(cases)*size
        for i,(entry,mode,state,label) in enumerate(cases):
            want,rc=expected(directory,entry,mode,state)
            got=observed[i*size:(i+1)*size];assert struct.unpack_from('<i',got)[0]==rc,('native-rc',i,label,mode)
            assert got[4:]==want,('native-state',i,label,mode,got[4:].hex(),want.hex())
            if mode in (0,7):
                machine.memory[meta['shared_state']][:]=state;machine.steps=0
                machine.execute(call(meta['buffered_dispatch'] if mode==7 else meta['buffered_entries'][entry]))
                assert bytes(machine.memory[meta['shared_state']])==want,('buffered',i,label,bytes(machine.memory[meta['shared_state']]).hex(),want.hex())
        retained=bytes(machine.memory[meta['shared_state']]);machine.execute((directory/'buffered-cleanup.bin').read_bytes())
        assert bytes(machine.memory[meta['shared_state']])==retained
        assert not set(meta['buffered_owned'])&machine.memory.keys()
        assert not set(meta['buffered_owned'])&set(meta['native_owned'])
        assert not {b['buffer'] for b in manifest['root']['bindings']}&set(meta['buffered_owned']+meta['native_owned'])
        receipts.append({'prefix':prefix,'cases':len(cases),'root_bytes':len(initial),'native_leases_released_on_all_paths':True,
                         'max_mock_live_leases':3,'shared_state_survives_private_cleanup':True,'manifest_sha256':hashlib.sha256((directory/'delivery-manifest.json').read_bytes()).hexdigest()})
    invalid=['--bind 1 0 0 1','--bind 1 65535 0 1','--bind 5 20000 0 1','--bind 1 20000 0 0',
             '--bind 1 20000 4294967295 2','--bind 1 20000 0 1 --bind 1 20001 0 1','--bind 1 20000 0 1 --bind 2 20000 0 1']
    for args in invalid:
        p=subprocess.run([str(BUILD/'compiler'),str(HERE/'probe.golem'),str(BUILD/'invalid'),*args.split()],capture_output=True)
        assert p.returncode
    maintained=run(['make','-C',ROOT/'src','check']).decode().splitlines()
    receipt={'baseline':verify_pin(),'packages':receipts,'checks':sum(r['cases'] for r in receipts),'cli_rejections':len(invalid),
             'maintained_checks':maintained,'elapsed_seconds':round(time.monotonic()-start,3)}
    (HERE/'host-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print('D0 host checks pass:',receipt['checks'],flush=True)
if __name__=='__main__':main()
