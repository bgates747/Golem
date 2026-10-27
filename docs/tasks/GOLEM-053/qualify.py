"""Qualify actual on-Agon compilation and stock command execution on Pi."""
from pathlib import Path
import importlib.util,json,shutil,struct,subprocess,sys,time
from check import HERE,ROOT,BUILD,prepare,vectors,word,run,sha
sys.path.insert(0,str(ROOT/'docs/tasks/GOLEM-051'))
spec=importlib.util.spec_from_file_location('golem051_session',ROOT/'docs/tasks/GOLEM-051/qualify.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
Session,stage=module.Session,module.stage

def prepare_guest():
    packages=prepare();app=stage();dest=app/'bitwise';dest.mkdir(exist_ok=True)
    compiler=ROOT/'docs/tasks/GOLEM-051/build/port/ez80-portable/golemc.bin'
    shutil.copyfile(compiler,dest/'compiler.bin');shutil.copyfile(compiler,app/'compiler/portable.bin');shutil.copyfile(HERE/'review.golem',dest/'review.golem')
    descriptions=[]
    for p in packages:
        meta,iface,cases,offsets=vectors(p);descriptions.append((meta,iface,cases,offsets))
        (dest/(p.name+'.dat')).write_bytes(b''.join(word(c['entry'])+c['initial']+c['array']+c['expected'] for c in cases))
        shutil.rmtree(dest/p.name,ignore_errors=True)
        shutil.copytree(p,dest/p.name,ignore=shutil.ignore_patterns('native-host','fixture.h'))
    meta,iface,cases,offsets=descriptions[0];other=descriptions[1][0]
    header=f'#define STATE_BYTES {meta["state_bytes"]}\n#define ARRAY_BYTES 6\n#define CASES {len(cases)}\n#define OFFSET_COUNT {len(offsets)}\n#define OBSERVATIONS {len(cases)*len(offsets)*2}\n'
    header+=f'#define STATE_DEFAULT {meta["shared_state"]}\n#define STATE_RELOCATED {other["shared_state"]}\n'
    header+=f'#define ARRAY_DEFAULT {iface["arrays"][0]["buffer"]}\n#define ARRAY_RELOCATED {descriptions[1][1]["arrays"][0]["buffer"]}\n'
    for name,values in [('offsets',offsets),('entries_default',meta['buffered_entries']),('entries_relocated',other['buffered_entries']),
                        ('display_offsets',[next(g['offset'] for g in iface['globals'] if g['name']==n) for n in ('uand','uor','uxor','unot')])]:
        header+='static const unsigned '+name+'[]={'+','.join(map(str,values))+'};\n'
    (BUILD/'fixture.h').write_text(header)
    tools=Path('/Agon/agondev/release')
    run([tools/'bin/ez80-none-elf-clang','-target','ez80-none-elf','-mllvm','-z80-gas-style','-mllvm','-z80-print-zero-offset',
         '-nostdinc','-isystem',tools/'include','-I'+str(BUILD),'-Oz','-DAGONDEV','-Wa,-march=ez80+full','-c',HERE/'guest.c','-o',BUILD/'guest.o'])
    run([tools/'bin/ez80-none-elf-ld','-defsym=RAM_START=0x40000','-defsym=RAM_SIZE=0x70000','-defsym=_has_exit_handler=0',
         '-T',tools/'config/linker.conf','--oformat','binary','-o',BUILD/'review.bin',BUILD/'guest.o','-L'+str(tools/'lib'),'-lagon'])
    shutil.copyfile(BUILD/'review.bin',dest/'review.bin')
    # One small compilation per type stays within the target compiler's RAM.
    # The combined stress fixture is compiled on the PC and executed on stock VDP.
    responses=[]
    for t,initial in [('uint16','5'),('sint16','-32768'),('uint32','4294967295'),('sint32','-2147483648')]:
        source=f'{t} a={initial} {t} b=3 {t} x=0 {t} y=0 {t} z=0 {t} n=0 Sub f x=a&b y=a|b z=a^b n=~a EndSub\n'
        (dest/(t+'.golem')).write_text(source)
        target=BUILD/('mos-reference-'+t)
        run([ROOT/'build/golemc','--portable',dest/(t+'.golem'),target],stdout=subprocess.DEVNULL)
        (dest/(t+'.args')).write_text(f'--portable {t}.golem mos-{t}\n')
        shutil.rmtree(dest/('mos-'+t),ignore_errors=True)
        responses.append(t+'.args')
    (dest/'compile.list').write_text('\n'.join(responses)+'\n')
    auto='SET KEYBOARD 1\nVDU 22 0\ncd /golem/bitwise\ngolemc --batch compile.list\nload review.bin\nrun .\n'
    (dest/'review.txt').write_bytes(auto.replace('\n','\r\n').encode())
    return dest,auto,packages,descriptions

def execute():
    dest,auto,packages,descriptions=prepare_guest()
    for n in ('done.dat','results.dat','keyboard.dat'):(dest/n).unlink(missing_ok=True)
    session=Session(auto,'bitwise-053');report={}
    try:
        session.wait(dest/'done.dat',180);session.wait(dest/'results.dat',20)
        expected=b''.join(bytes(c['expected'][o] for o in offsets) for meta,iface,cases,offsets in descriptions for c in cases)
        deadline=time.monotonic()+10
        while (dest/'results.dat').stat().st_size<len(expected):
            if time.monotonic()>deadline:raise TimeoutError('partial results')
            time.sleep(.05)
        assert (dest/'done.dat').read_bytes()==b'\0'
        actual=(dest/'results.dat').read_bytes();assert actual==expected,[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12]
        files=0
        for p in packages:
            for f in p.iterdir():
                if f.name in ('fixture.h','native-host'):continue
                assert f.read_bytes()==(dest/p.name/f.name).read_bytes(),str(f);files+=1
        for t in ('uint16','sint16','uint32','sint32'):
            for f in (BUILD/('mos-reference-'+t)).iterdir():
                assert f.read_bytes()==(dest/('mos-'+t)/f.name).read_bytes(),str(f)
                files+=1
        session.type('dir\nsave keyboard.dat 40000 3\n');session.wait(dest/'keyboard.dat',20)
        image=session.capture()
        report={'passed':True,'seconds':time.monotonic()-session.started,'actual_MOS_compiled_packages':4,'matching_package_files':files,
                'stock_byte_observations':len(expected),'calls':sum(len(d[2]) for d in descriptions),'keyboard_and_MOS_return':True,
                'compiler_sha256':sha(dest/'compiler.bin'),'picture':str(image),'human':'pending'}
    finally:report['frontend_exit']=session.close()
    (HERE/'stock-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    return dest
if __name__=='__main__':execute()
