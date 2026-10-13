"""Build both real emitters and compare bytes/C behavior with integer reference."""
from pathlib import Path
import hashlib
import json
import os
import struct
import subprocess
import sys
import time
from model import Machine, reference

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=HERE/'build'
SDK=ROOT.parent/'agon-vdplet'

def run(args,**kwargs):
    result=subprocess.run([str(x) for x in args],capture_output=True,**kwargs)
    if result.returncode:
        raise RuntimeError(result.stdout.decode()+result.stderr.decode())
    return result.stdout

def call(key):
    return bytes((23,0,160))+struct.pack('<H',key)+b'\x01'

def cases(directory):
    meta=json.loads((directory/'interface.json').read_text())
    ast=json.loads((directory/'program.json').read_text())
    initial=(directory/'state.bin').read_bytes()
    fields={f['name']:f for f in meta['globals']}
    vectors=[]
    def add(entry,updates):
        state=bytearray(initial)
        for key,value in updates.items():
            struct.pack_into('<H',state,fields[key]['offset'],value)
        vectors.append((entry,bytes(state)))
    if 'frame' in fields:
        for frame in (0,59,60,119,120,178,179):add(0,{'frame':frame})
        for updates in ({'frame':180},{'frame':65535},{'red':2},{'amber':255},{'green':256},
                        {'moving':65535},{'redColour':64},{'ticks':65535},{'ticks':65534}):add(0,updates)
        state=initial
        for _ in range(600):
            vectors.append((0,state));state=reference(meta,ast,state,0)
    elif 'bools' in directory.name:
        for entry in range(6):
            for a in (0,1,254,255,256,257,65534,65535):
                for b in (0,1,254,255,256,257,65534,65535):add(entry,{'a':a,'b':b})
        for entry in range(6,12):
            for flag,saved in ((0,0),(0,1),(1,0),(1,1)):
                for a,b in ((0,0),(0,1),(1,0),(1,2),(65535,0),(65535,65535)):
                    add(entry,{'flag':flag,'saved':saved,'a':a,'b':b})
        for key in ('flag','saved','spare'):
            for value in (2,255,256,65535):add(0,{key:value})
        add(0,{'earlier':65535});add(9,{'b':4,'result':65535})
    elif 'phase' in fields:
        for phase in (0,1,4,48,92,95,96):
            for direction in (0,1):add(0,{'phase':phase,'direction':direction})
        for updates in ({'phase':97},{'phase':65535},{'step':0},{'step':2},
                        {'ticks':65535},{'ticks':65534},{'direction':2},{'batch':4}):
            add(0,updates)
        state=initial
        for _ in range(600):
            vectors.append((0,state));state=reference(meta,ast,state,0)
    elif 'loops' in directory.name:
        for entry in range(17):
            add(entry,{})
            if entry not in (10,11):
                for updates in ({'input':1},{'input':101},{'input':65535},{'result':65535},
                                {'visits':65535},{'earlier':65535},{'guard':101}):add(entry,updates)
        add(7,{'input':100})
        add(14,{'input':65535,'result':0})
    elif 'conditions' in directory.name:
        for entry in range(6):
            for a in (0,1,254,255,256,257,65534,65535):
                for b in (0,1,254,255,256,257,65534,65535):add(entry,{'a':a,'b':b})
        for entry in range(6,12):
            for a,b in ((0,0),(0,65535),(1,65535),(255,256),(256,257),(256,255),
                        (65535,65535),(65535,0),(1,0),(2,0),(65534,65535)):
                add(entry,{'a':a,'b':b})
        add(0,{'earlier':65535})
        add(0,{'narrow':101})
    else:
        for entry in (0,1,2,3):
            for a,b in ((0,0),(0,1),(1,0),(1,1),(255,256),(256,256),(32768,2),(65535,0),(65535,1),(65535,65535)):
                add(entry,{'a':a,'b':b})
        add(0,{'a':1,'b':1,'earlier':65535})
        for a in (0,100,101,65535):
            add(4,{'a':a})
        add(0,{'narrow':101})
    return vectors

def main():
    started=time.monotonic()
    BUILD.mkdir(exist_ok=True)
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-ffp-contract=off']
    compiler=BUILD/'compiler'
    run(['g++',*flags,'-O2',HERE/'compiler.cpp','-o',compiler])
    sanitizer=BUILD/'compiler-asan'
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'compiler.cpp','-o',sanitizer])
    environment=os.environ.copy();environment['ASAN_OPTIONS']='detect_leaks=1'
    grammar=BUILD/'syntax-checks'
    run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',HERE/'syntax_checks.cpp','-o',grammar])
    grammar_result=run([grammar],env=environment).decode().strip()
    reports=[]
    for source in ('lights','bools','meter','arithmetic','conditions','loops'):
        for prefix in (0,254):
            directory=BUILD/(source if not prefix else source+'-relocated')
            command=[sanitizer,HERE/f'{source}.golem',directory]
            if prefix: command.append(prefix)
            print(run(command,env=environment).decode().strip(),flush=True)
            metadata=json.loads((directory/'backends.json').read_text())
            interface=json.loads((directory/'interface.json').read_text())
            ast=json.loads((directory/'program.json').read_text())
            run([sys.executable,SDK/'tools/build_module.py',directory/'native.c','--name',source,
                 '--build-id','0x474c3238','--output',directory])
            module=json.loads((directory/f'{source}.json').read_text())['module']
            run([SDK/'build/check_image',directory/f'{source}.vdpl',module['relative_relocations']])
            machine=Machine()
            machine.execute((directory/'common-install.bin').read_bytes())
            machine.execute((directory/'buffered-install.bin').read_bytes())
            shared=metadata['shared_state']
            assert shared not in metadata['buffered_owned']+metadata['native_owned']
            assert not set(metadata['buffered_owned'])&set(metadata['native_owned'])
            native_runner=directory/'native-host'
            run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-fsanitize=address,undefined',
                 '-I'+str(SDK/'include'),'-DGENERATED_SOURCE="'+str(directory/'native.c')+'"',
                 '-DSTATE_BYTES='+str(metadata['state_bytes']),HERE/'native_host.c','-o',native_runner])
            vectors=cases(directory)
            expected=[reference(interface,ast,state,entry) for entry,state in vectors]
            stdin=b''.join(struct.pack('<H',entry)+state for entry,state in vectors)
            observed=run([native_runner],input=stdin,env=environment)
            size=metadata['state_bytes']+4
            assert len(observed)==len(vectors)*size
            for index,((entry,state),want) in enumerate(zip(vectors,expected)):
                machine.memory[shared][:]=state
                machine.steps=0
                machine.execute(call(metadata['buffered_entries'][entry]))
                assert bytes(machine.memory[shared])==want,('buffered',source,prefix,index,bytes(machine.memory[shared]).hex(),want.hex())
                rc=struct.unpack_from('<i',observed,index*size)[0]
                actual=observed[index*size+4:(index+1)*size]
                assert actual==want,('generated-C-host',source,prefix,index,actual.hex(),want.hex())
                language_status=struct.unpack_from('<H',want,metadata['error_offset'])[0]
                assert rc==(1 if language_status else 0)
            retained=bytes(machine.memory[shared])
            machine.execute((directory/'buffered-cleanup.bin').read_bytes())
            assert bytes(machine.memory[shared])==retained
            assert not set(metadata['buffered_owned'])&machine.memory.keys()
            (directory/'vectors.bin').write_bytes(stdin)
            (directory/'expected-states.bin').write_bytes(b''.join(expected))
            reports.append({'source':source,'prefix':prefix,'cases':len(vectors),'state_bytes':metadata['state_bytes'],
                            'buffered_install_bytes':(directory/'buffered-install.bin').stat().st_size,
                            'native_module':module,'private_cleanup_preserves_shared_state':True})
    maintained=run(['make','-C',ROOT/'src','check']).decode()
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    report={'scope':'ASan/UBSan emitter and generated-C HOST behavior, byte-command model, ELF admission; target qualification separate',
            'packages':reports,'checks':sum(r['cases'] for r in reports),
            'source_sha256':{p.name:sha(p) for p in HERE.iterdir() if p.suffix in ('.cpp','.hpp','.c','.py','.golem')},
            'shared_frontend_sha256':sha(HERE/'portable.hpp'),
            'grammar_checks':grammar_result,
            'maintained_checks':maintained.splitlines(),'elapsed_seconds':round(time.monotonic()-started,3)}
    (HERE/'host-checks.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f"Host models/generated C agree on {report['checks']} cases; native packages admitted; maintained checks pass.")

if __name__=='__main__':
    main()
